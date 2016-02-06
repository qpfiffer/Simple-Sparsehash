/* vim: noet ts=4 sw=4
*/
#include <stdlib.h>
#include <string.h>
#include "simple_sparsehash.h"

#define FULL_ELEM_SIZE (arr->elem_size + sizeof(size_t))
#define MAX_ARR_SIZE ((arr->maximum - 1)/GROUP_SIZE + 1)
#define QUADRATIC_PROBE(maximum) (key_hash + num_probes * num_probes) & (maximum - 1)

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
static const uint32_t charbit(const uint32_t position) {
	/* Get enough bits to store 0 - 31. */
	return position >> 5;
}

static const uint32_t modbit(const uint32_t position) {
	/* Get the number of bits of this number that are 0 - 31,
	 * or something like that.
	 */
	return 1 << (position & 31);
}

/* This is one of the popcount implementations from Wikipedia.
 * http://en.wikipedia.org/wiki/Hamming_weight
 */
static inline uint32_t popcount_32(uint32_t x) {
	const uint32_t m1 = 0x55555555;
	const uint32_t m2 = 0x33333333;
	const uint32_t m4 = 0x0f0f0f0f;
	x -= (x >> 1) & m1;
	x = (x & m2) + ((x >> 2) & m2);
	x = (x + (x >> 4)) & m4;
	x += x >>  8;
	return (x + (x >> 16)) & 0x3f;
}

/* This function is used to map an item's 'position' (the user-facing index
 * into the array) with the 'offset' which is the actual position in the
 * array, memory-wise.
 *
 * The way we do this is by counting the number of 1s in the bitmap from
 * 0 .. i-1 in the bitmap. The original implementation uses a big table for the
 * popcount.
 */
static const uint32_t position_to_offset(const uint32_t *bitmap,
									   const uint32_t position) {
	uint32_t retval = 0;
	uint32_t pos = position;
	uint32_t bitmap_iter = 0;

	/* Here we loop through the bitmap a uint32_t at a time, and count the number
	 * of 1s in that chunk.
	 */
	for (; pos >= BITCHUNK_SIZE; pos -= BITCHUNK_SIZE)
		retval += popcount_32(bitmap[bitmap_iter++]);

	/* This last bit does the same thing as above, but takes care of the
	 * remainder that didn't fit cleanly into the 32 x 32 x 32 ... loop above. That
	 * is to say, it grabs the last 0 - 7 bits and adds the number of 1s in it to
	 * retval.
	 */
	return retval + popcount_32(bitmap[bitmap_iter] & (((uint32_t)1 << pos) - 1u));
}

/* Simple check to see whether a slot in the array is occupied or not. */
static const int is_position_occupied(const uint32_t *bitmap,
							 const uint32_t position) {
	return bitmap[charbit(position)] & modbit(position);
}

static void set_position(uint32_t *bitmap, const uint32_t position) {
	bitmap[charbit(position)] |= modbit(position);
}

/* Sparse Array */
static const int _sparse_array_group_set(struct sparse_array_group *arr, const uint32_t i,
						   const void *val, const size_t vlen) {
	uint32_t offset = 0;
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
		/* Reallocate the array to hold the new item */
		void *new_group = realloc(arr->group, (arr->count + 1) * FULL_ELEM_SIZE);
		if (new_group == NULL)
			return 0;

		/* Now take all of the old items and move them up a slot: */
		if (to_move_siz > 0) {
			memmove((unsigned char *)(new_group) + ((offset + 1) * FULL_ELEM_SIZE),
					(unsigned char *)(new_group) + (offset * FULL_ELEM_SIZE),
					to_move_siz);
		}

		/* Increase the bucket count because we've expanded: */
		arr->count++;
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
	 * anything.
	 */
	destination = (unsigned char *)destination + sizeof(vlen);
	memcpy(destination, val, vlen);

	return 1;
}

static const void *_sparse_array_group_get(struct sparse_array_group *arr,
							 const uint32_t i, size_t *outsize) {
	const uint32_t offset = position_to_offset(arr->bitmap, i);
	const unsigned char *item_siz = (unsigned char *)(arr->group) + (offset * FULL_ELEM_SIZE);
	const void *item = item_siz + sizeof(size_t);

	if (!is_position_occupied(arr->bitmap, i))
		return NULL;

	/* In a perfect world you could store 0 sized items and have that mean
	 * something, but I'll tolerate none of that right now.
	 */
	if (*(size_t *)item_siz == 0)
		return NULL;

	/* If the user wants to know the size (outsize is non-null), write it
	 * out.
	 */
	if (outsize)
		memcpy(outsize, item_siz, sizeof(size_t));

	return item;
}

static const int _sparse_array_group_free(struct sparse_array_group *arr) {
	free(arr->group);
	return 1;
}

struct sparse_array *sparse_array_init(const size_t element_size, const uint32_t maximum) {
	unsigned int i = 0;
	struct sparse_array *arr = NULL;
	/* CHECK YOUR SYSCALL RETURNS. Listen to djb. */
	arr = calloc(1, sizeof(struct sparse_array));
	if (arr == NULL)
		return NULL;

	/* This is a non-obvious hack I use. If we have const variables in a
	 * struct then to initialize them we can either cast them or use an
	 * initializer like this.
	 * Then we copy it into a heap-allocated blob. The compiler lets us
	 * do this.
	 */
	struct sparse_array stack_array = {
		.maximum = maximum,
	};

	memcpy(arr, &stack_array, sizeof(struct sparse_array));
	arr->groups = calloc(MAX_ARR_SIZE, sizeof(struct sparse_array_group));
	if (arr->groups == NULL) {
		free(arr);
		return NULL;
	}

	for (i = 0; i < MAX_ARR_SIZE; i++) {
		struct sparse_array_group *sag = &arr->groups[i];
		sag->elem_size = element_size;
	}

