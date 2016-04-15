/* vim: noet ts=4 sw=4
*/
#include <stdio.h>
#include <string.h>
#include "simple_sparsehash.h"

#define begin_tests() int test_return_val = 0;\
					  int tests_failed = 0;\
					  int tests_run = 0;
#define run_test(test) test_return_val = test();\
	if (!test_return_val) {\
		tests_failed++;\
		printf("%c[%dmFailed%c[%dm: %s\n", 0x1B, 31, 0x1B, 0, #test);\
	} else {\
		tests_run++;\
		printf("%c[%dmPassed%c[%dm: %s\n", 0x1B, 32, 0x1B, 0, #test);\
	}
#define finish_tests() printf("\n-----\nTests passed: (%i/%i)\n", tests_run,\
							  tests_run + tests_failed);
#define assert(x) if (!(x)) {\
		printf("%i: ", __LINE__);\
		return 0;\
	}


int test_empty_array_does_not_blow_up() {
	struct sparse_array *arr = NULL;
	arr = sparse_array_init(sizeof(uint64_t), 32);
	assert(arr);

	assert(!sparse_array_get(arr, 0, NULL));

	assert(sparse_array_free(arr));
	return 1;
}

int test_cannot_set_outside_bounds() {
	struct sparse_array *arr = NULL;
	const uint64_t test_num = 666;
	arr = sparse_array_init(sizeof(uint64_t), 32);
	assert(arr);

	assert(sparse_array_set(arr, 35, &test_num, sizeof(test_num)) == 0);

	assert(sparse_array_free(arr));
	return 1;
}

int test_cannot_get_outside_bounds() {
	struct sparse_array *arr = NULL;
	arr = sparse_array_init(sizeof(uint64_t), 32);
	assert(arr);

	assert(!sparse_array_get(arr, 35, NULL));

	assert(sparse_array_free(arr));
	return 1;
}

int test_cannot_set_bigger_elements() {
	struct sparse_array *arr = NULL;
	const uint64_t test_num = 666;
	arr = sparse_array_init(sizeof(char), 100);
	assert(arr);

	assert(sparse_array_set(arr, 0, &test_num, sizeof(test_num)) == 0);

	assert(sparse_array_free(arr));
	return 1;

}

int test_array_set_backwards() {
	int i;
	const int array_size = 120;
	struct sparse_array *arr = NULL;
	arr = sparse_array_init(sizeof(int), array_size);
	assert(arr);

	for (i = array_size - 1; i >= 0; i--) {
		int *returned = NULL;
		size_t siz = 0;
		assert(sparse_array_set(arr, i, &i, sizeof(i)));
		returned = (int *)sparse_array_get(arr, i, &siz);
		assert(returned);
		assert(*returned == i);
		assert(siz == sizeof(int));
	}

	for (i = array_size - 1; i >= 0; i--) {
		int *returned = NULL;
		size_t siz = 0;
		returned = (int *)sparse_array_get(arr, i, &siz);
		assert(*returned == i);
		assert(siz == sizeof(int));
	}

	assert(sparse_array_free(arr));
	return 1;
}

int test_array_set() {
	int i;
	const int array_size = 130;
	struct sparse_array *arr = NULL;
	arr = sparse_array_init(sizeof(int), array_size);
	assert(arr);

	for (i = 0; i < array_size; i++) {
		int *returned = NULL;
		size_t siz = 0;
		assert(sparse_array_set(arr, i, &i, sizeof(i)));
		returned = (int *)sparse_array_get(arr, i, &siz);
		assert(*returned == i);
		assert(siz == sizeof(int));
	}

	for (i = 0; i < array_size; i++) {
		/* Loop through again just to make sure. */
		int *returned = NULL;
		size_t siz = 0;
		returned = (int *)sparse_array_get(arr, i, &siz);
		assert(*returned == i);
		assert(siz == sizeof(int));
	}

	assert(sparse_array_free(arr));
	return 1;
}

