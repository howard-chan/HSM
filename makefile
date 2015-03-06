# Makefile for HSM example: Camera

# Source files
TARGET	= camera

# Compler 
CC    	= gcc
INC 	= -I .
OBJ 	= $(addsuffix .o, $(basename $(wildcard *.c)))
CFLAGS 	= -Werror $(INC)

# The targets
.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	$(CC) $(CFLAGS) -MM -MT"$(patsubst %.c,%.o,$<)" -MF $*.d $<

# Ensure all dependencies are built
-include *.d

clean:
	rm -f *.o
	rm -f *.d
	rm $(TARGET)
