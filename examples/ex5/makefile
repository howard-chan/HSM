# The MIT License (MIT)
#
# Copyright (c) 2015-2020 Howard Chan
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

#
# Makefile for HSM example: Camera
#
# Step 1: Define the project parameters
TARGET  = camera
# Add this define to enable color debug in color-aware terminals
DEFINES += -DHSM_COLOR_ENABLE
# Add this define for customized HSM_EVENT value to string function for human readable debug
$(warning HSM_DEBUG_EVT2STR is not set)
#DEFINES += -DHSM_DEBUG_EVT2STR=HSM_Evt2Str

# Step 2: Define root
ROOTDIR = ../..

# Step 3: Define src and include path relative to the root
SRCFILE = $(ROOTDIR)/examples/ex5/camera.cpp
SRCDIR  = $(ROOTDIR)/src/cpp
INCDIR  = $(SRCDIR)

# Step 4: Include the common makefile to build project
include $(ROOTDIR)/tools/common.mk
