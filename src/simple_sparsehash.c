/* vim: noet ts=4 sw=4
*/
#include <stdlib.h>
#include "simple_sparsehash.h"

struct sparse_dict *sparse_dict_init() {
	struct sparse_dict *new = NULL;
	new = calloc(1, sizeof(struct sparse_dict));
	return new;
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
	free(dict);
	return 0;
}
