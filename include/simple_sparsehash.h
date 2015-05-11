/* vim: noet ts=4 sw=4
*/
#pragma once
#include <inttypes.h>
#include <stdio.h>

static const uint16_t GROUP_SIZE = 48;

/* These are the objects that get stored in the sparse hash. */
struct sparse_bucket {
	const char		*key;
	const size_t	klen;
	const void		*val;
	const size_t	vlen;
};

/* A sparse array is how we store buckets in the sparse_dict. A sparse array
 * is an array that only uses the amount of memory that the objects stored in
 * it requires. That is to say, we aren't wasting space holding "empty" slots
 * in the array.
 */
struct sparse_array {
	uint32_t		count;     /* The number of items currently in this vector. */
	uint32_t		max_count; /* The current maximum size of this vector. */
};

struct sparse_dict {
	unsigned int			num_buckets; /* The number of buckets currently in our table. */
	struct sparse_array		**groups;    /* The number of groups we have. This is (num_buckets/GROUP_SIZE). */
};

/* Creates a new sparse dictionary. */
struct sparse_dict *sparse_dict_init();

/* Copies `value` into `dict`. */
const int sparse_dict_set(struct sparse_dict *dict,
						  const char *key, const size_t klen,
						  const void *value, const size_t vlen);

/* Returns the value of `key` from `dict`. */
const void *sparse_dict_get(struct sparse_dict *dict,
							const char *key, const size_t klen);

/* Frees and cleans up a sparse_dict created with sparse_dict_init(). */
const int sparse_dict_free(struct sparse_dict *dict);
