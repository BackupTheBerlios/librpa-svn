# librpa/Makefile
#
# Copyright (C) 2007 Roman Kulikov

CXX = g++
CXXFLAGS = -Wall -O2
CP = cp -f
AR = ar rcs
RM = rm -f
INSTALL_ROOT = /usr

###########

all .DEFAULT: static shared

rpa.o:
	$(CXX) $(CXXFLAGS) -o rpa.o -c rpa.cpp

static: rpa.o
	$(AR) librpa.a rpa.o

shared: rpa.o
	$(CXX) $(CXXFLAGS) -o librpa.so -shared rpa.o

test: static
	$(CXX) $(CXXFLAGS) -L. -o rpatest main.cpp -lrpa

install: all
	$(CP) rpa.h $(INSTALL_DIR)/include
	$(CP) librpa.so $(INSTALL_DIR)/lib

# Here I use system enviroment variable DEVEL_DIR for test installation
devinstall: all
	$(CP) rpa.h $(DEVEL_DIR)/include
	$(CP) librpa.so $(DEVEL_DIR)/lib

clean:
	$(RM) rpa.o rpatest librpa.so librpa.a
