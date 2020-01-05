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
    // Use this macro to set a custom function that returns a human readable string from HSM event per HSM
    #define HSM_SET_EVT2STR(hsm, evt2StrFn) { (hsm)->pfnEvt2Str = (evt2StrFn); }
    // Use this macro to changing the pcPrefix for that object
    #define HSM_SET_PREFIX(hsm, preFix) { (hsm)->pcPrefix = (preFix); }
    // Use this macro to Enable/Disable HSM debugging for that object
    #define HSM_SET_DEBUG(hsm, bmEnable) { (hsm)->bmDebugCfg = (hsm)->bmDebug = (bmEnable); }
    // Use this macro to get the current HSM debugging state for that object
    #define HSM_GET_DEBUG(hsm) ((hsm)->bmDebugCfg)
    // Use this macro to supress debug messages for a single call of HSM_Run (e.g. frequent timer events)
    #define HSM_SUPPRESS_DEBUG(hsm, bmEnable) { (hsm)->bmDebug = (hsm)->bmDebugCfg & ~(bmEnable); }
    // Below are the DEBUG options for HSM_SET_DEBUG(), HSM_SUPPRESS_DEBUG()
    #define HSM_SHOW_RUN                (1)
    #define HSM_SHOW_TRAN               (2)
    #define HSM_SHOW_INTACT             (4)
    #define HSM_SHOW_ALL                (HSM_SHOW_RUN | HSM_SHOW_TRAN | HSM_SHOW_INTACT)

    #if HSM_FEATURE_DEBUG_EMBEDDED
        // This section maybe customized for platform specific debug facilities
        // #include "your_embedded_DEBUG_OUT_here.h"
        #if HSM_FEATURE_DEBUG_NESTED_CALL
            #define HSM_DEBUGC1(x, ...) { if (This->bmDebug & HSM_SHOW_RUN)    DEBUG_OUT(HSM_COLOR_BLU "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], This->pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC2(x, ...) { if (This->bmDebug & HSM_SHOW_TRAN)   DEBUG_OUT(HSM_COLOR_CYN "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], This->pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC3(x, ...) { if (This->bmDebug & HSM_SHOW_INTACT) DEBUG_OUT(HSM_COLOR_CYN "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], This->pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUG(...)  { DEBUG_OUT(__VA_ARGS__); }
        #else
            #define HSM_DEBUGC1(x, ...) { if (This->bmDebug & HSM_SHOW_RUN)    DEBUG_OUT(HSM_COLOR_BLU "%s" x HSM_COLOR_NON HSM_NEWLINE, This->pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC2(x, ...) { if (This->bmDebug & HSM_SHOW_TRAN)   DEBUG_OUT(HSM_COLOR_CYN "%s" x HSM_COLOR_NON HSM_NEWLINE, This->pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC3(x, ...) { if (This->bmDebug & HSM_SHOW_INTACT) DEBUG_OUT(HSM_COLOR_CYN "%s" x HSM_COLOR_NON HSM_NEWLINE, This->pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUG(...)  { DEBUG_OUT(__VA_ARGS__); }
        #endif // HSM_FEATURE_DEBUG_NESTED_CALL
    #else
        // Using printf for DEBUG
        #include <stdio.h>
        #if HSM_FEATURE_DEBUG_NESTED_CALL
            #define HSM_DEBUGC1(x, ...) { if (This->bmDebug & HSM_SHOW_RUN)    printf(HSM_COLOR_BLU "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], This->pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC2(x, ...) { if (This->bmDebug & HSM_SHOW_TRAN)   printf(HSM_COLOR_CYN "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], This->pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC3(x, ...) { if (This->bmDebug & HSM_SHOW_INTACT) printf(HSM_COLOR_CYN "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], This->pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUG(x, ...)  { printf(x HSM_NEWLINE, __VA_ARGS__); }
        #else
            #define HSM_DEBUGC1(x, ...) { if (This->bmDebug & HSM_SHOW_RUN)    printf(HSM_COLOR_BLU "%s" x HSM_COLOR_NON HSM_NEWLINE, This->pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC2(x, ...) { if (This->bmDebug & HSM_SHOW_TRAN)   printf(HSM_COLOR_CYN "%s" x HSM_COLOR_NON HSM_NEWLINE, This->pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC3(x, ...) { if (This->bmDebug & HSM_SHOW_INTACT) printf(HSM_COLOR_CYN "%s" x HSM_COLOR_NON HSM_NEWLINE, This->pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUG(x, ...)  { printf(x HSM_NEWLINE, __VA_ARGS__); }
        #endif // HSM_FEATURE_DEBUG_NESTED_CALL
    #endif // HSM_FEATURE_DEBUG_EMBEDDED
