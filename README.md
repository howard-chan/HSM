/*
The MIT License (MIT)

Copyright (c) 2015-2017 Howard Chan
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

================================
HSM - Hierarchical State Machine
================================
1. Overview
2. Using HSM Framework
    2.1. Define the events and states
    2.2: Implement each of the state's event handler
    2.3: Create the HSM states and HSM instances
    2.4: Embed the HSM_Run() into your application
3. Using HSM Advance Features
    3.1. Debug Features
    3.2. HSME_INIT
    3.3. Embedding event strings
    3.4. Concurrent model
    3.5. Child state as a filter


1. Overview:
============
Generally a state machine is a "black box" whose output is driven by an input "event" and its internal state.  Another way to look at a state machine is that of an event filter which will only act on "events" relevant to its internal state.  To Illustrate this concept, we would like to design an LED controlled by a "MODE" button that toggles between "Off" -> "On" -> "Blinking" and back to "Off".  The following flat state machine diagram is an example implementation of this LED.

            +-------------------------+
    +------>|          Off            |<----O
    |       +-------------------------+
    |                   |
    |              MODE / LedOn()
    |                   V
    |       +-------------------------+
    |       |          On             |
    |       +-------------------------+
    |                   |
MODE / LedOff()    MODE / LedOff()
    |                   V
    |       +-------------------------+
    +-------|      BlinkingOff        |
    |        +------------------------+
    |            ^                 |
    |    TIMER / LedOff()    TIMER / LedOn()
    |            |                 V
    |       +-------------------------+
    +-------|      BlinkingOn         |
            +-------------------------+

In this state machine, there are 2 events that are generated ("TIMER" and "MODE").  TIMER is a periodic event, while MODE is asynchronous event from the user.  In all states, the "MODE" event is handled and causes a transition that sets the LED output.  However the "TIMER" event is only handled in the "BlinkingOff" and "BlinkingOn" states, while ignored (i.e. filtered) in the "Off" and "On" state.

Most state machines are flat (i.e. non-hierarchical) which is simple and easy to understand and implement.  When modeling / documenting the state machine on paper, it becomes increasing complex as the number of states and transitions increases.  At the same time, a large number of refactoring opportunities become apparent which is hard to realize in a flat state machine.  For example:
    1) Multiple states handling same event - Some states will only differ slight but still needs to handle the same event in the same way.
    2) Large number of transitions - Similar to 1), an event resulting in a transition to a common state will also result in duplicating the handler.
    3) Setup and Teardown - Managing a setup and teardown is normally handled in the transition event.  This becomes difficult to maintain as the number of states and transitions increase
    4) Unhandled events - States will normally handle explicit events it was designed for.  These unexpected events can indicate a bug or misbehavior

Fortunately using a Hierarchical State Machine Model addresses the issues above:
    1) Parent state - Common event handling in multiple states can be moved to the parent state.  Where the child state only handles differences in handling of the event.  Similar to Object-oriented practices.
    2) Parent Transition - Similar to 1), The parent state can manage common transitions out of the child state to a common state
    3) HSME_ENTRY and HSME_EXIT - These are built-in events that handle the Setup and Teardown events in a transition going from and into a state.
    4) Root Handler - All unhandled events get routed to the root handler which become detected.

Rather than describe the HSM here, there are far better references and discussions on Unified Modeling Language (UML) and HSM that can be found on the internet.  So instead the next section shall describe how to use the HSM framework and how to implement / map your state machine model into code.


2. Using HSM Framework:
=======================
After drawing your HSM model using the UML syntax, you have 4 basic steps:
    1) Define the events and states
    2) Implement each of the state's event handler
    3) Create the HSM states and HSM instances
    4) Embed the HSM_Run() into your application

To simply the discussion of the HSM framework, I shall refer to the following example HSM state diagram that models a simple point and shoot camera.

    +-------------------------+
    |           Off           |
    |-------------------------|<------@
    | entry / EnterLowPower() |
    | exit / ExitLowPower()   |
    +-------------------------+
        |               ^
       PWR             PWR
        V               |
    +-----------------------------------------------------------------------------+
    |                                    On                                       |
    |-----------------------------------------------------------------------------|
    | entry / OpenLens()                                                          |
    | exit / CloseLens()                                                          |
    | LOWBATT / BeepLowBattWarning()                                              |
    |                                                                             |
    |               @                  +----------------------------------------+ |
    |               |                  |                OnDisp                  | |
    |               V                  |----------------------------------------| |
    |  +-------------------------+     | entry / TurnOnLCD()                    | |
    |  |         OnShoot         |     | exit / TurnOffLCD()                    | |
    |  |-------------------------|     |                                        | |
    |  | entry / EnableSensor()  |     |          +---------------------------+ | |
    |  | exit / DisableSensor()  |------MODE----->|         OnDispPlay        | | |
    |  | RELEASE / TakePicture() |     |          |---------------------------| | |
    |  +-------------------------+     |          | entry / DisplayPicture()  | | |
    |               ^                  |          +---------------------------+ | |
    |               |                  |                        |               | |
    |               |                  |                      MODE              | |
    |               |                  |                        V               | |
    |               |                  |          +---------------------------+ | |
    |               |                  |          |        OnDispMenu         | | |
    |               +----------MODE---------------|---------------------------| | |
    |                                  |          | entry / DisplayMenu()     | | |
    |                                  |          +---------------------------+ | |
    |                                  |                                        | |
    |                                  +----------------------------------------+ |
    |                                                                             |
    +-----------------------------------------------------------------------------+

This camera features 3 user button events (PWR, RELEASE, MODE) and a LOWBATT event generated by hardware.  The following describes the events:
    PWR: This event can happen at anytime and must be handled in all states
    RELEASE: This shutter release event is only handled when the camera is in the picture shoot mode
    MODE: This event is handled only in the ON state of the camera and used to toggle the camera mode operation
    LOWBATT: This event can be generated at anytime while the camera is on

The camera is implemented using 6 states, 2 of which are parent states ("On" and "OnDisp").  The following describes the states:
    Off: In this state, the camera lens is closed and in a lower power mode
    On: This is a parent state where the camera lens cover is open and powered on
    OnShoot: This is a child state where the camera sensor is on and ready to take a picture
    OnDisp: This is a parent state where the LCD is turned on for Menu or Picture review
    OnDispPlay: This is a child state where the Picture can be reviewed on the LCD
    OnDispMenu: This is a child state where the Help Menu is displayed showing the status


2.1: Define the events and states:
----------------------------------
2.1.1: Create a new header that will define your HSM and include the "hsm.h" header
    #include "hsm.h"

2.1.2: Enumerate the HSM events (HSME) used by state machine.  HSME are normally 32-bit, but this can be changed to suit the platform.  The user-defined events can be any number as long as the following reserved values are not used (HSME_NULL(0), HSME_INIT(-3), HSME_ENTRY(-2), HSME_EXIT(-1)).  You can enumerate from the defined value HSME_START(1)
    #define HSME_PWR        (HSME_START)
    #define HSME_RELEASE    (HSME_START + 1)
    #define HSME_MODE       (HSME_START + 2)
    #define HSME_LOWBATT    (HSME_START + 3)

2.1.3: Derive the state machine class by inheriting the HSM class.  In C, define the state machine with struct and declare the first member as parent of type HSM.  Add additonal members that are relevant to the state machine's context.  This shall be used to instantiate any number of state machines objects
    typedef struct CAMERA_T
    {
        // Parent
        HSM parent;

        // Child members
        char param1;
        char param2;
    } CAMERA;

2.1.4: Create a new source file and declare the singleton HSM_STATE objects for each state modeled in your UML State Diagram.  These state objects are later used to bind with the state handler and used for state transitions.
    HSM_STATE CAMERA_StateOff;
    HSM_STATE CAMERA_StateOn;
    HSM_STATE CAMERA_StateOnShoot;
    HSM_STATE CAMERA_StateOnDisp;
    HSM_STATE CAMERA_StateOnDispPlay;
    HSM_STATE CAMERA_StateOnDispMenu;


2.2: Implement each of the state's event handler
------------------------------------------------
For this section, we shall refer to the example Camera state "OnShoot":

                 @
                 |
                 V
    +-------------------------+
    |         OnShoot         |
    |-------------------------|
    | entry / EnableSensor()  |
    | exit / DisableSensor()  |------MODE----->
    | RELEASE / TakePicture() |
    +-------------------------+

 1  HSM_EVENT CAMERA_StateOnShootHndlr(HSM *This, HSM_EVENT event, void *param)
 2  {
 3      if (event == HSME_ENTRY)
 4      {
 5          printf("\tEnable Sensor\n");
 6      }
 7      else if (event == HSME_EXIT)
 8      {
 9          printf("\tDisable Sensor\n");
10      }
11      else if (event == HSME_RELEASE)
12      {
13          printf("\tCLICK!, save photo\n");
14          return 0;
15      }
16      else if (event == HSME_MODE)
17      {
18          HSM_Tran(This, &CAMERA_StateOnDispPlay, 0, NULL);
19          return 0;
20      }
21      return event;
22  }

2.2.1: Here we define the State Handler that is associated with the HSM_STATE object.  Each state handler must use the following prototype:
    HSM_EVENT CAMERA_StateOnShootHndlr(HSM *This, HSM_EVENT event, void *param);
where:
    This  - Pointer to the state machine instance.  Typecast to access derived class members
    event - The enumerated HSM Events that maybe handled by this state as defined in section 2.1.2
    param - [Optional] argument associated with the event

2.2.2: When events are sent to the state machine (via HSM_Run(...)), the current state handler will be passed with the "event" and "param" which the state handler will perform some action to AND then perform one of the following:
    a. Consume the the event and returning NULL(0) (SEE LINE 14 and LINE 19)
    b. Or pass it to the parent state by returning the event (SEE LINE 21)
NOTE: The event can be handled by the state and still pass/defer the event to the parent for common handling.

2.2.3: If an event triggers a state transition, use the following API (SEE LINE 18):
    void HSM_Tran(HSM *This, HSM_STATE *nextState, void *param, void (*method)(HSM *This, void *param))
where:
    This      - Pointer to the state machine instance.
    nextState - pointer to the state object of the next state as defined in section 2.1.4
    param     - [optional] argument passed to next state handler and optional method
    method    - A callback for lamda like functions which is invoked after all HSME_ENTRY events and before any HSME_EXIT events
NOTE: The HSM_Tran(...) API will generate HSME_EXIT events to all exiting states (e.g. OnShoot) up to a common parent, invoke the "method" callback if set, generate HSME_ENTRY events to the entered states (e.g. OnDisp and OnDispPlay) from the common parent and then finally generate HSME_INIT event to the final state

2.2.4: Implement the optional handling of the HSME_ENTRY, HSME_EXIT, and/or HSME_INIT events as required by your HSM State Diagram.  These events are generated by the HSM framework on state transition.  It is not necessary to consume these events as the framework will never pass these to the parent state.  The following is a description of the Framework generated events:
    a. HSME_ENTRY - Used to trigger any SETUP required on entry to the state (SEE LINE 3)
    b. HSME_EXIT - Used to trigger any TEARDOWN required on exit from the state (SEE LINE 7)
    c. HSME_INIT - This event is generated after all HSME_ENTRY and HSME_EXIT have been handled and state has been set to the new state.  This is normally used to trigger another transition that is dependent on some guard condition (i.e. depends on some state machine context)


2.3: Create the HSM states and HSM instances
--------------------------------------------
2.3.1: The state machine is defined by its states.  So with the HSM_STATE objects declared and their respective State handlers defined, the state objects and handlers need to be binded using the following API:
    void HSM_STATE_Create(HSM_STATE *This, const char *name, HSM_FN handler, HSM_STATE *parent);
where:
    This    - Pointer to state object as defined in section 2.1.4
    name    - Name of State used in debug
    handler - State handler as defined in section 2.2 to bind with state object
    parent  - Pointer to parent state object as defined in section 2.2

For example using the Camera HSM Model in section 2, we can define the hierarchy as follows:
    HSM_STATE_Create(&CAMERA_StateOff, "Off", CAMERA_StateOffHndlr, NULL);
    HSM_STATE_Create(&CAMERA_StateOn, "On", CAMERA_StateOnHndlr, NULL);
    HSM_STATE_Create(&CAMERA_StateOnShoot, "On.Shoot", CAMERA_StateOnShootHndlr, &CAMERA_StateOn);
    HSM_STATE_Create(&CAMERA_StateOnDisp, "On.Disp", CAMERA_StateOnDispHndlr, &CAMERA_StateOn);
    HSM_STATE_Create(&CAMERA_StateOnDispPlay, "On.Disp.Play", CAMERA_StateOnDispPlayHndlr, &CAMERA_StateOnDisp);
    HSM_STATE_Create(&CAMERA_StateOnDispMenu, "On.Disp.Menu", CAMERA_StateOnDispMenuHndlr, &CAMERA_StateOnDisp);

2.3.2: There can be many instances of the state machine defined in 2.3.1 by declare HSM object and initialize the state machine instance using the following API:
    void HSM_Create(HSM *This, const char *name, HSM_STATE *initState);
where:
    This - Pointer to the state machine object
    name - Name of the state machine, Unique to this instance
    initState - Initialize state of the state machine and how the state machine is associated

For example:
    CAMERA basic;
    HSM_Create((HSM *)&basic name, &CAMERA_StateOff);
This will set the state machine state to CAMERA_StateOff and generate the HSME_ENTRY and HSME_INIT event to this state


2.4: Embed the HSM_Run() into your application
----------------------------------------------
Running the HSM is as simple as calling the HSM_Run() API and passing the HSM object along with the "event" and optional "param".  It is up to the state handler to decide what to do with the event and param.  There are no global variables except in one case (SEE 3.1.2), so all of the functions in the HSM framework are re-entrant.

A typical use is servicing an event by simply feeding the enumerated event and optional parameter to the HSM object.

For Example:
    HSM_Run(&basic, HSME_PWR, 0);
Will generate the following logs (assuming debug is enabled):
    [DBG] Run Canon[Off](evt:HSME_PWR, param:00000000)
    [DBG] Tran Canon[Off -> On]
    [DBG]   Canon[Off](EXIT)
        Exit Low Power Mode
    [DBG]   Canon[On](ENTRY)
        Open Lens
    [DBG]   Canon[On](INIT)


3. Using HSM Advance Features:
==============================
This HSM Framework provides numerous configurable options to control the level of debug as well optimize for memory constrained systems.  All of these features are configured in hsm.h

3.1. Basic Debug Features
-------------------------
It can be difficult to debug the HSM without any type of logging, so it is highly recommended to develop with the debug features on and then disable them in production.  Be sure to run the camera demo to get a feel for the debugging capabilities and the HSM framework.  To enable debugging:
    a) Enable the compiler flag HSM_FEATURE_DEBUG_ENABLE to compile in the HSM debug logging features
    b) If the system does not have printf(...) available, then enable the compiler flag HSM_FEATURE_DEBUG_EMBEDDED and implement application specific DEBUG_OUT(...)
    c) Set HSM_NEWLINE to proper carriage return and line feed matching the host environment (i.e. linux, OSX or windows)
    d) If the host is capable of rending terminal colors, then enable HSM_FEATURE_DEBUG_COLOR which colorizes the print for easier readability
    e) Each state machine object can enable or disable debug in run time using HSM_SET_DEBUG() and passing the flags: {HSM_SHOW_RUN, HSM_SHOW_TRAN, HSM_SHOW_ALL} to enable debug from HSM_Run() and HSM_Tran()

3.2 Advance Debug Features
--------------------------
These feature are only supported if HSM_FEATURE_DEBUG_ENABLE is enabled

3.2.1: HSM_SET_PREFIX
Each state machine instance can have its own prefix which is commonly used to filter the HSM debug messages with grep
For example, using HSM_SET_PREFIX((HSM *)This, "[DBG] ");
    [DBG] Run Canon[On.Disp.Menu](evt:HSME_LOWBATT, param:00000000)
    [DBG]   evt:HSME_LOWBATT unhandled, passing to Canon[On.Disp]
    [DBG]   evt:HSME_LOWBATT unhandled, passing to Canon[On]
        Beep low battery warning

3.2.2: HSM_SUPPRESS_DEBUG
Some state machines can generate a large number of periodic events which generates a lot of noise in the debug logs.  So it is often desireable to momentarily suppress the debug messages for one run (i.e. one call to HSM_Run()).  Simply call HSM_SUPPRESS_DEBUG() and pass the flags: {HSM_SHOW_RUN, HSM_SHOW_TRAN, HSM_SHOW_ALL} to suppress debug for one run.  The original flags for HSM_SET_DEBUG() are restored for the next run

3.2.3: HSM_DEBUG_EVT2STR
When reviewing the debug logs, it is difficult to manually decode the encoded event values like this:
    [DBG] Run Canon[On.Disp.Menu](evt:4, param:00000000)
    [DBG]   evt:4 unhandled, passing to Canon[On.Disp]
    [DBG]   evt:4 unhandled, passing to Canon[On]
It is significatly easier if the output was human readable like this:
    [DBG] Run Canon[On.Disp.Menu](evt:HSME_LOWBATT, param:00000000)
    [DBG]   evt:HSME_LOWBATT unhandled, passing to Canon[On.Disp]
    [DBG]   evt:HSME_LOWBATT unhandled, passing to Canon[On]
To enable this feature, define a function that will translate the enumerated event with a string.
For example:
    const char *HSM_Evt2Str(uint32_t event)
    {
        switch(event)
        {
        case HSME_PWR:
            return "HSME_PWR";
        case HSME_RELEASE:
            return "HSME_RELEASE";
        case HSME_MODE:
            return "HSME_MODE";
        case HSME_LOWBATT:
            return "HSME_LOWBATT";
        }
    }
Then in the makefile define HSM_DEBUG_EVT2STR to the above function:
    # Add this define for customized HSM_EVENT value to string function for human readable debug
    CFLAGS += -DHSM_DEBUG_EVT2STR=HSM_Evt2Str

3.2.4: HSM_FEATURE_DEBUG_NESTED_CALL
This is a special debug feature that is useful for constrained systems that have multiple running HSM instances operating in a single thread.  This necessitates calling the HSM_Run() from within the state handler (i.e. nesting the HSM) which makes debugging confusing (i.e. interleaved debug logs).

To eliminate the confusion, you can enable the flag HSM_FEATURE_DEBUG_NESTED_CALL which will indent the output by the number of nesting levels.  However there is one caveat, this will make calls to HSM_Run() non-reentrant as a global will need to be defined to tracked the nesting levels which isn't really a penalty.

It is important to note that the global apucHsmNestIndent[] may need to extended if the nesting gets too deep
    const char *apucHsmNestIndent[] = { "", "", "\t", "\t\t", "\t\t\t", "\t\t\t\t"};

3.3: Optimization Features
--------------------------
The following features can be configure to decrease the memory footprint and decrease the latency and should only be disabled for a system that has been well tested, else debugging can be difficult

3.3.1: HSM_FEATURE_SAFETY_CHECK
Enabling this feature performs sanity checks on the state handler performing illegal calls which result in erroneous behavior.  This can be disabled in production after full regression testing to save just a little memory and latency

3.3.2: HSM_FEATURE_INIT
Enabling this feature enables generation of the HSME_INIT event during a call to HSM_Tran() from the state handler.  If you do not use the HSME_INIT in your UML state diagram, you can disable this feature to save a little memory and improve latency

3.3.3: HSM_MAX_DEPTH
This is a configurable value for the maximum depth (i.e. nesting) of states you are designing.  HSM_Tran() uses this value to allocate the memory required to performs a run-time trace of the state ancestry.  HSM_STATE_Create() shall assert if the state creation exceeds the maximum depth, in which case you should either redesign your HSM or increase this value (Default is 5)


4. HSM Cookbook and Design Patterns:
====================================
4.1: Need to make a transition decision on state entry, try using HSME_INIT
Normally when we enter a state, there is some SETUP operation (initiated by HSME_ENTRY) that may fail (e.g allocating memory) which would cause the system to fail if allowed to continue.  In this case, we should take a course of action such as changing to another state.

To illustrate, we shall use our camera model again where we need to handle the case where the camera lens fails to open (e.g. its obstructed or jammed).  In this case we should go back to the Off state to prevent further damage.
    +-------------------------+
    |           Off           |
    |-------------------------|<------O
    | entry / EnterLowPower() |
    | exit / ExitLowPower()   |
    +-------------------------+
        |           ^       ^
       PWR         PWR      |
        V           |       |
    +-----------------------|-----------------------------------------------------+
    |                       |            On                                       |
    |---------------------- |-----------------------------------------------------|
    | entry / OpenLens()    |                                                     |
    | exit / CloseLens()    |                                                     |
    |                       |                                                     |
    |            [Lens Failed to Open]                                            |
    |                       |                                                     |
    |                       @          +----------------------------------------+ |
    |                       |          |                OnDisp                  | |
    |                       V          |----------------------------------------| |
    |  +-------------------------+     | entry / TurnOnLCD()                    | |
    |  |         OnShoot         |     | exit / TurnOffLCD()                    | |
    |  |-------------------------|     |                                        | |
    |  | entry / EnableSensor()  |     |          +---------------------------+ | |
    |  | exit / DisableSensor()  |------MODE----->|         OnDispPlay        | | |
    |  | RELEASE / TakePicture() |     |          +---------------------------+ | |
    |  +-------------------------+     |                                        | |
    |                                  +----------------------------------------+ |
    +-----------------------------------------------------------------------------+

Here we check if the lens is open successfully.  If its not, then we can transition back to Off (or some other safe state) instead of the OnShoot state.

    HSM_EVENT CAMERA_StateOnHndlr(HSM *This, HSM_EVENT event, void *param)
    {
        if (event == HSME_ENTRY)
        {
            printf ("\tOpen Lens\n");
        }
    ..
    ..
        else if (event == HSME_INIT)
        {
            if (isLensOpen())
            {
                HSM_Tran(This, &CAMERA_StateOnShoot, 0, NULL);
            }
            else
            {
                printf ("\tBeep Failure Warning\n");
                HSM_Tran(This, &CAMERA_StateOff, 0, NULL);
            }
        }
    ..
    ..
    }

NOTE!! It is illegal to invoke a HSM_Tran() for the HSME_ENTRY and HSME_EXIT event, since this would create a recursion in HSM_Tran().  Thus only HSM_Tran() can only be invoked by user event or HSME_INIT.

4.2. Need to run Concurrent model (i.e. concurrent states), try using the parent state as a proxy
TODO

4.3. Need to develop, try runt HSM_IsInState() for guard condition
TODO