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

//========== System Headers =====================
#include <stdint.h>

namespace hsm {
//===============================================
//----HSM OPTIONAL FEATURES SECTION[BEGIN]-------
//-----------------------------------------------
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
//-----------------------------------------------
//----HSM OPTIONAL FEATURES SECTION[END]---------
//===============================================

//========== Debug Macros =======================
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
    #define HSM_DEBUGC1(...)
    #define HSM_DEBUGC2(...)
    #define HSM_DEBUGC3(...)
    #define HSM_DEBUG(...)
#endif // HSM_FEATURE_DEBUG_ENABLE

//----Reserved HSM Event definitions-------------
#define HSME_NULL   (Hsm::kNULL)
#define HSME_START  (Hsm::kSTART)
#define HSME_INIT   (Hsm::kINIT)
#define HSME_ENTRY  (Hsm::kENTRY)
#define HSME_EXIT   (Hsm::kEXIT)

//========== Typedef ============================
typedef uint32_t hsm_event_t;

//========== Class Declaration ==================
class Hsm;

/**
 * @brief      This class describes a hsm state and its position in the state
 *             hierarchy.
 *
 * @details    All state objects are instances of HsmState.  Each state object
 *             is assigned a state event handler by using one of two methods.
 *
 *             1) state object is instantiated with a static event handler
 *             method.
 *
 *             2) A functor class is derived from HsmState and the virtual
 *             operator() is overridden with the static event handler method
 */
class HsmState {
    friend class Hsm;
public:
    typedef hsm_event_t (*handler_t)(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam);

private:
    HsmState *pxParent;         // parent state
    handler_t pfnHandler;       // associated event handler for state. Not used if operator() is overloaded
    const char *pcName;         // name of state
    uint8_t ucLevel;            // Depth level of the state

public:
    HsmState();                 // DO NOT USE: This constructor is reserved from the internal root handler

    /**
     * @brief      Constructs a new instance.
     *
     * @param[in]  pcName      Name of state (for debugging)
     * @param[in]  pfnHandler  Pointer to state event handler that implements
     *                         state behavior.  Set this to `nullptr` if this
     *                         constructed by a derived state
     * @param[in]  pxParent    [Optional] Pointer to the parent of state.  If NULL, then
     *                         internal root state is assigned as parent as a catch-all
     */
    HsmState(const char *pcName, handler_t pfnHandler, HsmState *pxParent=nullptr);

    /**
     * @brief        Calls the state's event handler
     *
     * @details      [Optional] The derived class of HsmState shall override
     *               this virtual function to implement the state's event
     *               handling behavior.
     *
     * @note         If this is overridden, then `pfnHandler` assigned to state
     *               constructor is effectively unused is not used.  However it
     *               should be set to nullptr for consistency.
     *
     * @param[inout] pxHsm    Pointer to Hsm object instance
     * @param[in]    xEvent   The event to be processed by the HSM object
     * @param[inout] pvParam  [Optional] Parameter associated with the xEvent
     *
     * @return       0 or NULL - Event has been consumed / handled, otherwise
     *               event is unhandled and forward to the parent state
     */
    virtual hsm_event_t operator()(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam) {
        return pfnHandler(pxHsm, xEvent, pvParam);
    }
};


/**
 * @brief      This is the base class that implements the HSM.  All HSMs shall
 *             derive from this base class.
 *
 * @details    This class provides the framework for implementing a user-defined
 *             HSM.
 */
class Hsm {
    friend class HsmState;

    HsmState *pxCurState;       // Current HSM State

    //----Optional Debug features-----
#if HSM_FEATURE_DEBUG_ENABLE
#ifdef HSM_DEBUG_EVT2STR
    const char *(*pfnEvt2Str)(hsm_event_t xEvent);
#endif // HSM_DEBUG_EVT2STR
    const char *pcName;         // Name of HSM Instance
    const char *pcPrefix;       // Prefix for debugging (e.g. grep)
    uint8_t bmDebugCfg;         // HSM debug configuration flag
    uint8_t bmDebug;            // HSM run-time debug flag
#endif // HSM_FEATURE_DEBUG_ENABLE
#if HSM_FEATURE_SAFETY_CHECK
    bool bIsTran;               // HSM Transition Flag
#endif // HSM_FEATURE_SAFETY_CHECK

