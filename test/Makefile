# Makefile for RocketShip (Node data for FSM analysis) pass

DEBUG_SYMBOLS = 1

# Path to top level of LLVM hierarchy
# Edit this to point to your LLVM source directory
LEVEL = ../../llvm-2.7/

# Name of the library to build
TOOLNAME = test_RocketShip

USEDLIBS = iberty.a 

CXXFLAGS += -L/usr/local/lib -lgtest
CXXFLAGS += -I/usr/include/boost/
CXXFLAGS += -DHAVE_DECL_BASENAME=1

# Include the makefile implementation stuff
include $(LEVEL)/Makefile.common
