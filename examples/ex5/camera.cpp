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

#include <iostream>

#include "hsm.hpp"

using namespace std;
using namespace hsm;

// Camera HSM Events
#define PWR_BUTTON_EVT        (HSME_START)
#define SHUTTER_BUTTON_EVT    (HSME_START + 1)
#define MODE_BUTTON_EVT       (HSME_START + 2)
#define LOWBATT_EVT           (HSME_START + 3)

const char *Camera_Evt2Str(uint32_t event)
{
    switch(event)
    {
    case PWR_BUTTON_EVT:
        return "PWR_BUTTON_EVT";
    case SHUTTER_BUTTON_EVT:
        return "SHUTTER_BUTTON_EVT";
    case MODE_BUTTON_EVT:
        return "MODE_BUTTON_EVT";
    case LOWBATT_EVT:
        return "LOWBATT_EVT";
    }
}

class Camera : public Hsm {
   /*
    Overview:
    Class that implements the Camera HSM and inherits from the HSM class
    Creating a HSM requires the following steps:
        1) Initialize the base HSM class
        2) Define the HSM states hierarchy
        3) Set the starting state
        4) Define the state handlers
            a) State handler must return HSME_NULL (i.e. 0) if the event IS handled
            b) State handler must return xEvent if the event IS NOT handled
            c) State handler may handle the HSME_ENTRY event for state setup
            d) State handler may handle the HSME_EXIT event for state teardown/cleanup
            e) State handler may handle the HSME_INIT for self transition to child state
            f) Self transition to child state MUST NOT be handled by HSME_ENTRY or HSME_EXIT event
            g) Events HSME_ENTRY, HSME_EXIT and HSME_INIT do no need to return HSME_NULL (i.e. 0) for brevity
    */

    //--------------------------------------
    // Define HSM States
    //
    // In this example, a callable/function object is used to define the state and event handler can be used instead of rather than defining static event handleres and assigning them to the state object
    // the states of the `Hsm` are child classes of `HsmState` and
    // are callable objects via operator().
    // 1) Inherit `HsmState` - Hsm works with `HsmState` objects
    // 2) Declare as `struct` or `public` - Allows `Hsm` to construct the object
    // 3) Csonstruct HsmState(name, nullptr, parent)
    // 4) Override and implement operator() which performs the state event handling
    //      hsm_event_t operator()(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam)
    //--------------------------------------
    struct StateOff : public HsmState {
        StateOff(const char *name="", HsmState *parent=nullptr) : HsmState(name, nullptr, parent) {}
        hsm_event_t operator()(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam)
        {
            Camera &camera = *static_cast<Camera *>(pxHsm);
            switch (xEvent)
            {
            case HSME_ENTRY:
                cout << "\tEnter Lower Power Mode" << endl;
                return 0;

            case HSME_EXIT:
                cout << "\tExit Lower Power Mode" << endl;
                return 0;

            case PWR_BUTTON_EVT:
                camera.tran(&camera.stateOn);
                return 0;
            }
            return xEvent;
        }
    } stateOff;


    struct StateOn : public HsmState {
        StateOn(const char *name="", HsmState *parent=nullptr) : HsmState(name, nullptr, parent) {}
        hsm_event_t operator()(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam)
        {
            Camera &camera = *static_cast<Camera *>(pxHsm);
            switch (xEvent)
            {
            case HSME_ENTRY:
                cout << "\tOpen Lens" << endl;
                return 0;

            case HSME_EXIT:
                cout << "\tClose Lens" << endl;
                return 0;

            case HSME_INIT:
                camera.tran(&camera.stateOnShoot);
                return 0;

            case PWR_BUTTON_EVT:
                camera.tran(&camera.stateOff);
                return 0;

            case LOWBATT_EVT:
                cout << "\tBeep low battery warning" << endl;
                return 0;
            }
            return xEvent;
        }
    } stateOn;


