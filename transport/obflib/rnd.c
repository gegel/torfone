//**************************************************************************
//ORFone project
//Transport layer encrypting and internal PRNG
//**************************************************************************


#include <stdio.h>
#include <string.h>

#include "serpent.h"
#include "rnd.h"

//number of serpent iterations for derive obfuscation keys from first data block
//this key will be used for traffic obfuscation before IKE compleet
//after common secret ready this keys will be updated and provide
//leyer of strong transport encrypting and authentification
#define HIDEKEYCNT 1024

rn_data rn; //internal data of PRNG module
char RN_DBG=0;  //debug flag

//*************************************************************************
//                               INTERFACE
//*************************************************************************

//create obfuscator and seed PRNG
void rn_create(unsigned char* datablock)
{
 int i;
 unsigned int* seed = (unsigned int*)datablock;
 
 //if(RN_DBG) printf("rn_setrnd\r\n");

 memset(&rn, 0, sizeof(rn)); //clear data structure
 rn_setbkey(rn.material); //set zero book key

 for(i=0;i<4;i++) rn.rndsid[i]^=seed[i];  //update sid

 rn.is_out_key=0;  //clear flags of having keys
 rn.is_in_key=0;
}


//reseed PRNG
void rn_setrnd(unsigned char* datablock)
{
 unsigned int* seed = (unsigned int*)datablock;
 int i;

 for(i=0;i<4;i++) rn.rndsid[i]^=seed[i]; //update seed
}

//--------------------------------------------------------------------
//get len PRNG bytes
//--------------------------------------------------------------------
 void rn_getrnd(unsigned char* datablock, int len)
 {
  unsigned int tbuf[4];
  int i;

  while(len>0) //loop
  {
   Serpent_encrypt(rn.book_key.rk, rn.rndsid, tbuf);  //key is not matter here
   for(i=0;i<4;i++) tbuf[i]^=rn.rndsid[i];   //one-way permute rn.rndsid to output
   if(len>16) i=16; else i=len;
   memcpy(datablock, tbuf, i);   //output 16 bytes or less (tail) random bytes
   datablock+=i; //move pointer to output block
   rn.rndsid[0]^=0xFFFFFFFF;  //change rn.rndsid
   Serpent_encrypt(rn.book_key.rk, rn.rndsid, tbuf);  //key is not matter here
   for(i=0;i<4;i++) //one-way permute rn.rndsid to himself
   {
    rn.rndsid[i]^=tbuf[i];
    tbuf[i]=0;  //clear temp datablock
   }
   len-=16; //count outputted bytes up to len
  }

 }

 //********************************************************************
 //Transportind data processing
 //********************************************************************

//--------------------------------------------------------------------
//clear session obfuscation keys
//--------------------------------------------------------------------
 void rn_rstkeys(void)
 {
  //if(RN_DBG) printf("rn_rstkeys\r\n");
  memset(rn.material, 0, 16); //clear material for new session
  ocb_init(&rn.out_key, rn.material); //set zero outgoing key
  ocb_init(&rn.in_key, rn.material); //set zero incoming key
  rn.is_out_key=0;  //clear flag is outgoing key ready
  rn.is_in_key=0;   //clear flag is incoming key ready
 }

