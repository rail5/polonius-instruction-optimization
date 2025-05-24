CXX = g++
CXXFLAGS = -std=gnu++17 -O2 -s -Wall

MAKEFLAGS += -j$(shell nproc)

all: block.o expression.o main.o test-suite/run.sh
	$(CXX) $(CXXFLAGS) -o optimize block.o expression.o main.o

block.o:
	$(CXX) $(CXXFLAGS) -c block.cpp

expression.o:
	$(CXX) $(CXXFLAGS) -c expression.cpp

main.o:
	$(CXX) $(CXXFLAGS) -c main.cpp

test-suite/run.sh:
	@cd test-suite && bpp -o run.sh run.bpp

test: test-suite/run.sh
	@cd test-suite && bash run.sh

clean:
	rm -f *.o optimize test-suite/run.sh
