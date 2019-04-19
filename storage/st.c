#include <string.h>
#include "if.h"

#include "curve25519donna.h"
#include "chacha20.h"

#include "trng.h"

#include "book.h"
#include "st.h"
#include "st_if.h"


//data of Storage module
st_data st;

//Helpers
void st_memclr( void *v, size_t n );
short st_decrypt(unsigned char* data);
void st_encrypt(unsigned char* data);

//===========================INIT===========================

//set book path, reset state, init TRNG in contact, return rand or zero(fail)
short st_storage_init(unsigned char* data)
{
 unsigned char* path = data+4;
 data[0]=CR_RND; //output type: rand[16]

 st_memclr(&st, sizeof(st)); //clear all storage data

 if(!st_test()) return 0; //check crypto is valid

 //set book path
 if(path[0]<'0') strcpy((char*)path, (char*)"bk"); //set default path
 
 bk_set_path((char*)path);
 st_memclr(path, 32);

 //get true random using system rand and Havage
 st_memclr(st.contact, 128); //clear contact data
 st.current_field=0;
 while(!st.current_field) st.current_field=getSid(st.contact, 128); //get true random

 if(!st.current_field)
 {
  st_memclr(path, 32);
  data[1]=STORAGE_TRNG_ERR;
  return CR_RND_LEN; //0 if TRNG fail
 }

  //generate random from TRNG
 chacha20(st.contact, st.contact); //permute first half of entropy
 chacha20(st.contact, st.contact); //permute first half of entropy
 chacha20(st.contact+64, st.contact+64); //permute second half of entropy
 chacha20(st.contact+64, st.contact+64); //permute second half of entropy
 chacha20(st.contact+32, st.contact+32); //permute both
 memcpy(st.stseed, st.contact+64, 16);  //output PRNG sid
 st_rand(st.secret, 32); //output material for secret key
 st_rand(path,32);//output sid for cryptor
 

 st_memclr(st.contact, 128); //clear contact data
 st.last_field=0; //must be 0: password not aplied yet, secret have random!
 data[1]=STORAGE_OK;

 return CR_RND_LEN; //0 if TRNG fail
}


//set psw  book path MUST be already initialized!!!
//input: psw for key and book, separated by space, return our pubA or zero(fail)
//seed PRNG, load keyfile, derive book-key and private key with password, output public key 
short st_set_password(unsigned char* data)
{
 #define ST_SALT_BOOK (char*)"$1$TF_saltadbook"
 #define ST_SALT_KEY (char*)"$1$TF_saltsecret"
 #define PKDFCNT 256   //number of hashing of password for key deriving
 
 short i;
 unsigned char blk[64];

 char* bstr=(char*)st.contact; //pointer to book password (after first space)
 unsigned char* psw = data+4;
 data[0]=CR_PUB;

 strncpy((char*)st.contact, (char*)psw, 32);
 st_memclr(psw, 32);
 
 //check storage already inited and psw not aplied yet
 if((!st.current_field)||(st.last_field))
 {
  data[1]=STORAGE_UNINIT;
  return CR_PUB_LEN;
 }

 //get book volume
 st.last_field=bk_search_last();
 if(!st.last_field)
 {
  data[1]=STORAGE_NOBOOK;
  return CR_PUB_LEN; //check book is OK
 }

 //load secret from file
 i=bk_load_secret(st.secret);
 if(!i)
 {
  data[1]=STORAGE_NOSECRET;
  return CR_PUB_LEN;  //return failure if secret file error
 }

 //separate key and book passwords
 bstr=strchr((char*)st.contact, ' '); //search space delimiter cryptor password from book password
 if(bstr) (*bstr++)=0; else bstr=(char*)st.contact;  //if two words then set pointer to book password

 //produce book secret keys
 memset(blk, 0, sizeof(blk)); //clear block
 strcpy((char*)blk, ST_SALT_BOOK); //set book salt
 memcpy(blk+16, st.secret+16, 16); //set second half of secret from file
 strncpy((char*)blk+32, bstr, 32); //set user password for book
 for(i=0;i<PKDFCNT;i++) chacha20(blk, blk);  //multiple permute
 bk_set_pws(blk); //set book masking key
 memcpy(st.book_psw, blk+16, 16);  //set book authenting key

 //produce X25519 private key
 memset(blk, 0, sizeof(blk)); //clear block
 strcpy((char*)blk, ST_SALT_KEY); //set key salt
 memcpy(blk+16, st.secret, 16); //set first half of secret from file
 strncpy((char*)blk+32, (char*)st.contact, 32); //set user password for key
 for(i=0;i<PKDFCNT;i++) chacha20(blk, blk); //multiple permute
 memcpy(st.secret, blk, 32); //set our X25519 private key
 cf_curve25519_mul_base(psw, st.secret); //compute our public key
 st_memclr(blk, sizeof(blk)); //clear data

 st_reset(0);
 data[1]=STORAGE_OK;
 return CR_PUB_LEN;
}

