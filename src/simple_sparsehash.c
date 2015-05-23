/* vim: noet ts=4 sw=4
*/
#include <stdlib.h>
#include <string.h>
#include "simple_sparsehash.h"

#define FULL_ELEM_SIZE (arr->elem_size + sizeof(size_t))

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
	for (; pos > 8; pos -= 8)
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

/* Sparse Array */
struct sparse_array *sparse_array_init(const size_t element_size) {
	struct sparse_array *arr = NULL;
	arr = calloc(1, sizeof(struct sparse_array));
	if (arr == NULL)
		return NULL;

	arr->elem_size = element_size;
	return arr;
}

const int sparse_array_set(struct sparse_array *arr, const size_t i,
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
	if (!is_position_occupied(arr->bitmap, offset)) {
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
			memcpy((unsigned char *)(new_group) + (offset * FULL_ELEM_SIZE + 1),
					(unsigned char *)(arr->group) + (offset * FULL_ELEM_SIZE),
					to_move_siz);
		}

		/* Increase the bucket count because we've expanded: */
		arr->count++;
		free(arr->group);
		arr->group = new_group;
	}

	/* Copy the size into the position, fighting -pedantic the whole
	 * time.
	 */
	destination = (unsigned char *)(arr->group) + (offset * FULL_ELEM_SIZE);
	memcpy(destination, &vlen, sizeof(vlen));

	/* Here we mutate a variable because we're writing C and we don't respect
	 * anything
	 */
	destination = (unsigned char *)destination + sizeof(size_t);
	memcpy(destination, val, vlen);

	return 1;
}

const void *sparse_array_get(struct sparse_array *arr,
							 const size_t i, size_t *outsize) {
	return NULL;
}

const int sparse_array_free(struct sparse_array *arr) {
	free(arr->group);
	free(arr);
	return 1;
}

/* Sparse Dictionary */
struct sparse_dict *sparse_dict_init() {
	struct sparse_dict *new = NULL;
	new = calloc(1, sizeof(struct sparse_dict));
	if (new == NULL)
		return NULL;
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
