/* vim: noet ts=4 sw=4
*/
#pragma once
#include <inttypes.h>
#include <stdio.h>

/* The maximum size of each sparse_array_group. */
#define GROUP_SIZE 48

/* The default size of the hash table. Used to init bucket_max. */
#define STARTING_SIZE 32

/* The default 'should we resize' percentage, out of 100 percent. */
#define RESIZE_PERCENT 80

/* The math here is, I believe, so that we
 * store exactly enough bits for our group size. The math returns the
 * minimum number of bytes to hold all the bits we need.
 */
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

struct sparse_array_group {
	size_t			count;							/* The number of items currently in this vector. */
	size_t			elem_size;						/* The maximum size of each element. */
	void *			group;							/* The place where we actually store things. */
	unsigned char	bitmap[BITMAP_SIZE];			/* This is how we store the state of what is occupied in group. */
	/* bitmap requires some explanation. We use the bitmap to store which
	 * `offsets` in the array are occupied. We do this through a series
	 * of bit-testing functions.
	 */
};

struct sparse_array {
	const size_t					maximum;		/* The maximum number of items that can be in this array. */
	struct sparse_array_group		*groups;		/* The number of groups we have. This is (num_buckets/GROUP_SIZE). */
};

struct sparse_dict {
	size_t bucket_max;					/* The current maximum number of buckets in this dictionary. */
	size_t bucket_count;				/* The number of occupied buckets in this dictionary. */
	struct sparse_array **buckets;		/* Array of `sparse_array` objects. Defaults to STARTING_SIZE elements in length. */
};

/* ------------ */
/* Sparse Array */
/* ------------ */

struct sparse_array *sparse_array_init(const size_t element_size, const size_t maximum);
const int sparse_array_set(struct sparse_array *arr, const size_t i,
						   const void *val, const size_t vlen);
const void *sparse_array_get(struct sparse_array *arr, const size_t i, size_t *outsize);
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
