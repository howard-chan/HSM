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
#ifndef __HSM_H__
#define __HSM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Add platform specific types here */
#include <stdint.h>

//----HSM OPTIONAL FEATURES SECTION[BEGIN]----
// Enable for HSM debugging
#define HSM_FEATURE_DEBUG_ENABLE            1
    // If HSM_FEATURE_DEBUG_ENABLE is defined, then select DEBUG OUT type
    //#define HSM_FEATURE_DEBUG_EMBEDDED
    // If HSM_FEATURE_DEBUG_ENABLE is defined, you can define HSM_DEBUG_EVT2STR for custom "event to string" function
    // For example:
    //     a) You can define here: #define HSM_DEBUG_EVT2STR(x) (sprintf(This->buffer, "0x%lx", (unsigned long)(x)), This->buffer)
    // or
    //     b) Supply your own function of type "const char *HSM_Evt2Str(uint32_t event)" and then define in a makefile
    //        (e.g. for gcc: "-DHSM_DEBUG_EVT2STR=HSM_Evt2Str")
    extern const char *HSM_Evt2Str(uint32_t event);
    // If HSM_FEATURE_DEBUG_ENABLE is defined, you can define HSM_FEATURE_DEBUG_COLOR to enable color print for color-aware terminals
    #define HSM_FEATURE_DEBUG_COLOR         1
    // If HSM_FEATURE_DEBUG_ENABLE is defined, debug messages are indented for each nested HSM_Run() call in a single threaded system
    #define HSM_FEATURE_DEBUG_NESTED_CALL   1
    // Sets the newline for host (i.e. linux - "\n", windows - "\r\n")
    #define HSM_NEWLINE                     "\n"
// Enable safety checks.  Can be disabled after validating all states
#define HSM_FEATURE_SAFETY_CHECK            1
// Enable HSME_INIT Handling.  Can be disabled if no states handles HSME_INIT
#define HSM_FEATURE_INIT                    1
//----HSM OPTIONAL FEATURES SECTION[END]----

// Set the maximum nested levels
#define HSM_MAX_DEPTH 5

//----State definitions----
#define HSME_NULL   0
#define HSME_START  1
#define HSME_INIT   ((HSM_EVENT)(-3))
#define HSME_ENTRY  ((HSM_EVENT)(-2))
#define HSME_EXIT   ((HSM_EVENT)(-1))

