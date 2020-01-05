/*
The MIT License (MIT)

Copyright (c) 2015-2020 Howard Chan
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
#include <stdint.h>
#include "hsm.h"

#if HSM_FEATURE_DEBUG_NESTED_CALL
uint8_t gucHsmNestLevel;
const char * const apucHsmNestIndent[] = { "", "", "\t", "\t\t", "\t\t\t", "\t\t\t\t" };
#endif // HSM_FEATURE_DEBUG_NESTED_CALL

static HSM_EVENT HSM_RootHandler(HSM *This, HSM_EVENT xEvent, void *pvParam)
{
#ifdef HSM_DEBUG_EVT2STR
    const char *pcEvtStr = This->pfnEvt2Str(xEvent);
    if (pcEvtStr)
    {
        HSM_DEBUG(HSM_COLOR_YEL "\tEvent:%s dropped, No Parent handling of %s[%s] param %#lx" HSM_COLOR_NON,
                  pcEvtStr, This->pcName, This->pxCurState->pcName, (unsigned long)pvParam);
    }
    else
#endif // HSM_DEBUG_EVT2STR
    {
        HSM_DEBUG(HSM_COLOR_YEL "\tEvent:%#lx dropped, No Parent handling of %s[%s] param %#lx" HSM_COLOR_NON,
                  (unsigned long)xEvent, This->pcName, This->pxCurState->pcName, (unsigned long)pvParam);
    }
    return HSME_NULL;
}

HSM_STATE const HSM_ROOT =
{
    .pxParent = ((void *)0),
    .pfnHandler = HSM_RootHandler,
    .pcName = ":ROOT:",
    .ucLevel = 0
};

void HSM_STATE_Create(HSM_STATE *This, const char *pcName, HSM_FN pfnHandler, HSM_STATE *pxParent)
{
    if (((void *)0) == pxParent)
    {
        pxParent = (HSM_STATE *)&HSM_ROOT;
    }
    This->pcName = pcName;
    This->pfnHandler = pfnHandler;
    This->pxParent = pxParent;
    This->ucLevel = pxParent->ucLevel + 1;
    if (This->ucLevel >= HSM_MAX_DEPTH)
    {
        HSM_DEBUG("Please increase HSM_MAX_DEPTH > %d", This->ucLevel);
        // assert(0, "Please increase HSM_MAX_DEPTH");
        while(1);
    }
}

void HSM_Create(HSM *This, const char *pcName, HSM_STATE *pxInitState)
{
    // Setup debug
#if HSM_FEATURE_DEBUG_ENABLE
#ifdef HSM_DEBUG_EVT2STR
    This->pfnEvt2Str = HSM_DEBUG_EVT2STR;
#endif // HSM_DEBUG_EVT2STR
    This->pcName = pcName;
    This->pcPrefix = "";
    This->bmDebugCfg = 0;
    This->bmDebug = 0;
#endif // HSM_FEATURE_DEBUG_ENABLE
    // Supress warning for unused variable if HSM_FEATURE_DEBUG_ENABLE is not defined
    (void)pcName;

    // Initialize state
    This->pxCurState = pxInitState;
    // Invoke ENTRY and INIT event
    HSM_DEBUGC1("  %s[%s](ENTRY)", This->pcName, pxInitState->pcName);
    This->pxCurState->pfnHandler(This, HSME_ENTRY, 0);
#if HSM_FEATURE_INIT
    HSM_DEBUGC1("  %s[%s](INIT)", This->pcName, pxInitState->pcName);
    This->pxCurState->pfnHandler(This, HSME_INIT, 0);
#endif // HSM_FEATURE_INIT
}

HSM_STATE *HSM_GetState(HSM *This)
{
    // This returns the current HSM state
    return This->pxCurState;
}

uint8_t HSM_IsInState(HSM *This, HSM_STATE *state)
{
    HSM_STATE *pxCurState;
    // Traverse the parents to find the matching state.
    for (pxCurState = This->pxCurState; pxCurState; pxCurState = pxCurState->pxParent)
    {
        if (state == pxCurState)
        {
            // Match found, HSM is in state or parent state
            return 1;
        }
    }
    // This HSM is not in state or parent state
    return 0;
}

void HSM_Run(HSM *This, HSM_EVENT event, void *pvParam)
{
#if HSM_FEATURE_DEBUG_ENABLE && HSM_FEATURE_DEBUG_NESTED_CALL
    // Increment the nesting count
    gucHsmNestLevel++;
#endif // HSM_FEATURE_DEBUG_ENABLE && HSM_FEATURE_DEBUG_NESTED_CALL

    // This runs the state's event handler and forwards unhandled events to
    // the parent state
    HSM_STATE *state = This->pxCurState;

#ifdef HSM_DEBUG_EVT2STR
    const char *pcEvtStr = This->pfnEvt2Str(event);
    if (pcEvtStr)
    {
        HSM_DEBUGC1("Run %s[%s](evt:%s, param:%#lx)", This->pcName, state->pcName, pcEvtStr, (unsigned long)pvParam);
    }
    else
#endif // HSM_DEBUG_EVT2STR
    {
        HSM_DEBUGC1("Run %s[%s](evt:%#lx, param:%#lx)", This->pcName, state->pcName, (unsigned long)event, (unsigned long)pvParam);
    }

    while (event)
    {
        event = state->pfnHandler(This, event, pvParam);
        state = state->pxParent;
        if (event)
        {
#ifdef HSM_DEBUG_EVT2STR
            pcEvtStr = This->pfnEvt2Str(event);
            if (pcEvtStr)
            {
                HSM_DEBUGC1("  Pass to %s[%s](evt:%s, param:%#lx)", This->pcName, state->pcName, pcEvtStr, (unsigned long)pvParam);
            }
            else
#endif // HSM_DEBUG_EVT2STR
            {
                HSM_DEBUGC1("  Pass to %s[%s](evt:%#lx, param:%#lx)", This->pcName, state->pcName, (unsigned long)event, (unsigned long)pvParam);
            }
        }
    }

#if HSM_FEATURE_DEBUG_ENABLE
    // Restore debug back to the configured debug
    This->bmDebug = This->bmDebugCfg;
#if HSM_FEATURE_DEBUG_NESTED_CALL
    if (gucHsmNestLevel)
    {
        // Decrement the nesting count
        gucHsmNestLevel--;
    }
#endif // HSM_FEATURE_DEBUG_NESTED_CALL
#endif // HSM_FEATURE_DEBUG_ENABLE
}

void HSM_Tran(HSM *This, HSM_STATE *pxNextState, void *pvParam, void (*method)(HSM *This, void *pvParam))
{
#if HSM_FEATURE_SAFETY_CHECK
    // [optional] Check for illegal call to HSM_Tran in HSME_ENTRY or HSME_EXIT
    if (This->bIsTran)
    {
        HSM_DEBUG("!!!!Illegal call of HSM_Tran[%s -> %s] in HSME_ENTRY or HSME_EXIT Handler!!!!",
            This->pxCurState->pcName, pxNextState->pcName);
        return;
    }
    // Guard HSM_Tran() from certain recursive calls
    This->bIsTran = 1;
#endif // HSM_FEATURE_SAFETY_CHECK

    HSM_STATE *axList_exit[HSM_MAX_DEPTH];
    HSM_STATE *axList_entry[HSM_MAX_DEPTH];
    uint8_t ucCnt_exit = 0;
    uint8_t ucCnt_entry = 0;
    uint8_t idx;
    // This performs the state transition with calls of exit, entry and init
    // Bulk of the work handles the exit and entry event during transitions
    HSM_DEBUGC2("Tran %s[%s -> %s]", This->pcName, This->pxCurState->pcName, pxNextState->pcName);
    // 1) Find the lowest common parent state
    HSM_STATE *src = This->pxCurState;
    HSM_STATE *dst = pxNextState;
    // 1a) Equalize the levels
    while (src->ucLevel != dst->ucLevel)
    {
        if (src->ucLevel > dst->ucLevel)
        {
            // source is deeper
            axList_exit[ucCnt_exit++] = src;
            src = src->pxParent;
        }
        else
        {
            // destination is deeper
            axList_entry[ucCnt_entry++] = dst;
            dst = dst->pxParent;
        }
    }
    // 1b) find the common parent
    while (src != dst)
    {
        axList_exit[ucCnt_exit++] = src;
        src = src->pxParent;
        axList_entry[ucCnt_entry++] = dst;
        dst = dst->pxParent;
    }
    // 2) Process all the exit events
    for (idx = 0; idx < ucCnt_exit; idx++)
    {
        src = axList_exit[idx];
        HSM_DEBUGC3("  %s[%s](EXIT)", This->pcName, src->pcName);
        src->pfnHandler(This, HSME_EXIT, pvParam);
    }
    // 3) Call the transitional method hook
    if (method)
    {
        method(This, pvParam);
    }
    // 4) Process all the entry events
    for (idx = 0; idx < ucCnt_entry; idx++)
    {
        dst = axList_entry[ucCnt_entry - idx - 1];
        HSM_DEBUGC3("  %s[%s](ENTRY)", This->pcName, dst->pcName);
        dst->pfnHandler(This, HSME_ENTRY, pvParam);
    }
    // 5) Now we can set the destination state
    This->pxCurState = pxNextState;

#if HSM_FEATURE_SAFETY_CHECK
    This->bIsTran = 0;
#endif // HSM_FEATURE_SAFETY_CHECK

#if HSM_FEATURE_INIT
    // 6) Invoke INIT signal, NOTE: Only HSME_INIT can recursively call HSM_Tran()
    HSM_DEBUGC3("  %s[%s](INIT)", This->pcName, pxNextState->pcName);
    This->pxCurState->pfnHandler(This, HSME_INIT, pvParam);
#endif // HSM_FEATURE_INIT
}
