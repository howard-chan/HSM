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
#include <stdio.h>

#include "hsm.h"

// Camera HSM Events
#define HSME_PWR        (HSME_START)
#define HSME_RELEASE    (HSME_START + 1)
#define HSME_MODE       (HSME_START + 2)
#define HSME_LOWBATT    (HSME_START + 3)

typedef struct
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
} Camera_t;

HSM_STATE Camera_StateOff;
HSM_STATE Camera_StateOn;
HSM_STATE Camera_StateOnShoot;
HSM_STATE Camera_StateOnDisp;
HSM_STATE Camera_StateOnDispPlay;
HSM_STATE Camera_StateOnDispMenu;

HSM_EVENT Camera_StateOffHndlr(HSM *This, HSM_EVENT event, void *param)
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
        HSM_Tran(This, &Camera_StateOn, 0, NULL);
        return 0;
    }
    return event;
}

HSM_EVENT Camera_StateOnHndlr(HSM *This, HSM_EVENT event, void *param)
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
        HSM_Tran(This, &Camera_StateOnShoot, 0, NULL);
    }
    else if (event == HSME_PWR)
    {
        HSM_Tran(This, &Camera_StateOff, 0, NULL);
        return 0;
    }
    else if (event == HSME_LOWBATT)
    {
        printf("\tBeep low battery warning\n");
        return 0;
    }
    return event;
}

HSM_EVENT Camera_StateOnShootHndlr(HSM *This, HSM_EVENT event, void *param)
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
        // NOTE: param is normally casted before used (e.g. structure or primitive)
        int button = (uintptr_t)param;
        if (button == 1)
        {
            printf("\tFocusing on subject\n");
        }
        else
        {
            printf("\tCLICK!, save photo\n");
        }
        return 0;
    }
    else if (event == HSME_MODE)
    {
        HSM_Tran(This, &Camera_StateOnDispPlay, 0, NULL);
        return 0;
    }
    return event;
}

HSM_EVENT Camera_StateOnDispHndlr(HSM *This, HSM_EVENT event, void *param)
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

HSM_EVENT Camera_StateOnDispPlayHndlr(HSM *This, HSM_EVENT event, void *param)
{
    if (event == HSME_ENTRY)
    {
        printf("\tDisplay Pictures\n");
    }
    else if (event == HSME_MODE)
    {
        HSM_Tran(This, &Camera_StateOnDispMenu, 0, NULL);
        return 0;
    }
    return event;
}

HSM_EVENT Camera_StateOnDispMenuHndlr(HSM *This, HSM_EVENT event, void *param)
{
    if (event == HSME_ENTRY)
    {
        printf("\tDisplay Menu\n");
    }
    else if (event == HSME_MODE)
    {
        HSM_Tran(This, &Camera_StateOnShoot, 0, NULL);
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

void Camera_Init(Camera_t *This, char *name)
{
    // Step 1: Create the HSM States
    HSM_STATE_Create(&Camera_StateOff, "Off", Camera_StateOffHndlr, NULL);
    HSM_STATE_Create(&Camera_StateOn, "On", Camera_StateOnHndlr, NULL);
    HSM_STATE_Create(&Camera_StateOnShoot, "On.Shoot", Camera_StateOnShootHndlr, &Camera_StateOn);
    HSM_STATE_Create(&Camera_StateOnDisp, "On.Disp", Camera_StateOnDispHndlr, &Camera_StateOn);
    HSM_STATE_Create(&Camera_StateOnDispPlay, "On.Disp.Play", Camera_StateOnDispPlayHndlr, &Camera_StateOnDisp);
    HSM_STATE_Create(&Camera_StateOnDispMenu, "On.Disp.Menu", Camera_StateOnDispMenuHndlr, &Camera_StateOnDisp);

    // Step 2: Initiailize the HSM and starting state
    HSM_Create((HSM *)This, name, &Camera_StateOff);

    // Step 3: [Optional] Enable HSM debug
    HSM_SET_PREFIX((HSM *)This, "[DBG] ");
    HSM_SET_DEBUG((HSM *)This, HSM_SHOW_ALL);

    // Step 4: Camera_t member initialization
    This->param1 = 0;
    This->param2 = 1;
}

void Camera_Run(Camera_t *This, HSM_EVENT event, void *param)
{
    // Uncomment below to suppress debug for a specific event (e.g. periodic timer event)
    // if (event == <NAME.OF.EVENT.YOU.WANT.TO.SUPPRESS>)
    //    HSM_SUPPRESS_DEBUG((HSM *)This, HSM_SHOW_ALL);

    HSM_Run((HSM *)This, event, param);
}

void main(void)
{
    Camera_t canon;
    // Instantiate Camera
    Camera_Init(&canon, "Canon PS");
    // Turn on the Power
    Camera_Run(&canon, HSME_PWR, 0);
    // Half-press release to focus camera.
    Camera_Run(&canon, HSME_RELEASE, (void *)1);
    // Take a picture
    Camera_Run(&canon, HSME_RELEASE, 0);
    // Take another picture
    Camera_Run(&canon, HSME_RELEASE, 0);
    // Playback the photo
    Camera_Run(&canon, HSME_MODE, 0);
    // Oops, pushed the release button by accident
    Camera_Run(&canon, HSME_RELEASE, 0);
    // Go to menu settings
    Camera_Run(&canon, HSME_MODE, 0);
    // Uh oh, low battery
    Camera_Run(&canon, HSME_LOWBATT, 0);
    // Time to turn it off
    Camera_Run(&canon, HSME_PWR, 0);
}
