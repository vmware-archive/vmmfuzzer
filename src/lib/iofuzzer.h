/** @file */

#ifndef IOFUZZER_H
#define IOFUZZER_H

#include "array.h"
#include "random.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct iofuzzer iofuzzer_t; /**< I/O address space fuzzer. */

iofuzzer_t *iofuzzer_free(iofuzzer_t *fuzzer);
array_t *iofuzzer_get_ports(iofuzzer_t *fuzzer);
random_t *iofuzzer_get_random(iofuzzer_t *fuzzer);
iofuzzer_t *iofuzzer_get_state(iofuzzer_t *fuzzer, char *state, size_t size);
array_t *iofuzzer_get_variates(iofuzzer_t *fuzzer);
iofuzzer_t *iofuzzer_iterate(iofuzzer_t *fuzzer);
iofuzzer_t *iofuzzer_iterate_with_state(iofuzzer_t *fuzzer, const char *state, size_t size);
iofuzzer_t *iofuzzer_new(void);
iofuzzer_t *iofuzzer_new_with_state(const char *state, size_t size);
iofuzzer_t *iofuzzer_ref(iofuzzer_t *fuzzer);
iofuzzer_t *iofuzzer_set_ports(iofuzzer_t *fuzzer, array_t *ports);
iofuzzer_t *iofuzzer_set_random(iofuzzer_t *fuzzer, random_t *random);
iofuzzer_t *iofuzzer_set_state(iofuzzer_t *fuzzer, const char *state, size_t size);
iofuzzer_t *iofuzzer_set_variates(iofuzzer_t *fuzzer, array_t *variates);
void iofuzzer_unref(iofuzzer_t *fuzzer);

#ifdef __cplusplus
}
#endif

#endif /* IOFUZZER_H */
