/*
 * ocb.h
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
 * Note that OCB mode itself is patent pending.
 *
 * This code is NOT optimized for speed; it is only
 * designed to clarify the algorithm and to provide a point
 * of comparison for other implementations.
 *
 * Limitiations:  Assumes a 4-byte integer and pointers are
 * 32-bit aligned. Acts on a byte string of less than 2^{36} - 16 bytes.
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

#ifndef __OCB__H
#define __OCB__H

#include "serpent.h"

#define PRE_COMP_BLOCKS 2     /* Must be between 0 and 31 */

typedef unsigned char block[16];
/* Opaque forward declaration of key structure */ 
typedef struct {
    unsigned int rk[KEYINTSIZE];     //serpent key
    block L[PRE_COMP_BLOCKS+1];     /* Precomputed L(i) values, L[0] = L */
    block L_inv;                    /* Precomputed L/x value             */
} keystruct;

/*
 * "ocb_aes_init" optionally creates an ocb keystructure in memory
 * and then initializes it using the supplied "enc_key". "tag_len"
 * specifies the length of tags that will subsequently be generated
 * and verified. If "key" is NULL a new structure will be created, but
 * if "key" is non-NULL, then it is assumed that it points to a previously
 * allocated structure, and that structure is initialized. "ocb_aes_init"
 * returns a pointer to the initialized structure, or NULL if an error
 * occurred.
 */
keystruct *                         /* Init'd keystruct or NULL      */
ocb_init(keystruct *key,           /* OCB key structure. NULL means */
            void   *enc_key);      /* key                       */
                                    /* Allocate/init new, non-NULL   */
                                    /* means init existing structure */

/* "ocb_done deallocates a key structure and returns NULL */
keystruct *
ocb_done(keystruct *key);
                              
/*
 * "ocb_aes_encrypt takes a key structure, four buffers and a length
 * parameter as input. "pt_len" bytes that are pointed to by "pt" are
 * encrypted and written to the buffer pointed to by "ct". A tag of length
 * "tag_len" (set in ocb_aes_init) is written to the "tag" buffer. "nonce"
 * must be a 16-byte buffer which changes for each new message being
 * encrypted. "ocb_aes_encrypt" always returns a value of 1.
 */
void
ocb_encrypt(keystruct *key,    /* Initialized key struct           */
                void      *nonce,  /* 16-byte nonce                    */
                void      *pt,     /* Buffer for (incoming) plaintext  */
                unsigned int pt_len, /* Byte length of pt                */
                void      *ct,     /* Buffer for (outgoing) ciphertext */
                void      *tag,
                unsigned int tag_len);   /* Buffer for generated tag         */

                              
/*
 * "ocb_aes_decrypt takes a key structure, four buffers and a length
 * parameter as input. "ct_len" bytes that are pointed to by "ct" are
 * decrypted and written to the buffer pointed to by "pt". A tag of length
 * "tag_len" (set in ocb_aes_init) is read from the "tag" buffer. "nonce"
 * must be a 16-byte buffer which changes for each new message being
 * encrypted. "ocb_aes_decrypt" returns 0 if the supplied
 * tag is not correct for the supplied message, otherwise 1 is returned if
 * the tag is correct.
 */
int                                /* Returns 0 iff tag is incorrect   */
ocb_decrypt(keystruct *key,    /* Initialized key struct           */
                void      *nonce,  /* 16-byte nonce                    */
                void      *ct,     /* Buffer for (incoming) ciphertext */
                unsigned int ct_len, /* Byte length of ct                */
                void      *pt,     /* Buffer for (outgoing) plaintext  */
                void      *tag,
                unsigned int tag_len);   /* Tag to be verified               */
                     


  /*************************************************************************
 * ecb_encrypt
 *************************************************************************/
void
ecb_encrypt(keystruct *key,    /* Initialized key struct           */
                void      *pt,     /* Buffer for (incoming) plaintext  */
                void      *ct);     /* Buffer for (outgoing) ciphertext */


/*************************************************************************
 * ecb_encrypt
 *************************************************************************/
void
ecb_decrypt(keystruct *key,    /* Initialized key struct           */
                void      *ct,     /* Buffer for (incoming) ciphertext  */
                void      *pt);     /* Buffer for (outgoing) plaintext  */



void
ocb_pmac(keystruct *key,    /* Initialized key struct           */
          void      *in,     /* Buffer for (incoming) message    */
          unsigned int in_len, /* Byte length of message           */
          void      *tag);    /* 16-byte buffer for generated tag */

#endif /* __OCB__H */
