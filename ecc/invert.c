#include "scalarmult.h"

void invert(int32_t *out, int32_t const *in) {
    int32_t t0[10], t1[10], t2[10], t3[10], i;

    sqr(t0,in);
    sqr(t1,t0);
    sqr(t1,t1);
    mul(t1,in,t1);
    mul(t0,t0,t1);
    sqr(t2,t0);
    mul(t1,t1,t2);
    sqr(t2,t1);
    for (i = 1;i < 5;++i)
        sqr(t2,t2);
    mul(t1,t2,t1);
    sqr(t2,t1);
    for (i = 1;i < 10;++i)
        sqr(t2,t2);
    mul(t2,t2,t1);
    sqr(t3,t2);
    for (i = 1;i < 20;++i)
        sqr(t3,t3);
    mul(t2,t3,t2);
    sqr(t2,t2);
    for (i = 1;i < 10;++i)
        sqr(t2,t2);
    mul(t1,t2,t1);
    sqr(t2,t1);
    for (i = 1;i < 50;++i)
        sqr(t2,t2);
    mul(t2,t2,t1);
    sqr(t3,t2);
    for (i = 1;i < 100;++i)
        sqr(t3,t3);
    mul(t2,t3,t2);
    sqr(t2,t2);
    for (i = 1;i < 50;++i)
        sqr(t2,t2);
    mul(t1,t2,t1);
    sqr(t1,t1);
    for (i = 1;i < 5;++i)
        sqr(t1,t1);
    mul(out,t1,t0);
}
