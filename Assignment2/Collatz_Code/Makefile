CXX                = g++ -std=c++17
OPTFLAGS	   = -O3 -march=native -ffast-math -ftree-vectorize 
CXXFLAGS          += -Wall 
INCLUDES	   = -I. -I./include
LIBS               = -pthread #-fopenmp
SOURCES            = $(wildcard *.cpp)
TARGET             = $(SOURCES:.cpp=)

.PHONY: all clean cleanall 

%: %.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS)


all: $(TARGET)

clean: 
	-rm -fr *.o *~
cleanall: clean
	-rm -fr $(TARGET)
