#!/usr/bin/python3
"""
The MIT License (MIT)

Copyright (c) 2015-2019 Howard Chan
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

"""
uml2hsm.py generates a code template from PlantUML syntax.  This provides a
mapping from the UML HSM model to HSM code.  For information on generating a
UML model diagram with PlantUML refer to http://plantuml.com/state-diagram

Linux Requirements:
* PlantUML - Download from http://plantuml.com/download
* JavaRE- sudo apt install default-jre
* Graphviz - sudo apt install graphviz
"""

# For system
import sys
import argparse
import io
# For parsing
import re
from collections import OrderedDict
import json

# Configurations
TAB_SIZE=4
USER_TEXT = "==> YOUR.CHANGES.GO.HERE <=="
EVT2STR_FUNC_NAME = "HSM_Evt2Str"


class Uml2Hsm:
    """
    @brief      This class parses code (i.e. PlantUML, "C", "C++", ".py") into
                an internal representation of the HSM model.  The HSM model can
                then be used to generate back to code of a different format
                (i.e. PlantUML, "C", "C++", ".py")
    """
    reUmlList = [
        #----RE match for HSM Name: title <name.of.your.hsm(1)>---
        # e.g.                         title camera
        ('NAME',                   r'^(?i)\s*title\s*(\w+)$'),
        # Anchor beginning           ^
        # Ignore case                 (?i)
        # Ignore spaces                   \s*     \s*
        # Match                              title
        # Group1 (name of HSM)                       (\w+)
        # Anchor to end of string                         $
        #----RE match for init state: [*] --> <state(1)> [: <guard>]---
        # e.g.                        [*] --> off
        ('INIT',                   r'\[\s*\*\s*\]\s*[-]+(?:\w*[-]+)?>\s*(\w+)(?:\s*:\s*(.*))?'),
        # Match [*] w/ whitespace    \[\s*\*\s*\]\s*
        # Match --> or ->                           [-]+            >\s*
        # Optional Arrow direction                      (?:\w*[-]+)?
        # Group1 (state)                                                (\w+)
        # Optional Non-Capture                                               (?:\s*:\s*    )?
        # Group2 (guard/action)                                                        (.*)
        #----RE match for events in state: state <state(3)> : <event/action(4)>----
        # e.g.                             state off : entry / power_on()
        ('EVENT',                   r'(?im)state\s+(\w*?)\s*:\s*(.*?)$'),
        # Multiline & IgnoreCase      (?im)
        # Match state                      state\s+
        # Group1 (state)                           (\w*?)
        # Match ' : '                                    \s*:\s*
        # Group2 (event/action)                                 (.*?)
        # Anchor to end of string                                    $
        #----RE match for state tran: <src(1)> --> <dst(2) : [event/action(3)]----
        # e.g.                             off --> on      : POWER_EVT / enable()
        ('TRAN',                   r'(\w+)\s*[-]+(?:\w*[-]+)?>\s*(\w+)\s*:\s*(.*?)$'),
        # Group1 (src state)         (\w+)
        # Match --> or ->                 \s*[-]+            >\s*
        # Optional Arrow direction               (?:\w*[-]+)?
        # Group2 (dst state)                                     (\w+)
        # Match ' : '                                                 \s*:\s*
        # Group3 (action)                                                    (.*?)
        # Anchor to end                                                           $
        #----RE match for nested state: state <state(1)> {----
        # e.g.                          state off {
        ('NEST',                   r'(?i)state\s+(\w+)\s+\{'),
        # Ignore case                (?i)
        # Match state                    state
        # Match white space                   \s+     \s+\{
        # Group1 (state)                         (\w+)
        #----RE match for unnest state: }----
        # e.g.                          }
        ('UNNEST',                 r'\}'),
        # Match ending nest          \}
        #----RE match for note 1:    note <position>   : <note>----
        # e.g.                       note top of state : this is a note
        ('NOTE1',                  r'note(.*)\s*:\s*(.*)$'),
        # Match note                 note
        # Group1 (Position)              (.*)
        # Match ' : '                        \s*:\s*
        # Group2 (body)                             (.*)
        # Anchor to end of string                       $
        #----RE match for note 2:    note <position>    <note>             end note----
        # e.g.                       note top of state\n  this is a note\n end note
        ('NOTE2',                  r'note(.*)((?:\n.+)+)\s*end note'),
        # Match note                 note
        # Group1 (Position)              (.*)
        # Optional Group2 (body)             ((?:\n.+)+)
        # Match end note                                \s*end note
        #----RE match for note 3:    note "<note>"" as <label>----
        # e.g.                       note "This is a note" as N
        ('NOTE3',                  r'note\s+"(.*)"\s+as\s+(\w*)$'),
        # Match note                 note
        # Spacing                        \s+
        # Group1 (body)                     "(.*)"
        # Match 'as'                              \s+as\s+
        # Group2 (label)                                  (\w*)
        # Anchor to end of string                              $
    ]
    # Combine the regex pattern into one
    reUmlStr = '|'.join('(?P<%s>%s)' % pair for pair in reUmlList)
    pattUmlTok = re.compile(reUmlStr)

    #----RE match for body: @startuml <image(1)> <body(2)> @enduml----
    # e.g.                  @startuml camera.png ...       @enduml
    pattUML =  re.compile(r'@startuml\s+(\w*?)(.*?)@enduml', re.M | re.S)
    # Start of UML          @startuml
    # Consume any space              \s+
    # Group1 (image name)               (\w*?)
    # Griyo2 (HSM model body)                 (.*?)
    # End of UML                                   @enduml

    #----RE match for event & action: <event(1)> [guard(2)] / <action(3)>----
    # e.g.                            POWER_EVT[In Low Power] / power_enable()
    pattEvent = re.compile(r'(\w+)(?:\s*\[(.*?)\])?(?:\s*\/\s*(.*))?$')
    # Group1 (event)         (\w+)
    # Optional Non-Capture        (?:\s*\[     \])?
    # Group2 (guard)                      (.*?)
    # Optional Non-Capture                         (?:\s*\/\s*    )?
    # Group3 (action)                                         (.*)
    # Anchor to end                                                 $

    def __init__(self, lang, debug=False):
        """
        @brief      Constructs the object.

        @param      self   The object
        @param      lang   The language to convert HSM model to
        @param      debug  Enable debug of parsing and internal HSM model
        """
        # list of supported languages
        self.supportedLang = { 'c' : Uml2Hsm._genHsmC, 'c++' : Uml2Hsm._genHsmCpp,
                               'python' : Uml2Hsm._genHsmPy, 'puml' : Uml2Hsm._genPlantUML }
        if lang not in self.supportedLang.keys():
            raise 'Language {} is not supported'.format(lang)
        self.lang = lang
        print('Selected Language: {}'.format(self.lang))
        self.debug = debug
        self.hsmList = []
        self.parent = [ None ]

    def _addState(self, state):
        """
        @brief      Adds a state.to current HSM

        @param      self   The object
        @param      state  The state name
        """
        # Check if object needs to be created
        if state not in self.curHsm['state'].keys():
            # State doesn't exist, so initialize one
            self.curHsm['state'][state] = OrderedDict([
                ('parent', self.parent[-1]),
                ('event', OrderedDict())
            ])

    def _addEvent(self, state, evtact, dst=None):
        """
        @brief      Adds an event handler to state

        @param      self    The object
        @param      state   The state name
        @param      evtact  The string of the event, guard and action
        @param      dst     The destination state for state transition
        """
        try:
            event, guard, action = self.pattEvent.search(evtact).groups()
            if self.debug:
                print("Adding Event to {} - {} : ({}, {}, {})".format(state, evtact, event, guard, action))
            # Add dictionary for new events
            if event not in self.curHsm['state'][state]['event'].keys():
                self.curHsm['state'][state]['event'][event] = OrderedDict()
            # Add event.  NOTE: Key None is the default action
            if guard in self.curHsm['state'][state]['event'][event].keys():
                print('Warning: Event "{}[{}]" for state "{}" is being replaced)'.format(event, guard, state))
            self.curHsm['state'][state]['event'][event][guard] = (action, dst)
            # Add to event list for defining event enumeration
            if event not in self.curHsm['event']:
                self.curHsm['event'] += [ event ]
        except AttributeError as e:
            print("Error @ {}: {}".format(self.matchCnt, e))

    def _addNote(self, body, pos=""):
        """
        @brief      Adds a note.

        @param      self  The object
        @param      body  The body of the note
        @param      pos   The position directive which contains the association
                          to a state or hsm

        @details    A note is associated with a state or hsm.  The note can
                    contain a <<stereotype>> of the types {include, code, test,
                    comment}.  Depending of the stereotype, the note's body can
                    be inserted into the code body at locations determined by
                    pos

        @return     { description_of_the_return_value }
        """
        # Determine note type
        match = re.search(r'<<(.*)>>', body)
        if match:
            ntype = match.group(1)
            body = body.replace('<<{}>>'.format(ntype),'')
        else:
            ntype = 'comment'
        # Determine state association
        match = re.search(r'(\w*)\s+of\s+(\w*)\s*', pos)
        if match:
            pos = match.group(2)
        else:
            pos = None
        # Split the body
        body = [ line for line in body.split('\n') if line.lstrip(' ') ]
        # Find indentation length
        indentLen = 1000
        for line in body:
            indentLen = min(indentLen, len(line) - len(line.lstrip(' ')))
        # Set base indentation
        body = [ line[indentLen:] for line in body ]
        # Add note to hsm
        if pos not in self.curHsm['note'].keys():
            self.curHsm['note'][pos] = []
        self.curHsm['note'][pos].append((ntype, body))
        # Show debug info
        if self.debug:
            print("Adding {} to {}".format(ntype, pos))
            for line in body:
                print("  {}".format(line))

    def _getNote(self, hsm, state, ntype):
        """
        @brief      Gets the note.body

        @param      self   The object
        @param      hsm    The hsm
        @param      state  The state
        @param      ntype  The note stereotype

        @return     The note body
        """
        ret = []
        if state in hsm['note'].keys():
            noteList = hsm['note'][state]
            for (curtype, body) in noteList:
                if curtype == ntype:
                    # Extend list with contents of body
                    ret.extend(body)
        return ret

    def openUml(self, srcfile):
        """
        @brief      Starts the parser

        @param      self     The object
        @param      srcfile  The srcfile

        @details    Refer to https://docs.python.org/3/library/re.html

        @return     List of HSM models
        """
        with open(srcfile, "r") as src:
            umlDoc = src.read()
            grpDict = self.pattUmlTok.groupindex
            # Extract the UML code from the file
            for match in self.pattUML.finditer(umlDoc):
                # UML Code found, Now process the UML Code
                self.curHsm = OrderedDict([
                    ("name", ""),
                    ("image", match.group(1)),
                    ("init", None),
                    ("state", OrderedDict()),
                    ("event", []),
                    ("note", OrderedDict()),
                ])
                umlBody = match.group(2)
                self.matchCnt = 0
                # Parse the uml and retain the order
                for mo in self.pattUmlTok.finditer(umlBody):
                    self.matchCnt += 1
                    grpName = mo.lastgroup
                    grpIdx = grpDict[grpName]
                    # Process the match accordingly
                    if grpName == 'NAME':
                        self.curHsm["name"] = mo.group(grpIdx + 1)
                        continue
                    elif grpName == 'INIT':
                        state = mo.group(grpIdx + 1)
                        guard = mo.group(grpIdx + 2)
                        self._addState(state)
                        if self.parent[-1]:
                            # Parent found, add INIT to parent
                            # Convert guard to event/action
                            evtact = 'init' if not guard else 'init ' + guard
                            self._addEvent(self.parent[-1], evtact, state)
                        else:
                            # No Parent, this is the initial state of the HSM
                            self.curHsm['init'] = state
                        continue
                    elif grpName == 'EVENT':
                        state = mo.group(grpIdx + 1)
                        evtact = mo.group(grpIdx + 2)
                        self._addState(state)
                        self._addEvent(state, evtact)
                        continue
                    elif grpName == 'TRAN':
                        src = mo.group(grpIdx + 1)
                        dst = mo.group(grpIdx + 2)
                        evtact = mo.group(grpIdx + 3)
                        self._addState(src)
                        self._addEvent(src, evtact, dst)
                        continue
                    elif grpName == 'NEST':
                        state = mo.group(grpIdx + 1)
                        self.parent.append(state)
                        self._addState(state)
                        continue
                    elif grpName == 'UNNEST':
                        self.parent.pop()
                        continue
                    elif grpName in ['NOTE1', 'NOTE2']:
                        pos  = mo.group(grpIdx + 1)
                        body = mo.group(grpIdx + 2)
                        self._addNote(body, pos)
                        continue
                    elif grpName == 'NOTE3':
                        body = mo.group(grpIdx + 1)
                        lbl  = mo.group(grpIdx + 2)
                        self._addNote(body)
                        continue
                # Perform validation tests
                # TODO
                # Add to hsm list
                self.hsmList.append(self.curHsm)
        return self.hsmList

    def openCode(self, srcfile):
        pass

    def _genHsmC(self, hsm, out):
        """
        @brief      Generate "C" implementation of HSM

        @param      self  The object
        @param      hsm   The hsm
        @param      out   output file
        """
        # Table for replacing PlantUML events with HSM "C" defines
        reservedEventMap = {
            'null'  : 'HSME_NULL',
            'init'  : 'HSME_INIT',
            'entry' : 'HSME_ENTRY',
            'exit'  : 'HSME_EXIT'
        }
        # Helper lambdas
        indent = lambda x:" "*TAB_SIZE*x
        stateObjTmpl = lambda state:'{}_State{}'.format(hsm['name'],state)

        #---Generate Header File---
        out.write('//----The following belongs to {}.h----\n'.format(hsm['name']))
        out.write('#include "hsm.h"\n\n')
        # Filter out reserved event values from Event list
        eventSet = set(hsm["event"]) - set([ k for k in reservedEventMap.keys() ] + [ v for v in reservedEventMap.keys() ])
        # Generate the Event Defines.  NOTE: First event starts at 1, since 0 is HSME_NULL
        out.write('// {} HSM Events\n'.format(hsm['name']))
        maxlen = max(map(lambda x:len(x), eventSet))
        for val, event in enumerate(eventSet, start=1):
            out.write('#define {:<{width}} ({})\n'.format(event, val, width=maxlen+TAB_SIZE))
        out.write('\n')

        # Define HSM class
        out.write('// Definition of {} class\n'.format(hsm['name']))
        out.write('typedef struct\n')
        out.write('{\n')
        out.write('{}// Parent  NOTE: HSM parent must be defined first\n'.format(indent(1)))
        out.write('{}HSM parent;\n'.format(indent(1)))
        out.write('\n')
        out.write('{}// Child members\n'.format(indent(1)))
        out.write('{}// {}\n'.format(indent(1), USER_TEXT))
        out.write('} %s_t;\n' % hsm['name'])
        out.write('\n')

        #---Generate Source File---
        out.write('//----The following belongs to {}.c----\n'.format(hsm['name']))
        # Insert include section
        body = self._getNote(hsm, hsm['name'], 'include')
        for line in body:
            out.write('{}\n'.format(line))
        out.write('\n')

        # Insert code if any
        body = self._getNote(hsm, hsm['name'], 'code')
        for line in body:
            out.write('{}\n'.format(line))
        out.write('\n')

        # Declare the State Objects
        out.write('// {} States\n'.format(hsm['name']))
        for state in hsm["state"].keys():
            out.write("HSM_STATE {};\n".format(stateObjTmpl(state)))
        out.write('\n')

        # Define the State Handler
        out.write('// {} State Handlers\n'.format(hsm['name']))
        for name, state in hsm["state"].items():
            out.write('HSM_EVENT {}_Hndlr(HSM *This, HSM_EVENT event, void *param)\n'.format(stateObjTmpl(name)))
            out.write('{\n')
            out.write('{}{}_t *p{} = ({}_t *)This;\n'.format(indent(1), hsm['name'], hsm['name'], hsm['name']))
            # Insert comments if any
            body = self._getNote(hsm, name, 'comment')
            for line in body:
                out.write('{}// {}\n'.format(indent(1), line))
            # Insert code if any
            body = self._getNote(hsm, name, 'code')
            for line in body:
                out.write('{}{}\n'.format(indent(1), line))
            # Insert Events handler
            out.write('{}switch (event)\n'.format(indent(1)))
            out.write('{}{{\n'.format(indent(1)))
            for event, actDict in state['event'].items():
                # For C, Replace keywords {init, entry, exit} with reserved Events Defines
                if event in reservedEventMap.keys():
                    event = reservedEventMap[event]
                # Add the event handler
                out.write('{}case {}:\n'.format(indent(1), event))
                # Check for guard conditions
                guardCnt = len(actDict)
                # Reorder default event (i.e. guard=None) to last, so it can be used as the default case
                if None in actDict.keys():
                    val = actDict.pop(None)
                    actDict[None] = val
                # Add the event handler including guard conditions
                for idx, (guard, (action, tran)) in enumerate(actDict.items()):
                    # Add guard condition if required
                    if guard:
                        if idx == 0:
                            out.write('{}if ({})\n'.format(indent(2), guard))
                        else:
                            out.write('{}else if ({})\n'.format(indent(2), guard))
                    elif guardCnt > 1:
                        out.write('{}else\n'.format(indent(2), guard))
                    # Open block if required
                    if guard or guardCnt > 1:
                        out.write('{}{{\n'.format(indent(2)))
                        indentCnt = 3
                    else:
                        indentCnt = 2
                    # Insert the action and/or transition
                    if action:
                        # Split '\n ' in PlantUML doc as a seperate action
                        actList = [ x.lstrip(' ') for x in action.split('\\n') ]
                        for act in actList:
                            if act:
                                out.write('{}{}\n'.format(indent(indentCnt), act))
                    if tran:
                        out.write('{}HSM_Tran(This, &{}, 0, NULL);\n'.format(indent(indentCnt), stateObjTmpl(tran)))
                    # Close block if required
                    if guard or guardCnt > 1:
                        out.write('{}}}\n'.format(indent(2)))
                # Consume event
                #TODO: Add option for deferred processing
                out.write('{}return 0;\n\n'.format(indent(2)))
            out.write('{}}}\n'.format(indent(1)))
            out.write('{}return event;\n'.format(indent(1)))
            out.write('}\n')
            out.write('\n')

        # Define the HSM Init
        out.write('void {}_Init({}_t *This, char *name)\n'.format(hsm["name"], hsm["name"]))
        out.write('{\n')
        out.write('{}// Step 1: Create the HSM States\n'.format(indent(1)))
        for name, state in hsm['state'].items():
            stateObj = stateObjTmpl(name)
            parent = "&"+stateObjTmpl(state['parent']) if state['parent'] else "NULL"
            out.write('{}HSM_STATE_Create(&{}, "{}", {}_Hndlr, {});\n'.format(indent(1), stateObj, name, stateObj, parent))
        out.write('\n')
        out.write('{}// Step 2: Initiailize the HSM and starting state\n'.format(indent(1)))
        out.write('{}HSM_Create((HSM *)This, name, &{});\n'.format(indent(1), stateObjTmpl(hsm["init"])))
        out.write('\n')
        out.write('{}// Step 3: [Optional] Enable HSM debug\n'.format(indent(1)))
        out.write('{}HSM_SET_PREFIX((HSM *)This, "[{}] ");\n'.format(indent(1), hsm['name']))
        out.write('{}HSM_SET_DEBUG((HSM *)This, HSM_SHOW_ALL);\n'.format(indent(1), hsm['name']))
        out.write('\n')
        out.write('{}// Step 4: {} object initialization\n'.format(indent(1), hsm['name']))
        out.write('{}// {}\n'.format(indent(1), USER_TEXT))
        out.write('}\n\n')

        # Define the HSM Run
        out.write('void {}_Run({}_t *This, HSM_EVENT event, void *param)\n'.format(hsm["name"], hsm["name"]))
        out.write('{\n')
        out.write('{}// Uncomment below to suppress debug for a specific event (e.g. periodic timer event)\n'.format(indent(1)))
        out.write('{}// if (event == <NAME.OF.EVENT.YOU.WANT.TO.SUPPRESS>)\n'.format(indent(1)))
        out.write('{}//{}HSM_SUPPRESS_DEBUG((HSM *)This, HSM_SHOW_ALL);\n'.format(indent(1),indent(1)))
        out.write('\n')
        out.write('{}// Invoke HSM\n'.format(indent(1)))
        out.write('{}HSM_Run((HSM *)This, event, param);\n'.format(indent(1)))
        out.write('}\n\n')

        # Add HSM Events to Strings function
        out.write('const char *{}(uint32_t event)\n'.format(EVT2STR_FUNC_NAME))
        out.write('{\n')
        out.write('{}switch (event)\n'.format(indent(1)))
        out.write('{}{{\n'.format(indent(1)))
        for event in eventSet:
            out.write('{}case {}:\n'.format(indent(1), event))
            out.write('{}return "{}";\n'.format(indent(2), event))
        out.write('{}}}\n'.format(indent(1)))
        out.write('{}return "Undefined";\n'.format(indent(1), event))
        out.write('}\n\n')

        # Insert test code if any
        body = self._getNote(hsm, hsm['name'], 'test')
        for line in body:
            out.write('{}\n'.format(line))
        out.write('\n')

    def _genHsmCpp(self, hsm, out):
        pass

    def _genHsmPy(self, hsm, out):
        pass

    def _genPlantUML(self, hsm, out):
        pass

    def genHsm(self, outfile=None, hsmIdx=0):
        try:
            out = open(outfile, "w") if outfile else io.StringIO()
            genLang = self.supportedLang[self.lang]
            genLang(self, self.hsmList[hsmIdx], out)
            if not outfile:
                print(out.getvalue())
        finally:
            out.close()

    def genUml(self, outfile=None):
        pass


def main(args):
    parser = argparse.ArgumentParser(description='UML to HSM model code generator')
    parser.add_argument('-r', '--reverse', action='store_true',
                        help='Reverse operation from HSM to UML model generation')
    parser.add_argument('-l', '--lang', action='store', default="c",
                        help='Language to translate to/from <c|c++|python|puml>')
    parser.add_argument('--debug', action='store_true',
                        help='Print internal HSM model for debug')
    parser.add_argument('-o', '--output', action='store', default=None,
                        help='output file of conversion')
    parser.add_argument('src', action='store', help='source file to convert from')
    args = parser.parse_args()

    # Create the parsing object
    uml2c = Uml2Hsm(args.lang, args.debug)
    # Generate the hsm models
    if not args.reverse:
        hsmList = uml2c.openUml(args.src)
    else:
        hsmList = uml2c.openCode(args.src)

    # Print internal HSM model
    if args.debug:
        for hsm in hsmList:
            print("{}".format(json.dumps(hsm, indent=4)))

    # Generate the output
    if not args.reverse:
        uml2c.genHsm(args.output)
    else:
        uml2c.genUml(args.output)


if __name__ == "__main__":
    main(sys.argv)
