#include "scalarmult.h"

void mul121665(int32_t *out, int32_t const *in) {

   int64_t c;

    int64_t tmp0 = in[0];
    int64_t tmp1 = in[1];
    int64_t tmp2 = in[2];
    int64_t tmp3 = in[3];
    int64_t tmp4 = in[4];
    int64_t tmp5 = in[5];
    int64_t tmp6 = in[6];
    int64_t tmp7 = in[7];
    int64_t tmp8 = in[8];
    int64_t tmp9 = in[9];

    tmp0 *= 121665;
    tmp1 *= 121665;
    tmp2 *= 121665;
    tmp3 *= 121665;
    tmp4 *= 121665;
    tmp5 *= 121665;
    tmp6 *= 121665;
    tmp7 *= 121665;
    tmp8 *= 121665;
    tmp9 *= 121665;


    c = tmp0 >> 26; tmp1 += c; tmp0 -= c << 26; 
    c = tmp1 >> 26; tmp2 += c; tmp1 -= c << 26;
    c = tmp2 >> 25; tmp3 += c; tmp2 -= c << 25;
    c = tmp3 >> 26; tmp4 += c; tmp3 -= c << 26;
    c = tmp4 >> 25; tmp5 += c; tmp4 -= c << 25;
    c = tmp5 >> 26; tmp6 += c; tmp5 -= c << 26;
    c = tmp6 >> 26; tmp7 += c; tmp6 -= c << 26;
    c = tmp7 >> 25; tmp8 += c; tmp7 -= c << 25;
    c = tmp8 >> 26; tmp9 += c; tmp8 -= c << 26;
    c = tmp9 >> 25; tmp0 += c * 38; tmp9 -= c << 25;
    c = tmp0 >> 26; tmp1 += c; tmp0 -= c << 26;

    out[0] = tmp0;
    out[1] = tmp1;
    out[2] = tmp2;
    out[3] = tmp3;
    out[4] = tmp4;
    out[5] = tmp5;
    out[6] = tmp6;
    out[7] = tmp7;
    out[8] = tmp8;
    out[9] = tmp9;
}
