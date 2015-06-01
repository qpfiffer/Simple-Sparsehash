/* vim: noet ts=4 sw=4
*/
#include <stdlib.h>
#include <string.h>
#include "simple_sparsehash.h"

#define FULL_ELEM_SIZE (arr->elem_size + sizeof(size_t))
#define MAX_ARR_SIZE ((arr->maximum - 1)/GROUP_SIZE + 1)
#define QUADRATIC_PROBE (key_hash & (dict->bucket_max - 1)) + (num_probes * num_probes)

/* One of the simplest hashing functions, FNV-1a. See the wikipedia article for more info:
 * http://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 */
static const uint64_t hash_fnv1a(const char *key, const size_t klen) {
	static const uint64_t fnv_prime = 1099511628211ULL;
	static const uint64_t fnv_offset_bias = 14695981039346656037ULL;

	const int iterations = klen;

	uint8_t i;
	uint64_t hash = fnv_offset_bias;

	for(i = 0; i < iterations; i++) {
		hash = hash ^ key[i];
		hash = hash * fnv_prime;
	}

	return hash;
}

/* TODO: Figure out better names for charbit/modbit */
static const size_t charbit(const size_t position) {
	/* Get enough bits to store 0 - 7. */
	return position >> 3;
}

static const size_t modbit(const size_t position) {
	/* Get the number of bits of this number that are 0 - 7,
	 * or something like that.
	 */
	return 1 << (position & 7);
}

/* This function is used to map an item's 'position' (the user-facing index
 * into the array) with the 'offset' which is the actual position in the
 * array, memory-wise.
 *
 * The way we do this is by counting the number of 1s in the bitmap from
 * 0 .. i-1 in the bitmap. The original implementation uses a big table for the
 * popcount, I've opted to just use a GCC builtin.
 */
static const size_t position_to_offset(const unsigned char *bitmap,
									   const size_t position) {
	size_t retval = 0;
	size_t pos = position;
	size_t bitmap_iter = 0;

	/* Here we loop through the bitmap a char at a time (a char is 8 bits)
	 * and count the number of 1s in that chunk.
	 */
	for (; pos >= 8; pos -= 8)
		retval += __builtin_popcount(bitmap[bitmap_iter++]);

	/* This last bit does the same thing as above, but takes care of the
	 * remainder that didn't fit cleanly into the 8 x 8 x 8 ... loop above. That
	 * is to say, it grabs the last 0 - 7 bits and adds the number of 1s in it to
	 * retval.
	 */
	return retval + __builtin_popcount(bitmap[bitmap_iter] & ((1 << pos) - 1));
}

static const int is_position_occupied(const unsigned char *bitmap,
							 const size_t position) {
	return bitmap[charbit(position)] & modbit(position);
}

static void set_position(unsigned char *bitmap, const size_t position) {
	bitmap[charbit(position)] |= modbit(position);
}

/* Sparse Array */
static const int _sparse_array_group_set(struct sparse_array_group *arr, const size_t i,
						   const void *val, const size_t vlen) {
	size_t offset = 0;
	void *destination = NULL;
	if (vlen > arr->elem_size)
		return 0;
	/* So what needs to happen in this function:
	 * 1. Convert the position (i) to the 'offset'
	 * 2. Check to see if this slot is already occupied (bmtest).
	 *    overwrite the old element if this is the case.
	 * 3. Otherwise, expand the array by a single element and increase
	 *    our bucket count (arr->count). Finally, OR the bit in our state
	 *    bitmap that shows this position is occupied.
	 * 4. After doing all that, create a copy of val and stick it in the right
	 *    position in our array.
	 */

	offset = position_to_offset(arr->bitmap, i);
	if (!is_position_occupied(arr->bitmap, i)) {
		const size_t to_move_siz = (arr->count - offset) * FULL_ELEM_SIZE;
		/* We could do a realloc here and some memmove stuff, but this
		 * is easier to think about. Allocate a new chunk of memory:
		 */
		void *new_group = malloc((arr->count + 1) * FULL_ELEM_SIZE);
		if (new_group == NULL)
			return 0;

		if (offset * FULL_ELEM_SIZE > 0) {
			/* Now we move all of the buckets from 0 .. offset to the new
			 * chunk of memory:
			 */
			const size_t to_copy_siz = offset * FULL_ELEM_SIZE;
			memcpy(new_group, arr->group, to_copy_siz);
		}

		/* Now take all of the old items and move them up a slot: */
		if (to_move_siz > 0) {
			memcpy((unsigned char *)(new_group) + ((offset + 1) * FULL_ELEM_SIZE),
					(unsigned char *)(arr->group) + (offset * FULL_ELEM_SIZE),
					to_move_siz);
		}

		/* Increase the bucket count because we've expanded: */
		arr->count++;
		free(arr->group);
		arr->group = new_group;
		/* Remember to modify the bitmap: */
		set_position(arr->bitmap, i);
	}

	/* Copy the size into the position, fighting -pedantic the whole
	 * time.
	 */
	destination = (unsigned char *)(arr->group) + (offset * FULL_ELEM_SIZE);
	memcpy(destination, &vlen, sizeof(vlen));

	/* Here we mutate a variable because we're writing C and we don't respect
	 * anything
	 */
	destination = (unsigned char *)destination + sizeof(vlen);
	memcpy(destination, val, vlen);

	return 1;
}