//reset storage module for new session
short st_reset(unsigned char* data)
{
 short i;

 st_memclr(st.address, 64);
 st.current_field=0; //clear current contact
 if(data) //remote reset from cryptor
 {
   //get entropy from TRNG
  memset(st.contact, 0, sizeof(st.contact)); //clear entropy buffer
  for(i=0;i<32;i++) //try 32 attempts
  {  //collect 128 bytes of enctropy
   if( getSid(st.contact, 128)) break; //up to acceptable statistic
  }
  //reseed PRNG
  chacha20(st.contact, st.contact); //permute first half of entropy
  chacha20(st.contact+64, st.contact+64); //permute second half of entropy
  chacha20(st.contact+32, st.contact+32); //permute both (skip a half)
  memcpy(st.contact+32, data+4, 32); //add seed from CR
  chacha20(st.contact+32, st.contact+32); //permute
  memcpy( st.contact+32, st.contact+64, 32); //replace (skip) first part with second part
  chacha20(st.contact+32, st.contact+32); //permute once more
  for(i=0;i<16;i++) st.stseed[i]^=st.contact[i+64]; //update PRNG sid
  st_memclr(st.contact, 128); //clear entropy buffer

  //send reset to UI
  st_rand(data+4,32);//output sid for UI
  memset(data, 0, 4);
  data[0]=UI_RST; //pass reset to UI module
 }
 return 36;
}


//====================KEY PROCESSING=========================

//process their efemaral public key with our private key
//returns hashed result
short st_get_sec(unsigned char* data)
{
 unsigned char pub[32];
 unsigned char* key = data+4;

 data[0]=CR_SEC;


 if(!st.last_field)
 {
  st_rand(key, 32); //setup random
  data[1]=STORAGE_UNINIT;
  return CR_SEC_LEN;
 }

 cf_curve25519_mul(pub,st.secret,key);//Y*x
 memcpy(key, pub, 32); //output Y*x
 
 data[1]=STORAGE_OK;
 return CR_SEC_LEN;
}

//=================CONTACTS PROCESSING===========================

//search_by_name
//input: name, return key or random
//search by name
//check tag after

short st_search_by_name(unsigned char* data)
{
 short i;
 unsigned char* name = data+4;
 data[0]=CR_KEY;



 if(!st.last_field)
 {
  st_rand(name, 32); //set random value
  data[1]=STORAGE_UNINIT;
  return CR_KEY_LEN;
 }

 if(name[0]=='*')  //check request myself pub key
 {
  //output our public key
  cf_curve25519_mul_base(name, st.secret); //compute our public key
  data[1]=STORAGE_OK;
  return CR_KEY_LEN;
 }


 if(name[0]) i=bk_search_by_name((char*)name); //search contact by name
 else i=0;
 st_memclr(name, 32);  //clear output

 if(!i)
 {
  st_rand(name, 32); //set random value
  data[1]=STORAGE_NOTFIND;
  return CR_KEY_LEN;
 }

 i=bk_read_field(i, st.contact);   //read contact data
 if(!i)
 {
  st_rand(name, 32); //set random value
  data[1]=STORAGE_RDFAIL;
  return CR_KEY_LEN;
 }

 i=st_decrypt(st.contact);  //decrypt contact data
 if(!i)
 {
  st_rand(name, 32); //set random value
  data[1]= STORAGE_MACFAIL;
  return CR_KEY_LEN;
 }

 memcpy(name, st.contact+16, 32); //output key or zero
 data[1]=STORAGE_OK;
 return CR_KEY_LEN;
}

