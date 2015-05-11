/* vim: noet ts=4 sw=4
*/
#include "simple_sparsehash.h"

struct sparse_dict *sparse_dict_init() {
	return NULL;
}

const int sparse_dict_set(struct sparse_dict *dict,
						  const char *key, const size_t klen,
						  const void *value, const size_t vlen) {
	return 0;
}

const void *sparse_dict_get(struct sparse_dict *dict,
							const char *key, const size_t klen) {
	return NULL;
}

const int sparse_dict_free(struct sparse_dict *dict) {
	return 0;
}
