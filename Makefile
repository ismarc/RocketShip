# Makefile for RocketShip (Node data for FSM analysis) pass

# Path to top level of LLVM hierarchy
# Edit this to point to your LLVM source directory
LEVEL = ../llvm-2.7/

# Name of the library to build
LIBRARYNAME = libRocketShip
BUILD_ARCHIVE = 1

# Make the shared library become a loadable module so the tools can 
# dlopen/dlsym on the resulting library.
LOADABLE_MODULE = 1

USEDLIBS = iberty.a

DIRS = test

CXXFLAGS += -I/usr/include/boost/
CXXFLAGS += -DHAVE_DECL_BASENAME=1

# Include the makefile implementation stuff
include $(LEVEL)/Makefile.common
