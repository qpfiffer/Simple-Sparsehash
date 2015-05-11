/* vim: noet ts=4 sw=4
*/
#include <stdio.h>
#include "simple_sparsehash.h"

#define begin_tests() int test_return_val = 0;\
					  int tests_failed = 0;\
					  int tests_run = 0;
#define run_test(test) printf("----- %s -----\n", #test);\
	test_return_val = test();\
	if (!test_return_val) {\
		tests_failed++;\
		printf("%c[%dmFailed.%c[%dm\n", 0x1B, 31, 0x1B, 0);\
	} else {\
		tests_run++;\
		printf("%c[%dmPassed.%c[%dm\n", 0x1B, 32, 0x1B, 0);\
	}
#define finish_tests() printf("\n-----\nTests passed: (%i/%i)\n", tests_run,\
							  tests_run + tests_failed);

int test_insert() {
	return 0;
}

int main(int argc, char *argv[]) {
	begin_tests();
	run_test(test_insert);
	finish_tests();

	return 0;
}