//search_by_key
//input: key + sas at[2-3], return name or empty
//pass data[1-3] unchanged
short st_search_by_key(unsigned char* data)
{
 short i;
 unsigned char* key = data+4;
 data[0]=UI_NAME;

 if(data[1]) //authentication error  FE or FF
 {
  st_memclr(key, 32);
  return UI_NAME_LEN;  //resend error code to UI
 }


 if(!st.last_field)
 {
  st_memclr(key, 32);
  data[1]=STORAGE_UNINIT;
  return UI_NAME_LEN;
 }

 i=bk_search_by_key(key);
 st_memclr(key, 32);  //clear output
 if(!i)
 {
  data[1]=STORAGE_NOTFIND;
  return UI_NAME_LEN;
 }

 i=bk_read_field(i, st.contact);
 if(!i)
 {
  data[1]=STORAGE_RDFAIL;
  return UI_NAME_LEN;
 }

 i=st_decrypt(st.contact);
 if(!i)
 {
  data[1]=STORAGE_MACFAIL;
  return UI_NAME_LEN;
 }

 memcpy(key, st.contact, 16); //output name
 data[1]=STORAGE_OK;
 return UI_NAME_LEN;
}

//read_addr_by_name
//input: name, return addr or empty
//search by name
//check tag
//read name
//read addr
//decrypt
short st_get_addr_by_name(unsigned char* data)
{
 int i;
 int fld;
 unsigned char* name = data+4;
 data[0]=UI_ADR;

 if(!st.last_field)
 {
  st_memclr(name, 32);
  data[1]=STORAGE_UNINIT;
  return UI_ADR_LEN;
 }

 st_reset(0); //reset storage state (clear current field)
 if(!name[0])
 {
  st_memclr(name, 32);
  data[1]=STORAGE_NONAME;
  return UI_ADR_LEN;
 }


 if(name[0]=='*') //check request myself address
 {
  st_memclr(name, 32);
  data[1]=STORAGE_MYSELF;
  return UI_ADR_LEN;  //return * for mark myself
 }

 fld=bk_search_by_name((char*)name);  //search contact by name
 st_memclr(name, 32);  //clear output

 if(!fld)
 {
  st_memclr(name, 32);
  data[1]=STORAGE_NOTFIND;
  return UI_ADR_LEN;
 }

 i=bk_read_field(fld, st.contact); //read contact data
 if(!i)
 {
  st_memclr(name, 32);
  data[1]=STORAGE_RDFAIL;
  return UI_ADR_LEN;
 }

 i=st_decrypt(st.contact); //decrypt contact data
 if(!i)
 {
  st_memclr(name, 32);
  data[1]= STORAGE_MACFAIL;
  return UI_ADR_LEN;
 }

 memcpy(name, st.contact+48, 32);   //output address
 memcpy(st.address, st.contact+48, 32);  //set current address
 st.current_field=fld; //set current contact

 data[1]=STORAGE_OK;
 return UI_ADR_LEN;
}


