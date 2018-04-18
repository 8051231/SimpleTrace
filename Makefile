
# Makefile to build trace plugin
# Copyright ARM Limited 2009-2010 All Rights Reserved.
#

CPPFLAGS = -I $(PVLIB_HOME)/include/fmruntime -I $(PVLIB_HOME)/include/fmruntime/eslapi
CXXFLAGS = -W  -O3 -DNDEBUG -fomit-frame-pointer -fPIC 

all: SimpleTrace.so

SimpleTrace.o: SimpleTrace.cpp SimpleTrace.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

SimpleTrace.so: SimpleTrace.o
	$(CXX) -pthread -ldl -lrt -shared -o $@ SimpleTrace.o 

clean:
	rm -f *.o
	rm -f SimpleTrace.so

# Use C++ linker:
CC=CXX

# End of Makefile
