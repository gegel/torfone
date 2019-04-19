#include <stdint.h>
#include "scalarmult.h"

void from_bytes(int32_t *dst, unsigned char const *src) {
    int32_t tmp0 = 0, tmp1 = 0, tmp2 = 0, tmp3 = 0, tmp4 = 0, 
            tmp5 = 0, tmp6 = 0, tmp7 = 0, tmp8 = 0, tmp9 = 0;

    tmp0 = src[0];
    tmp0 |= src[1] << 8;
    tmp0 |= src[2] << 16;
    tmp0 |= (src[3] & 3) << 24;

    tmp1 = src[3] >> 2;
    tmp1 |= src[4] << 6;
    tmp1 |= src[5] << 14;
    tmp1 |= (src[6] & 15) << 22;

    tmp2 = src[6] >> 4;
    tmp2 |= src[7] << 4;
    tmp2 |= src[8] << 12;
    tmp2 |= (src[9] & 31) << 20;

    tmp3 = src[9] >> 5;
    tmp3 |= src[10] << 3;
    tmp3 |= src[11] << 11;
    tmp3 |= (src[12] & 127) << 19;

    tmp4 = src[12] >> 7;
    tmp4 |= src[13] << 1;
    tmp4 |= src[14] << 9;
    tmp4 |= src[15] << 17;

    tmp5 = src[16];
    tmp5 |= src[17] << 8;
    tmp5 |= src[18] << 16;
    tmp5 |= (src[19] & 3) << 24;

    tmp6 = src[19] >> 2;
    tmp6 |= src[20] << 6;
    tmp6 |= src[21] << 14;
    tmp6 |= (src[22] & 15) << 22;

    tmp7 = src[22] >> 4;
    tmp7 |= src[23] << 4;
    tmp7 |= src[24] << 12;
    tmp7 |= (src[25] & 31) << 20;

    tmp8 = src[25] >> 5;
    tmp8 |= src[26] << 3;
    tmp8 |= src[27] << 11;
    tmp8 |= (src[28] & 127) << 19;

    tmp9 = src[28] >> 7;
    tmp9 |= src[29] << 1;
    tmp9 |= src[30] << 9;
    tmp9 |= (src[31] & 127) << 17;  // MUST mask top bit to zero

    dst[0] = tmp0;
    dst[1] = tmp1;
    dst[2] = tmp2;
    dst[3] = tmp3;
    dst[4] = tmp4;
    dst[5] = tmp5;
    dst[6] = tmp6;
    dst[7] = tmp7;
    dst[8] = tmp8;
    dst[9] = tmp9;
}
