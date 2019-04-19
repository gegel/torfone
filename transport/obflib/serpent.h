

#ifndef SERPENT_H
#define SERPENT_H


// ** Thread-safe implementation

// ** Serpent cipher
// ** 128bit block size
// ** 256bit key

#define KEYINTSIZE 140

extern	void Serpent_set_key(unsigned int *l_key,const unsigned int *in_key, const unsigned int key_len);
extern	void Serpent_encrypt(const unsigned int *l_key,const unsigned int *in_blk, unsigned int *out_blk);
extern	void Serpent_decrypt(const unsigned int *l_key,const unsigned int *in_blk, unsigned int *out_blk);
#endif
