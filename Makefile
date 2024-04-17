# Makefile for tempsensor.cpp
# Adjust compiler and flags as needed

# Compiler
CC = g++
# Compiler flags
CFLAGS = -Wall -Wextra -pedantic -std=c++20
# Include directories
INCLUDES = -I/usr/include -I./inc/
# Libraries
# LIBS = -lwiringPi

# Source files
SRCS = ./src/main.cpp
SRCS += ./src/D6T.cpp

# Object files directory
OBJDIR = ./obj
# Object files
OBJS = $(patsubst ./src/%.cpp,$(OBJDIR)/%.o,$(SRCS))
# Executable directory
BINDIR = ./bin
# Executable
MAIN = $(BINDIR)/tempsensor

.PHONY: depend clean

all: $(BINDIR) $(OBJDIR) $(MAIN)
	@echo Compilation complete.

$(BINDIR):
	mkdir -p $(BINDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: ./src/%.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LIBS)

clean:
	$(RM) -r $(OBJDIR) $(BINDIR) *~ 

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# End of Makefile
