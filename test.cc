#include "prof.h"

/** silly function to test timing recursive methods */
void sleepFor(int secs) {
	PROF_ENTER(sleepFor, secs);
	if (secs > 0) {
		sleep(1);
		sleepFor(secs - 1);
	}
}

void testFun(int secs) {
	PROF_ENTER(testFun, secs);
	sleepFor(secs);
}

int main(int argc, char** argv) {
	PROF_ENTER(main, argc, argv);
	// profile entries span their scope. if there is not already an appropriate 
	// scope (say, an if (...) {} block or a function body, you can introduce 
	// one with braces
	{ 
		PROF_ENTER(main1);
		testFun(1);
	}
	{
		PROF_ENTER(main2);
		testFun(4);
	}
	Prof::Engine::report();
	Prof::Engine::summaryReport();
	return 0;
}
