
#include "scalarmult.h"

void mul(int32_t *ab, int32_t const *a, int32_t const *b) {

    int64_t c;

    int64_t F0 = a[0];
    int64_t F1 = a[1];
    int64_t F2 = a[2];
    int64_t F3 = a[3];
    int64_t F4 = a[4];
    int64_t F5 = a[5];
    int64_t F6 = a[6];
    int64_t F7 = a[7];
    int64_t F8 = a[8];
    int64_t F9 = a[9];

    int64_t G0 = b[0];
    int64_t G1 = b[1];
    int64_t G2 = b[2];
    int64_t G3 = b[3];
    int64_t G4 = b[4];
    int64_t G5 = b[5];
    int64_t G6 = b[6];
    int64_t G7 = b[7];
    int64_t G8 = b[8];
    int64_t G9 = b[9];

    // Compute F0G0
    int64_t F0G00 = F0 * G0;
    int64_t F0G01 = F0 * G1 + F1 * G0;
    int64_t F0G02 = F0 * G2 + F1 * G1 + F2 * G0;
    int64_t F0G03 = (F1 * G2 + F2 * G1) * 2 + F3 * G0 + F0 * G3;
    int64_t F0G04 = F2 * G2 * 2 + F0 * G4 + F1 * G3 + F3 * G1 + F4 * G0;
    int64_t F0G05 = (F1 * G4 + F2 * G3 + F3 * G2 + F4 * G1) * 2;
    int64_t F0G06 = (F2 * G4 + F4 * G2) * 2 + F3 * G3;
    int64_t F0G07 = F3 * G4 + F4 * G3;
    int64_t F0G08 = F4 * G4 * 2;

    // Compute F1G1
    int64_t F1G10 = F5 * G5;
    int64_t F1G11 = F5 * G6 + F6 * G5;
    int64_t F1G12 = F5 * G7 + F6 * G6 + F7 * G5;
    int64_t F1G13 = (F6 * G7 + F7 * G6) * 2 + F8 * G5 + F5 * G8;
    int64_t F1G14 = F7 * G7 * 2 + F5 * G9 + F6 * G8 + F8 * G6 + F9 * G5;
    int64_t F1G15 = (F6 * G9 + F7 * G8 + F8 * G7 + F9 * G6) * 2;
    int64_t F1G16 = (F7 * G9 + F9 * G7) * 2 + F8 * G8;
    int64_t F1G17 = F8 * G9 + F9 * G8;
    int64_t F1G18 = F9 * G9 * 2;

    // Compute (F0 + F1)(G0 + G1)
    int64_t F0pF10 = F0 + F5;
    int64_t F0pF11 = F1 + F6;
    int64_t F0pF12 = F2 + F7;
    int64_t F0pF13 = F3 + F8;
    int64_t F0pF14 = F4 + F9;

    int64_t G0pG10 = G0 + G5;
    int64_t G0pG11 = G1 + G6;
    int64_t G0pG12 = G2 + G7;
    int64_t G0pG13 = G3 + G8;
    int64_t G0pG14 = G4 + G9;

    int64_t F0pF1G0pG10 = F0pF10 * G0pG10;
    int64_t F0pF1G0pG11 = F0pF10 * G0pG11 + F0pF11 * G0pG10;
    int64_t F0pF1G0pG12 = F0pF10 * G0pG12 + F0pF11 * G0pG11 + F0pF12 * G0pG10;
    int64_t F0pF1G0pG13 = (F0pF11 * G0pG12 + F0pF12 * G0pG11) * 2 + F0pF13 * G0pG10 + F0pF10 * G0pG13;
    int64_t F0pF1G0pG14 = F0pF12 * G0pG12 * 2 + F0pF10 * G0pG14 + F0pF11 * G0pG13 + F0pF13 * G0pG11 + F0pF14 * G0pG10;
    int64_t F0pF1G0pG15 = (F0pF11 * G0pG14 + F0pF12 * G0pG13 + F0pF13 * G0pG12 + F0pF14 * G0pG11) * 2;
    int64_t F0pF1G0pG16 = (F0pF12 * G0pG14 + F0pF14 * G0pG12) * 2 + F0pF13 * G0pG13;
    int64_t F0pF1G0pG17 = F0pF13 * G0pG14 + F0pF14 * G0pG13;
    int64_t F0pF1G0pG18 = F0pF14 * G0pG14 * 2;

    // 2.1
    int64_t out0 = F0G00 - 38 * F1G15;
    int64_t out1 = F0G01 - 38 * F1G16;
    int64_t out2 = F0G02 - 38 * F1G17;
    int64_t out3 = F0G03 - 38 * F1G18;
    int64_t out4 = F0G04;
    int64_t out5 = F0G05 - F1G10;
    int64_t out6 = F0G06 - F1G11;
    int64_t out7 = F0G07 - F1G12;
    int64_t out8 = F0G08 - F1G13;
    int64_t out9 = F1G14;

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
    out5 += F0pF1G0pG10;
    out6 += F0pF1G0pG11;
    out7 += F0pF1G0pG12;
    out8 += F0pF1G0pG13;
    out9 = F0pF1G0pG14 - out9;
    out10 -= F0pF1G0pG15;
    out11 -= F0pF1G0pG16;
    out12 -= F0pF1G0pG17;
    out13 -= F0pF1G0pG18;

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

    ab[0] = out0;
    ab[1] = out1;
    ab[2] = out2;
    ab[3] = out3;
    ab[4] = out4;
    ab[5] = out5;
    ab[6] = out6;
    ab[7] = out7;
    ab[8] = out8;
    ab[9] = out9;
}