static const void *_sparse_array_group_get(struct sparse_array_group *arr,
							 const size_t i, size_t *outsize) {
	const size_t offset = position_to_offset(arr->bitmap, i);
	const unsigned char *item_siz = (unsigned char *)(arr->group) + (offset * FULL_ELEM_SIZE);
	const void *item = item_siz + sizeof(size_t);

	if (!is_position_occupied(arr->bitmap, i))
		return NULL;

	if (item_siz == NULL)
		return NULL;

	if (outsize)
		memcpy(outsize, item_siz, sizeof(size_t));

	return item;
}

static const int _sparse_array_group_free(struct sparse_array_group *arr) {
	free(arr->group);
	return 1;
}

struct sparse_array *sparse_array_init(const size_t element_size, const size_t maximum) {
	int i = 0;
	struct sparse_array *arr = NULL;
	arr = calloc(1, sizeof(struct sparse_array));
	if (arr == NULL)
		return NULL;

	struct sparse_array stack_array = {
		.maximum = maximum,
	};

	memcpy(arr, &stack_array, sizeof(struct sparse_array));
	arr->groups = calloc(MAX_ARR_SIZE, sizeof(struct sparse_array_group));

	for (i = 0; i < MAX_ARR_SIZE; i++) {
		struct sparse_array_group *sag = &arr->groups[i];
		sag->elem_size = element_size;
	}

	return arr;
}

const int sparse_array_set(struct sparse_array *arr, const size_t i,
						   const void *val, const size_t vlen) {
	if (i > arr->maximum)
		return 0;
	return _sparse_array_group_set(&arr->groups[i / GROUP_SIZE], i % GROUP_SIZE, val, vlen);
}

const void *sparse_array_get(struct sparse_array *arr, const size_t i, size_t *outsize) {
	if (i > arr->maximum)
		return NULL;
	return _sparse_array_group_get(&arr->groups[i / GROUP_SIZE], i % GROUP_SIZE, outsize);
}

const int sparse_array_free(struct sparse_array *arr) {
	int i = 0;
	for (; i < MAX_ARR_SIZE; i++) {
		struct sparse_array_group *sag = &arr->groups[i];
		_sparse_array_group_free(sag);
	}
	free(arr);
	return 1;
}

/* Sparse Dictionary */
struct sparse_dict *sparse_dict_init() {
	struct sparse_dict *new = NULL;
	new = calloc(1, sizeof(struct sparse_dict));
	if (new == NULL)
		return NULL;

	new->bucket_max = STARTING_SIZE;
	new->bucket_count = 0;
	new->buckets = sparse_array_init(sizeof(struct sparse_bucket), STARTING_SIZE);
	if (new->buckets == NULL)
		goto error;

	return new;

error:
	free(new);
	return NULL;
}

static inline const int _create_and_insert_new_bucket(
						struct sparse_array *array, const unsigned int i,
						const char *key, const size_t klen,
						const void *value, const size_t vlen) {
	char *copied_key = strndup(key, klen);
	if (copied_key == NULL)
		goto error;

	void *copied_value = malloc(vlen);
	if (copied_value == NULL)
		goto error;
	memcpy(copied_value, value, vlen);

	struct sparse_bucket bct = {
		.key = copied_key,
		.klen = klen,
		.val = copied_value,
		.vlen = vlen
	};

	if (!sparse_array_set(array, i, &bct, sizeof(bct)))
		goto error;

	return 1;

error:
	return 0;
}

