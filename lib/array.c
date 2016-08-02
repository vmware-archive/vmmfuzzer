/** @file */

#include "array.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MINLENGTH 16

#define _array_length_to_size(array, length) \
	((array)->element_size * (length))

#define _array_index(array, index) \
	(array_index((array), char, _array_length_to_size((array), (index))))

struct array {
	void *data;
	pthread_mutex_t mutex;
	size_t refcount;
	size_t element_size;
	float growth_factor;
	size_t length;
	size_t size;
};

static array_t *_array_grow(array_t *array, size_t length);
static array_t *_array_set_length(array_t *array, size_t length);
static array_t *_array_shrink(array_t *array, size_t length);

/**
 * Appends a given number of elements to the array.
 *
 * @param [in] array The array.
 * @param [in] data The elements.
 * @param [in] count The number of elements.
 * @return The array.
 */
array_t *
array_append_vals(array_t *array, const void *data, size_t count)
{
	size_t length;

	if (array == NULL || data == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&array->mutex);
	length = array->length;
	if (_array_set_length(array, length + count) != NULL)
		memcpy(&_array_index(array, length), data, _array_length_to_size(array, count));

	pthread_mutex_unlock(&array->mutex);

	return array;
}

/**
 * Frees the memory allocated for the array.
 *
 * @param [in] array The array.
 * @return The array.
 */
array_t *
array_free(array_t *array)
{
	if (array == NULL)
		return NULL;

	free(array->data);
	pthread_mutex_destroy(&array->mutex);
	free(array);

	return NULL;
}

/**
 * Returns the length of the array.
 *
 * @param [in] array The array.
 * @return The length of the array.
 */
size_t
array_get_length(array_t *array)
{
	size_t length;

	if (array == NULL) {
		errno = EINVAL;
		return 0;
	}

	pthread_mutex_lock(&array->mutex);
	length = array->length;
	pthread_mutex_unlock(&array->mutex);

	return length;
}

/**
 * Inserts a given number of elements into the array.
 *
 * @param [in] array The array.
 * @param [in] index The index.
 * @param [in] data The elements.
 * @param [in] count The number of elements.
 * @return The array.
 */
array_t *
array_insert_vals(array_t *array, unsigned long index, const void *data, size_t count)
{
	size_t length;

	if (array == NULL || data == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&array->mutex);
	length = array->length;
	if (index >= length) {
		errno = EINVAL;
		goto err;
	}

	if (_array_set_length(array, length + count) != NULL) {
		memmove(&_array_index(array, index + count), &_array_index(array, index), _array_length_to_size(array, length - index));
		memcpy(&_array_index(array, index), data, _array_length_to_size(array, count));
	}

err:
	pthread_mutex_unlock(&array->mutex);

	return array;
}

/**
 * Creates an array.
 *
 * @param [in] size The size of the element.
 * @return An array.
 */
array_t *
array_new(size_t size)
{
	array_t *array;

	array = calloc(1, sizeof(*array));
	if (array == NULL)
		return NULL;

	errno = pthread_mutex_init(&array->mutex, NULL);
	if (errno != 0)
		goto err;

	array->data = calloc(MINLENGTH, size);
	if (array->data == NULL)
		goto err;

	array->element_size = size;
	array->growth_factor = 2;
	array->length = 0;
	array->size = size;
	array_ref(array);

	return array;

err:
	array_free(array);

	return NULL;
}

/**
 * Creates an array with a given length.
 *
 * @param [in] size The size of an element of the array.
 * @param [in] length The length of the array.
 * @return An array.
 */
array_t *
array_new_with_length(size_t size, size_t length)
{
	array_t *array;

	array = array_new(size);
	if (array == NULL)
		return NULL;

	if (_array_set_length(array, length) == NULL) {
		array_unref(array);
		return NULL;
	}

	return array;
}

/**
 * Prepends a given number of elements to the array.
 *
 * @param [in] array The array.
 * @param [in] data The elements.
 * @param [in] count The number of elements.
 * @return The array.
 */
