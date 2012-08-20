LIBS := 
ifeq ($(shell uname), Linux)
	LIBS := -lrt
endif
prof.o: prof.cc prof.h
	g++ -Wall -Wextra -Weffc++ -std=c++0x $(LIBS) -DPROFILE -c -fPIC -o prof.o prof.cc 
prof.so:
	gcc -shared -Wl,-soname,libprof.so -o libprof.so prof.o
install: prof.so
	cp libprof.so /usr/lib
test: test.cc libtest.o prof.o
	g++ -Wall -Wextra -Weffc++ -std=c++0x $(LIBS) -DPROFILE -o test test.cc libtest.o -lprof
	./test
clean:
	-rm -f *.o *.so test