#else
    #define HSM_SET_EVT2STR(hsm, evt2StrHook)
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
    HSM_STATE *pxParent;                            // parent state
    HSM_FN pfnHandler;                              // associated event handler for state
    const char *pcName;                             // name of state
    uint8_t ucLevel;                                // depth level of the state
};

struct HSM_T
{
    HSM_STATE *pxCurState;                          // Current HSM State
#if HSM_FEATURE_DEBUG_ENABLE
#ifdef HSM_DEBUG_EVT2STR
    const char *(*pfnEvt2Str)(HSM_EVENT xEvent);    // Hook to convert event to string
#endif // HSM_DEBUG_EVT2STR
    const char *pcName;                             // Name of HSM Machine
    const char *pcPrefix;                           // Prefix for debugging (e.g. grep)
    uint8_t bmDebugCfg;                             // HSM debug configuration flag
    uint8_t bmDebug;                                // HSM run-time debug flag
#endif // HSM_FEATURE_DEBUG_ENABLE
#if HSM_FEATURE_SAFETY_CHECK
    uint8_t bIsTran;                                // HSM Transition Flag
#endif // HSM_FEATURE_SAFETY_CHECK
};

//---- External Globals----
#if HSM_FEATURE_DEBUG_NESTED_CALL
extern uint8_t gucHsmNestLevel;
extern const char * const apucHsmNestIndent[];
#endif // HSM_FEATURE_DEBUG_NESTED_CALL

//----Function Declarations----

/**
 * @brief        Create an HSM STATE object
 *
 * @param[inout] This        Pointer to HSM_STATE object
 * @param[in]    pcName      Name of state (for debugging)
 * @param[in]    pfnHandler  Pointer to state event handler that implements
 *                           state behavior
 * @param[in]    pxParent    Pointer to Parent of state, If NULL, then internal
 *                           ROOT handler is used as catch-all
 */
void HSM_STATE_Create(HSM_STATE *This, const char *pcName, HSM_FN pfnHandler, HSM_STATE *pxParent);

/**
 * @brief        Create the HSM instance.  Required for each instance
 *
 * @details      When the HSM instance is create, Initial state event handler
 *               will receive the HSME_ENTRY and HSME_INIT events.
 *
 * @param[inout] This         Pointer to HSM object
 * @param[in]    pcName       Name of state machine (for debugging)
 * @param[in]    pxInitState  Initial state of statemachine
 */
void HSM_Create(HSM *This, const char *pcName, HSM_STATE *pxInitState);

/**
 * @brief      Get the current HSM STATE
 *
 * @param[in]  This  Pointer to HSM object
 *
 * @return     Pointer to current HSM State
 */
HSM_STATE *HSM_GetState(HSM *This);

/**
 * @brief      Tests whether HSM is in state or parent state
 *
 * @param[in]  This     Pointer to HSM object
 * @param[in]  pxState  Pointer to HSM_STATE to test against current state
 *
 * @return     1 - HSM instance is in state or parent state, 0 - otherwise
 */
uint8_t HSM_IsInState(HSM *This, HSM_STATE *pxState);

/**
 * @brief      Run the HSM with event
 *
 * @details    The event and parameter is passed to the current event handler.
 *             If the event is not consumed by current state, then it is passed
 *             to the parent until the event is consumed / handled.
 *
 * @param[in]  This     Pointer to HSM object
 * @param[in]  xEvent   HSM_EVENT processed by HSM object
 * @param[in]  pvParam  [Optional] Parameter associated with HSM_EVENT
 */
void HSM_Run(HSM *This, HSM_EVENT xEvent, void *pvParam);

/**
 * @brief        Transition to another HSM STATE
 *
 * @details      On transition, the Lowest Common Ancestor (LCA) is calculated.
 *               HSME_EXIT events are sent to the event handlers of the current
 *               state to LCA.  The optional method if passed is called for
 *               special handling.  Then HSME_ENTRY events are sent to the event
 *               handlers of LCA to next state.  Finally HSME_INIT is called to
 *               the next state handler.
 *
 * @note         It is illegal to call HSM_Tran during handling of HSME_ENTRY or
 *               HSME_EXIT.  But it is ok to call HSM_Tran during the handling
 *               of HSME_INIT.
 *
 * @param[inout] This         Pointer to HSM object
 * @param[in]    pxNextState  Pointer to next HSM STATE
 * @param[in]    pvParam      Optional Parameter associated with HSME_ENTRY and
 *                            HSME_EXIT event
 * @param[in]    method       Optional function hook between the HSME_ENTRY and
 *                            HSME_EXIT event handling
 */
void HSM_Tran(HSM *This, HSM_STATE *pxNextState, void *pvParam, void (*method)(HSM *This, void *pvParam));

#ifdef __cplusplus
}
#endif

#endif // __HSM_H__
