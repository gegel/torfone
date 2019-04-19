#include "scalarmult.h"

void sqr(int32_t *out, int32_t const *in) {
    int64_t c;

    int64_t in0 = in[0];
    int64_t in1 = in[1];
    int64_t in2 = in[2];
    int64_t in3 = in[3];
    int64_t in4 = in[4];
    int64_t in5 = in[5];
    int64_t in6 = in[6];
    int64_t in7 = in[7];
    int64_t in8 = in[8];
    int64_t in9 = in[9];

    // Compute F0_
    int64_t F0_0 = in0 * in0;
    int64_t F0_1 = 2 * in0 * in1;
    int64_t F0_2 = 2 * in0 * in2 + in1 * in1;
    int64_t F0_3 = 2 * in0 * in3 + 4 * in1 * in2;
    int64_t F0_4 = 2 * in0 * in4 + 2 * in1 * in3 + 2 * in2 * in2;
    int64_t F0_5 = 4 * in1 * in4 + 4 * in2 * in3;
    int64_t F0_6 = 4 * in2 * in4 + in3 * in3;
    int64_t F0_7 = 2 * in3 * in4;
    int64_t F0_8 = 2 * in4 * in4;

    // Compute F1_
    int64_t F1_0 = in5 * in5;
    int64_t F1_1 = 2 * in5 * in6;
    int64_t F1_2 = 2 * in5 * in7 + in6 * in6;
    int64_t F1_3 = 2 * in5 * in8 + 4 * in6 * in7;
    int64_t F1_4 = 2 * in5 * in9 + 2 * in6 * in8 + 2 * in7 * in7;
    int64_t F1_5 = 4 * in6 * in9 + 4 * in7 * in8;
    int64_t F1_6 = 4 * in7 * in9 + in8 * in8;
    int64_t F1_7 = 2 * in8 * in9;
    int64_t F1_8 = 2 * in9 * in9;

    // Compute (F0 + F1)(G0 + G1)
    int64_t F01_0 = in0 + in5;
    int64_t F01_1 = in1 + in6;
    int64_t F01_2 = in2 + in7;
    int64_t F01_3 = in3 + in8;
    int64_t F01_4 = in4 + in9;

    int64_t F01F01_0 = F01_0 * F01_0;
    int64_t F01F01_1 = 2 * F01_0 * F01_1;
    int64_t F01F01_2 = 2 * F01_0 * F01_2 + F01_1 * F01_1;
    int64_t F01F01_3 = 2 * F01_0 * F01_3 + 4 * F01_1 * F01_2;
    int64_t F01F01_4 = 2 * F01_0 * F01_4 + 2 * F01_1 * F01_3 + 2 * F01_2 * F01_2;
    int64_t F01F01_5 = 4 * F01_1 * F01_4 + 4 * F01_2 * F01_3;
    int64_t F01F01_6 = 4 * F01_2 * F01_4 + F01_3 * F01_3;
    int64_t F01F01_7 = 2 * F01_3 * F01_4;
    int64_t F01F01_8 = 2 * F01_4 * F01_4;

    // 2.1
    int64_t out0 = F0_0 - 38 * F1_5;
    int64_t out1 = F0_1 - 38 * F1_6;
    int64_t out2 = F0_2 - 38 * F1_7;
    int64_t out3 = F0_3 - 38 * F1_8;
    int64_t out4 = F0_4;
    int64_t out5 = F0_5 - F1_0;
    int64_t out6 = F0_6 - F1_1;
    int64_t out7 = F0_7 - F1_2;
    int64_t out8 = F0_8 - F1_3;
    int64_t out9 = F1_4;

    // 2.2
    // Do not reduce, subtract self at 2^128 boundary
    int64_t out10 = out5;
    int64_t out11 = out6;
    int64_t out12 = out7;
    int64_t out13 = out8;
    int64_t out14 = out9;
    out5 -= out0;
    out6 -= out1;
    out7 -= out2;
    out8 -= out3;
    out9 += out4;

    // 2.3
    out5 += F01F01_0;
    out6 += F01F01_1;
    out7 += F01F01_2;
    out8 += F01F01_3;
    out9 = F01F01_4 - out9;
    out10 -= F01F01_5;
    out11 -= F01F01_6;
    out12 -= F01F01_7;
    out13 -= F01F01_8;

    out0 -= 38 * out10;
    out1 -= 38 * out11;
    out2 -= 38 * out12;
    out3 -= 38 * out13;
    out4 += 38 * out14;


    c = out0 >> 26; out1 += c; out0 -= c << 26; 
    c = out1 >> 26; out2 += c; out1 -= c << 26;
    c = out2 >> 25; out3 += c; out2 -= c << 25;
    c = out3 >> 26; out4 += c; out3 -= c << 26;
    c = out4 >> 25; out5 += c; out4 -= c << 25;
    c = out5 >> 26; out6 += c; out5 -= c << 26;
    c = out6 >> 26; out7 += c; out6 -= c << 26;
    c = out7 >> 25; out8 += c; out7 -= c << 25;
    c = out8 >> 26; out9 += c; out8 -= c << 26;
    c = out9 >> 25; out0 += c * 38; out9 -= c << 25;
    c = out0 >> 26; out1 += c; out0 -= c << 26;

    out[0] = out0;
    out[1] = out1;
    out[2] = out2;
    out[3] = out3;
    out[4] = out4;
    out[5] = out5;
    out[6] = out6;
    out[7] = out7;
    out[8] = out8;
    out[9] = out9;
}