	return arr;
}

const int sparse_array_set(struct sparse_array *arr, const uint32_t i,
						   const void *val, const size_t vlen) {
	/* Don't let users set outside the bounds of the array. */
	if (i > arr->maximum)
		return 0;
	/* Since our hashtable is divided into many arrays, we need to pick the one
	 * relevant to `i` in this case:
	 */
	struct sparse_array_group *operating_group = &arr->groups[i / GROUP_SIZE];
	const int position = i % GROUP_SIZE;
	return _sparse_array_group_set(operating_group, position, val, vlen);
}

const void *sparse_array_get(struct sparse_array *arr, const uint32_t i, size_t *outsize) {
	if (i > arr->maximum)
		return NULL;
	struct sparse_array_group *operating_group = &arr->groups[i / GROUP_SIZE];
	const int position = i % GROUP_SIZE;
	return _sparse_array_group_get(operating_group, position, outsize);
}

const int sparse_array_free(struct sparse_array *arr) {
	unsigned int i = 0;
	for (; i < MAX_ARR_SIZE; i++) {
		struct sparse_array_group *sag = &arr->groups[i];
		_sparse_array_group_free(sag);
	}
	free(arr->groups);
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

static const int _create_and_insert_new_bucket(
						struct sparse_array *array, const unsigned int i,
						const char *key, const size_t klen,
						const void *value, const size_t vlen,
						const uint64_t key_hash) {
	void *copied_value = NULL;
	char *copied_key = NULL;

	copied_value = malloc(vlen + klen);
	if (copied_value == NULL)
		goto error;
	memcpy(copied_value, value, vlen);

	copied_key = copied_value + vlen;
	strncpy(copied_key, key, klen);

	struct sparse_bucket bct = {
		.key = copied_key,
		.klen = klen,
		.val = copied_value,
		.vlen = vlen,
		.hash = key_hash
	};

	if (!sparse_array_set(array, i, &bct, sizeof(bct)))
		goto error;

	return 1;

error:
	free(copied_value);
	return 0;
}

static const int _rehash_and_grow_table(struct sparse_dict *dict) {
	/* We've reached our chosen 'rehash the table' point, so
	 * we need to resize the table now.
	 */
	unsigned int i = 0, buckets_rehashed = 0;
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
			uint64_t key_hash = bucket->hash;
			while (1) {
				/* Quadratically probe along the hash table for an empty slot. */
				probed_val = QUADRATIC_PROBE(new_bucket_max);
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
			if (!sparse_array_set(new_buckets, probed_val,
						bucket, sizeof(struct sparse_bucket)))
				goto error;
			buckets_rehashed++;
		}

		/* Short circuit to see if we can quit early: */
		if (buckets_rehashed == dict->bucket_count)
			break;
	}

	/* Finally, swap out the old array with the new one: */
	sparse_array_free(dict->buckets);
	dict->buckets = new_buckets;
	dict->bucket_max = new_bucket_max;

	return 1;

error:
	if (new_buckets)
		sparse_array_free(new_buckets);
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
		const unsigned int probed_val = QUADRATIC_PROBE(dict->bucket_max);
		const void *current_value = sparse_array_get(dict->buckets, probed_val, &current_value_siz);

		if (current_value_siz == 0 && current_value == NULL) {
			/* Awesome, the slot we want is empty. Insert as normal. */
			if (_create_and_insert_new_bucket(dict->buckets, probed_val, key, klen, value, vlen, key_hash))
				break;
			else
				goto error;
		} else {
			/* We found a bucket. Check to see if it has the same key as we do. */
			struct sparse_bucket *existing_bucket = (struct sparse_bucket *)current_value;
			const size_t lrgr_key = existing_bucket->klen > klen ? existing_bucket->klen : klen;
			if (existing_bucket->hash == key_hash && strncmp(existing_bucket->key, key, lrgr_key) == 0) {
				/* Great, we probed along the hashtable and found a bucket with the same key as
				 * the key we want to insert. Replace it. */
				char *existing_key = existing_bucket->key;
				void *existing_val = existing_bucket->val;
				if (_create_and_insert_new_bucket(dict->buckets, probed_val, key, klen, value, vlen, key_hash)) {
					/* We return here because we don't want to execute the 'resize the table'
					 * logic. We overwrote a bucket instead of adding a new one, so we know
					 * we don't need to resize anything.
					 */
					free(existing_key);
					free(existing_val);
					return 1;
				} else {
					goto error;
				}
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

	/* See if we've hit our 'we should rehash the table' occupancy number: */
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
		const unsigned int probed_val = QUADRATIC_PROBE(dict->bucket_max);
		const void *current_value = sparse_array_get(dict->buckets, probed_val, &current_value_siz);

		if (current_value_siz != 0 && current_value != NULL) {
			/* We have to do a string comparison here because we use quadratic probing.
			 * The value we pulled from the underlying array could be anything.
			 */
			struct sparse_bucket *existing_bucket = (struct sparse_bucket *)current_value;
			const size_t lrgr_key = existing_bucket->klen > klen ? existing_bucket->klen : klen;
			if (existing_bucket->hash == key_hash && strncmp(existing_bucket->key, key, lrgr_key) == 0) {
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
	unsigned int i = 0;
	for (i = 0; i < dict->bucket_max; i++) {
		size_t current_value_siz = 0;
		const void *current_value = sparse_array_get(dict->buckets, i, &current_value_siz);

		if (current_value_siz != 0 && current_value != NULL) {
			struct sparse_bucket *existing_bucket = (struct sparse_bucket *)current_value;
			free(existing_bucket->val);
		}
	}
	sparse_array_free(dict->buckets);
	free(dict);
	return 1;
}
