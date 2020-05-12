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

# HSM Class in Python
# H.Chan

class HsmEvent:
    """
    This class describes a hsm event.
    """
    INIT, ENTRY, EXIT = range(-3, 0)


class HSM(object):
    """
    This class describes a hsm.
    """

    RED = "\033[1;31m"
    GRN = "\033[1;32m"
    YEL = "\033[1;33m"
    BLU = "\033[1;34m"
    MAG = "\033[1;35m"
    CYN = "\033[1;36m"
    NON = "\033[0m"

    class State:
        """
        This is a state class used internally
        """

        def __init__(self, name, handler, parent=None):
            """
            Creates a state instance

            :param      name:     Name of the state (for debugging)
            :type       name:     string
            :param      handler:  State handler that implements state behavior.
            :type       handler:  function object
            :param      parent:   The parent state. If None, then internal root
                                  state is assigned as parent as a catch-all
            :type       parent:   State
            """
            self.level = 0                  # depth level of the state
            self.name = name                # name of state
            self.handler = handler          # associated event handler for state
            self.parent = parent            # parent state
            if isinstance(parent, HSM.State):
                self.level = parent.level + 1
            elif parent is not None:
                print("Raise exception here: The parent is not a State")
                raise "Raise exception here: The parent is not a State"


    def __root_handler(self, event):
        """
        Default Root handler which reports an unhandled event

        :param      event:  The event
        :type       event:  number

        :returns:   None - which indicates the event has been consumed
        :rtype:     None
        """
        print(HSM.YEL + "\tEvent:%s dropped, No Parent handling of %s[%s]" %
              (event, self.name, self.cur_state.name) + HSM.NON)
        return None

    def __debug_run(self, msg):
        if self.debug:
            print(HSM.BLU + msg + HSM.NON)

    def __debug_tran(self, msg):
        if self.debug:
            print(HSM.CYN + msg + HSM.NON)

    def __debug_intact(self, msg):
        if self.debug:
            print(HSM.CYN + msg + HSM.NON)

    def __init__(self, name=""):
        """
        Creates the HSM instance. Required for each instance.

        :param      name:  The name of the state machine instance (for debugging)
        :type       name:  string
        """
        self.lock = False
        self.name = name
        self.debug = False
        self.state_root = self.State(":root:", self.__root_handler)
        self.cur_state = self.state_root

    def create_state(self, name, handler, parent=None):
        """
        Create an HSM STATE object

        :param      name:     Name of the state
        :type       name:     string
        :param      handler:  State handler
        :type       handler:  function object
        :param      parent:   The parent state
        :type       parent:   State

        :returns:   created State object instance
        :rtype:     State
        """
        if parent is None:
            parent = self.state_root
        return self.State(name, handler, parent)

    def set_init_state(self, init_state):
        """
        This sets the initial HSM state

        :param      init_state:  The initial state of HSM
        :type       init_state:  State
        """
        if isinstance(init_state, self.State):
            self.cur_state = init_state
        else:
            print(HSM.RED + "This is not a HSM State" + HSM.NON)

    def get_state(self):
        """
        This returns the current HSM state

        :returns:   The current state of HSM
        :rtype:     State
        """
        return self.cur_state

    def run(self, event=None):
        """
        This runs the state's event handler and forwards unhandled events to the
        parent state

        :param      event:  The event
        :type       event:  Number
        """
        state = self.cur_state
        self.__debug_run("Run %s[%s](evt:%s)" % (self.name, state.name, event))
        while event:
            event = state.handler(event)
            state = state.parent
            if event:
                self.__debug_run("  evt:%s unhandled, passing to %s[%s]" %
                                 (event, self.name, state.name))

    def tran(self, next_state, method=None):
        """
        This performs the state transition with calls of exit, entry and init
        Bulk of the work handles the exit and entry event during transitions

        :param      next_state:  The next state
        :type       next_state:  State
        :param      method:      The method
        :type       method:      Function Object
        """
        # Check for illegal call to HSM tran in HSME_ENTRY or HSME_EXIT
        if self.lock:
            print(HSM.RED + "!!!!Illegal call of HSM_Tran[%s -> %s] in HSME_ENTRY or HSME_EXIT Handler!!!!" %
                  (self.cur_state.name, next_state.name) + HSM.NON)
            return
        self.lock = True

        self.__debug_tran("Tran %s[%s -> %s]" % (self.name, self.cur_state.name, next_state.name))
        # 1) Find the lowest common parent state
        src = self.cur_state
        dst = next_state
        list_exit = []
        list_entry = []
        # 1a) Equalize the levels
        while src.level != dst.level:
            if src.level > dst.level:
                #source is deeper
                list_exit = list_exit + [src]
                src = src.parent
            else:
                #destination is deeper
                list_entry = [dst] + list_entry
                dst = dst.parent
        # 1b) find the common parent
        while src != dst:
            list_exit = list_exit + [src]
            src = src.parent
            list_entry = [dst] + list_entry
            dst = dst.parent
        # 2) Process all the exit events
        for src in list_exit:
            self.__debug_intact("  %s[%s](%s)" % (self.name, src.name, "EXIT"))
            src.handler(HsmEvent.EXIT)
        # 3) Call the transitional method
        if method and hasattr(method, '__call__'):
            method()
        # 4) Process all the entry events
        for dst in list_entry:
            self.__debug_intact("  %s[%s](%s)" % (self.name, dst.name, "ENTRY"))
            dst.handler(HsmEvent.ENTRY)
        # 5) Now we can set the destination state
        self.cur_state = next_state

        # Unlock safety check
        self.lock = False
        # 7) Invoke INIT signal
        self.__debug_intact("  %s[%s](%s)" % (self.name, next_state.name, "INIT"))
        self.cur_state.handler(HsmEvent.INIT)
