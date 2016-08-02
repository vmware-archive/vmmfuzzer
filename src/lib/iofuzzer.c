/** @file */

#include "array.h"
#include "iofuzzer.h"
#include "random.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXPORT 0xffff
#define MAXSIZE 256
#define NUM_VARIATES 7

struct iofuzzer {
	pthread_mutex_t mutex;
	size_t refcount;
	array_t *ports;
	random_t *random;
	char state[8];
	char *variate5;
	char *variate6;
	array_t *variates;
};

#include "io.h"

#define X(a) func_##a,
enum { FUNCS NUM_FUNCS };
#undef X

static iofuzzer_t *_iofuzzer_iterate(iofuzzer_t *fuzzer);
static unsigned long _iofuzzer_random_number(iofuzzer_t *fuzzer);
static iofuzzer_t *_iofuzzer_randomize(iofuzzer_t *fuzzer);
static iofuzzer_t *_iofuzzer_set_state(iofuzzer_t *fuzzer, const char *state, size_t size);

/**
 * Frees the memory allocated for the fuzzer.
 *
 * @param [in] fuzzer The fuzzer.
 * @return The fuzzer.
 */
iofuzzer_t *
iofuzzer_free(iofuzzer_t *fuzzer)
{
	if (fuzzer == NULL)
		return NULL;

	array_unref(fuzzer->ports);
	random_unref(fuzzer->random);
	free(fuzzer->variate5);
	free(fuzzer->variate6);
	array_unref(fuzzer->variates);
	pthread_mutex_destroy(&fuzzer->mutex);
	free(fuzzer);

	return NULL;
}

/**
 * Returns the ports of the fuzzer.
 *
 * @param [in] fuzzer The fuzzer.
 * @return The ports of the fuzzer.
 */
array_t *
iofuzzer_get_ports(iofuzzer_t *fuzzer)
{
	array_t *ports;

	if (fuzzer == NULL) {
		errno = EINVAL;
		return 0;
	}

	pthread_mutex_lock(&fuzzer->mutex);
	ports = fuzzer->ports;
	pthread_mutex_unlock(&fuzzer->mutex);

	return ports;
}

/**
 * Returns the pseudo-random number generator of the fuzzer.
 *
 * @param [in] fuzzer The fuzzer.
 * @return The pseudo-random number generator of the fuzzer.
 */
random_t *
iofuzzer_get_random(iofuzzer_t *fuzzer)
{
	random_t *random;

	if (fuzzer == NULL) {
		errno = EINVAL;
		return 0;
	}

	pthread_mutex_lock(&fuzzer->mutex);
	random = fuzzer->random;
	pthread_mutex_unlock(&fuzzer->mutex);

	return random;
}

/**
 * Returns the state of the fuzzer.
 *
 * @param [in] fuzzer The fuzzer.
 * @param [out] state The state of the fuzzer.
 * @param [in] size The size of the state.
 * @return The fuzzer.
 */
iofuzzer_t *
iofuzzer_get_state(iofuzzer_t *fuzzer, char *state, size_t size)
{
	if (fuzzer == NULL || state == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&fuzzer->mutex);
	memcpy(state, fuzzer->state, sizeof(fuzzer->state));
	pthread_mutex_unlock(&fuzzer->mutex);

	return fuzzer;
}

/**
 * Returns the variates of the fuzzer. The variates are:
 *
 *   1. I/O instruction/operation
 *   2. Data
 *   3. Implementation specific
 *   4. Counter for string operations
 *   5. I/O port address
 *   6. Source pointer for string operations
 *   7. Destination pointer for string operations
 *
 * The I/O instructions/operations are:
 *
 *   1. inb (input byte from port)
 *   2. inw (input word from port)
 *   3. inl (input doubleword from port)
 *   4. insb (input byte string from port)
 *   5. insw (input word string from port)
 *   6. insl (input doubleword string from port)
 *   7. outb (output byte to port)
 *   8. outw (output word to port)
 *   9. outl (output doubleword to port)
 *   10. outsb (output byte string to port)
 *   11. outsw (output word string to port)
 *   12. outsl (output doubleword string to port)
 *
 * @param [in] fuzzer The fuzzer.
 * @return The variates of the fuzzer.
 */
array_t *
iofuzzer_get_variates(iofuzzer_t *fuzzer)
{
	array_t *variates;

	if (fuzzer == NULL) {
		errno = EINVAL;
		return 0;
	}

	pthread_mutex_lock(&fuzzer->mutex);
	variates = fuzzer->variates;
	pthread_mutex_unlock(&fuzzer->mutex);

	return variates;
}

/**
 * Performs an iteration.
 *
 * @param [in] fuzzer The fuzzer.
 * @return The fuzzer.
 */
