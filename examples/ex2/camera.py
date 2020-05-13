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

import sys
import os
sys.path.append(os.path.abspath('../../src/python'))

from hsm import HSM
from hsm import HsmEvent

class evt(HsmEvent):
    '''Class that implements the Camera events and inherits from HSM_Event class
    Since this is python, we can just use strings instead of enumeration which
    makes this demo easier to demonstrate the event handling
    '''
    #PWR, RELEASE, MODE, LOWBATT = range(HSM_Event.FIRST, HSM_Event.FIRST+4)
    PWR = "PWR"
    RELEASE = "RELEASE"
    MODE = "MODE"
    LOWBATT = "LOWBATT"

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
        self.state_Off = self.create_state("Off", self.StateOffHndlr)
        self.state_On = self.create_state("On", self.StateOnHndlr)
        self.state_OnShoot = self.create_state("On.Shoot", self.StateOnShootHndlr, self.state_On)
        self.state_OnDisp = self.create_state("On.Disp", self.StateOnDispHndlr, self.state_On)
        self.state_OnDispPlay = self.create_state("On.Disp.Play", self.StateOnDispPlayHndlr, self.state_OnDisp)
        self.state_OnDispMenu = self.create_state("On.Disp.Menu", self.StateOnDispMenuHndlr, self.state_OnDisp)
        # Step 3: Set the starting state
        self.set_init_state(self.state_Off)
        # Step 4[Optional] Enable HSM debug
        self.debug_set(HSM.SHOW_ALL)
        self.prefix = "[DBG] "

    def StateOffHndlr(self, event, param):
        if event == evt.ENTRY:
            print("\tEnter Low Power Mode")
        elif event == evt.EXIT:
            print("\tExit Low Power Mode")
        elif event == evt.PWR:
            self.tran(self.state_On)
            return None
        return event

    def StateOnHndlr(self, event, param):
        if event == evt.ENTRY:
            print("\tOpen Lens")
        elif event == evt.EXIT:
            print("\tClose Lens")
        elif event == evt.INIT:
            self.tran(self.state_OnShoot)
        elif event == evt.PWR:
            self.tran(self.state_Off)
            return None
        elif event == evt.LOWBATT:
            print("\tBeep low battery warning")
            return None
        return event

    def StateOnShootHndlr(self, event, param):
        if event == evt.ENTRY:
            print("\tEnable Sensor")
        elif event == evt.EXIT:
            print("\tDisable Sensor")
        elif event == evt.RELEASE:
            print("\tCLICK!, save photo")
            return None
        elif event == evt.MODE:
            self.tran(self.state_OnDispPlay)
            return None
        return event

    def StateOnDispHndlr(self, event, param):
        if event == evt.ENTRY:
            print("\tTurn on LCD")
        elif event == evt.EXIT:
            print("\tTurn off LCD")
        return event

    def StateOnDispPlayHndlr(self, event, param):
        if event == evt.ENTRY:
            print("\tDisplay Pictures")
        elif event == evt.MODE:
            self.tran(self.state_OnDispMenu)
            return None
        return event

    def StateOnDispMenuHndlr(self, event, param):
        if event == evt.ENTRY:
            print("\tDisplay Menu")
        elif event == evt.MODE:
            self.tran(self.state_OnShoot)
            return None
        return event

def main():
    # Instantiate Camera
    canon = Camera("Canon")
    # Turn on the Power
    canon.run(evt.PWR)
    # Take a picture
    canon.run(evt.RELEASE)
    # Take another picture
    canon.run(evt.RELEASE)
    # Playback the photo
    canon.run(evt.MODE)
    # Oops, pushed the release button by accident
    canon.run(evt.RELEASE)
    # Go to menu settings
    canon.run(evt.MODE)
    # Uh oh, low battery
    canon.run(evt.LOWBATT)
    # Time to turn it off
    canon.run(evt.PWR)


if __name__ == "__main__":
    main()
