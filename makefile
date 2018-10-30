# The MIT License (MIT)
#
# Copyright (c) 2015-2018 Howard Chan
# https://github.com/howard-chan/HSM
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Makefile for HSM example: Camera

# Source files
TARGET	= camera

# Compler
CC    	= gcc
INC 	= -I .
OBJ 	= $(addsuffix .o, $(basename $(wildcard *.c)))
CFLAGS 	= -Werror $(INC) -g
# Add this define to enable color debug in color-aware terminals
CFLAGS += -DHSM_COLOR_ENABLE
# Add this define for customized HSM_EVENT value to string function for human readable debug
CFLAGS += -DHSM_DEBUG_EVT2STR=HSM_Evt2Str

# The targets
.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -Wl,-Map=$(TARGET).map -o $(TARGET) $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	$(CC) $(CFLAGS) -MM -MT"$(patsubst %.c,%.o,$<)" -MF $*.d $<

# Ensure all dependencies are built
-include *.d

clean:
	rm -f *.o
	rm -f *.d
	rm -f *.map
	rm -f $(TARGET)
