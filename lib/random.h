/** @file */

#ifndef RANDOM_H
#define RANDOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * Returns the next uniformly distributed pseudo-random unsigned long in
 * the range given by the interval [0,1].
 *
 * @param [in] random The pseudo-random number generator.
 * @return A uniformly distributed pseudo-random unsigned long in the
 *   range given by the interval [0,1].
 */
#define random_boolean(random) \
        (random_ulong_with_range(random, 0, 1))

/**
 * Returns the next uniformly distributed pseudo-random unsigned long in
 * the range given by the interval [0,2^32).
 *
 * @param [in] random The pseudo-random number generator.
 * @return A uniformly distributed pseudo-random unsigned long in the
 *   range given by the interval [0,2^32).
 */
#define random_number random_ulong

/**
 * Returns the next uniformly distributed pseudo-random unsigned long in
 * the range given by the interval [begin,end].
 *
 * @param [in] random The pseudo-random number generator.
 * @return A uniformly distributed pseudo-random unsigned long in the
 *   range given by the interval [begin,end].
 */
#define random_number_with_range random_ulong_with_range

typedef struct random random_t; /**< Pseudo-random number generator. */

double random_double(random_t *random);
double random_double_with_range(random_t *random, double begin, double end);
unsigned long random_fermat_number(random_t *random);
random_t *random_free(random_t *random);
random_t *random_get_state(random_t *random, char *state, size_t size);
unsigned long random_mersenne_number(random_t *random);
random_t *random_new(void);
random_t *random_new_with_state(const char *state, size_t size);
random_t *random_ref(random_t *random);
random_t *random_set_state(random_t *random, const char *state, size_t size);
char *random_string(random_t *random, char *string, size_t length);
unsigned long random_ulong(random_t *random);
unsigned long random_ulong_with_range(random_t *random, unsigned long begin, unsigned long end);
void random_unref(random_t *random);

#ifdef __cplusplus
}
#endif

#endif /* RANDOM_H */
