//----The following belongs to Camera.h----
#include "hsm.h"

// Camera HSM Events
#define RELEASE         (1)
#define MODE_CMD        (2)
#define LOWBATT_EVT     (3)
#define PWR_CMD         (4)

// Definition of Camera class
typedef struct
{
    // Parent  NOTE: HSM parent must be defined first
    HSM parent;

    // Child members
    // ==> YOUR.CHANGES.GO.HERE <==
} Camera_t;

//----The following belongs to Camera.c----
#include <stdio.h>

void EnterLowPower(void) { printf("\tEnter Low Power Mode\n"); }
void ExitLowPower(void) { printf("\tExit Low Power Mode\n"); }
void OpenLens(void) { printf("\tOpen Lens\n"); }
void CloseLens(void) { printf("\tClose Lens\n"); }
void BeepLowBattWarning(void) { printf("\tBeep LowBatt Warning\n"); }
void EnableSensor(void) { printf("\tEnable Sensor\n"); }
void DisableSensor(void) { printf("\tDisable Sensor\n"); }
void OpenViewFinder(void) { printf("\tOpen ViewFinder\n"); }
void CloseViewFinder(void) { printf("\tClose ViewFinder\n"); }
void TakePicture(void) { printf("\tTake Picture\n"); }
void SaveImage(void) { printf("\tSave Image\n"); }
void MemoryFull(void) { printf("\tMemory Full\n"); }
void TurnOnLCD(void) { printf("\tTurn On LCD\n"); }
void TurnOffLCD(void) { printf("\tTurn Off LCD\n"); }
void DisplayPicture(void) { printf("\tDisplay Pictures\n"); }
void DisplayMenu(void) { printf("\tDisplay Menu\n"); }

// Camera States
HSM_STATE Camera_StateOff;
HSM_STATE Camera_StateOn;
HSM_STATE Camera_StateOnShoot;
HSM_STATE Camera_StateOnDisplay;
HSM_STATE Camera_StateOnDisplayPlay;
HSM_STATE Camera_StateOnDisplayMenu;

// Camera State Handlers
HSM_EVENT Camera_StateOff_Hndlr(HSM *This, HSM_EVENT event, void *param)
{
    Camera_t *pCamera = (Camera_t *)This;
    // This is a note on a composite state
    switch (event)
    {
    case HSME_ENTRY:
        EnterLowPower();
        return 0;

    case HSME_EXIT:
        ExitLowPower();
        return 0;

    case PWR_CMD:
        HSM_Tran(This, &Camera_StateOn, 0, NULL);
        return 0;

    }
    return event;
}

HSM_EVENT Camera_StateOn_Hndlr(HSM *This, HSM_EVENT event, void *param)
{
    Camera_t *pCamera = (Camera_t *)This;
    switch (event)
    {
    case PWR_CMD:
        HSM_Tran(This, &Camera_StateOff, 0, NULL);
        return 0;

    case HSME_ENTRY:
        OpenLens();
        return 0;

    case HSME_EXIT:
        CloseLens();
        return 0;

    case LOWBATT_EVT:
        BeepLowBattWarning();
        return 0;

    case HSME_INIT:
        HSM_Tran(This, &Camera_StateOnShoot, 0, NULL);
        return 0;

    }
    return event;
}

HSM_EVENT Camera_StateOnShoot_Hndlr(HSM *This, HSM_EVENT event, void *param)
{
    Camera_t *pCamera = (Camera_t *)This;
    // A note can also
    // be defined on
    // several lines
    switch (event)
    {
    case HSME_ENTRY:
        EnableSensor();
        OpenViewFinder();
        return 0;

    case HSME_EXIT:
        DisableSensor();
        CloseViewFinder();
        return 0;

    case RELEASE:
        if (param > 0)
        {
            TakePicture();
            SaveImage();
        }
        else if (param == 0)
        {
            MemoryFull();
        }
        return 0;

    case MODE_CMD:
        HSM_Tran(This, &Camera_StateOnDisplayPlay, 0, NULL);
        return 0;

    }
    return event;
}

