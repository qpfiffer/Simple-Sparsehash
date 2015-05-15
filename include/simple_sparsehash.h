/* vim: noet ts=4 sw=4
*/
#pragma once
#include <inttypes.h>
#include <stdio.h>

#define GROUP_SIZE 48
#define BITMAP_SIZE (GROUP_SIZE-1)/8 + 1

/* These are the objects that get stored in the sparse arrays that
 * make up a sparse dictionary.
 */
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
	size_t		count;							/* The number of items currently in this vector. */
	void *			group;							/* The place where we actually store things. */
	unsigned char	bitmap[BITMAP_SIZE];	/* This is how we store the state of what is occupied in group. */
	/* bitmap requires some explanation. We use the bitmap to store which
	 * `offsets` in the array are occupied. We do this through a series
	 * of bit-testing functions.
	 *
	 * The math here (see '#define BITMAP_SIZE`) is, I believe, so that we
	 * store exactly enough bits for our group size. The math returns the
	 * minimum number of bytes to hold all the bits we need.
	 */
};

struct sparse_dict {
	struct sparse_array		**groups;    /* The number of groups we have. This is (num_buckets/GROUP_SIZE). */
};

/* ------------ */
/* Sparse Array */
/* ------------ */

struct sparse_array *sparse_array_init(const size_t element_size);
const int sparse_array_set(struct sparse_array *arr, const size_t i,
						   const void *val, const size_t vlen);
const void *sparse_array_get(struct sparse_array *arr, const size_t i);
const int sparse_array_free(struct sparse_array *arr);


/* ----------------- */
/* Sparse Dictionary */
/* ----------------- */

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