//----Debug Macros----
#if HSM_FEATURE_DEBUG_ENABLE
    // Terminal Colors
    #if HSM_FEATURE_DEBUG_COLOR
        #define HSM_COLOR_RED           "\033[1;31m"
        #define HSM_COLOR_GRN           "\033[1;32m"
        #define HSM_COLOR_YEL           "\033[1;33m"
        #define HSM_COLOR_BLU           "\033[1;34m"
        #define HSM_COLOR_MAG           "\033[1;35m"
        #define HSM_COLOR_CYN           "\033[1;36m"
        #define HSM_COLOR_NON           "\033[0m"
    #else
        #define HSM_COLOR_RED
        #define HSM_COLOR_GRN
        #define HSM_COLOR_YEL
        #define HSM_COLOR_BLU
        #define HSM_COLOR_MAG
        #define HSM_COLOR_CYN
        #define HSM_COLOR_NON
    #endif // HSM_FEATURE_DEBUG_COLOR
    // Use this macro to changing the prefix for that object
    #define HSM_SET_PREFIX(hsm, preFix) { (hsm)->prefix =   (preFix); }
    // Use this macro to Enable/Disable HSM debugging for that object
    #define HSM_SET_DEBUG(hsm, bmEnable) { (hsm)->hsmDebugCfg = (hsm)->hsmDebug = (bmEnable); }
    // Use this macro to get the current HSM debugging state for that object
    #define HSM_GET_DEBUG(hsm) ((hsm)->hsmDebugCfg)
    // Use this macro to supress debug messages for a single call of HSM_Run (e.g. frequent timer events)
    #define HSM_SUPPRESS_DEBUG(hsm, bmEnable) { (hsm)->hsmDebug = (hsm)->hsmDebugCfg & ~(bmEnable); }
    // Below are the DEBUG options for HSM_SET_DEBUG(), HSM_SUPPRESS_DEBUG()
    #define HSM_SHOW_RUN                (1)
    #define HSM_SHOW_TRAN               (2)
    #define HSM_SHOW_INTACT             (4)
    #define HSM_SHOW_ALL                (HSM_SHOW_RUN | HSM_SHOW_TRAN | HSM_SHOW_INTACT)

    #if HSM_FEATURE_DEBUG_EMBEDDED
        // This section maybe customized for platform specific debug facilities
        // #include "your_embedded_DEBUG_OUT_here.h"
        #if HSM_FEATURE_DEBUG_NESTED_CALL
            #define HSM_DEBUGC1(x, ...) { if (This->hsmDebug & HSM_SHOW_RUN)    DEBUG_OUT(HSM_COLOR_BLU "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], This->prefix, __VA_ARGS__); }
            #define HSM_DEBUGC2(x, ...) { if (This->hsmDebug & HSM_SHOW_TRAN)   DEBUG_OUT(HSM_COLOR_CYN "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], This->prefix, __VA_ARGS__); }
            #define HSM_DEBUGC3(x, ...) { if (This->hsmDebug & HSM_SHOW_INTACT) DEBUG_OUT(HSM_COLOR_CYN "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], This->prefix, __VA_ARGS__); }
            #define HSM_DEBUG(...)  { DEBUG_OUT(__VA_ARGS__); }
        #else
            #define HSM_DEBUGC1(x, ...) { if (This->hsmDebug & HSM_SHOW_RUN)    DEBUG_OUT(HSM_COLOR_BLU "%s" x HSM_COLOR_NON HSM_NEWLINE, This->prefix, __VA_ARGS__); }
            #define HSM_DEBUGC2(x, ...) { if (This->hsmDebug & HSM_SHOW_TRAN)   DEBUG_OUT(HSM_COLOR_CYN "%s" x HSM_COLOR_NON HSM_NEWLINE, This->prefix, __VA_ARGS__); }
            #define HSM_DEBUGC3(x, ...) { if (This->hsmDebug & HSM_SHOW_INTACT) DEBUG_OUT(HSM_COLOR_CYN "%s" x HSM_COLOR_NON HSM_NEWLINE, This->prefix, __VA_ARGS__); }
            #define HSM_DEBUG(...)  { DEBUG_OUT(__VA_ARGS__); }
        #endif // HSM_FEATURE_DEBUG_NESTED_CALL
    #else
        // Using printf for DEBUG
        #include <stdio.h>
        #if HSM_FEATURE_DEBUG_NESTED_CALL
            #define HSM_DEBUGC1(x, ...) { if (This->hsmDebug & HSM_SHOW_RUN)    printf(HSM_COLOR_BLU "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], This->prefix, __VA_ARGS__); }
            #define HSM_DEBUGC2(x, ...) { if (This->hsmDebug & HSM_SHOW_TRAN)   printf(HSM_COLOR_CYN "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], This->prefix, __VA_ARGS__); }
            #define HSM_DEBUGC3(x, ...) { if (This->hsmDebug & HSM_SHOW_INTACT) printf(HSM_COLOR_CYN "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], This->prefix, __VA_ARGS__); }
            #define HSM_DEBUG(x, ...)  { printf(x HSM_NEWLINE, __VA_ARGS__); }
        #else
            #define HSM_DEBUGC1(x, ...) { if (This->hsmDebug & HSM_SHOW_RUN)    printf(HSM_COLOR_BLU "%s" x HSM_COLOR_NON HSM_NEWLINE, This->prefix, __VA_ARGS__); }
            #define HSM_DEBUGC2(x, ...) { if (This->hsmDebug & HSM_SHOW_TRAN)   printf(HSM_COLOR_CYN "%s" x HSM_COLOR_NON HSM_NEWLINE, This->prefix, __VA_ARGS__); }
            #define HSM_DEBUGC3(x, ...) { if (This->hsmDebug & HSM_SHOW_INTACT) printf(HSM_COLOR_CYN "%s" x HSM_COLOR_NON HSM_NEWLINE, This->prefix, __VA_ARGS__); }
            #define HSM_DEBUG(x, ...)  { printf(x HSM_NEWLINE, __VA_ARGS__); }
        #endif // HSM_FEATURE_DEBUG_NESTED_CALL
    #endif // HSM_FEATURE_DEBUG_EMBEDDED
