LIBS := 
ifeq ($(shell uname), 'Linux')
	LIBS := -lrt
endif
test: test.cc prof.cc prof.h 
	g++ -Wall -Wextra -Weffc++ -std=c++0x $(LIBS) -DPROFILE -o test test.cc
	./test
