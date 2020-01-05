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

# To use common.mk, define the following variables to your makefile
# 
# Step 1: The caller makefile MUST have the following variables defined
#   $(TARGET):      Target binary name (i.e. executable)
#   $(ROOTDIR):     Root path of the repo source relative to project makefile
#   $(SRCDIR):      list of source directories seperated by space
#   $(INCDIR):      list of include directories seperated by space
#   $(SRCFILE):     list of sources files NOT in SRCDIR to include into build
#
# Step 2: The caller makefile MAY define the toolchain variables, else defaults
#   $(DEFINES):     Defines shared between C and C++ Compiler
#   $(CXXFLAGS):    C++ flags
#   $(CCFLAGS):     C flags
#   $(LDFLAGS):     Linker Flags

#
# Default Toolchain, override in the makefile if required
#
CC    	    ?= gcc
CXX         ?= g++
LN          ?= g++
NM          ?= nm

#
# Default Toolchain Settings, override in the makefile if required
#
CXXFLAGS    ?= -g -std=c++11 -Werror
CCFLAGS     ?= -g -std=gnu11 -Werror
LDFLAGS     ?= -Wl,-Map=$(BINDIR)/$(TARGET).map
BINDIR      ?= ./bin
SRCPATH     ?= $(ROOTDIR)
#
# The builds SHALL use the following variables to build the target
#
CXXFLAGS    += $(DEFINES) $(addprefix -I,$(INCDIR))
CCFLAGS     += $(DEFINES) $(addprefix -I,$(INCDIR))
SRC         =  $(foreach srcdir,$(SRCDIR),$(wildcard $(srcdir)/*.c))
SRC         += $(foreach srcdir,$(SRCDIR),$(wildcard $(srcdir)/*.cpp))
SRC         += $(SRCFILE)
OBJ         += $(patsubst $(SRCPATH)/%,$(BINDIR)/%,$(addsuffix .o,$(basename $(SRC))))

#
# The targets
#
.PHONY: default all clean

# Builds the target
default all: $(TARGET)

# Cleans the build directory
clean:
	rm -rf $(BINDIR)

# For debug, Usage: "make print-<Makefile.Variable.To.Inspect>"
print-%:
	$(info $* = $($*))

# SECONDEXPANSION is used to resolve path to the .sentinel to create the subdirectory
.SECONDEXPANSION:
$(BINDIR)/%.sentinel:
	@mkdir -p $(dir $@)
	@touch $@

# Builds target from objects (.o)
$(TARGET): $(OBJ)
	$(LN) $(LDFLAGS) -o $(BINDIR)/$@ $^ $(CCLIBS) $(CXXLIBS)

# Generate .o,.d from .cpp
$(BINDIR)/%.o: $(SRCPATH)/%.cpp $$(dir $(BINDIR)/%).sentinel
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	# Build dependency file (.d)
	$(CXX) $(CXXFLAGS) -MM -MP -MT"$@" -MF $(BINDIR)/$*.d $<

# Generate .o,.d from .c
$(BINDIR)/%.o: $(SRCPATH)/%.c $$(dir $(BINDIR)/%).sentinel
	$(CC) $(CCFLAGS) -c -o $@ $<
	# Build dependency file (.d)
	$(CC) $(CCFLAGS) -MM -MP -MT"$@" -MF $(BINDIR)/$*.d $<

# Ensure all dependencies are built
-include $(patsubst %.o, %.d, $(OBJ))
