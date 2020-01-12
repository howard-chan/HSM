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
#ifndef __HSM_HPP__
#define __HSM_HPP__

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
    #define HSM_FEATURE_DEBUG_NESTED_CALL   0
    // Sets the newline for host (i.e. linux - "\n", windows - "\r\n")
    #define HSM_NEWLINE                     "\n"
// Enable safety checks.  Can be disabled after validating all states
#define HSM_FEATURE_SAFETY_CHECK            1
// Enable HSME_INIT Handling.  Can be disabled if no states handles HSME_INIT
#define HSM_FEATURE_INIT                    1
//----HSM OPTIONAL FEATURES SECTION[END]----

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
            #define HSM_DEBUGC1(x, ...) { if (bmDebug & HSM_SHOW_RUN)    DEBUG_OUT(HSM_COLOR_BLU "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC2(x, ...) { if (bmDebug & HSM_SHOW_TRAN)   DEBUG_OUT(HSM_COLOR_CYN "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC3(x, ...) { if (bmDebug & HSM_SHOW_INTACT) DEBUG_OUT(HSM_COLOR_CYN "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUG(...)  { DEBUG_OUT(__VA_ARGS__); }
        #else
            #define HSM_DEBUGC1(x, ...) { if (bmDebug & HSM_SHOW_RUN)    DEBUG_OUT(HSM_COLOR_BLU "%s" x HSM_COLOR_NON HSM_NEWLINE, pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC2(x, ...) { if (bmDebug & HSM_SHOW_TRAN)   DEBUG_OUT(HSM_COLOR_CYN "%s" x HSM_COLOR_NON HSM_NEWLINE, pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC3(x, ...) { if (bmDebug & HSM_SHOW_INTACT) DEBUG_OUT(HSM_COLOR_CYN "%s" x HSM_COLOR_NON HSM_NEWLINE, pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUG(...)  { DEBUG_OUT(__VA_ARGS__); }
        #endif // HSM_FEATURE_DEBUG_NESTED_CALL
    #else
        // Using printf for DEBUG
        #include <stdio.h>
        #if HSM_FEATURE_DEBUG_NESTED_CALL
            #define HSM_DEBUGC1(x, ...) { if (bmDebug & HSM_SHOW_RUN)    printf(HSM_COLOR_BLU "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC2(x, ...) { if (bmDebug & HSM_SHOW_TRAN)   printf(HSM_COLOR_CYN "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC3(x, ...) { if (bmDebug & HSM_SHOW_INTACT) printf(HSM_COLOR_CYN "%s%s" x HSM_COLOR_NON HSM_NEWLINE, apucHsmNestIndent[gucHsmNestLevel], pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUG(x, ...)  { printf(x HSM_NEWLINE, __VA_ARGS__); }
        #else
            #define HSM_DEBUGC1(x, ...) { if (bmDebug & HSM_SHOW_RUN)    printf(HSM_COLOR_BLU "%s" x HSM_COLOR_NON HSM_NEWLINE, pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC2(x, ...) { if (bmDebug & HSM_SHOW_TRAN)   printf(HSM_COLOR_CYN "%s" x HSM_COLOR_NON HSM_NEWLINE, pcPrefix, __VA_ARGS__); }
            #define HSM_DEBUGC3(x, ...) { if (bmDebug & HSM_SHOW_INTACT) printf(HSM_COLOR_CYN "%s" x HSM_COLOR_NON HSM_NEWLINE, pcPrefix, __VA_ARGS__); }
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


//----State definitions----
// TODO: Replace with enum
#define HSME_NULL   0
#define HSME_START  1
#define HSME_INIT   (-3)
#define HSME_ENTRY  (-2)
#define HSME_EXIT   (-1)

enum class HsmEvent
{
    kNULL = 0,
    kSTART = 1,
    kINIT = -3,
    kENTRY = -2,
    kEXIT = -1
};


typedef uint32_t hsm_event_t;

class Hsm;

class HsmState {
    friend class Hsm;
public:
    typedef hsm_event_t (*handler_t)(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam);
private:
    HsmState *pxParent;         // parent state
    const char *pcName;         // name of state
    handler_t pfnHandler;       // State Handler
    uint8_t ucLevel;            // Depth level of the state
public:
    HsmState();
    HsmState(const char *pcName, handler_t pfnHandler, HsmState *pxParent=nullptr);
    virtual hsm_event_t operator()(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam) {
        return pfnHandler(pxHsm, xEvent, pvParam);
    }
};


