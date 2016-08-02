/** @file */

#include "random.h"

#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXSIZE 256
#define MINSIZE (sizeof(unsigned short) * 3) /* unsigned short int xsubi[3] */

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

struct random {
	pthread_mutex_t mutex;
	size_t refcount;
	char *state;
};

/**
 * Returns the next uniformly distributed pseudo-random double in the
 * range given by the interval [0,1).
 *
 * @param [in] random The pseudo-random number generator.
 * @return A uniformly distributed pseudo-random double in the range given
 *   by the interval [0,1).
 */
double
random_double(random_t *random)
{
	double retval;

	pthread_mutex_lock(&random->mutex);
	retval = erand48((unsigned short *)random->state);
	pthread_mutex_unlock(&random->mutex);

	return retval;
}

/**
 * Returns the next uniformly distributed pseudo-random double in the
 * range given by the interval [begin,end].
 *
 * @param [in] random The pseudo-random number generator.
 * @return A uniformly distributed pseudo-random double in the range given
 *   by the interval [begin,end].
 */
double
random_double_with_range(random_t *random, double begin, double end)
{
	return random_double(random) * (end - begin + 1) + begin;
}

/**
 * Returns the next uniformly distributed pseudo-random Fermat number
 * given by the binomial number of the form (2^n)+1 in the range given by
 * the interval [3,(2^31)+1].
 *
 * @param [in] random The pseudo-random number generator.
 * @return A uniformly distributed pseudo-random Fermat number given by
 *   the binomial number of the form (2^n)+1 in the range given by the
 *   interval [3,(2^31)+1].
 */
unsigned long
random_fermat_number(random_t *random)
{
	return (unsigned long)(pow(2, random_number_with_range(random, 1, 31)) + 1);
}

/**
 * Frees the memory allocated for the pseudo-random number generator.
 *
 * @param [in] random The pseudo-random number generator.
 * @return The pseudo-random number generator.
 */
random_t *
random_free(random_t *random)
{
	if (random == NULL)
		return NULL;

	free(random->state);
	pthread_mutex_destroy(&random->mutex);
	free(random);

	return NULL;
}

/**
 * Returns the state of the pseudo-random number generator.
 *
 * @param [in] random The pseudo-random number generator.
 * @param [out] state The state of the pseudo-random number generator.
 * @param [in] size The size of the state.
 * @return The pseudo-random number generator.
 */
random_t *
random_get_state(random_t *random, char *state, size_t size)
{
	if (random == NULL || state == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&random->mutex);
	memcpy(state, random->state, MIN(MAX(size, MINSIZE), MAXSIZE));
	pthread_mutex_unlock(&random->mutex);

	return random;
}

/**
 * Returns the next uniformly distributed pseudo-random Mersenne number
 * given by the binomial number of the form (2^n)-1 in the range given by
 * the interval [1,2^32).
 *
 * @param [in] random The pseudo-random number generator.
 * @return A uniformly distributed pseudo-random Mersenne number given by
 *   the binomial number of the form (2^n)-1 in the range given by the
 *   interval [1,2^32).
 */
unsigned long
random_mersenne_number(random_t *random)
{
	return (unsigned long)(pow(2, random_number_with_range(random, 1, 32)) - 1);
}

/**
 * Creates a pseudo-random number generator.
 *
 * @return A pseudo-random number generator.
 */
random_t *
random_new(void)
{
	random_t *random;

	random = calloc(1, sizeof(*random));
	if (random == NULL)
		return NULL;

	errno = pthread_mutex_init(&random->mutex, NULL);
	if (errno != 0)
		goto err;

	random->state = calloc(1, MAXSIZE);
	if (random->state == NULL)
		goto err;

	random_ref(random);

	return random;

err:
	random_free(random);

	return NULL;
}

/**
 * Creates a pseudo-random number generator with a given state.
 *
 * @param [in] state The state of the pseudo-random number generator.
 * @param [in] size The size of the state.
 * @return A pseudo-random number generator.
 */
random_t *
random_new_with_state(const char *state, size_t size)
{
	random_t *random;

	random = random_new();
	if (random == NULL)
		return NULL;

	if (random_set_state(random, state, size) == NULL) {
		random_unref(random);
		return NULL;
	}

	return random;
}

/**
 * Increments the reference count of the pseudo-random number generator.
 *
 * @param [in] random The pseudo-random number generator.
 * @return The pseudo-random number generator.
 */
random_t *
random_ref(random_t *random)
{
	if (random == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&random->mutex);
	random->refcount++;
	pthread_mutex_unlock(&random->mutex);

	return random;
}

/**
 * Sets the state of the pseudo-random number generator.
 *
 * @param [in] random The pseudo-random number generator.
 * @param [in] state The state of the pseudo-random number generator.
 * @param [in] size The size of the state.
 * @return The pseudo-random number generator.
 */
random_t *
random_set_state(random_t *random, const char *state, size_t size)
{
	if (random == NULL || state == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&random->mutex);
	memcpy(random->state, state, MIN(MAX(size, MINSIZE), MAXSIZE));
	pthread_mutex_unlock(&random->mutex);

	return random;
}

/**
 * Generates a pseudo-random null-terminated string of a given length.
 *
 * @param [in] random The pseudo-random number generator.
 * @param [out] string A pseudo-random null-terminated string.
 * @param [in] length The length of the pseudo-random string.
 * @return A pseudo-random null-terminated string.
 */
char *
random_string(random_t *random, char *string, size_t length)
{
	int i;
	const char charset[] = " !\"#$%%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

	if (random == NULL || string == NULL) {
		errno = EINVAL;
		return NULL;
	}

	for (i = 0; i < length - 2; i++)
		string[i] = charset[random_number_with_range(random, 0, sizeof(charset) - 1)];

	string[i] = '\0';

	return string;
}

/**
 * Returns the next uniformly distributed pseudo-random unsigned long in
 * the range given by the interval [0,2^32).
 *
 * @param [in] random The pseudo-random number generator.
 * @return A uniformly distributed pseudo-random unsigned long in the
 *   range given by the interval [0,2^32).
 */
unsigned long
random_ulong(random_t *random)
{
	unsigned long retval;

	pthread_mutex_lock(&random->mutex);
	retval = (unsigned long)jrand48((unsigned short *)random->state);
	pthread_mutex_unlock(&random->mutex);

	return retval;
}

/**
 * Returns the next uniformly distributed pseudo-random unsigned long in
 * the range given by the interval [begin,end].
 *
 * @param [in] random The pseudo-random number generator.
 * @return A uniformly distributed pseudo-random unsigned long in the
 *   range given by the interval [begin,end].
 */
unsigned long
random_ulong_with_range(random_t *random, unsigned long begin, unsigned long end)
{
	return (unsigned long)(random_double(random) * (end - begin + 1) + begin);
}

/**
 * Decrements the reference count of the pseudo-random number generator.
 *
 * @param [in] random The pseudo-random number generator.
 */
void
random_unref(random_t *random)
{
	if (random == NULL)
		return;

	pthread_mutex_lock(&random->mutex);
	random->refcount--;
	if (random->refcount > 0) {
		pthread_mutex_unlock(&random->mutex);
		return;
	}

	pthread_mutex_unlock(&random->mutex);
	random_free(random);
}