iofuzzer_t *
iofuzzer_iterate(iofuzzer_t *fuzzer)
{
	if (fuzzer == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&fuzzer->mutex);
	_iofuzzer_iterate(fuzzer);
	pthread_mutex_unlock(&fuzzer->mutex);

	return fuzzer;
}

/**
 * Performs an iteration with a given state.
 *
 * @param [in] fuzzer The fuzzer.
 * @param [in] state The state of the fuzzer.
 * @param [in] size The size of the state.
 * @return The fuzzer.
 */
iofuzzer_t *
iofuzzer_iterate_with_state(iofuzzer_t *fuzzer, const char *state, size_t size)
{
	if (fuzzer == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&fuzzer->mutex);
	if (_iofuzzer_set_state(fuzzer, state, size) == NULL) {
		pthread_mutex_unlock(&fuzzer->mutex);
		return NULL;
	}

	_iofuzzer_iterate(fuzzer);
	pthread_mutex_unlock(&fuzzer->mutex);

	return fuzzer;
}

/**
 * Creates a fuzzer.
 *
 * @return A fuzzer.
 */
iofuzzer_t *
iofuzzer_new(void)
{
	iofuzzer_t *fuzzer;
	uintptr_t *variates;

	fuzzer = calloc(1, sizeof(*fuzzer));
	if (fuzzer == NULL)
		return NULL;

	errno = pthread_mutex_init(&fuzzer->mutex, NULL);
	if (errno != 0)
		goto err;

	fuzzer->random = random_new();
	if (fuzzer->random == NULL)
		goto err;

	fuzzer->variate5 = calloc(MAXSIZE, sizeof(char));
	if (fuzzer->variate5 == NULL)
		goto err;

	fuzzer->variate6 = calloc(MAXSIZE, sizeof(char));
	if (fuzzer->variate6 == NULL)
		goto err;

	fuzzer->variates = array_new_with_length(sizeof(uintptr_t), NUM_VARIATES);
	if (fuzzer->variates == NULL)
		goto err;

	variates = &array_index(fuzzer->variates, uintptr_t, 0);
	variates[5] = (uintptr_t)fuzzer->variate5;
	variates[6] = (uintptr_t)fuzzer->variate6;

	_iofuzzer_randomize(fuzzer);
	iofuzzer_ref(fuzzer);

	return fuzzer;

err:
	iofuzzer_free(fuzzer);

	return NULL;
}

/**
 * Creates a fuzzer with a given state.
 *
 * @param [in] state The state of the fuzzer.
 * @param [in] size The size of the state.
 * @return A fuzzer.
 */
iofuzzer_t *
iofuzzer_new_with_state(const char *state, size_t size)
{
	iofuzzer_t *fuzzer;

	fuzzer = iofuzzer_new();
	if (fuzzer == NULL)
		return NULL;

	if (iofuzzer_set_state(fuzzer, state, size) == NULL) {
		iofuzzer_unref(fuzzer);
		return NULL;
	}

	return fuzzer;
}

/**
 * Increments the reference count of the fuzzer.
 *
 * @param [in] fuzzer The fuzzer.
 * @return The fuzzer.
 */
iofuzzer_t *
iofuzzer_ref(iofuzzer_t *fuzzer)
{
	if (fuzzer == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&fuzzer->mutex);
	fuzzer->refcount++;
	pthread_mutex_unlock(&fuzzer->mutex);

	return fuzzer;
}

/**
 * Sets the ports of the fuzzer.
 *
 * @param [in] fuzzer The fuzzer.
 * @param [in] ports The ports of the fuzzer.
 * @return The fuzzer.
 */
iofuzzer_t *
iofuzzer_set_ports(iofuzzer_t *fuzzer, array_t *ports)
{
	if (fuzzer == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&fuzzer->mutex);
	array_unref(fuzzer->ports);
	fuzzer->ports = ports;
	array_ref(fuzzer->ports);
	_iofuzzer_set_state(fuzzer, fuzzer->state, sizeof(fuzzer->state));
	_iofuzzer_randomize(fuzzer);
	pthread_mutex_unlock(&fuzzer->mutex);

	return fuzzer;
}

/**
 * Sets the pseudo-random number generator of the fuzzer.
 *
 * @param [in] fuzzer The fuzzer.
 * @param [in] variates The pseudo-random number generator of the fuzzer.
 * @return The fuzzer.
 */
iofuzzer_t *
iofuzzer_set_random(iofuzzer_t *fuzzer, random_t *random)
{
	if (fuzzer == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&fuzzer->mutex);
	random_unref(fuzzer->random);
	fuzzer->random = random;
	random_ref(fuzzer->random);
	_iofuzzer_randomize(fuzzer);
	pthread_mutex_unlock(&fuzzer->mutex);

	return fuzzer;
}