int test_array_set_high_num() {
	const int test_num = 65555555;
	const int index = GROUP_SIZE - 1;
	int *returned = NULL;
	size_t siz = 0;
	struct sparse_array *arr = NULL;

	arr = sparse_array_init(sizeof(int), 140);
	assert(arr);

	assert(sparse_array_set(arr, index, &test_num, sizeof(test_num)));
	returned = (int *)sparse_array_get(arr, index, &siz);
	assert(returned);
	assert(*returned == test_num);
	assert(siz == sizeof(int));

	assert(sparse_array_free(arr));
	return 1;
}

int test_array_set_overwrites_old_values() {
	struct sparse_array *arr = NULL;
	const int test_num = 666;
	const int test_num2 = 1024;
	arr = sparse_array_init(sizeof(int), 150);
	assert(arr);

	assert(sparse_array_set(arr, 0, &test_num, sizeof(test_num)));
	assert(sparse_array_set(arr, 0, &test_num2, sizeof(test_num2)));

	assert(*(const int *)sparse_array_get(arr, 0, NULL) == 1024);

	assert(sparse_array_free(arr));
	return 1;
}

int test_array_get() {
	struct sparse_array *arr = NULL;
	const int test_num = 666;
	size_t item_size = 0;
	arr = sparse_array_init(sizeof(int), 200);
	assert(arr);

	assert(sparse_array_set(arr, 0, &test_num, sizeof(test_num)));
	assert(*(const int *)sparse_array_get(arr, 0, &item_size) == 666);
	assert(item_size == sizeof(int));

	assert(sparse_array_free(arr));
	return 1;
}

int test_dict_set() {
	struct sparse_dict *dict = NULL;
	dict = sparse_dict_init();
	assert(dict);

	assert(sparse_dict_set(dict, "key", strlen("key"), "value", strlen("value")));

	assert(sparse_dict_free(dict));
	return 1;
}

int test_dict_get() {
	struct sparse_dict *dict = NULL;
	size_t outsize = 0;
	const char *value = NULL;

	dict = sparse_dict_init();
	assert(dict);

	assert(sparse_dict_set(dict, "key", strlen("key"), "value", strlen("value")));


	value = sparse_dict_get(dict, "key", strlen("key"), &outsize);
	assert(value);
	assert(outsize == strlen("value"));
	assert(strncmp(value, "value", outsize) == 0);

	assert(sparse_dict_free(dict));
	return 1;
}

int test_dict_lots_of_set() {
	struct sparse_dict *dict = NULL;
	int i = 0;

	dict = sparse_dict_init();
	assert(dict);

	const int iterations = 1000000;
	for (i = 0; i < iterations; i++) {
		char key[64] = {0};
		snprintf(key, sizeof(key), "crazy hash%i", i);

		char val[64] = {0};
		snprintf(val, sizeof(val), "value%i", i);

		assert(sparse_dict_set(dict, key, strlen(key), val, strlen(val)));
		assert(dict->bucket_count == (unsigned int)(i + 1));

		size_t outsize = 0;
		const char *retrieved_value = sparse_dict_get(dict, key, strlen(key), &outsize);
		assert(retrieved_value);
		assert(outsize == strlen(val));
		assert(strncmp(retrieved_value, val, outsize) == 0);
	}

	for (i = iterations - 1; i >= 0; i--) {
		/* Do they same thing but just retrieve values. */
		char key[64] = {0};
		snprintf(key, sizeof(key), "crazy hash%i", i);

		char val[64] = {0};
		snprintf(val, sizeof(val), "value%i", i);

		size_t outsize = 0;
		const char *retrieved_value = sparse_dict_get(dict, key, strlen(key), &outsize);
		assert(retrieved_value);
		assert(outsize == strlen(val));
		assert(strncmp(retrieved_value, val, outsize) == 0);
	}

	assert(sparse_dict_free(dict));
	return 1;
}

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	begin_tests();
	run_test(test_cannot_set_bigger_elements);
	run_test(test_cannot_set_outside_bounds);
	run_test(test_cannot_get_outside_bounds);
	run_test(test_empty_array_does_not_blow_up);
	run_test(test_array_set);
	run_test(test_array_set_backwards);
	run_test(test_array_set_overwrites_old_values);
	run_test(test_array_set_high_num);
	run_test(test_array_get);
	run_test(test_dict_set);
	run_test(test_dict_get);
	run_test(test_dict_lots_of_set);
	finish_tests();

	return 0;
}
