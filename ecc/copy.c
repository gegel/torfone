#include "scalarmult.h"

void copy(int32_t *dst, int32_t const *src) {
    unsigned idx;
    for (idx = 0; idx < 10; ++idx)
        dst[idx] = src[idx];
}
