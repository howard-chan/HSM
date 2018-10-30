/*
The MIT License (MIT)

Copyright (c) 2015-2018 Howard Chan
https://github.com/howard-chan/HSM

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "hsm.h"

#if HSM_FEATURE_DEBUG_NESTED_CALL
uint8_t gucHsmNestLevel;
const char * const apucHsmNestIndent[] = { "", "", "\t", "\t\t", "\t\t\t", "\t\t\t\t"};
#endif // HSM_FEATURE_DEBUG_NESTED_CALL

HSM_EVENT HSM_RootHandler(HSM *This, HSM_EVENT event, void *param)
{
#ifdef HSM_DEBUG_EVT2STR
    HSM_DEBUG("\tEvent:%s dropped, No Parent handling of %s[%s] param %lx",
              HSM_DEBUG_EVT2STR(event), This->name, This->curState->name, (unsigned long)param);
#else
    HSM_DEBUG("\tEvent:%lx dropped, No Parent handling of %s[%s] param %lx",
              (unsigned long)event, This->name, This->curState->name, (unsigned long)param);
#endif // HSM_DEBUG_EVT2STR
    return HSME_NULL;
}

HSM_STATE const HSM_ROOT =
{
    .parent = ((void *)0),
    .handler = HSM_RootHandler,
    .name = ":ROOT:",
    .level = 0
};

void HSM_STATE_Create(HSM_STATE *This, const char *name, HSM_FN handler, HSM_STATE *parent)
{
    if (((void *)0) == parent)
    {
        parent = (HSM_STATE *)&HSM_ROOT;
    }
    This->name = name;
    This->handler = handler;
    This->parent = parent;
    This->level = parent->level + 1;
    if (This->level >= HSM_MAX_DEPTH)
    {
        HSM_DEBUG("Please increase HSM_MAX_DEPTH > %d", This->level);
        // assert(0, "Please increase HSM_MAX_DEPTH");
        while(1);
    }
}

void HSM_Create(HSM *This, const char *name, HSM_STATE *initState)
{
    // Setup debug
#if HSM_FEATURE_DEBUG_ENABLE
    This->name = name;
    This->prefix = "";
    This->hsmDebugCfg = 0;
    This->hsmDebug = 0;
#endif // HSM_FEATURE_DEBUG_ENABLE
    // Supress warning for unused variable if HSM_FEATURE_DEBUG_ENABLE is not defined
    (void)name;

    // Initialize state
    This->curState = initState;
    // Invoke ENTRY and INIT event
    HSM_DEBUGC1("  %s[%s](ENTRY)", This->name, initState->name);
    This->curState->handler(This, HSME_ENTRY, 0);
    HSM_DEBUGC1("  %s[%s](INIT)", This->name, initState->name);
    This->curState->handler(This, HSME_INIT, 0);
}

HSM_STATE *HSM_GetState(HSM *This)
{
    // This returns the current HSM state
    return This->curState;
}

uint8_t HSM_IsInState(HSM *This, HSM_STATE *state)
{
    HSM_STATE *curState;
    // Traverse the parents to find the matching state.
    for (curState = This->curState; curState; curState = curState->parent)
    {
        if (state == curState)
        {
            // Match found, HSM is in state or parent state
            return 1;
        }
    }
    // This HSM is not in state or parent state
    return 0;
}

void HSM_Run(HSM *This, HSM_EVENT event, void *param)
{
#if HSM_FEATURE_DEBUG_ENABLE && HSM_FEATURE_DEBUG_NESTED_CALL
    // Increment the nesting count
    gucHsmNestLevel++;
#endif // HSM_FEATURE_DEBUG_ENABLE && HSM_FEATURE_DEBUG_NESTED_CALL

    // This runs the state's event handler and forwards unhandled events to
    // the parent state
    HSM_STATE *state = This->curState;
#ifdef HSM_DEBUG_EVT2STR
    HSM_DEBUGC1("Run %s[%s](evt:%s, param:%08lx)", This->name, state->name, HSM_DEBUG_EVT2STR(event), (unsigned long)param);
#else
    HSM_DEBUGC1("Run %s[%s](evt:%lx, param:%08lx)", This->name, state->name, (unsigned long)event, (unsigned long)param);
#endif // HSM_DEBUG_EVT2STR
    while (event)
    {
        event = state->handler(This, event, param);
        state = state->parent;
        if (event)
        {
#ifdef HSM_DEBUG_EVT2STR
            HSM_DEBUGC1("  evt:%s unhandled, passing to %s[%s]", HSM_DEBUG_EVT2STR(event), This->name, state->name);
#else
            HSM_DEBUGC1("  evt:%lx unhandled, passing to %s[%s]", (unsigned long)event, This->name, state->name);
#endif // HSM_DEBUG_EVT2STR
        }
    }
#if HSM_FEATURE_DEBUG_ENABLE
    // Restore debug back to the configured debug
    This->hsmDebug = This->hsmDebugCfg;
#if HSM_FEATURE_DEBUG_NESTED_CALL
    if (gucHsmNestLevel)
    {
        // Decrement the nesting count
        gucHsmNestLevel--;
    }
#endif // HSM_FEATURE_DEBUG_NESTED_CALL
#endif // HSM_FEATURE_DEBUG_ENABLE
}

void HSM_Tran(HSM *This, HSM_STATE *nextState, void *param, void (*method)(HSM *This, void *param))
{
#if HSM_FEATURE_SAFETY_CHECK
    // [optional] Check for illegal call to HSM_Tran in HSME_ENTRY or HSME_EXIT
    if (This->hsmTran)
    {
        HSM_DEBUG("!!!!Illegal call of HSM_Tran[%s -> %s] in HSME_ENTRY or HSME_EXIT Handler!!!!",
            This->curState->name, nextState->name);
        return;
    }
    // Guard HSM_Tran() from certain recursive calls
    This->hsmTran = 1;
#endif // HSM_FEATURE_SAFETY_CHECK

    HSM_STATE *list_exit[HSM_MAX_DEPTH];
    HSM_STATE *list_entry[HSM_MAX_DEPTH];
    uint8_t cnt_exit = 0;
    uint8_t cnt_entry = 0;
    uint8_t idx;
    // This performs the state transition with calls of exit, entry and init
    // Bulk of the work handles the exit and entry event during transitions
    HSM_DEBUGC2("Tran %s[%s -> %s]", This->name, This->curState->name, nextState->name);
    // 1) Find the lowest common parent state
    HSM_STATE *src = This->curState;
    HSM_STATE *dst = nextState;
    // 1a) Equalize the levels
    while (src->level != dst->level)
    {
        if (src->level > dst->level)
        {
            // source is deeper
            list_exit[cnt_exit++] = src;
            src = src->parent;
        }
        else
        {
            // destination is deeper
            list_entry[cnt_entry++] = dst;
            dst = dst->parent;
        }
    }
    // 1b) find the common parent
    while (src != dst)
    {
        list_exit[cnt_exit++] = src;
        src = src->parent;
        list_entry[cnt_entry++] = dst;
        dst = dst->parent;
    }
    // 2) Process all the exit events
    for (idx = 0; idx < cnt_exit; idx++)
    {
        src = list_exit[idx];
        HSM_DEBUGC3("  %s[%s](EXIT)", This->name, src->name);
        src->handler(This, HSME_EXIT, param);
    }
    // 3) Call the transitional method hook
    if (method)
    {
        method(This, param);
    }
    // 4) Process all the entry events
    for (idx = 0; idx < cnt_entry; idx++)
    {
        dst = list_entry[cnt_entry - idx - 1];
        HSM_DEBUGC3("  %s[%s](ENTRY)", This->name, dst->name);
        dst->handler(This, HSME_ENTRY, param);
    }
    // 5) Now we can set the destination state
    This->curState = nextState;
#if HSM_FEATURE_SAFETY_CHECK
    This->hsmTran = 0;
#endif // HSM_FEATURE_SAFETY_CHECK
#if HSM_FEATURE_INIT
    // 6) Invoke INIT signal, NOTE: Only HSME_INIT can recursively call HSM_Tran()
    HSM_DEBUGC3("  %s[%s](INIT)", This->name, nextState->name);
    This->curState->handler(This, HSME_INIT, param);
#endif // HSM_FEATURE_INIT
}
