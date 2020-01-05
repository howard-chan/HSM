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
#ifndef __HSM_HPP__
#define __HSM_HPP__

#include <stdint.h>
#include <string>

enum class HsmEvent
{
    kNULL = 0,
    kSTART = 1,
    kINIT = -3,
    kENTRY = -2,
    kEXIT = -1
};

typedef uint32_t HSM_EVENT;
typedef uint32_t HSM_STATE;
typedef HSM_EVENT (*pFnHandler_t)(HSM_EVENT event, uint32_t param);

class Hsm
{
public:
    HSM_EVENT __RootHandler(HSM_EVENT event, uint32_t param)
    {
        // #TODO: Add HSM_DEBUG_EVT2STR
        printf("\tEvent:%x dropped, No Parent handling of %s[%s] param %x\n",
            event, hsmName.c_str(), pCurState->name.c_str(), param);
        //#TODO: Replace with kNULL
        return 0;
    }

    struct State {
        int level;                  // Depth level of the state
        std::string name;           // name of state
        pFnHandler_t handler;       // associated event handler for state
        State *parent;              // parent state

        State(std::string name, pFnHandler_t handler, State *parent=nullptr)
            : name{name}, handler{handler}, parent{parent}
        {
            // Set the depth level of state
            level = (parent) ? (parent->level + 1) : 0;
        };
    };

private:
    std::string hsmName;
    bool bDebug;
    State *pCurState;
    State stateRoot;

public:
    Hsm(std::string name="") 
        : hsmName{name}, bDebug{false} //, stateRoot({":root:"}, Hsm::__RootHandler)
    {
    };
    void run(HSM_EVENT event, void *param);
    void tran(HSM_STATE *nextState, void *param, void (*method)(void *param));
    void getState(void);
    bool isInState(HSM_STATE *state);
};

#endif // __HSM_HPP__
