// sha3.h
// 19-Nov-11  Markku-Juhani O. Saarinen <mjos@iki.fi>

#ifndef SHAKE_H
#define SHAKE_H

#include <stddef.h>
#include <stdint.h>

#define STLEN 100 //state length in byte for Keccak800
#define MDLEN 16  //R length in bytes for Shake128





// state context
typedef struct {
    union {                                 // state:
        uint8_t b[100];                     // 8-bit bytes
        uint32_t q[25];                     // 32-bit words
    } st;
    short pt, rsiz, mdlen;                    // these don't overflow
} sha3_ctx_t;

//Interface
void sh_ini(void);
void sh_upd(const void *data, short len);
void sh_xof(void);
void sh_out(void *out, short len);
void sh_crp(void *out, short len);
void sh_clr(void);

#endif

