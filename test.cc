#include "prof.cc"

void sleepFor(int secs) {
	PROF_ENTER(sleepFor, secs);
	if (secs > 0) {
		sleep(1);
		sleepFor(secs - 1);
	}
	PROF_EXIT();
}

void testFun(int secs) {
	PROF_ENTER(testFun, secs);
	sleepFor(secs);
	PROF_EXIT();
}

int main(int argc, char** argv) {
	PROF_ENTER(main, argc, argv);
	testFun(1);
	testFun(4);
	PROF_EXIT();
	Prof::Engine::report();
	Prof::Engine::summaryReport();
	return 0;
}
