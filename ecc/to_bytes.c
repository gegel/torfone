#include "scalarmult.h"

void to_bytes(unsigned char *dst, int32_t const *src) {
    int idx;
    int32_t q;
    int32_t c;

    int64_t tmp[10];

    for (idx = 0; idx < 10; ++idx)
        tmp[idx] = src[idx];

    // Make sure the result is reduced mod p
    // This format most closely resembles the assembler version
    q = 19 * tmp[9];
    q = tmp[0] + (q >> 24);
    q = tmp[1] + (q >> 26);
    q = tmp[2] + (q >> 26);
    q = tmp[3] + (q >> 25);
    q = tmp[4] + (q >> 26);
    q = tmp[5] + (q >> 25);
    q = tmp[6] + (q >> 26);
    q = tmp[7] + (q >> 26);
    q = tmp[8] + (q >> 25);
    q = tmp[9] + (q >> 26);
    tmp[0] += 19 * (q >> 24);

    // Compress the result into 25.5 bit limbs

    c = tmp[0] >> 26; tmp[1] += c; tmp[0] -= c << 26;
    c = tmp[1] >> 26; tmp[2] += c; tmp[1] -= c << 26;
    c = tmp[2] >> 25; tmp[3] += c; tmp[2] -= c << 25;
    c = tmp[3] >> 26; tmp[4] += c; tmp[3] -= c << 26;
    c = tmp[4] >> 25; tmp[5] += c; tmp[4] -= c << 25;
    c = tmp[5] >> 26; tmp[6] += c; tmp[5] -= c << 26;
    c = tmp[6] >> 26; tmp[7] += c; tmp[6] -= c << 26;
    c = tmp[7] >> 25; tmp[8] += c; tmp[7] -= c << 25;
    c = tmp[8] >> 26; tmp[9] += c; tmp[8] -= c << 26;
    c = tmp[9] >> 24; tmp[9] -= c << 24;

    // Now transform the data into byte arrays
    dst[0] = tmp[0] & 255;
    dst[1] = (tmp[0] >> 8) & 255;
    dst[2] = (tmp[0] >> 16) & 255;
    dst[3] = (tmp[0] >> 24) & 3;

    dst[3] |= (tmp[1] & 63) << 2;
    dst[4] = (tmp[1] >> 6) & 255;
    dst[5] = (tmp[1] >> 14) & 255;
    dst[6] = (tmp[1] >> 22) & 15;

    dst[6] |= (tmp[2] & 15) << 4;
    dst[7] = (tmp[2] >> 4) & 255;
    dst[8] = (tmp[2] >> 12) & 255;
    dst[9] = (tmp[2] >> 20) & 31;

    dst[9] |= (tmp[3] & 7) << 5;
    dst[10] = (tmp[3] >> 3) & 255;
    dst[11] = (tmp[3] >> 11) & 255;
    dst[12] = (tmp[3] >> 19) & 127;

    dst[12] |= (tmp[4] & 1) << 7;
    dst[13] = (tmp[4] >> 1) & 255;
    dst[14] = (tmp[4] >> 9) & 255;
    dst[15] = (tmp[4] >> 17) & 255;


    dst[16] = tmp[5] & 255;
    dst[17] = (tmp[5] >> 8) & 255;
    dst[18] = (tmp[5] >> 16) & 255;
    dst[19] = (tmp[5] >> 24) & 3;

    dst[19] |= (tmp[6] & 63) << 2;
    dst[20] = (tmp[6] >> 6) & 255;
    dst[21] = (tmp[6] >> 14) & 255;
    dst[22] = (tmp[6] >> 22) & 15;

    dst[22] |= (tmp[7] & 15) << 4;
    dst[23] = (tmp[7] >> 4) & 255;
    dst[24] = (tmp[7] >> 12) & 255;
    dst[25] = (tmp[7] >> 20) & 31;

    dst[25] |= (tmp[8] & 7) << 5;
    dst[26] = (tmp[8] >> 3) & 255;
    dst[27] = (tmp[8] >> 11) & 255;
    dst[28] = (tmp[8] >> 19) & 127;

    dst[28] |= (tmp[9] & 1) << 7;
    dst[29] = (tmp[9] >> 1) & 255;
    dst[30] = (tmp[9] >> 9) & 255;
    dst[31] = (tmp[9] >> 17) & 255;
}
