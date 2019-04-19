#ifndef SCALARMULT_INT__
#define SCALARMULT_INT__

#include <stdint.h>

void to_bytes(unsigned char *dst, int32_t const *src);
void from_bytes(int32_t *dst, unsigned char const *src);
void cswap(int32_t *a, int32_t *b, uint32_t bit);
void copy(int32_t *dst, int32_t const *src);
void add(int32_t *out, int32_t const *a, int32_t const *b);
void sub(int32_t *out, int32_t const *a, int32_t const *b);
void sqr(int32_t *out, int32_t const *in);
void mul(int32_t *out, int32_t const *a, int32_t const *b);
void mul121665(int32_t *out, int32_t const *in);
void invert(int32_t *out, int32_t const *in);

#endif
