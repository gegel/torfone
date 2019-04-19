//**************************************************************************
//ORFone project
//OCB Cifer mode realization for transport module
//**************************************************************************


/*
 * ocb.c
 *
 * Author:  Ted Krovetz (tdk@acm.org)
 * History: 1 April 2000 - first release (TK) - version 0.9
 *
 * OCB-AES-n reference code based on NIST submission "OCB Mode"
 * (dated 1 April 2000), submitted by Phillip Rogaway, with
 * auxiliary submitters Mihir Bellare, John Black, and Ted Krovetz.
 *
 * This code is freely available, and may be modified as desired.
 * Please retain the authorship and change history.
 *
 * This code is NOT optimized for speed; it is only
 * designed to clarify the algorithm and to provide a point
 * of comparison for other implementations.
 *
 * Limitiations:  Assumes a 4-byte integer type and and pointers that are
 * 32-bit aligned. Acts on a byte string of at most 2^36-16 bytes.
 *
 * Rijndael source available at www.esat.kuleuven.ac.be/~rijmen/rijndael/
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "serpent.h"
#include "ocb.h"




#if (INT_MAX != 0x7fffffff)
#error -- Assumes 4-byte int
#endif

/* 
 * This implementation precomputes L(-1), L(0), L(1), L(PRE_COMP_BLOCKS),
 * where L(0) = L and L(-1) = L/x and L(i) = x*L(i) for i>0.  
 * Normally, one would select PRE_COMP_BLOCKS to be a small number
 * (like 0-6) and compute any larger L(i) values "on the fly", when they
 * are needed.  This saves space in _keystruct and needn't adversely
 * impact running time.  But in this implementation, to keep things as 
 * simple as possible, we compute all the L(i)-values we might ever see.
 */ 






/************************************************************************* 
 * xor_block  
 *************************************************************************/
static void
xor_block(void *dst, void *src1, void *src2)
/* 128-bit xor: *dst = *src1 xor *src2. Pointers must be 32-bit aligned  */
{
    ((unsigned int *)dst)[0] = ((unsigned int *)src1)[0] ^ ((unsigned int *)src2)[0];
    ((unsigned int *)dst)[1] = ((unsigned int *)src1)[1] ^ ((unsigned int *)src2)[1];
    ((unsigned int *)dst)[2] = ((unsigned int *)src1)[2] ^ ((unsigned int *)src2)[2];
    ((unsigned int *)dst)[3] = ((unsigned int *)src1)[3] ^ ((unsigned int *)src2)[3];
}


/************************************************************************* 
 * shift_left  
 *************************************************************************/
static void
shift_left(unsigned char *x)
/* 128-bit shift-left by 1 bit: *x <<= 1.                                */
{
    int i;
    for (i = 0; i < 15; i++) {
        x[i] = (x[i] << 1) | (x[i+1] & 0x80 ? 1 : 0);
    }
    x[15] = (x[15] << 1);
}

/************************************************************************* 
 * shift_right 
 *************************************************************************/
static void
shift_right(unsigned char *x)
/* 128-bit shift-right by 1 bit:  *x >>= 1                               */
{
    int i;
    for (i = 15; i > 0; i--) {
        x[i] = (x[i] >> 1) | (x[i-1] & 1 ? 0x80u : 0);
    }
    x[0] = (x[0] >> 1);
}

/************************************************************************* 
 * ntz 
 *************************************************************************/
static int
ntz(unsigned int i)
/* Count the number of trailing zeroes in integer i.                     */
{
#if (_MSC_VER && _M_IX86)  /* Only non-C sop */
    __asm bsf eax, i
#elif (__GNUC__ && __i386__)
    int rval;
    asm volatile("bsf %1, %0" : "=r" (rval) : "g" (i));
    return rval;    
#else
    int rval = 0;
    while ((i & 1) == 0) {
        i >>= 1;
        rval++;
    }
    return rval;
#endif
}

/************************************************************************* 
 * ocb_init
 *************************************************************************/
keystruct *                         /* Init'd keystruct or NULL      */
ocb_init(keystruct *key,           /* OCB key structure. NULL means */
            void   *enc_key)      /* key                       */
                                    /* Allocate/init new, non-NULL   */
                                    /* means init existing structure */
{
    unsigned char tmp[16] = {0,};
    unsigned int first_bit, last_bit, i;

    if (key == NULL)
        key = (keystruct *)malloc(sizeof(keystruct));
    if (key != NULL) {
        memset(key, 0, sizeof(keystruct));

        Serpent_set_key(key->rk,(unsigned int*)enc_key, 128);

        Serpent_encrypt(key->rk, (unsigned int*)tmp, (unsigned int*)tmp);


        for (i = 0; i <= PRE_COMP_BLOCKS; i++) {
            memcpy(key->L + i, tmp, 16);   /* Copy tmp to L[i] */
            first_bit = tmp[0] & 0x80u;    /* and multiply tmp by x */
            shift_left(tmp);
            if (first_bit) 
                tmp[15] ^= 0x87;
        }

        /* Precompute L_inv = L . x^{-1} */
        memcpy(tmp, key->L, 16);
        last_bit = tmp[15] & 0x01;
        shift_right(tmp);
        if (last_bit) {
            tmp[0] ^= 0x80;
            tmp[15] ^= 0x43;
        }
        memcpy(key->L_inv, tmp, 16);
    }
    
    return key;
}


