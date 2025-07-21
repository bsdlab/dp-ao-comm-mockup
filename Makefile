########################################################################
####################### Makefile Template ##############################
########################################################################

# Compiler settings - Can be customized.
CC = g++


#### Includes
# Path to your python installation / Python.h file
INCS = C:\Users\bsdlab\anaconda3\include 

# Dependencies from this project
INCS += ./src/include/libAO/include
INCS += ./src/include/liblsl/include

INC_PARAMS =$(addprefix -I, $(INCS))

#### Linking
# for .lib
LINKS = ./src/include/libAO/lib
LINKS += ./src/include/liblsl/lib
LINK_PARAMS =$(addprefix -L, $(LINKS))
# LINK_PARAMS += -/src/include/liblsl/bin/lsl.dll


CXXFLAGS = -std=c++11 -Wall $(INC_PARAMS)
LDFLAGS = $(LINK_PARAMS) -llsl -lethernetStandAlone_X64 -lPacket -lwpcap -lws2_32

APPNAME = AOStreamer
EXT = .cpp
SRCDIR = ./src
OBJDIR = ./build

############## Do not change anything from here downwards! #############
SRC = $(wildcard $(SRCDIR)/*$(EXT))
OBJ = $(SRC:$(SRCDIR)/%$(EXT)=$(OBJDIR)/%.o)
DEP = $(OBJ:$(OBJDIR)/%.o=$(OBJDIR)/%.d)
# UNIX-based OS variables & settings
EXE = .exe

########################################################################
####################### Targets beginning here #########################
########################################################################

all: $(APPNAME)

# Builds the app
$(APPNAME): $(OBJ)
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Creates the dependecy rules
$(OBJDIR)/%.d: $(SRCDIR)/%$(EXT)
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:%.d=$(OBJDIR)/%.o) >$@

# Includes all .h files
-include $(DEP)

# Building rule for .o files and its .c/.cpp in combination with all .h
$(OBJDIR)/%.o: $(SRCDIR)/%$(EXT)
	$(CC) $(CXXFLAGS) -o $@ -c $<

################### Cleaning rule ###################
.PHONY: clean
clean:
	if exist build\* del /q build\*
	if exist $(APPNAME).exe del $(APPNAME).exe
