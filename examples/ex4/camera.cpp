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

class Camera : public Hsm {
   /*
    Class that implements the Camera HSM and inherits from the HSM class
    Creating a HSM requires the following steps:
        1) Initialize the base HSM class
        2) Define the HSM states hierarchy
        3) Set the starting state
        4) Define the state handlers
            a) State handler must return "None" if the event IS handled
            b) State handler must return "event" if the event IS NOT handled
            c) State handler may handle the ENTRY event for state setup
            d) State handler may handle the EXIT event for state teardown/cleanup
            e) State handler may handle the INIT for self transition to child state
            f) Self transition to child state MUST NOT be handled by ENTRY or EXIT event
            g) Events ENTRY, EXIT and INIT do no need to return None for brevity
    */

    // Declare HSM States here:
    HsmState stateOff;
    HsmState stateOn;
    HsmState stateOnShoot;
    HsmState stateOnDisp;
    HsmState stateOnDispPlay;
    HsmState stateOnDispMenu;

    // Declare member variables
    int m_shots;

    // Define the "static" state handlers here
    static hsm_event_t stateOff_handler(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam) {
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

    static hsm_event_t stateOn_handler(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam) {
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

    static hsm_event_t stateOnShoot_handler(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam) {
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
            // NOTE: param is normally casted before used (e.g. structure or primitive)
            if ((uintptr_t)pvParam == 1) {
                cout << "\tFocusing on subject" << endl;
            } else {
                cout << "\tCLICK!, save photo #" << ++camera.m_shots << endl;
            }
            return 0;

        case MODE_BUTTON_EVT:
            camera.tran(&camera.stateOnDispPlay);
            return 0;
        }
        return xEvent;
    }

    static hsm_event_t stateOnDisp_handler(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam) {
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

    static hsm_event_t stateOnDispPlay_handler(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam) {
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

    static hsm_event_t stateOnDispMenu_handler(Hsm *pxHsm, hsm_event_t xEvent, void *pvParam) {
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

public:
    Camera(const char *pcName) :
        // Step 1: Create the HSM States
        stateOff("Off", stateOff_handler),
        stateOn("On", stateOn_handler),
        stateOnShoot("On.Shoot", stateOnShoot_handler, &stateOn),
        stateOnDisp("On.Disp", stateOnDisp_handler, &stateOn),
        stateOnDispPlay("On.Disp.Play", stateOnDispPlay_handler, &stateOnDisp),
        stateOnDispMenu("On.Disp.Menu", stateOnDispMenu_handler, &stateOnDisp),
        // Step 2: Initiailize the HSM and starting state
        Hsm(pcName, &stateOff),
        m_shots{0}
    {
        // Step 3: [Optional] Enable HSM debug
        setPrefix("[DBG] ");
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
    simple(PWR_BUTTON_EVT, 0);
    // Half-press to focus camera
    simple(SHUTTER_BUTTON_EVT, (void *)1);
    // Take a picture
    simple(SHUTTER_BUTTON_EVT, 0);
    // Take another picture
    simple(SHUTTER_BUTTON_EVT, 0);
    // Playback the photo
    simple(MODE_BUTTON_EVT, 0);
    // Oops, pushed the release button by accident
    simple(SHUTTER_BUTTON_EVT, 0);
    // Go to menu settings
    simple(MODE_BUTTON_EVT, 0);
    // Uh oh, low battery
    simple(LOWBATT_EVT, 0);
    // Time to turn it off
    simple(PWR_BUTTON_EVT, 0);
}
