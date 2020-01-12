"""
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
"""
import sys, os
sys.path.append(os.path.abspath('../../src/python'))

from hsm import HSM
from hsm import HSM_Event

class evt(HSM_Event):
    '''Class that implements the Camera events and inherits from HSM_Event class
    Since this is python, we can just use strings instead of enumeration which
    makes this demo easier to demonstrate the event handling
    '''
    #PWR, RELEASE, MODE, LOWBATT = range(HSM_Event.FIRST, HSM_Event.FIRST+4)
    PWR="PWR"
    RELEASE="RELEASE"
    MODE="MODE"
    LOWBATT="LOWBATT"

class Camera(HSM):
    '''Class that implements the Camera HSM and inherits from the HSM class
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
    '''
    def __init__(self, name):
        # Step 1: Initialize the HSM
        super(Camera, self).__init__(name)
        # Step 2: Create the HSM States
        self.stateOff = self.CreateState("Off", self.StateOffHndlr)
        self.stateOn = self.CreateState("On", self.StateOnHndlr)
        self.stateOnShoot = self.CreateState("On.Shoot", self.StateOnShootHndlr, self.stateOn)
        self.stateOnDisp = self.CreateState("On.Disp", self.StateOnDispHndlr, self.stateOn)
        self.stateOnDispPlay = self.CreateState("On.Disp.Play", self.StateOnDispPlayHndlr, self.stateOnDisp)
        self.stateOnDispMenu = self.CreateState("On.Disp.Menu", self.StateOnDispMenuHndlr, self.stateOnDisp)
        # Step 3: Set the starting state
        self.SetInitState(self.stateOff)
        # Step 4[Optional] Enable HSM debug
        self.hsmDebug = True

    def StateOffHndlr(self, event):
        if event == evt.ENTRY:
            print("\tEnter Low Power Mode")
        elif event == evt.EXIT:
            print("\tExit Low Power Mode")
        elif event == evt.PWR:
            self.Tran(self.stateOn)
            return None
        return event

    def StateOnHndlr(self, event):
        if event == evt.ENTRY:
            print("\tOpen Lens")
        elif event == evt.EXIT:
            print("\tClose Lens")
        elif event == evt.INIT:
            self.Tran(self.stateOnShoot)
        elif event == evt.PWR:
            self.Tran(self.stateOff)
            return None
        elif event == evt.LOWBATT:
            print("\tBeep low battery warning")
            return None
        return event

    def StateOnShootHndlr(self, event):
        if event == evt.ENTRY:
            print("\tEnable Sensor")
        elif event == evt.EXIT:
            print("\tDisable Sensor")
        elif event == evt.RELEASE:
            print("\tCLICK!, save photo")
            return None
        elif event == evt.MODE:
            self.Tran(self.stateOnDispPlay)
            return None
        return event

    def StateOnDispHndlr(self, event):
        if event == evt.ENTRY:
            print("\tTurn on LCD")
        elif event == evt.EXIT:
            print("\tTurn off LCD")
        return event

    def StateOnDispPlayHndlr(self, event):
        if event == evt.ENTRY:
            print("\tDisplay Pictures")
        elif event == evt.MODE:
            self.Tran(self.stateOnDispMenu)
            return None
        return event

    def StateOnDispMenuHndlr(self, event):
        if event == evt.ENTRY:
            print("\tDisplay Menu")
        elif event == evt.MODE:
            self.Tran(self.stateOnShoot)
            return None
        return event

def main():
    # Instantiate Camera
    basic = Camera("Canon")
    # Turn on the Power
    basic.Run(evt.PWR)
    # Take a picture
    basic.Run(evt.RELEASE)
    # Take another picture
    basic.Run(evt.RELEASE)
    # Playback the photo
    basic.Run(evt.MODE)
    # Oops, pushed the release button by accident
    basic.Run(evt.RELEASE)
    # Go to menu settings
    basic.Run(evt.MODE)
    # Uh oh, low battery
    basic.Run(evt.LOWBATT)
    # Time to turn it off
    basic.Run(evt.PWR)


if __name__ == "__main__":
    main()