HSM_EVENT Camera_StateOnDisplay_Hndlr(HSM *This, HSM_EVENT event, void *param)
{
    Camera_t *pCamera = (Camera_t *)This;
    switch (event)
    {
    case HSME_ENTRY:
        TurnOnLCD();
        return 0;

    case HSME_EXIT:
        TurnOffLCD();
        return 0;

    }
    return event;
}

HSM_EVENT Camera_StateOnDisplayPlay_Hndlr(HSM *This, HSM_EVENT event, void *param)
{
    Camera_t *pCamera = (Camera_t *)This;
    switch (event)
    {
    case HSME_ENTRY:
        DisplayPicture();
        return 0;

    case MODE_CMD:
        HSM_Tran(This, &Camera_StateOnDisplayMenu, 0, NULL);
        return 0;

    }
    return event;
}

HSM_EVENT Camera_StateOnDisplayMenu_Hndlr(HSM *This, HSM_EVENT event, void *param)
{
    Camera_t *pCamera = (Camera_t *)This;
    switch (event)
    {
    case HSME_ENTRY:
        DisplayMenu();
        return 0;

    case MODE_CMD:
        HSM_Tran(This, &Camera_StateOnShoot, 0, NULL);
        return 0;

    }
    return event;
}

void Camera_Init(Camera_t *This, char *name)
{
    // Step 1: Create the HSM States
    HSM_STATE_Create(&Camera_StateOff, "Off", Camera_StateOff_Hndlr, NULL);
    HSM_STATE_Create(&Camera_StateOn, "On", Camera_StateOn_Hndlr, NULL);
    HSM_STATE_Create(&Camera_StateOnShoot, "OnShoot", Camera_StateOnShoot_Hndlr, &Camera_StateOn);
    HSM_STATE_Create(&Camera_StateOnDisplay, "OnDisplay", Camera_StateOnDisplay_Hndlr, &Camera_StateOn);
    HSM_STATE_Create(&Camera_StateOnDisplayPlay, "OnDisplayPlay", Camera_StateOnDisplayPlay_Hndlr, &Camera_StateOnDisplay);
    HSM_STATE_Create(&Camera_StateOnDisplayMenu, "OnDisplayMenu", Camera_StateOnDisplayMenu_Hndlr, &Camera_StateOnDisplay);

    // Step 2: Initiailize the HSM and starting state
    HSM_Create((HSM *)This, name, &Camera_StateOff);

    // Step 3: [Optional] Enable HSM debug
    HSM_SET_PREFIX((HSM *)This, "[Camera] ");
    HSM_SET_DEBUG((HSM *)This, HSM_SHOW_ALL);

    // Step 4: Camera object initialization
    // ==> YOUR.CHANGES.GO.HERE <==
}

void Camera_Run(Camera_t *This, HSM_EVENT event, void *param)
{
    // Uncomment below to suppress debug for a specific event (e.g. periodic timer event)
    // if (event == <NAME.OF.EVENT.YOU.WANT.TO.SUPPRESS>)
    //    HSM_SUPPRESS_DEBUG((HSM *)This, HSM_SHOW_ALL);

    // Invoke HSM
    HSM_Run((HSM *)This, event, param);
}

const char *HSM_Evt2Str(uint32_t event)
{
    switch (event)
    {
    case RELEASE:
        return "RELEASE";
    case MODE_CMD:
        return "MODE_CMD";
    case LOWBATT_EVT:
        return "LOWBATT_EVT";
    case PWR_CMD:
        return "PWR_CMD";
    }
    return "Undefined";
}

Camera_t basic;
void main(void)
{
    // Instantiate Camera
    Camera_Init(&basic, "Canon");
    // Turn on the Power
    Camera_Run(&basic, PWR_CMD, 0);
    // Take a picture
    Camera_Run(&basic, RELEASE, (void*)1);
    // Take another picture
    Camera_Run(&basic, RELEASE, 0);
    // Playback the photo
    Camera_Run(&basic, MODE_CMD, 0);
    // Oops, pushed the release button by accident
    Camera_Run(&basic, RELEASE, 0);
    // Go to menu settings
    Camera_Run(&basic, MODE_CMD, 0);
    // Uh oh, low battery
    Camera_Run(&basic, LOWBATT_EVT, 0);
    // Time to turn it off
    Camera_Run(&basic, PWR_CMD, 0);
}

