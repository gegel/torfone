#ifndef CHACHA20_H
#define CHACHA20_H

#define ROUNDS "20"
#define CHACHA_PERMUTATION chacha20_perm_asm

void chacha20_perm_asm(unsigned char* out, const unsigned char* in);

#include <stdint.h>

typedef uint32_t u32 ;
typedef unsigned char byte ;

void chacha20(unsigned char* blk, unsigned char* out);
void chacha20_wordtobyte4(byte output[64],const u32 input[16]);
void chacha20_wordtobyte(byte output[64],const u32 input[16]);
void chacha_keysetup(u32 *x,const byte *k,u32 kbits,u32 ivbits);
void chacha_ivsetup(u32 *x,const byte *iv);
void chacha_encrypt_bytes(u32 *x,const byte *m,byte *c,u32 bytes);
void chacha_keystream_bytes(u32 *x,byte *stream,u32 bytes);
#endif
