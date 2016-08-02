/** @file */

#ifndef ARRAY_H
#define ARRAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * Appends an element to the array.
 *
 * @param [in] array The array.
 * @param [in] data The element.
 * @return The array.
 */
#define array_append_val(array, data) \
        (array_append_vals((array), (data), 1))

/**
 * Returns the element at the given index.
 *
 * @param [in] array The array.
 * @param [in] type The type of the element.
 * @param [in] index The index.
 * @return The element at the given index.
 */
#define array_index(array, type, index) \
        (((type *)((void **)(array))[0])[(index)])

/**
 * Inserts an element into the array.
 *
 * @param [in] array The array.
 * @param [in] index The index.
 * @param [in] data The element.
 * @return The array.
 */
#define array_insert_val(array, index, data) \
        (array_insert_vals((array), (index), (data), 1))

/**
 * Prepends an element to the array.
 *
 * @param [in] array The array.
 * @param [in] data The element.
 * @return The array.
 */
#define array_prepend_val(array, data) \
        (array_prepend_vals((array), (data), 1))

/**
 * Removes an element from the array.
 *
 * @param [in] array The array.
 * @param [in] index The index.
 * @return The array.
 */
#define array_remove_index(array, index) \
        (array_remove_vals((array), (index), 1))

/**
 * Removes an element from the array.
 *
 * @param [in] array The array.
 * @param [in] index The index.
 * @return The array.
 */
#define array_remove_index_fast array_remove_val_fast

/**
 * Removes an element from the array.
 *
 * @param [in] array The array.
 * @param [in] index The index.
 * @return The array.
 */
#define array_remove_val(array, index) \
        (array_remove_vals((array), (index), 1))

typedef struct array array_t; /**< Dynamic array. */

array_t *array_append_vals(array_t *array, const void *data, size_t count);
array_t *array_free(array_t *array);
size_t array_get_length(array_t *array);
array_t *array_insert_vals(array_t *array, unsigned long index, const void *data, size_t count);
array_t *array_new(size_t size);
array_t *array_new_with_length(size_t size, size_t length);
array_t *array_prepend_vals(array_t *array, const void *data, size_t count);
array_t *array_ref(array_t *array);
array_t *array_remove_val_fast(array_t *array, unsigned long index);
array_t *array_remove_vals(array_t *array, unsigned long index, size_t count);
array_t *array_set_length(array_t *array, size_t length);
void array_unref(array_t *array);

#ifdef __cplusplus
}
#endif

#endif /* ARRAY_H */
