/*
The MIT License (MIT)

Copyright (c) 2015 Howard Chan

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

#ifdef HSM_DEBUG
#include <stdio.h>
#define DEBUG(...)	printf(__VA_ARGS__)
#else
#define DEBUG(...)
#endif // HSM_DEBUG

HSM_EVENT HSM_RootHandler(HSM *This, HSM_EVENT event, UINT32 param)
{
	DEBUG("\tUnhandled event:%d %s[%s]\n", event, This->name, This->curState->name);
	return HSME_NULL;
}

HSM_STATE HSM_ROOT =
{
	.parent = NULL,
	.handler = HSM_RootHandler,
	.name = ":ROOT:",
	.level = 0
};

void HSM_STATE_Create(HSM_STATE *This, char *name, HSM_FN handler, HSM_STATE *parent)
{
	if (NULL == parent)
	{
		parent = &HSM_ROOT;
	}
	This->name = name;
	This->handler = handler;
	This->parent = parent;
	This->level = parent->level + 1;
	if (This->level >= HSM_MAX_DEPTH)
	{
		DEBUG("Please increase HSM_MAX_DEPTH > %d", This->level);
		// assert(0, "Please incrase HSM_MAX_DEPTH");
	}
}

void HSM_Create(HSM *This, char *name, HSM_STATE *initState)
{
	// Initialize state
	This->curState = initState;
	This->name = name;
	This->hsmDebug = FALSE;
}

HSM_STATE *HSM_GetState(HSM *This)
{
	// This returns the current HSM state
	return This->curState;
}

void HSM_Run(HSM *This, HSM_EVENT event, UINT32 param)
{
	// This runs the state's event handler and forwards unhandled events to
	// the parent state
	HSM_STATE *state = This->curState;
	if(This->hsmDebug)
		DEBUG("Run %s[%s](evt:%d, param:%08x)\n", This->name, state->name, event, param);
	while (event)
	{
		event = state->handler(This, event, param);
		state = state->parent;
		if (This->hsmDebug && event)
			DEBUG("  evt:%d unhandled, passing to %s[%s]\n", event, This->name, state->name);
	}
}

void HSM_Tran(HSM *This, HSM_STATE *nextState, UINT32 param, void (*method)(void))
{
	HSM_STATE *list_exit[HSM_MAX_DEPTH];
	HSM_STATE *list_entry[HSM_MAX_DEPTH];
	UINT8 cnt_exit = 0;
	UINT8 cnt_entry = 0;
	UINT8 idx;
	// This performs the state transition with calls of exit, entry and init
	// Bulk of the work handles the exit and entry event during transitions
	if (This->hsmDebug)
		DEBUG("Tran %s[%s -> %s]\n", This->name, This->curState->name, nextState->name);
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
	//#TODO: Add check to ensure "Tran()" not called on exit
	for (idx = 0; idx < cnt_exit; idx++)
	{
		src = list_exit[idx];
		if (This->hsmDebug)
			DEBUG("  %s[%s](EXIT)\n", This->name, src->name);
		src->handler(This, HSME_EXIT, param);
	}
	// 3) Call the transitional method hook
	if (method)
	{
		method();
	}
	// 4) Process all the entry events
	//#TODO: Add check to ensure "Tran()" not called on entry
	for (idx = 0; idx < cnt_entry; idx++)
	{
		dst = list_entry[cnt_entry - idx - 1];
		if (This->hsmDebug)
			DEBUG("  %s[%s](ENTRY)\n", This->name, dst->name);
		dst->handler(This, HSME_ENTRY, param);
	}
	// 5) Now we can set the destination state
	This->curState = nextState;
	// 6) Invoke INIT signal
	if (This->hsmDebug)
		DEBUG("  %s[%s](INIT)\n", This->name, nextState->name);
	This->curState->handler(This, HSME_INIT, 0);
}
