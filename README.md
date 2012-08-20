prof
====

Tiny and crappy profiler for C++0x code

Usage
-----
 * Include prof.cc (nb: not .h)
 * Call PROF_ENTER(funcName, arg1, arg2, ...)
 * Call PROF_EXIT()
 * When your program is exiting, or at least when it's done with the part you're concerned about profiling, call Prof::Engine::report() or (more likely) Prof::Engine::summaryReport()
 * Compile with -DPROFILE to enable

Example
-------
<pre><code>
\#include "prof.cc"

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
</code></pre>

<pre>
<code>
g++ -Wall -Wextra -Weffc++ -std=c++0x -DPROFILE -o test test.cc
./test
test.cc:19 - main(1, 0x7fff5fbff930) - 5.001148108
        test.cc:13 - testFun(1) - 1.000185503
                test.cc:4 - sleepFor(1) - 1.000176334
                        test.cc:4 - sleepFor(0) - 0.000009963
        test.cc:13 - testFun(4) - 4.000934463
                test.cc:4 - sleepFor(4) - 4.000917057
                        test.cc:4 - sleepFor(3) - 3.000720966
                                test.cc:4 - sleepFor(2) - 2.000489546
                                        test.cc:4 - sleepFor(1) - 1.000320839
                                                test.cc:4 - sleepFor(0) - 0.000009414
test.cc:main - 5.001148108
test.cc:testFun - 5.001119966
test.cc:sleepFor - 5.001093391
</code>
</pre>