    // Common root event handler if no event is consumed
    static hsm_event_t root_handler(Hsm *This, hsm_event_t xEvent, void *pvParam);

public:
    enum
    {
        kNULL   =  0,
        kSTART  =  1,
        kINIT   = -3,
        kENTRY  = -2,
        kEXIT   = -1
    };

    /**
     * @brief      Create the HSM instance.  Required for each instance
     *
     * @details    When the HSM instance is create, Initial state event handler
     *             will receive the HSME_ENTRY and HSME_INIT events when start()
     *             is called.
     *
     * @param[in]  pcName       Name of state machine (for debugging)
     * @param[in]  pxInitState  Initial state of statemachine
     */
    Hsm(const char *pcName, HsmState *pxInitState) :
#if HSM_FEATURE_DEBUG_ENABLE
        pcName{pcName}, pcPrefix{""}, bmDebugCfg{0}, bmDebug{0}, bIsTran{false},
#ifdef HSM_DEBUG_EVT2STR
        pfnEvt2Str{nullptr},
#endif // HSM_DEBUG_EVT2STR
#endif // HSM_FEATURE_DEBUG_ENABLE
        pxCurState{pxInitState}
    {
    }

    /**
     * @brief      Starts the HSM by invoking the HSME_ENTRY and HSME_INIT
     *             events to the Init State
     *
     * @details    [REQUIRED] This must be called in the constructor body of the
     *             derived Hsm class.  This is necessary because during the base
     *             Hsm object constructor is called before the states objects
     *             are constructed.
     */
    void start(void);

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
     * @param[in]  pxNextState  Pointer to next HSM STATE
     * @param[in]  pvParam      Optional Parameter associated with HSME_ENTRY
     *                          and HSME_EXIT event
     * @param[in]  method       Optional function hook between the HSME_ENTRY
     *                          and HSME_EXIT event handling
     */
    void tran(HsmState *pxNextState, void *pvParam=nullptr, void (*method)(Hsm *This, void *pvParam)=nullptr);

#if HSM_FEATURE_DEBUG_ENABLE
    /**
     * @brief      Get the current HSM debugging state
     *
     * @return     The debug.
     */
    uint8_t getDebug(void) { return bmDebugCfg; }

    /**
     * @brief      Sets the HSM debugging state for output
     *
     * @param[in]  bmEnable  bitmask of debug options (e.g. HSM_SHOW_ALL)
     */
    void setDebug(uint8_t bmEnable) { bmDebugCfg = bmDebug = bmEnable; }

    /**
     * @brief      Sets a prefix to the debug messages
     *
     * @param[in]  prefix  The prefix string
     */
    void setPrefix(const char *prefix) { pcPrefix = prefix; }

    /**
     * @brief      Suppress debug messages for a single call of HSM run (e.g.
     *             frequent timer events)
     *
     * @param[in]  bmEnable  bitmask of debug options (e.g. HSM_SHOW_ALL)
     */
    void supress(uint8_t bmEnable) { bmDebug = bmDebugCfg & ~(bmEnable); }

#ifdef HSM_DEBUG_EVT2STR
    /**
     * @brief      Sets a custom function that returns a human readable string
     *             from HSM event
     *
     * @param[in]  evt2StrFn  The event 2 string function
     */
    void setEvt2Str(const char *(*evt2StrFn)(hsm_event_t xEvent) = nullptr) { pfnEvt2Str = evt2StrFn; }
#endif // HSM_DEBUG_EVT2STR
#endif // HSM_FEATURE_DEBUG_ENABLE
};

} // namespace hsm
#endif // __HSM_HPP__
