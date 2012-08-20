prof
====

Tiny and crappy profiler for C++0x code running on OS X or Linux

Usage
-----
 * Include prof.h
 * Call PROF_ENTER(funcName, arg1, arg2, ...) in the scope you want to test. funcName is arbitrary, you can use any identifier that makes sense
 * When your program is exiting, or at least when it's done with the part you're concerned about profiling, call PROF_REPORT() or (more likely) PROF_SUMMARY_REPORT()
 * Compile with -lprof, and use -DPROFILE to toggle profiling on

Example
-------
<pre><code>
\#include "prof.h"

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
	// scope (say, an if (...) {} block or a function body), you can introduce 
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
</code></pre>

<pre>
<code>
$ make test
g++ -Wall -Wextra -Weffc++ -std=c++0x  -DPROFILE -o test test.cc
./test
test.cc:17 - main(1, 0x7fff5fbff930) - 5.001346541
        test.cc:19 - main1() - 1.000321061
                test.cc:12 - testFun(1) - 1.000311560
                        test.cc:4 - sleepFor(1) - 1.000303063
                                test.cc:4 - sleepFor(0) - 0.000021584
        test.cc:23 - main2() - 4.000996166
                test.cc:12 - testFun(4) - 4.000972488
                        test.cc:4 - sleepFor(4) - 4.000954693
                                test.cc:4 - sleepFor(3) - 3.000625934
                                        test.cc:4 - sleepFor(2) - 2.000475269
                                                test.cc:4 - sleepFor(1) - 1.000181852
                                                        test.cc:4 - sleepFor(0) - 0.000011693
test.cc:main - 5.001346541
test.cc:testFun - 5.001284048
test.cc:sleepFor - 5.001257756
test.cc:main2 - 4.000996166
test.cc:main1 - 1.000321061
</code>
</pre>
