  #include "ocb.h"
  //#define KEYLENGTH 140

  #define CTRL_FLAG 0x80

  typedef struct {
   unsigned int rndsid[4];  //PRF sid
   keystruct out_key;
   keystruct in_key;
   keystruct book_key;
   unsigned char secret[16]; //common secret
   unsigned char material[16]; //key material
   char is_out_key;             //flag of session e_key was set
   char is_in_key;             //flag of session d_key was set
  } rn_data;

  void rn_create(unsigned char* datablock); //create obfuscator, seed PRNG
  void rn_setrnd(unsigned char* datablock); //seeding PRNG by 16 bytes
  void rn_getrnd(unsigned char* datablock, int len); //get 16 random bytes

  void rn_setobf(unsigned char* datablock); //set obfuscation keymaterial
  void rn_setekey(unsigned char* datablock); //set hide key from 16 bytes datablock
  void rn_setdkey(unsigned char* datablock); //set unhide key from 16 bytes
  void rn_updkeys(unsigned char* datablock); //update keys with common secret

  void rn_hide(unsigned char* datablock);  //pad datablock, hide first 16 bytes block, returns length+pad
  int rn_unhide(unsigned char* datablock); //unhide first 16 bytes block of datablock, returns followed tail length
  void rn_rstkeys(void);

  unsigned short rn_setbkey(unsigned char* datablock);
  void rn_encbook(unsigned char* iv, unsigned char* data);
  void rn_decbook(unsigned char* iv, unsigned char* data);

  void rn_set_secret(unsigned char* secret);
  void rn_upd_ekey(void);
  void rn_upd_dkey(void);

  int rn_encrypt(unsigned char* data, int len);
  int rn_decrypt(unsigned char* data, int len);