class Hsm {
    friend class HsmState;
    HsmState *pxCurState;       // Current HSM State
    // Debug features
    const char *pcName;         // Name of HSM Instance
    const char *pcPrefix;       // Prefix for debugging (e.g. grep)
    uint8_t bmDebugCfg;         // HSM debug configuration flag
    uint8_t bmDebug;            // HSM run-time debug flag
    bool bIsTran;               // HSM Transition Flag
    // Methods
    static hsm_event_t root_handler(Hsm *This, hsm_event_t xEvent, void *pvParam);

public:
    /**
     * @brief      Create the HSM instance.  Required for each instance
     *
     * @details    When the HSM instance is create, Initial state event handler
     *             will receive the HSME_ENTRY and HSME_INIT events.
     *
     * @param[in]  pcName       Name of state machine (for debugging)
     * @param[in]  pxInitState  Initial state of statemachine
     */
    Hsm(const char *pcName="Hsm", HsmState *pxInitState=nullptr)
        : pxCurState{pxInitState}, pcName{pcName}, pcPrefix{""}, bmDebugCfg{0}, bmDebug{0}, bIsTran{false}
    {
        HSM_DEBUGC1("  %s[%s](ENTRY)", pcName, pxInitState->pcName);
        // (*pxCurState)(this, HSME_ENTRY, 0);
#if HSM_FEATURE_INIT
        HSM_DEBUGC1("  %s[%s](INIT)", pcName, pxInitState->pcName);
        // (*pxCurState)(this, HSME_INIT, 0);
#endif // HSM_FEATURE_INIT

        // // https://www.giannistsakiris.com/2012/09/07/c-calling-a-member-function-pointer/
        // (this->*((Hsm*)this)->Hsm::pxCurState->HsmState::pfnHandler) (HSME_ENTRY, 0);
        // (this->*((Hsm*)this)->Hsm::pxCurState->HsmState::pfnHandler) (HSME_INIT, 0);
    }

    void setInitState(HsmState *pxInitState);

    /**
     * @brief      Sets the HSM debug output
     *
     * @param[in]  bmEnable  bitmask of debug options
     */
    void setDebug(uint8_t bmEnable) { bmDebugCfg = bmDebug = bmEnable; }

    /**
     * @brief      Adds a prefix to debug messages
     *
     * @param[in]  prefix  The prefix string
     */
    void setPrefix(const char *prefix) { pcPrefix = prefix; }

    /**
     * @brief      Get the current HSM STATE
     *
     * @return     Pointer to current HSM State
     */
    HsmState *getState(void) { return pxCurState; };

    /**
     * @brief      Tests whether HSM is in state or parent state
     *
     * @param[in]  pxState  Pointer to State to test against current state
     *
     * @return     true - HSM instance is in state or parent state, false -
     *             otherwise
     */
    bool isInState(HsmState *pxState);

    /**
     * @brief      Run the HSM with event
     *
     * @details    The event and parameter is passed to the current event
     *             handler. If the event is not consumed by current state, then
     *             it is passed to the parent until the event is consumed /
     *             handled.
     *
     * @param[in]  xEvent   HSM_EVENT processed by HSM object
     * @param[in]  pvParam  [Optional] Parameter associated with HSM_EVENT
     */
    void operator()(hsm_event_t xEvent, void *pvParam=nullptr);
    inline void run(hsm_event_t xEvent, void *pvParam=nullptr) {
        (*this)(xEvent, pvParam);
    }

    /**
     * @brief      Transition to another HSM STATE
     *
     * @details    On transition, the Lowest Common Ancestor (LCA) is
     *             calculated. HSME_EXIT events are sent to the event handlers
     *             of the current state to LCA.  The optional method if passed
     *             is called for special handling.  Then HSME_ENTRY events are
     *             sent to the event handlers of LCA to next state.  Finally
     *             HSME_INIT is called to the next state handler.
     *
     * @note       It is illegal to call HSM_Tran during handling of HSME_ENTRY
     *             or HSME_EXIT.  But it is ok to call HSM_Tran during the
     *             handling of HSME_INIT.
     *
     * @param      nextState  The next state
     * @param      param      The parameter
     * @param[in]  method     The method
     */
    void tran(HsmState *pxNextState, void *pvParam=nullptr, void (*method)(Hsm *This, void *pvParam)=nullptr);
#if 0
    void tran(HsmState *nextState) {
        printf("  Tran %s[%s]->[%s]:\n", pcName, pxCurState->pcName, nextState->pcName);
        pxCurState = nextState;
    }
    void tran(HsmState& nextState) {
        printf("  Tran %s[%s]->[%s]:\n", pcName, pxCurState->pcName, nextState.pcName);
        pxCurState = &nextState;
    }
#endif
};
#endif // __HSM_HPP__