    struct StateOnShoot : public HsmState {
        StateOnShoot(const char *name="", HsmState *parent=nullptr) : HsmState(name, nullptr, parent) {}
        hsm_event_t operator()(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam)
        {
            Camera &camera = *static_cast<Camera *>(pxHsm);
            switch (xEvent)
            {
            case HSME_ENTRY:
                cout << "\tEnable Sensor" << endl;
                return 0;

            case HSME_EXIT:
                cout << "\tDisable Sensor" << endl;
                return 0;

            case SHUTTER_BUTTON_EVT:
                {
                    size_t value = reinterpret_cast<size_t>(pvParam);
                    if (value == 1) {
                        cout << "\tFocusing" << endl;
                    } else if (value == 2) {
                        cout << "\tCLICK!, save photo #" << ++camera.m_shots << endl;
                    }
                }
                return 0;

            case MODE_BUTTON_EVT:
                camera.tran(&camera.stateOnDispPlay);
                return 0;
            }
            return xEvent;
        }
    } stateOnShoot;


    struct StateOnDisp : public HsmState {
        StateOnDisp(const char *name="", HsmState *parent=nullptr) : HsmState(name, nullptr, parent) {}
        hsm_event_t operator()(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam)
        {
            Camera &camera = *static_cast<Camera *>(pxHsm);
            switch (xEvent)
            {
            case HSME_ENTRY:
                cout << "\tTurn on LCD" << endl;
                return 0;

            case HSME_EXIT:
                cout << "\tTurn off LCD" << endl;
                return 0;
            }
            return xEvent;
        }
    } stateOnDisp;


    struct StateOnDispPlay : public HsmState {
        StateOnDispPlay(const char *name="", HsmState *parent=nullptr) : HsmState(name, nullptr, parent) {}
        hsm_event_t operator()(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam)
        {
            Camera &camera = *static_cast<Camera *>(pxHsm);
            switch (xEvent)
            {
            case HSME_ENTRY:
                cout << "\tDisplay " << camera.m_shots << " pictures" << endl;
                return 0;

            case MODE_BUTTON_EVT:
                camera.tran(&camera.stateOnDispMenu);
                return 0;
            }
            return xEvent;
        }
    } stateOnDispPlay;


    struct StateOnDispMenu : public HsmState {
        StateOnDispMenu(const char *name="", HsmState *parent=nullptr) : HsmState(name, nullptr, parent) {}
        hsm_event_t operator()(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam)
        {
            Camera &camera = *static_cast<Camera *>(pxHsm);
            switch (xEvent)
            {
            case HSME_ENTRY:
                cout << "\tDisplay Menu" << endl;
                return 0;

            case MODE_BUTTON_EVT:
                camera.tran(&camera.stateOnShoot);
                return 0;
            }
            return xEvent;
        }
    } stateOnDispMenu;

    //--------------------------------------
    // Declare member variables
    //--------------------------------------
    int m_shots;


public:
    Camera(const char *pcName) :
        // Step 1: Create the HSM States
        stateOff("Off"),
        stateOn("On"),
        stateOnShoot("On.Shoot", &stateOn),
        stateOnDisp("On.Disp", &stateOn),
        stateOnDispPlay("On.Disp.Play", &stateOnDisp),
        stateOnDispMenu("On.Disp.Menu", &stateOnDisp),
        // Step 2: Initiailize the HSM and starting state
        Hsm(pcName, &stateOff),
        m_shots{0}
    {
        // Step 3: [Optional] Enable HSM debug
        setPrefix("[DBG] ");
        setEvt2Str(&Camera_Evt2Str);
        setDebug(HSM_SHOW_ALL);
        // Start the initial state
        start();
    }
};

int main()
{
    cout << "HSM Demo" << endl;
    Camera simple("canon");
    // Turn on the Power
    simple(PWR_BUTTON_EVT);
    // Half-press shutter button to auto-focus
    simple(SHUTTER_BUTTON_EVT, (void *)1);
    // Take picture
    simple(SHUTTER_BUTTON_EVT, (void *)2);
    // Take another picture
    simple(SHUTTER_BUTTON_EVT, (void *)2);
    // Playback the photo
    simple(MODE_BUTTON_EVT);
    // Oops, pushed the release button by accident
    simple(SHUTTER_BUTTON_EVT);
    // Go to menu settings
    simple(MODE_BUTTON_EVT);
    // Uh oh, low battery
    simple(LOWBATT_EVT);
    // Time to turn it off
    simple(PWR_BUTTON_EVT);
}
