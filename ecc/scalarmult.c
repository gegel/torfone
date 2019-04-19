#include "scalarmult.h"
#include "ecc.h"

void scalarmult(unsigned char *q, unsigned char const *n, unsigned char const *p) {

    int idx;

    int32_t x1[10], x2[10] = {1}, x3[10], z2[10] = {0}, z3[10] = {1};
    int32_t t0[10], t1[10];
    unsigned char e[32];
    uint32_t b, swap = 0;

    for (idx = 0; idx < 32; ++idx)
        e[idx] = n[idx];
    e[0] &= 248;
    e[31] &= 127;
    e[31] |= 64;

    from_bytes(x1, p);
    copy(x3, x1);

    for (idx = 254; idx >= 0; --idx) {
        b = e[idx / 8] >> (idx & 7);
        b &= 1;
        swap ^= b;
        cswap(x2, x3, swap);
        cswap(z2, z3, swap);
        swap = b;

        add(t0, x2, z2);        // t0 <- A
        add(t1, x3, z3);        // t1 <- C
        sub(x2, x2, z2);        // x2 <- B
        sub(z3, x3, z3);        // z3 <- D
        mul(t1, t1, x2);        // t1 <- CB
        mul(z3, z3, t0);        // z3 <- DA
        add(x3, z3, t1);        // x3 <- DA + CB
        sub(z3, z3, t1);        // z3 <- DA - CB
        sqr(x3, x3);            // x3 <- (DA + CB)^2
        sqr(z3, z3);            // z3 <- (DA - CB)^2
        mul(z3, x1, z3);        // z3 <- x1 * (DA - CB)^2
        sqr(t0, t0);            // t0 <- A^2
        sqr(t1, x2);            // t1 <- B^2
        mul(x2, t0, t1);        // x2 <- A^2 * B^2
        sub(t1, t0, t1);        // t1 <- A^2 - B^2
        mul121665(z2, t1);      // z2 <- a24 * E
        add(z2, t0, z2);        // z2 <- A^2 + a24 * E
        mul(z2, z2, t1);        // z2 <- E * (A^2 + a24 * E)
    }
    cswap(x2, x3, swap);
    cswap(z2, z3, swap);
    
//  q = x2 * (z2^(p-2))
    invert(z2,z2);
    mul(x2, x2, z2);
    to_bytes(q, x2);
}    
