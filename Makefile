test: test.cc prof.cc prof.h 
	g++ -Wall -Wextra -Weffc++ -std=c++0x -DPROFILE -o test test.cc
	./test