/************************************************************************* 
 * ocb_pmac   -- move to a separate file when everything final
 *************************************************************************/
void
ocb_pmac(keystruct *key,    /* Initialized key struct           */
          void      *in,     /* Buffer for (incoming) message    */
          unsigned int in_len, /* Byte length of message           */
          void      *tag)    /* 16-byte buffer for generated tag */
{
    unsigned int i;                    /* Block counter                    */
    unsigned char  tmp[16];        /* temporary buffer                 */
    block          *in_blk;        /* Block-typed alias to in          */
    block Offset;                  /* Offset (Z[i]) for current block  */
    block checksum;                /* Checksum for computing tag       */

    /* 
     * Initializations
     */
    i = 1;                      /* Start with first block              */
    in_blk = (block *)in - 1;   /* Offset so in_blk[1] is first block. */
    memset(checksum, 0, 16);    /* Initlize the checksum and           */
    memset(Offset, 0, 16);      /*   current Offset to the zero block  */
    
    /*
     * Process blocks 1 .. m-1.   
     */
    while (in_len > 16) {

        /* Update Offset (Z[i] from Z[i-1]) */
        xor_block(Offset, key->L + ntz(i), Offset);   

        xor_block(tmp, Offset, in_blk + i); /* xor input block with Z[i] */
        Serpent_encrypt(key->rk, (unsigned int*)tmp, (unsigned int*)tmp);

        xor_block(checksum, checksum, tmp); /* Update checksum */

        in_len -= 16;                       /* and the loop variables */
        i++;
    }
    
    /*
     * Process block m
     */

    if (in_len == 16) {     /* full final block */
        xor_block(checksum, checksum, in_blk + i);  
        xor_block(checksum, checksum, key->L_inv);  
    } else {                /* short final block */
        memset(tmp, 0, 16);
        memcpy(tmp, in_blk + i, in_len);
        tmp[in_len] = 0x80;
        xor_block(checksum, checksum, tmp);
    }

    Serpent_encrypt(key->rk, (unsigned int*)checksum, (unsigned int*)tag);
}

/*************************************************************************
 * ocb_encrypt
 *************************************************************************/
void
ocb_encrypt(keystruct *key,    /* Initialized key struct           */
                void      *nonce,  /* 16-byte nonce                    */
                void      *pt,     /* Buffer for (incoming) plaintext  */
                unsigned int pt_len, /* Byte length of pt                */
                void      *ct,     /* Buffer for (outgoing) ciphertext */
                void      *tag,      /* Buffer for generated tag         */
                unsigned int tag_len)
{
    unsigned int i;                      /* Block counter                   */
    block tmp, tmp2;                 /* temporary buffers               */
    block *pt_blk, *ct_blk;          /* block-typed aliases for pt / ct */
    block Offset;                    /* Offset (Z[i]) for current block */
    block checksum;                  /* Checksum for computing tag      */

    /* 
     * Initializations
     */
    i = 1;                      /* Start with first block              */
    pt_blk = (block *)pt - 1;   /* These are adjusted so, for example, */
    ct_blk = (block *)ct - 1;   /* pt_blk[1] refers to the first block */
    memset(checksum, 0, 16);    /* Zero the checksum                   */

    /* Calculate R, aka Z[0] */
    xor_block(Offset, nonce, key->L);

    Serpent_encrypt(key->rk, (unsigned int*)Offset, (unsigned int*)Offset);
    /*
     * Process blocks 1 .. m-1
     */
    while (pt_len > 16) {

        /* Update the Offset (Z[i] from Z[i-1]) */
        xor_block(Offset, key->L + ntz(i), Offset);

        /* xor the plaintext block block with Z[i] */
        xor_block(tmp, Offset, pt_blk + i);

           /* Update checksum */
        xor_block(checksum, checksum, pt_blk + i);

        /* Encipher the block */
        Serpent_encrypt(key->rk, (unsigned int*)tmp, (unsigned int*)tmp);
        /* xor Z[i] again, writing result to ciphertext pointer */
        xor_block(ct_blk + i, Offset, tmp);
        
        /* Update checksum */
    //    xor_block(checksum, checksum, pt_blk + i);

        /* Update loop variables */
        pt_len -= 16;
        i++;
    }
    
    /*
     * Process block m
     */

    /* Update Offset (Z[m] from Z[m-1]) */
    xor_block(Offset, key->L + ntz(i), Offset);
    
    /* xor L . x^{-1} and Z[m] */
    xor_block(tmp, Offset, key->L_inv);

    /* Add in final block bit-length */
    tmp[15] ^= (pt_len << 3);

    Serpent_encrypt(key->rk, (unsigned int*)tmp, (unsigned int*)tmp);

    /* xor 'pt' with block-cipher output, copy valid bytes to 'ct' */
    memcpy(tmp2, pt_blk + i, pt_len);
    xor_block(tmp2, tmp2, tmp);


    /* Add to checksum the pt_len bytes of plaintext followed by */
    /* the last (16 - pt_len) bytes of block-cipher output */
    memcpy(tmp, pt_blk + i, pt_len);
    xor_block(checksum, checksum, tmp);

    
    memcpy(ct_blk + i, tmp2, pt_len);



    /*
     * Calculate tag
     */
    xor_block(checksum, checksum, Offset);
    Serpent_encrypt(key->rk, (unsigned int*)checksum, (unsigned int*)tmp);
    memcpy(tag, tmp, tag_len);
}