//read_next_name
//input: nothing, return name or empty
//increment current
//read name
//check tag ?
short st_get_next_name(unsigned char* data)
{
 short i;
 unsigned char* name = data+4;
 char mask[16]={0};
 
 if(!st.last_field)
 {
  st_memclr(name, 32);
  data[1]=STORAGE_UNINIT;
  return UI_LST_LEN;
 }

 if(data[3]) st.current_field=0; //initialize list to start 
 strncpy(mask, (char*)name, sizeof(mask)); //store mask
 mask[15]=0; //terminate mask string
 
 data[0]=UI_LST; //set answer packet type
 data[3]=0; //clear list init flag 
 name[0]=0;  //clear name
 while(1) //search loop
 {
  st.current_field++; //move current contact to next
  if(st.current_field>st.last_field) //check this is a last contact in book
  {
   st_reset(0);
   data[1]=STORAGE_NOTFIND;
   st_memclr(name, 32);
   break;
  }
  i=bk_read_field(st.current_field, st.contact);
  if(i<=0) continue;
  if(!st.contact[0]) continue;
  if(mask[0]) //check mask required
  {
   st.contact[15]=0;
   if(!strstr((char*)st.contact, mask)) continue;
  }
  memcpy(name, st.contact, 16);
  data[1]=STORAGE_OK;
  break;
 }
 return UI_LST_LEN;
}



//set new adress as a current adress
//input: adress[32]
//output OK[1]
//store adress int current field buffer for next 
short st_set_address(unsigned char* data)
{
 
 unsigned char* addr = data+4;
 data[0]=UI_ADROK;

 if(!st.last_field)
 {
  st_memclr(addr, 32);
  data[1]=STORAGE_UNINIT;
  return UI_ADROK_LEN;
 }

 memset(st.address, 0, sizeof(st.address));
 addr[31]=0;
 strncpy((char*)st.address, (char*)addr, 32);

 memset(addr, 0, 32);
 memcpy(addr,st.contact,16); //output current contact name
 
 data[1]=STORAGE_OK;
 return UI_ADROK_LEN;
}



//save_key
//input: key, sas at [2-3] return nothing
//search empty, compose new name, save to empty
//update tag
short st_save_key(unsigned char* data)
{
 short i=0;
 unsigned short ss=256*data[2]+data[3]; //SAS
 unsigned int fp;
 short fld;
 unsigned char* key = data+4;
 data[0]=UI_KEY;


 if(!st.last_field)
 {
  st_memclr(key, 32);
  data[1]=STORAGE_UNINIT;
  return UI_KEY_LEN;
 }

 //check key is already exist
 i=bk_search_by_key(key);
 if(i>0)
 {
  st_memclr(key, 32);
  data[1]=STORAGE_EXIST; //already exist
  return UI_KEY_LEN; //return field number with same key
 }

 //create new contact for key
 //search first empty field in book
 fld=bk_search_empty();
 if(!fld)
 {
  st_memclr(key, 32);
  data[1]=STORAGE_NOSPACE;
  return UI_KEY_LEN;
 }
 if(fld>st.last_field) st.last_field=fld;

 //contact name is SAS+current key fingerprint
 fp=st_crc32(key, 32); //compute key fingerprint
 st.contact[0]='+'; //fingerprint symbol

 for(i=1;i<5;i++) //convert 2 bytes of ss to hex
 {
  st.contact[i]='0'+(ss&0x0F); //4 bits to hex digit
  if(st.contact[i]>'9') st.contact[i]+=7; //correct for A-F
  ss>>=4; //move to next 4 bits
 }
 st.contact[5]='-';
 for(i=6;i<14;i++) //convert 4 bytes of fp to hex
 {
  st.contact[i]='0'+(fp&0x0F); //4 bits to hex digit
  if(st.contact[i]>'9') st.contact[i]+=7; //correct for A-F
  fp>>=4; //move to next 4 bits
 }
 st.contact[14]='-';
 st.contact[15]=0; //terminate string

 //check this name is alredy exist
 i=bk_search_by_name((char*)st.contact); //check the fingerprint already exist
 if(i>0)
 {
  st_memclr(key, 32);
  data[1]=STORAGE_NONAME;
  return UI_KEY_LEN;  //abort
 }

 //save new contact
 memcpy(st.contact+16, key, 32);  //set key
 st_memclr(st.contact+48, 64); //clear address field
 if(st.address[0]) strncpy((char*)st.contact+48, (char*)st.address, 64); //copy stored adress

 st_memclr(key, 32);  //clear output
 memcpy(key,st.contact,16); //output new contact name
 
 st_encrypt(st.contact); //set new tag

 i=bk_save_field(fld, st.contact); //save contact
 if(!i)
 {
  data[1]=STORAGE_WRFAIL; //already exist
  return UI_KEY_LEN;
 }

 data[1]=STORAGE_CREATE;
 return UI_KEY_LEN;
}

 //add new contact by name
