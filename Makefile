# Makefile for RocketShip (Node data for FSM analysis) pass

DEBUG_SYMBOLS = 1

# Path to top level of LLVM hierarchy
# Edit this to point to your LLVM source directory
LEVEL = ../llvm-2.7/

# Name of the library to build
LIBRARYNAME = RocketShip

# Make the shared library become a loadable module so the tools can 
# dlopen/dlsym on the resulting library.
LOADABLE_MODULE = 1

USEDLIBS = iberty.a

# Include the makefile implementation stuff
include $(LEVEL)/Makefile.common

CXXFLAGS = -DHAVE_DECL_BASENAME=1