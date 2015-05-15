/* vim: noet ts=4 sw=4
*/
#include <stdlib.h>
#include "simple_sparsehash.h"

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
 */
static const size_t position_to_offset(const unsigned char *bitmap,
									   size_t position) {
	return 0;
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
	const size_t offset = position_to_offset(arr->bitmap, i);
	if (is_position_occupied(arr->bitmap, offset)) {
		return 0;
	} else {
		return 0;
	}

	return 1;
}

const void *sparse_array_get(struct sparse_array *arr,
							 const size_t i, size_t outsize) {
	return NULL;
}

const int sparse_array_free(struct sparse_array *arr) {
	free(arr);
	return 0;
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