short st_new_contact(unsigned char* data)
{
  short i=0;
  short fld;
  unsigned char* name = data+4;
  data[0]=UI_NEWOK;

  if(!st.last_field)
  {
   st_memclr(data, 32);
   data[1]=STORAGE_UNINIT;
   return UI_SAVEOK_LEN;
  }

  st.current_field=0; //clear current field
  memset(st.address, 0, sizeof(st.address)); //clear adress;

  i=bk_search_by_name((char*)name); //check the name already exist
  if(i)
  {
   st_memclr(name, 32);
   data[1]=STORAGE_EXIST;
   return UI_SAVEOK_LEN;  //abort
  }

  fld=bk_search_empty();  //search first empty field in book
  if(!fld)
  {
   st_memclr(name, 32);
   data[1]=STORAGE_NOSPACE;
   return UI_SAVEOK_LEN;  //abort
  }
  if(fld>st.last_field) st.last_field=fld;

  memcpy(st.contact, name, 16);  //set name
  st_rand(st.contact+16, 32); //set random key
  st_memclr(st.contact+48, 64); //clear address
  //if(address[0]) memcpy(contact+48, address, 64);  //set addr
  st_encrypt(st.contact); //set new tag
  i=bk_save_field(fld, st.contact); //save contact
  st.current_field=i;               //set current field to created contact
  if(!i) data[1]=STORAGE_WRFAIL; //write fail
  else data[1]=STORAGE_CREATE;   //write OK
  return UI_SAVEOK_LEN;
}


//save_name_address
//input name, new name return nothing
//search contact by old name
//add adress and change name to newname
//or delete if no newname
short st_save_contact(unsigned char* data)
{
  short i=0;
  short fld;

  unsigned char* name = data+4;
  unsigned char* newname = data+20;

  data[0]=UI_SAVEOK;

  //check is ST inited
  if(!st.last_field)
  {
   st_memclr(name, 32);
   data[1]=STORAGE_UNINIT;
   return UI_SAVEOK_LEN;
  }


  if((!name[0])||(!newname[0])) //both old and new names must specified
  {
   data[1]=STORAGE_NOTFIND;
   st_memclr(name, 32);
   return UI_SAVEOK_LEN;
  }

  fld=bk_search_by_name((char*)name);   //search contact by name
  if(!fld)
  {
   data[1]=STORAGE_NOTFIND;
   st_memclr(name, 32);
   return UI_SAVEOK_LEN;
  }


  i=bk_read_field(fld, st.contact);  //read contact data
  if(!i)
  {
   st_memclr(name, 32);
   data[1]=STORAGE_RDFAIL; //already exist
   return UI_SAVEOK_LEN;
  }

  i=st_decrypt(st.contact);  //check tag
  if(!i)
  {
   st_memclr(name, 32);
   data[1]=STORAGE_MACFAIL; //already exist
   return UI_SAVEOK_LEN;
  }


  memset(st.contact, 0, 16);
  strncpy((char*)st.contact, (char*)newname, 16); //name is non-zero: change name
  if(st.address[0]) memcpy(st.contact+48, st.address, 64);  //optionally set addr
  st_encrypt(st.contact); //set new tag
  i=bk_save_field(fld, st.contact); //save contact
  if(!i) data[1]=STORAGE_WRFAIL; //update fail
  else data[1]=STORAGE_UPDATE; //update OK
  return UI_SAVEOK_LEN;

}


