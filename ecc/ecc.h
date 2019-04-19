#ifndef SCALARMULT__
#define SCALARMULT__


#define crypto_scalarmult_curve25519 scalarmult
#define elligator2_isrt r2p
#define elligator2_gen p2r


//Diffie-Hellmann
void scalarmult(unsigned char *q, const unsigned char *n, const unsigned char *p);
//public key from private key
void scalarmultbase(unsigned char *p, const unsigned char *n);
//public key p from id n
void r2p(unsigned char *p, const unsigned char *n);
//representation r from random x
void p2r(unsigned char *r, unsigned char* x);


#endif