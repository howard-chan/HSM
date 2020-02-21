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

// Camera HSM Events
#define HSME_PWR        (HSME_START)
#define HSME_RELEASE    (HSME_START + 1)
#define HSME_MODE       (HSME_START + 2)
#define HSME_LOWBATT    (HSME_START + 3)

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

    // Declare member variables
    int m_shots;

    // Declare HSM States here:
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

            case HSME_PWR:
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

            case HSME_PWR:
                camera.tran(&camera.stateOff);
                return 0;

            case HSME_LOWBATT:
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

            case HSME_RELEASE:
                cout << "\tCLICK!, save photo #" << ++camera.m_shots << endl;
                return 0;

            case HSME_MODE:
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

            case HSME_MODE:
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

            case HSME_MODE:
                camera.tran(&camera.stateOnShoot);
                return 0;
            }
            return xEvent;
        }
    } stateOnDispMenu;


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
        setDebug(HSM_SHOW_ALL);
        // Set the initial state
        setInitState(&stateOff);
    }
};


int main()
{
    cout << "HSM Demo" << endl;
    Camera basic("canon");
    // Turn on the Power
    basic(HSME_PWR, 0);
    // Take a picture
    basic(HSME_RELEASE, 0);
    // Take another picture
    basic(HSME_RELEASE, 0);
    // Playback the photo
    basic(HSME_MODE, 0);
    // Oops, pushed the release button by accident
    basic(HSME_RELEASE, 0);
    // Go to menu settings
    basic(HSME_MODE, 0);
    // Uh oh, low battery
    basic(HSME_LOWBATT, 0);
    // Time to turn it off
    basic(HSME_PWR, 0);
}
