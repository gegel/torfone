#include "scalarmult.h"

void cswap(int32_t *a, int32_t *b, uint32_t bit) {
    int64_t dummy0 = a[0] ^ b[0];
    int64_t dummy1 = a[1] ^ b[1];
    int64_t dummy2 = a[2] ^ b[2];
    int64_t dummy3 = a[3] ^ b[3];
    int64_t dummy4 = a[4] ^ b[4];
    int64_t dummy5 = a[5] ^ b[5];
    int64_t dummy6 = a[6] ^ b[6];
    int64_t dummy7 = a[7] ^ b[7];
    int64_t dummy8 = a[8] ^ b[8];
    int64_t dummy9 = a[9] ^ b[9];

    bit = -bit;

    dummy0 &= bit;
    dummy1 &= bit;
    dummy2 &= bit;
    dummy3 &= bit;
    dummy4 &= bit;
    dummy5 &= bit;
    dummy6 &= bit;
    dummy7 &= bit;
    dummy8 &= bit;
    dummy9 &= bit;

    a[0] ^= dummy0;
    a[1] ^= dummy1;
    a[2] ^= dummy2;
    a[3] ^= dummy3;
    a[4] ^= dummy4;
    a[5] ^= dummy5;
    a[6] ^= dummy6;
    a[7] ^= dummy7;
    a[8] ^= dummy8;
    a[9] ^= dummy9;

    b[0] ^= dummy0;
    b[1] ^= dummy1;
    b[2] ^= dummy2;
    b[3] ^= dummy3;
    b[4] ^= dummy4;
    b[5] ^= dummy5;
    b[6] ^= dummy6;
    b[7] ^= dummy7;
    b[8] ^= dummy8;
    b[9] ^= dummy9;
}