/************************************************************************* 
 * ocb_decrypt
 *************************************************************************/
int                                /* Returns 0 iff tag is incorrect   */
ocb_decrypt(keystruct *key,    /* Initialized key struct           */
                void      *nonce,  /* 16-byte nonce                    */
                void      *ct,     /* Buffer for (incoming) ciphertext */
                unsigned int  ct_len, /* Byte length of ct                */
                void      *pt,     /* Buffer for (outgoing) plaintext  */
                void      *tag,
                unsigned int tag_len)    /* Tag to be verified               */
{
    unsigned int i;                     /* Block counter                   */
    block tmp, tmp2;                /* temporary buffers               */
    block  *ct_blk, *pt_blk;        /* block-typed aliases for ct / pt */
    block Offset;                   /* Offset (Z[i]) for current block */
    block checksum;                 /* Checksum for computing tag      */

    /* 
     * Initializations
     */
    i = 1;                      /* Start with first block              */
    ct_blk = (block *)ct - 1;   /* These are adjusted so, for example, */
    pt_blk = (block *)pt - 1;   /* ct_blk[1] refers to the first block */

    /* Zero checksum */
    memset(checksum, 0, 16);

    /* Calculate R, aka Z[0] */
    xor_block(Offset, nonce, key->L);
    Serpent_encrypt(key->rk, (unsigned int*)Offset, (unsigned int*)Offset);
    /*
     * Process blocks 1 .. m-1
     */
    while (ct_len > 16) {

        /* Update Offset (Z[i] from Z[i-1]) */
        xor_block(Offset, key->L + ntz(i), Offset);

        /* xor ciphertext block with Z[i] */
        xor_block(tmp, Offset, ct_blk + i);
        
        /* Decipher the next block-cipher block */
        Serpent_decrypt(key->rk, (unsigned int*)tmp, (unsigned int*)tmp);
        /* xor Z[i] again, writing result to plaintext ponter */
        xor_block(pt_blk + i, Offset, tmp);
        
        /* Update checksum */
        xor_block(checksum, checksum, pt_blk + i);

        /* Update loop variables */
        ct_len -= 16;
        i++;
    }
    
    /*
     * Process block m
     */

    /* Update Offset (Z[m] from Z[m-1]) */
    xor_block(Offset, key->L + ntz(i), Offset);

    /* xor L . x^{-1} and Z[m] */
    xor_block(tmp, Offset, key->L_inv);

    /* Add in final block bit-length */
    tmp[15] ^= (ct_len << 3);

    Serpent_encrypt(key->rk, (unsigned int*)tmp, (unsigned int*)tmp);

    /* Form the final ciphertext block, C[m]  */
    memset(tmp2, 0, 16);
    memcpy(tmp2, ct_blk + i, ct_len);
    xor_block(tmp, tmp2, tmp);
    memcpy(pt_blk + i, tmp, ct_len);

    /* After the xor above, tmp will have ct_len bytes of plaintext  */
    /* then (16 - ct_len) block-cipher bytes, perfect for checksum.  */
    xor_block(checksum, checksum, tmp);

    /* 
     * Calculate tag
     */
    xor_block(checksum, checksum, Offset);
    Serpent_encrypt(key->rk, (unsigned int*)checksum, (unsigned int*)tmp);
    return (memcmp(tag, tmp, tag_len) == 0 ? 1 : 0);
}


/*************************************************************************
 * ecb_encrypt
 *************************************************************************/
void
ecb_encrypt(keystruct *key,    /* Initialized key struct           */
                void      *pt,     /* Buffer for (incoming) plaintext  */
                void      *ct)     /* Buffer for (outgoing) ciphertext */
{
 Serpent_encrypt(key->rk, (unsigned int*)pt, (unsigned int*)ct);
}


/*************************************************************************
 * ecb_encrypt
 *************************************************************************/
void
ecb_decrypt(keystruct *key,    /* Initialized key struct           */
                void      *ct,     /* Buffer for (incoming) ciphertext  */
                void      *pt)     /* Buffer for (outgoing) plaintext  */
{
 Serpent_decrypt(key->rk, (unsigned int*)ct, (unsigned int*)pt);
}



/************************************************************************* 
 * ocb_done 
 *************************************************************************/
keystruct *
ocb_done(keystruct *key)
{
    if (key) {
        memset(key, 0, sizeof(keystruct));
        free(key);
    }
    return NULL;
}

