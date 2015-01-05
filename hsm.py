"""
    hsm.py - Copyright 2014 - Howard Chan
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

# HSM Class in Python
# H.Chan

class HSM_Event:
    INIT, ENTRY, EXIT, FIRST = range(1, 5)

class HSM(object):
    class State:
        """This is a state class used internally"""    
        def __init__(self, name, handler, parent=None):
            """Creates a state instance"""
            self.level = 0                  # depth level of the state
            self.name = name                # name of state
            self.handler = handler          # associated event handler for state
            self.parent = parent            # parent state
            if isinstance(parent, HSM.State):
                self.level = parent.level + 1
            elif parent != None:
                print "Raise exception here: The parent is not a State"
                raise "Raise exception here: The parent is not a State"

    def __RootHandler(self, event):
        print "\tUnhandled event:%s %s[%s]" % (event, self.name, self.curState)
        raise "Unhandled event:", (event, self.name, self.curState)
        return None

    def __init__(self, name=""):
        """Creates the HSM instance"""
        self.name = name
        self.hsmDebug = False
        self.stateRoot = self.State(":root:", self.__RootHandler)
        self.curState = self.stateRoot
    
    def CreateState(self, name, handler, parent=None):
        """This adds a state to the HSM"""
        if parent == None:
            parent = self.stateRoot
        return self.State(name, handler, parent)

    def SetInitState(self, initState):
        """This sets the initial HSM state"""
        if isinstance(initState, self.State):
            self.curState = initState
        else:
            print "This is not a HSM State"        

    def GetState(self):
        """This returns the current HSM state"""
        return self.curState

    def Run(self, event = None):
        """This runs the state's event handler and forwards unhandled events to
        the parent state"""
        state = self.curState
        if self.hsmDebug:
            pass
            print "Run %s[%s](evt:%s)" % (self.name, state.name, event)
        while event:
            event = state.handler(event)
            state = state.parent
            if self.hsmDebug and event:
                print "  evt:%s unhandled, passing to %s[%s]" % (event, self.name, state.name)

    def Tran(self, nextState, method = None):
        """This performs the state transition with calls of exit, entry and init
        Bulk of the work handles the exit and entry event during transitions"""
        if self.hsmDebug:
            print "Tran %s[%s -> %s]" % (self.name, self.curState.name, nextState.name)
        # 1) Find the lowest common parent state
        src = self.curState
        dst = nextState
        list_exit = []
        list_entry = []
        # 1a) Equalize the levels
        while (src.level != dst.level):
            if (src.level > dst.level):
                #source is deeper
                list_exit = list_exit + [src]
                src = src.parent
            else:
                #destination is deeper
                list_entry = [dst] + list_entry
                dst = dst.parent
        # 1b) find the common parent
        while (src != dst):
            list_exit = list_exit + [src]
            src = src.parent
            list_entry = [dst] + list_entry
            dst = dst.parent
        # 2) Process all the exit events
        #TODO: Add check to ensure "Tran()" not called on exit
        for src in list_exit:
            if self.hsmDebug:
                print "  %s[%s](%s)" % (self.name, src.name, "EXIT")
            src.handler(HSM_Event.EXIT)
        # 3) Call the transitional method
        if method and hasattr(method, '__call__'):
            method()
        # 4) Process all the entry events
        #TODO: Add check to ensure "Tran()" not called on entry
        for dst in list_entry:
            if self.hsmDebug:
                print "  %s[%s](%s)" % (self.name, dst.name, "ENTRY")
            dst.handler(HSM_Event.ENTRY)
        #5) Now we can set the destination state
        self.curState = nextState
        #6) Invoke INIT signal
        if self.hsmDebug:
            print "  %s[%s](%s)" % (self.name, nextState.name, "INIT")
        self.curState.handler(HSM_Event.INIT)
