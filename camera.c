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
#include "hsm.h"
#include <stdio.h>

// Camera HSM Events
#define HSME_PWR        (HSME_START)
#define HSME_RELEASE    (HSME_START + 1)
#define HSME_MODE       (HSME_START + 2)
#define HSME_LOWBATT    (HSME_START + 3)


typedef struct CAMERA_T
{
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
    // Parent
    HSM parent;
    // Child members
    char param1;
    char param2;
} CAMERA;

CAMERA basic;
HSM_STATE CAMERA_StateOff;
HSM_STATE CAMERA_StateOn;
HSM_STATE CAMERA_StateOnShoot;
HSM_STATE CAMERA_StateOnDisp;
HSM_STATE CAMERA_StateOnDispPlay;
HSM_STATE CAMERA_StateOnDispMenu;

HSM_EVENT CAMERA_StateOffHndlr(HSM *This, HSM_EVENT event, void *param)
{
    if (event == HSME_ENTRY)
    {
        printf("\tEnter Low Power Mode\n");
    }
    else if (event == HSME_EXIT)
    {
        printf("\tExit Low Power Mode\n");
    }
    else if (event == HSME_PWR)
    {
        HSM_Tran(This, &CAMERA_StateOn, 0, NULL);
        return 0;
    }
    return event;
}

HSM_EVENT CAMERA_StateOnHndlr(HSM *This, HSM_EVENT event, void *param)
{
    if (event == HSME_ENTRY)
    {
        printf ("\tOpen Lens\n");
    }
    else if (event == HSME_EXIT)
    {
        printf ("\tClose Lens\n");
    }
    else if (event == HSME_INIT)
    {
        HSM_Tran(This, &CAMERA_StateOnShoot, 0, NULL);
    }
    else if (event == HSME_PWR)
    {
        HSM_Tran(This, &CAMERA_StateOff, 0, NULL);
        return 0;
    }
    else if (event == HSME_LOWBATT)
    {
        printf("\tBeep low battery warning\n");
        return 0;
    }
    return event;
}

HSM_EVENT CAMERA_StateOnShootHndlr(HSM *This, HSM_EVENT event, void *param)
{
    if (event == HSME_ENTRY)
    {
        printf("\tEnable Sensor\n");
    }
    else if (event == HSME_EXIT)
    {
        printf("\tDisable Sensor\n");
    }
    else if (event == HSME_RELEASE)
    {
        printf("\tCLICK!, save photo\n");
        return 0;
    }
    else if (event == HSME_MODE)
    {
        HSM_Tran(This, &CAMERA_StateOnDispPlay, 0, NULL);
        return 0;
    }
    return event;
}

HSM_EVENT CAMERA_StateOnDispHndlr(HSM *This, HSM_EVENT event, void *param)
{
    if (event == HSME_ENTRY)
    {
        printf("\tTurn on LCD\n");
    }
    else if (event == HSME_EXIT)
    {
        printf("\tTurn off LCD\n");
    }
    return event;
}

HSM_EVENT CAMERA_StateOnDispPlayHndlr(HSM *This, HSM_EVENT event, void *param)
{
    if (event == HSME_ENTRY)
    {
        printf("\tDisplay Pictures\n");
    }
    else if (event == HSME_MODE)
    {
        HSM_Tran(This, &CAMERA_StateOnDispMenu, 0, NULL);
        return 0;
    }
    return event;
}

HSM_EVENT CAMERA_StateOnDispMenuHndlr(HSM *This, HSM_EVENT event, void *param)
{
    if (event == HSME_ENTRY)
    {
        printf("\tDisplay Menu\n");
    }
    else if (event == HSME_MODE)
    {
        HSM_Tran(This, &CAMERA_StateOnShoot, 0, NULL);
        return 0;
    }
    return event;
}

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

void CAMERA_Init(CAMERA *This, char *name)
{
    // Step 1: Create the HSM States
    HSM_STATE_Create(&CAMERA_StateOff, "Off", CAMERA_StateOffHndlr, NULL);
    HSM_STATE_Create(&CAMERA_StateOn, "On", CAMERA_StateOnHndlr, NULL);
    HSM_STATE_Create(&CAMERA_StateOnShoot, "On.Shoot", CAMERA_StateOnShootHndlr, &CAMERA_StateOn);
    HSM_STATE_Create(&CAMERA_StateOnDisp, "On.Disp", CAMERA_StateOnDispHndlr, &CAMERA_StateOn);
    HSM_STATE_Create(&CAMERA_StateOnDispPlay, "On.Disp.Play", CAMERA_StateOnDispPlayHndlr, &CAMERA_StateOnDisp);
    HSM_STATE_Create(&CAMERA_StateOnDispMenu, "On.Disp.Menu", CAMERA_StateOnDispMenuHndlr, &CAMERA_StateOnDisp);
    // Step 2: Initiailize the HSM and starting state
    HSM_Create((HSM *)This, name, &CAMERA_StateOff);
    // Step 3: [Optional] Enable HSM debug
    HSM_SET_PREFIX((HSM *)This, "[DBG] ");
    HSM_SET_DEBUG((HSM *)This, HSM_SHOW_ALL);
    // Step 4: CAMERA member initialization
    This->param1 = 0;
    This->param2 = 1;
}

void CAMERA_Run(CAMERA *This, HSM_EVENT event, void *param)
{
    HSM_Run((HSM *)This, event, param);
}

void main(void)
{
    // Instantiate Camera
    CAMERA_Init(&basic, "Canon");
    // Turn on the Power
    CAMERA_Run(&basic, HSME_PWR, 0);
    // Take a picture
    CAMERA_Run(&basic, HSME_RELEASE, 0);
    // Take another picture
    CAMERA_Run(&basic, HSME_RELEASE, 0);
    // Playback the photo
    CAMERA_Run(&basic, HSME_MODE, 0);
    // Oops, pushed the release button by accident
    CAMERA_Run(&basic, HSME_RELEASE, 0);
    // Go to menu settings
    CAMERA_Run(&basic, HSME_MODE, 0);
    // Uh oh, low battery
    CAMERA_Run(&basic, HSME_LOWBATT, 0);
    // Time to turn it off
    CAMERA_Run(&basic, HSME_PWR, 0);
}