#else
    #define HSM_SET_PREFIX(hsm, preFix)
    #define HSM_SET_DEBUG(hsm, bEnable)
    #define HSM_GET_DEBUG(hsm)  (0)
    #define HSM_SUPPRESS_DEBUG(hsm, bmEnable)
    #define HSM_DEBUGC1(...)
    #define HSM_DEBUGC2(...)
    #define HSM_DEBUGC3(...)
    #define HSM_DEBUG(...)
#endif // HSM_FEATURE_DEBUG_ENABLE

//----Structure declaration----
typedef uint32_t HSM_EVENT;
typedef struct HSM_STATE_T HSM_STATE;
typedef struct HSM_T HSM;

typedef HSM_EVENT (* HSM_FN)(HSM *This, HSM_EVENT event, void *param);

struct HSM_STATE_T
{
    HSM_STATE *parent;          // parent state
    HSM_FN handler;             // associated event handler for state
    const char *name;           // name of state
    uint8_t level;              // depth level of the state
};

struct HSM_T
{
    HSM_STATE *curState;        // Current HSM State
#if HSM_FEATURE_DEBUG_ENABLE
    const char *name;           // Name of HSM Machine
    const char *prefix;         // Prefix for debugging (e.g. grep)
    uint8_t hsmDebugCfg;        // HSM debug configuration flag
    uint8_t hsmDebug;           // HSM run-time debug flag
#endif // HSM_FEATURE_DEBUG_ENABLE
#if HSM_FEATURE_SAFETY_CHECK
    uint8_t hsmTran;            // HSM Transition Flag
#endif // HSM_FEATURE_SAFETY_CHECK
};

//---- External Globals----
#if HSM_FEATURE_DEBUG_NESTED_CALL
extern uint8_t gucHsmNestLevel;
extern const char * const apucHsmNestIndent[];
#endif // HSM_FEATURE_DEBUG_NESTED_CALL

//----Function Declarations----
// Func: void HSM_STATE_Create(HSM_STATE *This, const char *name, HSM_FN handler, HSM_STATE *parent)
// Desc: Create an HSM State for the HSM.
// This: Pointer to HSM_STATE object
// name: Name of state (for debugging)
// handler: State Event Handler
// parent: Parent state.  If NULL, then internal ROOT handler is used as catch-all
void HSM_STATE_Create(HSM_STATE *This, const char *name, HSM_FN handler, HSM_STATE *parent);

// Func: void HSM_Create(HSM *This, const char *name, HSM_STATE *initState)
// Desc: Create the HSM instance.  Required for each instance
// name: Name of state machine (for debugging)
// initState: Initial state of statemachine
void HSM_Create(HSM *This, const char *name, HSM_STATE *initState);

// Func: HSM_STATE *HSM_GetState(HSM *This)
// Desc: Get the current HSM STATE
// This: Pointer to HSM instance
// return|HSM_STATE *: Pointer to HSM STATE
HSM_STATE *HSM_GetState(HSM *This);

// Func: uint8_t HSM_IsInState(HSM *This, HSM_STATE *state)
// Desc: Tests whether HSM is in state or parent state
// This: Pointer to HSM instance
// state: Pointer to HSM_STATE to test
// return|uint8_t: 1 - HSM instance is in state or parent state, 0 - otherwise
uint8_t HSM_IsInState(HSM *This, HSM_STATE *state);

// Func: void HSM_Run(HSM *This, HSM_EVENT event, void *param)
// Desc: Run the HSM with event
// This: Pointer to HSM instance
// event: HSM_EVENT processed by HSM
// param: Parameter associated with HSM_EVENT
void HSM_Run(HSM *This, HSM_EVENT event, void *param);

// Func: void HSM_Tran(HSM *This, HSM_STATE *nextState, void *param, void (*method)(HSM *This, void *param))
// Desc: Transition to another HSM STATE
// This: Pointer to HSM instance
// nextState: Pointer to next HSM STATE
// param: Optional Parameter associated with HSME_ENTRY and HSME_EXIT event
// method: Optional function hook between the HSME_ENTRY and HSME_EXIT event handling
void HSM_Tran(HSM *This, HSM_STATE *nextState, void *param, void (*method)(HSM *This, void *param));

#ifdef __cplusplus
}
#endif

#endif // __HSM_H__