//delete contact by name
short st_delete_contact(unsigned char* data)
{
  short i=0;
  short fld;

  unsigned char* name = data+4;

  data[0]=UI_DELOK;

  //check is ST inited
  if(!st.last_field)
  {
   st_memclr(name, 32);
   data[1]=STORAGE_UNINIT;
   return UI_SAVEOK_LEN;
  }


  if(!name[0]) //name must specified
  {
   data[1]=STORAGE_NOTFIND;
   st_memclr(name, 32);
   return UI_SAVEOK_LEN;
  }

  fld=bk_search_by_name((char*)name);   //search contact by name
  if(!fld)
  {
   data[1]=STORAGE_NOTFIND;
   st_memclr(name, 32);
   return UI_SAVEOK_LEN;
  }

  i=bk_delete_field(fld);  //delete current field
  if(!i)
  {
   st_memclr(name, 32);
   data[1]=STORAGE_WRFAIL; //deleting fail
   return UI_SAVEOK_LEN;
  }

  st.current_field=0;  //clear current field
  data[1]=STORAGE_DELETE; //deleted OK
  return UI_SAVEOK_LEN;
}




//copy contact oldname to newname
//input name, new name return nothing
//search contact by old name
//search empty
//change name to newname
//save
short st_copy_contact(unsigned char* data)
{
  short i=0;
  short fld;

  unsigned char* name = data+4;
  unsigned char* newname = data+20;

  data[0]=UI_COPYOK;
 //------------------------check ST is init---------------------------
  //check is ST inited
  if(!st.last_field)
  {
   st_memclr(name, 32);
   data[1]=STORAGE_UNINIT;
   return UI_SAVEOK_LEN;
  }

  //----------------------search and read contact for coping--------------------
  if((!name[0])||(!newname[0])) //both old and new names must specified
  {
   data[1]=STORAGE_NOTFIND;
   st_memclr(name, 32);
   return UI_SAVEOK_LEN;
  }


  fld=bk_search_by_name((char*)name);   //search contact by name
  if(!fld)
  {
   data[1]=STORAGE_NOTFIND;
   st_memclr(name, 32);
   return UI_SAVEOK_LEN;
  }


  i=bk_read_field(fld, st.contact);  //read contact data
  if(!i)
  {
   st_memclr(name, 32);
   data[1]=STORAGE_RDFAIL; //already exist
   return UI_SAVEOK_LEN;
  }

  i=st_decrypt(st.contact);  //check tag
  if(!i)
  {
   st_memclr(name, 32);
   data[1]=STORAGE_MACFAIL; //already exist
   return UI_SAVEOK_LEN;
  }

  //----search emty and create new contact--------------------
  i=bk_search_by_name((char*)newname); //check the newname already exist
  if(i)
  {
   st_memclr(name, 32);
   data[1]=STORAGE_EXIST;
   return UI_SAVEOK_LEN;  //abort
  }

  fld=bk_search_empty();  //search first empty field in book
  if(!fld)
  {
   st_memclr(name, 32);
   data[1]=STORAGE_NOSPACE;
   return UI_SAVEOK_LEN;  //abort
  }
  if(fld>st.last_field) st.last_field=fld;

  //-------------replace name and save new contact-------------------
  strncpy((char*)st.contact, (char*)newname, 16); //name is non-zero: change name
  if(st.address[0]) memcpy(st.contact+48, st.address, 64);  //optionally set addr
  st_encrypt(st.contact); //set new tag
  i=bk_save_field(fld, st.contact); //save contact
  if(!i) data[1]=STORAGE_WRFAIL; //update fail
  else data[1]=STORAGE_COPY; //update OK
  return UI_SAVEOK_LEN;
}



//===================HELPERS====================
//encrypt and mac contact data
void st_encrypt(unsigned char* data)
{
 short i;
 unsigned char blk[64];

 memcpy(blk, data, 64); //use name, pubkey and 16 bytes of addr
 for(i=0;i<16;i++) blk[i]^=st.book_psw[i]; //apply password
 chacha20(blk, blk);  //permute
 memset(data+112, 0, 16); //clear MAC field
 for(i=0;i<64;i++) data[i+64]^=blk[i]; //mask 48 bytes of addr and produce 16 bytes of mac
 st_memclr(blk, sizeof(blk)); //clear data
}