/**
 * Sets the state of the fuzzer.
 *
 * @param [in] fuzzer The fuzzer.
 * @param [in] state The state of the fuzzer.
 * @param [in] size The size of the state.
 * @return The fuzzer.
 */
iofuzzer_t *
iofuzzer_set_state(iofuzzer_t *fuzzer, const char *state, size_t size)
{
	if (fuzzer == NULL || state == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&fuzzer->mutex);
	if (_iofuzzer_set_state(fuzzer, state, size) == NULL) {
		pthread_mutex_unlock(&fuzzer->mutex);
		return NULL;
	}

	pthread_mutex_unlock(&fuzzer->mutex);

	return fuzzer;
}

/**
 * Sets the variates of the fuzzer.
 *
 * @param [in] fuzzer The fuzzer.
 * @param [in] variates The variates of the fuzzer.
 * @return The fuzzer.
 * @see iofuzzer_get_variates
 */
iofuzzer_t *
iofuzzer_set_variates(iofuzzer_t *fuzzer, array_t *variates)
{
	if (fuzzer == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pthread_mutex_lock(&fuzzer->mutex);
	array_unref(fuzzer->variates);
	fuzzer->variates = variates;
	array_ref(fuzzer->variates);
	pthread_mutex_unlock(&fuzzer->mutex);

	return fuzzer;
}

/**
 * Decrements the reference count of the fuzzer.
 *
 * @param [in] fuzzer The fuzzer.
 */
void
iofuzzer_unref(iofuzzer_t *fuzzer)
{
	if (fuzzer == NULL)
		return;

	pthread_mutex_lock(&fuzzer->mutex);
	fuzzer->refcount--;
	if (fuzzer->refcount > 0) {
		pthread_mutex_unlock(&fuzzer->mutex);
		return;
	}

	pthread_mutex_unlock(&fuzzer->mutex);
	iofuzzer_free(fuzzer);
}

static iofuzzer_t *
_iofuzzer_iterate(iofuzzer_t *fuzzer)
{
	uintptr_t *variates;

	if (fuzzer == NULL) {
		errno = EINVAL;
		return NULL;
	}

	variates = &array_index(fuzzer->variates, uintptr_t, 0);
	#define X(a) case func_##a: _iofuzzer_##a(fuzzer); break;
	switch (variates[0]) { FUNCS }
	#undef X

	_iofuzzer_randomize(fuzzer);

	return fuzzer;
}

static unsigned long
_iofuzzer_random_number(iofuzzer_t *fuzzer)
{
	switch (random_number_with_range(fuzzer->random, 0, 2)) {
	case 0:
		return random_number(fuzzer->random);

	case 1:
		return random_fermat_number(fuzzer->random);

	case 2:
		return random_mersenne_number(fuzzer->random);
	}
}

static iofuzzer_t *
_iofuzzer_randomize(iofuzzer_t *fuzzer)
{
	uintptr_t *variates;
	unsigned long *ports;

	if (fuzzer == NULL) {
		errno = EINVAL;
		return NULL;
	}

	random_get_state(fuzzer->random, fuzzer->state, sizeof(fuzzer->state));
	variates = &array_index(fuzzer->variates, uintptr_t, 0);
	variates[0] = random_number_with_range(fuzzer->random, 0, NUM_FUNCS - 1);
	variates[1] = _iofuzzer_random_number(fuzzer);
	variates[2] = _iofuzzer_random_number(fuzzer);
	variates[3] = random_number_with_range(fuzzer->random, 1, MAXSIZE / 4);
	if (fuzzer->ports != NULL) {
		ports = &array_index(fuzzer->ports, unsigned long, 0);
		variates[4] = ports[random_number_with_range(fuzzer->random, 0, array_get_length(fuzzer->ports) - 1)];
	} else
		variates[4] = random_number_with_range(fuzzer->random, 0, MAXPORT);

	variates[5] = (uintptr_t)random_string(fuzzer->random, (char *)variates[5], MAXSIZE);
	variates[6] = (uintptr_t)random_string(fuzzer->random, (char *)variates[6], MAXSIZE);

	return fuzzer;
}

static iofuzzer_t *
_iofuzzer_set_state(iofuzzer_t *fuzzer, const char *state, size_t size)
{
	if (fuzzer == NULL || state == NULL) {
		errno = EINVAL;
		return NULL;
	}

	if (random_set_state(fuzzer->random, state, size) == NULL)
		return NULL;

	_iofuzzer_randomize(fuzzer);

	return fuzzer;
}