//---------------------------------------------------------------------
//set shared keymaterial for session obfuscation
//---------------------------------------------------------------------
 void rn_setobf(unsigned char* datablock)
 {
  memcpy(rn.material, datablock, 16); //set obfuscation keymaterial
 }

 //--------------------------------------------------------------------
 //pkdf compute session obfuscation key from PRF datablock
 //this key will be used for obfuscation (not encrypting) before IKE compleet
 //--------------------------------------------------------------------
 void rn_setekey(unsigned char* datablock)
 {
  unsigned int tbuf[4];
  int i;

  memcpy(tbuf, rn.material, 16); //initial keymaterial for obfuscation
  Serpent_set_key(rn.out_key.rk, tbuf, 128);  //set pkdf encrypting key
  memcpy(tbuf, datablock, 16); //save plane first datablock
  //multy encrypt first datablock block with pkdf key
  for(i=0;i<HIDEKEYCNT;i++)  Serpent_encrypt(rn.out_key.rk, (unsigned int*)datablock, (unsigned int*)datablock);
  ocb_init(&rn.out_key, tbuf); //derive outgoing key from plane datablock
  memset(tbuf, 0, 16); //clear
  rn.is_out_key=1; //set flag outgoing obfuscation key is ready
 }

 //--------------------------------------------------------------------
 //pkdf compute session deobfuscation key from first datablock
 //this key will be used for deobfuscation (not decrypting) before IKE compleet
 //--------------------------------------------------------------------
 void rn_setdkey(unsigned char* datablock)
 {
  unsigned int tbuf[4]={0};
  int i;

  memcpy(tbuf, rn.material, 16); //initial keymaterial for deobfuscation
  Serpent_set_key(rn.in_key.rk, tbuf, 128); //set pkdf decrypting key
  //multy decrypt first datablock block with zero key
  for(i=0;i<HIDEKEYCNT;i++) Serpent_decrypt(rn.in_key.rk, (unsigned int*)datablock, (unsigned int*)datablock);
  ocb_init(&rn.in_key, datablock); //derive outgoing key from plane datablock is pkdf result
  rn.is_in_key=1; //set flag incoming deobfuscation key is ready
 }


 //--------------------------------------------------------------------
 //set common secret after IKE for further updating hide and unhide keys
 //--------------------------------------------------------------------
 void rn_set_secret(unsigned char* secret)
 {
  memcpy(rn.secret, secret, 16); //save material
 }

 //--------------------------------------------------------------------
 //update hide key using common secret
 //--------------------------------------------------------------------
 void rn_upd_ekey(void)
 {
  unsigned int tbuf[4];
  //if(RN_DBG) printf("rn_upd_ekey\r\n");

  //derive enc keymaterial from common secret
  Serpent_encrypt(rn.out_key.rk, (unsigned int*)(rn.secret), tbuf);
  ocb_init(&rn.out_key, tbuf); //set new outgoing key
 }

 //--------------------------------------------------------------------
 //update unhide key using common secret
 //--------------------------------------------------------------------
 void rn_upd_dkey(void)
 {
  unsigned int tbuf[4];
  //if(RN_DBG) printf("rn_updkeys\r\n");

  Serpent_encrypt(rn.in_key.rk, (unsigned int*)(rn.secret), tbuf); //derive dec keymaterial from common secret
  ocb_init(&rn.in_key, tbuf); //set new incoming key
 }


 //--------------------------------------------------------------------
 //this procedure calls from rn_encrypt procedure
 //obfuscate header (in first datablock) before sending to remote
 //input: 16 bytes datablock
 //output: encrypted datablock
 //--------------------------------------------------------------------
 void rn_hide(unsigned char* datablock)
 {
  int i;
  //if(RN_DBG) printf("rn_hide\r\n");

  if(rn.is_out_key) //check we already have obfuscation key for this session
  {
   for(i=0;i<16;i++) datablock[i]^=0xA5; //hide first datablock
   //first block contain one-way counter and can be ECB encrypted
   Serpent_encrypt(rn.out_key.rk, (unsigned int *)(datablock), (unsigned int *)(datablock));
  }
  else rn_setekey(datablock); //pkdf derive obfuscation key and obfuscate first datablock
 }


 //--------------------------------------------------------------------
 //encrypt len bytes in data using Serpent-128 OCB mode before sending to remote
 //incoming length must be up to 124 bytes and will be pads to at least 16 bytes
 //then data will be pads to a up to 9 full 12 bytes blocks except first 16 bytes block
 //and finally authentication tag with 4-11 bytes random length will be add
 //Note: the first data byte will be changed!
 //returns resulting packet length for send to remote (20-135 bytes)
 //--------------------------------------------------------------------
 int rn_encrypt(unsigned char* data, int len)
 {
  unsigned char flag; //control flag
  int npad=0;  //number of pad bytes will be add
  int tail;    //length of tail
  unsigned char nonce[16]; //plane first block[16] is nonce for OCB mode

  flag=len&0x80;   //obtain control flag from length
  len&=0x7F; //obtain incoming data length
  if(len>124) len=124; //restrict data length
  if(len<16) npad=16-len; //pad to full 16 bytes first block
  tail=(len+npad-4)%12; //obtain tail to full 12 bytes block except first block
  if(tail) npad+=(12-tail);  //pad to full 12 bytes block
  rn_getrnd(data+len, npad+1); //set random pad bytes + one for rand
  len+=npad; //set data length including pad bytes
  tail=4+(7&data[len]);  //obtain random number of tag bytes (4-11)
  data[0]=len-16+tail+flag; //set first byte is total data+tag lenght except first block (with control flag at MSB)
  memcpy(nonce, data, 16); //save nonce before first block will be ECB encrypted
  rn_hide(data); //ECB encrypt first datablock (with one-way counter)
  //OCB encrypt: key, nonce[16], indata, datalen, outdata, outtag, taglen
  ocb_encrypt(&rn.out_key, nonce, data+16, len-16, data+16, data+len, tail);
  len+=tail; //total length for transmitting
  return len;
 }


 //--------------------------------------------------------------------
 //this procedure calls before rn_decrypt procedure
 //deobfuscate header (in first datablock) after receiving from remote
 //input: 16 bytes encrypted datablock
 //output: decrypted datablock
 //--------------------------------------------------------------------
 int rn_unhide(unsigned char* datablock)
 {
  int len;
  int i;

  //if(RN_DBG) printf("rn_unhide: is_in_key=%d\r\n",rn.is_in_key);

  if(rn.is_in_key) //check decrypting key is already exist
  {
   Serpent_decrypt(rn.in_key.rk, (unsigned int *)datablock, (unsigned int *)datablock); //decrypt first block
   for(i=0;i<16;i++) datablock[i]^=0xA5; //unhide first datablock
  }
  else rn_setdkey(datablock); //pkdf set decrypting key from first datablock and unhide datablock

  len=0x7F&datablock[0]; //number of bytes must be received after this block
  if(len>119) len=-1; //check for bundaries failure
  //if(RN_DBG) printf("rn_unhide: reslen=%d\r\n",len);
  return len;
 }

  //--------------------------------------------------------------------
 //decrypt 20-135 bytes in data using Serpent-128 OCB mode after receiving tfrom remote
 //the first 16 bytes block must be decrypted before calling this procedure
 //this block is nonce will be unchanged in this procedure
 //returns resulting packet length with all pads but without tag (16-124 bytes)
 //or 0 if authentication is failure
 //--------------------------------------------------------------------
 int rn_decrypt(unsigned char* data, int len)
 {
  int tail; //variable length of authentication tag (4-11 bytes) 
  int i;

  //first 16 bytes block are alredy decrypted is nonce
  if(len<16) return 0; //must be at least one full block
  tail=(len-16)%12; //obtain tag length %12
  len=len-tail; //set data len without tag
  //OCB decrypt: key, nonce[16], indata, datalen, outdata, outtag, taglen
  i=ocb_decrypt(&rn.in_key, data, data+16, len-16, data+16, data+len, tail);
  len*=i; //set zero length if authentication failure
  return len; //returns data length without tag
 }

 //********************************************************************
 //Addressbook processing
 //********************************************************************

 //--------------------------------------------------------------------
 //set key for crypt databook addresses fields
 //--------------------------------------------------------------------
 unsigned short rn_setbkey(unsigned char* datablock)
 {
  unsigned int tbuf[4];
  //if(RN_DBG) printf("rn_setbkey\r\n");
  ocb_init(&rn.book_key, datablock); //init Serpent-128 OCB key from keymaterial
  memset(tbuf, 0x01, sizeof(tbuf)); //set temporary data
  Serpent_encrypt(rn.book_key.rk, tbuf, tbuf); //encrypt temporary data
  return (unsigned short)tbuf[0]; //returns keys fingerprint for check key by user
 }

 //--------------------------------------------------------------------
 //encrypt 60 bytes book address field in OCB mode
 //IV is 16 bytes book name field (it is unique for each entry)
 //last 4 bytes of data is authentication tag to be verified
 //--------------------------------------------------------------------
 void rn_encbook(unsigned char* iv, unsigned char* data)
 {
  //OCB encrypt with Serpent-128
  ocb_encrypt(&rn.book_key, iv, data, 60, data, data+60, 4);
 }


 //--------------------------------------------------------------------
 //decrypt 60 bytes book address field in OCB mode
 //IV is 16 bytes book name field (it is unique for each entry)
 //last 4 bytes of data is authentication tag
 //--------------------------------------------------------------------
 void rn_decbook(unsigned char* iv, unsigned char* data)
 {
   int i;
   //OCB decrypt with Serpent-128
   i=ocb_decrypt(&rn.book_key, iv, data, 60, data, data+60, 4);
   memset(data+60, 0, 4); //clear tag
   if(!i) memset(data, 0, 6); //clear data if authentication was wrong
 }

