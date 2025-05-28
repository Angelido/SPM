# Compiler Makefile for C++ projects with MPI support
CXX        = g++ -std=c++20
MPICXX     = mpicxx -std=c++20

INCLUDES   = -I . -I./include 
CXXFLAGS  += -Wall -Wno-uninitialized
OPTFLAGS   = -O3 -ffast-math -DNDEBUG
LIBS       = -pthread

SOURCES    = $(wildcard *.cpp)
TARGET     = $(filter-out mergesort_parallel_mpi, $(SOURCES:.cpp=))

# Generic rule for compiling C++ source files
%: %.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS)

# Specific rules for MPI programs
mergesort_mpi: mergesort_mpi.cpp
	$(MPICXX) $(INCLUDES) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS)

all: $(TARGET) mergesort_parallel_mpi

clean: 
	-rm -f *.o *~

cleanall: clean
	-rm -f $(TARGET) mergesort_parallel_mpi