//decrypt and check contact data
short st_decrypt(unsigned char* data)
{
 short i;
 unsigned char blk[64];
 unsigned char mac=0; //result of checking MAC

 memcpy(blk, data, 64); //use name, pubkey and 16 bytes of addr
 for(i=0;i<16;i++) blk[i]^=st.book_psw[i]; //apply password
 chacha20(blk, blk);  //permute
 for(i=0;i<64;i++) data[i+64]^=blk[i]; //unmask 48 bytes of addr and check 16 bytes of mac
 for(i=0;i<16;i++) mac|=data[i+112]; //check mac
 mac=(!mac)-1;
 st_memclr(blk, sizeof(blk)); //clear data
 return (short)(!mac); //return 1 if tag OK, 0 if fail
}

//Clear data array
// Implementation that should never be optimized out by the compiler
void st_memclr( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

//output random up to 32 bytes
void st_rand(unsigned char* data, short len)
{
  unsigned char blk[64];

  if(len>32) len=32;  //restrict rand len is 32 bytes maximum
  memcpy(blk, st.stseed, 16); //fill block by current sid
  memcpy(blk+16, st.stseed, 16);
  memcpy(blk+32, st.stseed, 16);
  memcpy(blk+48, st.stseed, 16);
  chacha20(blk, blk); //permute
  memcpy(st.stseed, blk, 16); //update sid
  memcpy(data, blk+16, len); //output rand
  st_memclr(blk, sizeof(blk));  //clear data
}

//compact (but slow) crc32 for compute public key fingerprint
unsigned int st_crc32(unsigned char const *p, unsigned int len)
{
  #define CRCPOLY_LE 0xedb88320
  
  unsigned int i;
  unsigned int crc4=0xffffffff;
  while(len--) {
    crc4^=*p++;
    for(i=0; i<8; i++)
      crc4=(crc4&1) ? (crc4>>1)^CRCPOLY_LE : (crc4>>1);
    }
  crc4^=0xffffffff;
  return crc4;
}

//test of storage crypto
unsigned char st_test(void)
{
 #define TST_STVECTOR 0xCCD171AE
 unsigned char blk[64];
 unsigned char* sec=blk+32;
 unsigned char pub[32];
 short i;

 memset(blk, 0x55, sizeof(blk)); //set initial value
 for(i=0;i<16;i++)
 {
  chacha20(blk, blk); //permute: random x,y in blk
  cf_curve25519_mul_base(pub, blk); //compute point X from x
  cf_curve25519_mul(blk, sec, pub); //output X^y back to blk
 }
 return (short)(TST_STVECTOR==st_crc32(blk, sizeof(blk)));
}


//===================INTRFACE====================

//process data in ST
//input: data in st_buf[68], len
//output: answer in st_buf[68]
//returns len of answer
//process data in st_buf and send answer to st_if
short st_process(unsigned char* st_buf, short len)
{
 unsigned char type=st_buf[0]; //check data type
 short st_len=0; //answer len

 switch(type) //process inciming data by type
 {
  case(ST_PTH):{st_len=st_storage_init(st_buf);break;}
  case(ST_PSW):{st_len=st_set_password(st_buf);break;}
  case(ST_SEC):{st_len=st_get_sec(st_buf);break;}
  case(ST_REQ):{st_len=st_get_next_name(st_buf);break;}
  case(ST_GETADR):{st_len=st_get_addr_by_name(st_buf);break;}
  case(ST_GETKEY):{st_len=st_search_by_name(st_buf);break;}
  case(ST_GETNAME):{st_len=st_search_by_key(st_buf);break;}
  case(ST_ADR):{st_len=st_set_address(st_buf);break;}
  case(ST_NEW):{st_len=st_new_contact(st_buf);break;}
  case(ST_SAVE):{st_len=st_save_contact(st_buf);break;}
  case(ST_COPY):{st_len=st_copy_contact(st_buf);break;}
  case(ST_DEL):{st_len=st_delete_contact(st_buf);break;}
  case(ST_ADD):{st_len=st_save_key(st_buf);break;}
  case(ST_RST):{st_len=st_reset(st_buf);break;}
 }
 if(st_len) st_send(st_buf, st_len); //check answer len and send answer to st_if
 return st_len;
}


