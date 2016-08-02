/** @file */

#ifndef IO_H
#define IO_H

#define A(a, b) \
static inline void \
_iofuzzer_in##a(iofuzzer_t *fuzzer) \
{ \
	uintptr_t *variates; \
\
	variates = &array_index(fuzzer->variates, uintptr_t, 0); \
	asm volatile("in" #a " %w3, %" #b "0" :: "a" (variates[1]), "b" (variates[2]), "c" (variates[3]), "d" (variates[4]), "S" (variates[5]), "D" (variates[6])); \
} \
\
static inline void \
_iofuzzer_ins##a(iofuzzer_t *fuzzer) \
{ \
	uintptr_t *variates; \
\
	variates = &array_index(fuzzer->variates, uintptr_t, 0); \
	asm volatile("rep; ins" #a :: "a" (variates[1]), "b" (variates[2]), "c" (variates[3]), "d" (variates[4]), "S" (variates[5]), "D" (variates[6])); \
} \
\
static inline void \
_iofuzzer_out##a(iofuzzer_t *fuzzer) \
{ \
	uintptr_t *variates; \
\
	variates = &array_index(fuzzer->variates, uintptr_t, 0); \
	asm volatile("out" #a " %" #b "0, %w3" :: "a" (variates[1]), "b" (variates[2]), "c" (variates[3]), "d" (variates[4]), "S" (variates[5]), "D" (variates[6])); \
} \
\
static inline void \
_iofuzzer_outs##a(iofuzzer_t *fuzzer) \
{ \
	uintptr_t *variates; \
\
	variates = &array_index(fuzzer->variates, uintptr_t, 0); \
	asm volatile("rep; outs" #a :: "a" (variates[1]), "b" (variates[2]), "c" (variates[3]), "d" (variates[4]), "S" (variates[5]), "D" (variates[6])); \
}

A(b, b)
A(w, w)
A(l,)
#undef A

#define FUNCS \
	X(inb) \
	X(inw) \
	X(inl) \
	X(insb) \
	X(insw) \
	X(insl) \
	X(outb) \
	X(outw) \
	X(outl) \
	X(outsb) \
	X(outsw) \
	X(outsl)

#endif /* IO_H */