static const int _rehash_and_grow_table(struct sparse_dict *dict) {
	/* We've reached our chosen 'rehash the table' point, so
	 * we need to resize the table now.
	 */
	int i = 0, buckets_rehashed = 0;
	const size_t new_bucket_max = dict->bucket_max * 2;
	struct sparse_array *new_buckets = NULL;

	new_buckets = sparse_array_init(sizeof(struct sparse_bucket), new_bucket_max);
	if (new_buckets == NULL)
		goto error;

	/* Loop through each bucket and stick it into the new array. */
	for (i = 0; i < dict->bucket_max; i++) {
		size_t bucket_siz = 0;
		const struct sparse_bucket *bucket = sparse_array_get(dict->buckets, i, &bucket_siz);

		if (bucket_siz != 0 && bucket != NULL) {
			/* We found a bucket. */
			unsigned int probed_val = 0, num_probes = 0;
			uint64_t key_hash = hash_fnv1a(bucket->key, bucket->klen);
			while (1) {
				/* Quadratically probe along the hash table for an empty slot. */
				probed_val = QUADRATIC_PROBE;
				size_t current_value_siz = 0;
				const void *current_value = sparse_array_get(new_buckets, probed_val, &current_value_siz);

				if (current_value_siz == 0 && current_value == NULL)
					break;

				/* If the following ever happens, there are deeply troubling
				 * things that no longer make sense in the universe.
				 */
				if (num_probes > dict->bucket_count)
					goto error;

				num_probes++;
			}
			_create_and_insert_new_bucket(new_buckets, probed_val, bucket->key, bucket->klen,
										  bucket->val, bucket->vlen);
			buckets_rehashed++;
		}

		/* Short circuit to see if we can quit early: */
		if (buckets_rehashed == dict->bucket_count)
			break;
	}

	sparse_array_free(dict->buckets);
	dict->buckets = new_buckets;
	dict->bucket_max = new_bucket_max;

	return 1;

error:
	return 0;
}

const int sparse_dict_set(struct sparse_dict *dict,
						  const char *key, const size_t klen,
						  const void *value, const size_t vlen) {
	const uint64_t key_hash = hash_fnv1a(key, klen);
	unsigned int num_probes = 0;

	/* First check the array to see if we have an object already stored in
	 * 'out' position.
	 */
	while (1) {
		size_t current_value_siz = 0;
		/* Use quadratic probing here to insert into the table.
		 * Further reading: https://en.wikipedia.org/wiki/Quadratic_probing
		 */
		const unsigned int probed_val = QUADRATIC_PROBE;
		const void *current_value = sparse_array_get(dict->buckets, probed_val, &current_value_siz);

		if (current_value_siz == 0 && current_value == NULL) {
			/* Awesome, the slot we want is empty. Insert as normal. */
			if (_create_and_insert_new_bucket(dict->buckets, probed_val, key, klen, value, vlen))
				break;
			else
				goto error;
		} else {
			/* We found a bucket. Check to see if it has the same key as we do. */
			struct sparse_bucket *existing_bucket = (struct sparse_bucket *)current_value;
			const size_t lrgr_key = existing_bucket->klen > klen ? existing_bucket->klen : klen;
			if (strncmp(existing_bucket->key, key, lrgr_key) == 0) {
				/* Great, we probed along the hashtable and found a bucket with the same key as
				 * the key we want to insert. Replace it. */
				free(existing_bucket->key);
				free(existing_bucket->val);
				if (_create_and_insert_new_bucket(dict->buckets, probed_val, key, klen, value, vlen)) {
					/* We return here because we don't want to execute the 'resize the table'
					 * logic because we overwrote a bucket instead of adding a new one.
					 */
					return 1;
				} else
					goto error;
			}
		}

		num_probes++;

		if (num_probes > dict->bucket_count) {
			/* If this ever happens something has gone very, very wrong.
			 * The hash table is full.
			 */
			printf("Could not find an open slot in the table.\n");
			goto error;
		}
	}

	dict->bucket_count++;

	if (dict->bucket_count / (float)dict->bucket_max >= RESIZE_PERCENT/100.0f)
		return _rehash_and_grow_table(dict);

	return 1;

error:
	return 0;
}

const void *sparse_dict_get(struct sparse_dict *dict, const char *key,
							const size_t klen, size_t *outsize) {
	const uint64_t key_hash = hash_fnv1a(key, klen);
	unsigned int num_probes = 0;

	while (1) {
		size_t current_value_siz = 0;
		const unsigned int probed_val = QUADRATIC_PROBE;
		const void *current_value = sparse_array_get(dict->buckets, probed_val, &current_value_siz);

		if (current_value_siz != 0 && current_value != NULL) {
			/* We have to do a string comparison here because we use quadratic probing.
			 * The value we pulled from the underlying array could be anything.
			 */
			struct sparse_bucket *existing_bucket = (struct sparse_bucket *)current_value;
			const size_t lrgr_key = existing_bucket->klen > klen ? existing_bucket->klen : klen;
			if (strncmp(existing_bucket->key, key, lrgr_key) == 0) {
				if (outsize)
					memcpy(outsize, &existing_bucket->vlen, sizeof(existing_bucket->vlen));

				return existing_bucket->val;
			}
		} else {
			/* We found nothing where we expected something. */
			return NULL;
		}

		num_probes++;

		if (num_probes > dict->bucket_count)
			return NULL;
	}

	return NULL;
}

const int sparse_dict_free(struct sparse_dict *dict) {
	sparse_array_free(dict->buckets);
	free(dict);
	return 1;
}
