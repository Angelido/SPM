CXX		= g++ -std=c++20
INCLUDES	= -I . -I miniz -I./include
CXXFLAGS  	+= -Wall 

LDFLAGS 	= -fopenmp #-pthread
OPTFLAGS	= -O3 -ffast-math -DNDEBUG

TARGETS		= minizseq minizpar 

.PHONY: all clean cleanall
.SUFFIXES: .cpp 


%: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< ./miniz/miniz.c $(LDFLAGS)

all		: $(TARGETS)

minizseq	: minizseq.cpp cmdline.hpp utility.hpp
minizpar	: minizpar.cpp cmdline.hpp utility.hpp


clean		: 
	rm -f $(TARGETS) 
cleanall	: clean
	\rm -f *.o *~