array_t *
array_prepend_vals(array_t *array, const void *data, size_t count)
{
	size_t length;

	if (array == NULL || data == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&array->mutex);
	length = array->length;
	if (_array_set_length(array, length + count) != NULL) {
		memmove(&_array_index(array, count), &_array_index(array, 0), _array_length_to_size(array, length));
		memcpy(&_array_index(array, 0), data, _array_length_to_size(array, count));
	}

	pthread_mutex_unlock(&array->mutex);

	return array;
}

/**
 * Increments the reference count of the array.
 *
 * @param [in] array The array.
 * @return The array.
 */
array_t *
array_ref(array_t *array)
{
	if (array == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&array->mutex);
	array->refcount++;
	pthread_mutex_unlock(&array->mutex);

	return array;
}

/**
 * Removes an element from the array.
 *
 * @param [in] array The array.
 * @param [in] index The index.
 * @return The array.
 */
array_t *
array_remove_val_fast(array_t *array, unsigned long index)
{
	size_t length;

	if (array == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&array->mutex);
	length = array->length;
	if (index >= length) {
		errno = EINVAL;
		goto err;
	}

	memmove(&_array_index(array, index), &_array_index(array, length - 1), _array_length_to_size(array, 1));
	_array_set_length(array, length - 1);

err:
	pthread_mutex_unlock(&array->mutex);

	return array;
}

/**
 * Removes a given number of elements from the array.
 *
 * @param [in] array The array.
 * @param [in] index The index.
 * @param [in] count The number of elements.
 * @return The array.
 */
array_t *
array_remove_vals(array_t *array, unsigned long index, size_t count)
{
	size_t length;

	if (array == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&array->mutex);
	length = array->length;
	if (index >= length) {
		errno = EINVAL;
		goto err;
	}

	memmove(&_array_index(array, index), &_array_index(array, index + count), _array_length_to_size(array, length - (index + count)));
	_array_set_length(array, length - count);

err:
	pthread_mutex_unlock(&array->mutex);

	return array;
}

/**
 * Sets the length of the array.
 *
 * @param [in] array The array.
 * @param [in] length The length of the array.
 * @return The array.
 */
array_t *
array_set_length(array_t *array, size_t length)
{
	array_t *retval;

	if (array == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&array->mutex);
	retval = _array_set_length(array, length);
	pthread_mutex_unlock(&array->mutex);

	return retval;
}

/**
 * Decrements the reference count of the array.
 *
 * @param [in] array The array.
 */
void
array_unref(array_t *array)
{
	if (array == NULL)
		return;

	pthread_mutex_lock(&array->mutex);
	array->refcount--;
	if (array->refcount > 0) {
		pthread_mutex_unlock(&array->mutex);
		return;
	}

	pthread_mutex_unlock(&array->mutex);
	array_free(array);
}

/**
 * Increases the length of the array.
 *
 * @param [in] array The array.
 * @param [in] length The length of the array.
 * @return The array.
 */
static array_t *
_array_grow(array_t *array, size_t length)
{
	size_t size;
	size_t m;
	size_t n;
	void *data;

	if (array == NULL) {
		errno = EINVAL;
		return NULL;
	}

	size = _array_length_to_size(array, length);
	if (size <= array->size)
		return NULL;

	for (m = 1, n = array->size; size > n; m++)
		n *= array->growth_factor * m;

	data = realloc(array->data, n);
	if (data == NULL)
		return NULL;

	array->data = data;
	array->length = length;
	array->size = n;

	return array;
}

/**
 * Sets the length of the array.
 *
 * @param [in] array The array.
 * @param [in] length The length of the array.
 * @return The array.
 */
static array_t *
_array_set_length(array_t *array, size_t length)
{
	size_t size;

	if (array == NULL) {
		errno = EINVAL;
		return NULL;
	}

	size = _array_length_to_size(array, length);
	if (size <= array->size)
		return _array_shrink(array, length);

	return _array_grow(array, length);
}

/**
 * Decreases the length of the array.
 *
 * @param [in] array The array.
 * @param [in] length The length of the array.
 * @return The array.
 */
static array_t *
_array_shrink(array_t *array, size_t length)
{
	size_t size;

	if (array == NULL) {
		errno = EINVAL;
		return NULL;
	}

	size = _array_length_to_size(array, length);
	if (size > array->size)
		return NULL;

	array->length = length;

	return array;
}
