/* vim: noet ts=4 sw=4
*/
#include <stdio.h>
#include <string.h>
#include "simple_sparsehash.h"

#define begin_tests() int test_return_val = 0;\
					  int tests_failed = 0;\
					  int tests_run = 0;
#define run_test(test) printf("%s: ", #test);\
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
#define assert(x) if (!x) {\
		printf("failed on line %i ", __LINE__);\
		return 0;\
	}


int test_set() {
	struct sparse_dict *dict = NULL;
	dict = sparse_dict_init();
	assert(dict);

	assert(sparse_dict_set(dict, "key", strlen("key"), "value", strlen("value")));

	assert(sparse_dict_free(dict));
	return 1;
}

int test_get() {
	struct sparse_dict *dict = NULL;
	const char *value = NULL;

	dict = sparse_dict_init();
	assert(dict);

	assert(sparse_dict_set(dict, "key", strlen("key"), "value", strlen("value")));

	value = sparse_dict_get(dict, "key", strlen("key"));
	assert(strncmp(value, "value", strlen("value")) == 0);

	assert(sparse_dict_free(dict));
	return 1;
}

int main(int argc, char *argv[]) {
	begin_tests();
	run_test(test_set);
	run_test(test_get);
	finish_tests();

	return 0;
}
