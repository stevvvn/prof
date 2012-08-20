CC := g++
CCFLAGS := -Wall -Wextra -Weffc++ -std=c++0x
LIBS := 
LDFLAGS := -dynamiclib -Wl,-headerpad_max_install_names,-undefined,dynamic_lookup,-compatibility_version,1.0,-current_version,1.0,-install_name,/usr/local/lib/libprof.dylib -o libprof.dylib 
ifeq ($(shell uname), Linux)
	LIBS := -lrt
	LDFLAGS := -shared -Wl,-soname,libprof.so -o libprof.so
endif
prof.o: prof.cc prof.h
	$(CC) $(CCFLAGS) $(LIBS) -DPROFILE -c -fPIC -fno-common -o prof.o prof.cc 
prof.so:
	$(CC) $(LDFLAGS) prof.o
install: prof.so
	-test -e libprof.so && cp libprof.so /usr/local/lib
	-test -e libprof.dylib && cp libprof.dylib /usr/local/lib
test: test.cc  prof.o
	$(CC) $(CCFLAGS) $(LIBS) -DPROFILE -o test test.cc prof.o
	./test
clean:
	-rm -f *.o *.so test
