//shake128 with Keccak800
//non-standart but more suitable for 32 bits architecture

#include "KeccakP800.h"
#include "shake.h"


sha3_ctx_t sh3; //statically defined context


//Initialize the context
void sh_ini(void)
{
    KeccakP800_Initialize(sh3.st.q);
    sh3.mdlen = MDLEN;
    sh3.rsiz = STLEN - 2 * MDLEN;
    sh3.pt = 0;
}

//update state with more data
void sh_upd(const void *data, short len)
{
    short i;
    short j;

    j = sh3.pt;
    for (i = 0; i < len; i++) {
        sh3.st.b[j++] ^= ((const uint8_t *) data)[i];
        if (j >= sh3.rsiz) {
            KeccakP800_Permute_22rounds(sh3.st.q);
            j = 0;
        }
    }
    sh3.pt = j;
}


//extensible-output functionality
void sh_xof(void)
{   //shake128 compatible padding 
    sh3.st.b[sh3.pt] ^= 0x1F;
    sh3.st.b[sh3.rsiz - 1] ^= 0x80;
    KeccakP800_Permute_22rounds(sh3.st.q);
    sh3.pt = 0;
}

//output
void sh_out(void *out, short len)
{
    short i;
    short j;

    j = sh3.pt;
    for (i = 0; i < len; i++) {
        if (j >= sh3.rsiz) {
            KeccakP800_Permute_22rounds(sh3.st.q);
            j = 0;
        }
        ((uint8_t *) out)[i] = sh3.st.b[j++];
    }
    sh3.pt = j;
}

//crypting
void sh_crp(void *out, short len)
{
    short i;
    short j;

    j = sh3.pt;
    for (i = 0; i < len; i++) {
        if (j >= sh3.rsiz) {
            KeccakP800_Permute_22rounds(sh3.st.q);
            j = 0;
        }
        ((uint8_t *) out)[i] ^= sh3.st.b[j++];  //xor output with data
    }
    sh3.pt = j;
}

//safe clear the state
void sh_clr(void)
{
 short n=sizeof(sh3);
 volatile unsigned char *p = (unsigned char*)&sh3;
 while( n-- ) *p++ = 0;
}


