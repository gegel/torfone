//**************************************************************************
//ORFone project
//Address book storage procedures for adressbook module
//This is OS version (supporting file system storage)
//**************************************************************************

#ifdef _WIN32 
 #include <stddef.h>
 #include <stdlib.h>
 #include <basetsd.h>
 #include <stdint.h>
 #include <windows.h>
#else //Linux

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chacha20.h"
#include "book.h"


#define BK_DBG 0
#define MAX_FIELDS 512


char book_path[512]={0}; //adress book filepath
int book_path_len=0;
unsigned short field_count=0;  //number of fields
unsigned char bk_psw[16]; //book password
unsigned char bk_blk[64]; //data block


void bk_memclr( void *v, size_t n );



//********************************************************************
//INTERFACE OF TORFONE ADRESSBOOK
//********************************************************************

//--------------------------------------------------------------------





//--------------------------------------------------------------------
//--------------------------------------------------------------------
//read data from field by specified index
//output: data[128]
//returns: index>0 or 0-not found
//--------------------------------------------------------------------
int bk_read_field(int field, unsigned char* data)
{
  FILE* fl=0;
  int i;
  unsigned char c=0;


  // if(BK_DBG) printf("bk_get_contact_info: field=%d\r\n", field);

  bk_memclr(data, 128);

  if(field<=0) return 0; //index must be specified
  if(field>field_count) return 0;
  field--; //index from 0

  if(!(fl = fopen(book_path, "rb" ))) //open book
  {
    //if(BK_DBG) printf("bk_get_contact_info: Book file not found!\r\n");
   return 0;
  }

  i=fseek(fl,field*128,SEEK_SET);  //set to data of requested contact
  if(i)
  {
   //if(BK_DBG) printf("bk_get_contact_info: fseek error\r\n");
   fclose(fl);
   return 0;
  }

  fread(data,1,128,fl);  //read data and first keymaterial
  for(i=0;i<16;i++) c|=data[i]; //check name field not empty
  if(c) bk_mask(data); //unmask contact
  fclose(fl); //close book
  return (field+1);
}

//--------------------------------------------------------------------
//save data to field by specified index
//input: data[128]
//returns: index>0 or 0-book failure
//--------------------------------------------------------------------
int bk_save_field(int field, unsigned char* data)
{
  FILE* fl=0;
  int i;

  // if(BK_DBG) printf("bk_rekey: field=%d\r\n", field);

  if(field<=0) return 0; //index must be specified
  if(field>field_count) return 0;
  field--; //index from 0

  if(!(fl = fopen(book_path, "r+b" ))) //open book
  {
   // if(BK_DBG) printf("bk_rekey: Book file not found!\r\n");
   return 0;
  }

  i=fseek(fl,field*128,SEEK_SET); //set pointer to second id
  if(i)
  {
   // if(BK_DBG) printf("bk_rekey: fseek error\r\n");
   fclose(fl);
   return 0;
  }
  bk_mask(data);
  fwrite(data,1,128,fl);
  fclose(fl);  //close book
  return (field+1);
}

//--------------------------------------------------------------------
//clear field by specified index in book
//in: index>0
//returns index>0 or 0 if not found
//--------------------------------------------------------------------
int bk_delete_field(int field)
{
  FILE* fl=0;
  int i;
  unsigned char data[128];

  // if(BK_DBG) printf("bk_delete_field: field=%d\r\n", field);

  if(field<=0) return 0;
  field--;
  bk_memclr(data, 128);


  if(!(fl = fopen(book_path, "r+b" ))) //open book
  {
   // if(BK_DBG) printf("bk_delete_field: Book file not found!\r\n");

    if(!(fl = fopen(book_path, "wb" ))) //open book
    {
     //if(BK_DBG) printf("bk_create_book: Book file create error\r\n");
     return 0;
    }
    fwrite(data,1,128,fl);
    fclose(fl);
    return 1;
  }
  
  i=fseek(fl,field*128,SEEK_SET);
  if(i)
  {
   //if(BK_DBG) printf("bk_delete_field: fseek error\r\n");
   fclose(fl);
   return 0;
  }

  fwrite(data,1,128,fl);
  fclose(fl);
  return (field+1);
}


//--------------------------------------------------------------------
//search index of last non-empty field, create book if not exist
//input: no
//returns: index>0 or 0- book creating failure
//--------------------------------------------------------------------
int bk_search_last(void)
{
 FILE* fl=0;
 int fields;


 if(!(fl = fopen(book_path, "rb" ))) //open book
 {
  fields=bk_delete_field(1);
 }
 else
 {
  fseek(fl,0,SEEK_END);
  fields=ftell(fl)/128;
  fclose(fl);
 }
 field_count=fields;
 return fields;
}


//--------------------------------------------------------------------
//search index of first field with empty name
//input: no
//returns: index>0 or 0-book full
//--------------------------------------------------------------------
int bk_search_empty(void)
{

   FILE* fl=0;
   int i,j;
   unsigned char data[128];
   unsigned char c;



   if(!(fl = fopen(book_path, "rb" ))) //open book
   {

    // if(BK_DBG) printf("bk_get_empty: Book file not found!\r\n");
    return 1;
   }

   //check for empty
   for(i=0;i<field_count;i++)
   {
    c=0;
    fseek(fl,i*128,SEEK_SET);
    fread(data,1,16,fl); //read name field
    for(j=0;j<16;j++) c^=data[j]; //check for empty
    if(!c) break; //find first empty filed
   }
   fclose(fl);
   bk_memclr(data, 128);
   if(i<field_count) return (i+1);
   if(field_count==MAX_FIELDS) return 0;

   field_count++;
   bk_delete_field(field_count);
   return field_count;
}





//--------------------------------------------------------------------
//search index of field contains specified name
//input: name[16]
//returns: index>0 or 0-not found
//--------------------------------------------------------------------
int bk_search_by_name(char* name)
{
 FILE* fl=0;
 int len, i;

 unsigned char ndata[16];
 unsigned char bdata[128];


 //if(BK_DBG) printf("bk_search_by_name %s\r\n", name);

 if(!field_count) return 0;
 memcpy(ndata, name, 16);
 ndata[15]=0; //safe copy name string
 len=strlen((char*)ndata); //length of name for comparing

  //if(BK_DBG) printf("bk_search_by_name %s (%d)\r\n", name, len);


  if(!(fl = fopen(book_path, "rb" ))) //open book
  {
    //if(BK_DBG) printf("bk_search_by_name: Book file not found!\r\n");
    bk_memclr(ndata, 16);
   return 0;
  }

 for(i=0;i<field_count;i++)   //compare all field with request
 {
  fseek(fl,i*128,SEEK_SET);  //set file pointer to name in next field
  fread(bdata,1,128,fl); //read name
  bk_mask(bdata); //deobfuscate
  if(!memcmp(bdata, ndata, len+1)) break; //matched including termination zero
 }
 
 fclose(fl); //close book
 bk_memclr(ndata, 16);
 bk_memclr(bdata, 16);
 if(i==field_count)
 {
  // if(BK_DBG) printf("bk_search_by_name: res=0\r\n");
  return 0;  //not found
 }
 else
 {
  //if(BK_DBG) printf("bk_search_by_name: res=%d\r\n", i+1);
  return (i+1);  //field number (from 1)
 }
}

//--------------------------------------------------------------------
//search index of field contains specified key
//input: key[32]
//returns: index>0 or 0-not found
//--------------------------------------------------------------------

int bk_search_by_key(unsigned char* key)
{
 FILE* fl=0;
 int i;
 int find=-1;

 unsigned char nm_uids[128];

  if(!field_count) return 0;

  if(!(fl = fopen(book_path, "rb" ))) //open book
  {
   // if(BK_DBG) printf("bk_search_by_id: Book file not found!\r\n");
   return 0;
  }

 for(i=0;i<field_count;i++) //compare all fields with id
 {
  fseek(fl,i*128,SEEK_SET); //set file pointer to name in next field
  fread(nm_uids,1,128,fl);  //read two ids
  bk_mask(nm_uids); //unmusk
  if(!memcmp(nm_uids+16, key, 32)) //key matches
  {
   find=i; //save finded field position
   if(nm_uids[0]!='=') break; //stop searching if non-alias is finded
  }
 }
 fclose(fl);  //close book
 bk_memclr(nm_uids, 16+32); //clear data
 find++; //index from 1
 return find; //0-not found, >0 is index
}

//--------------------------------------------------------------------
//optional set path to book folder (called from GUI)
void bk_init_path(char* path)
{
 strncpy(book_path, path, sizeof(book_path)-32); //set path to folder
 book_path_len=strlen(book_path); //set len of this path
}


 //set path to book file
int bk_set_path(char* path)
{  //add file name to folder's path
 strncpy(book_path+book_path_len, path, sizeof(book_path)-book_path_len-5);
 return 1;
}

//set password for unmask contacts in book file
void bk_set_pws(unsigned char* p)
{
 memcpy(bk_psw, p, 16);
}

//mask/unmask name and public key fields of contact in book file
void bk_mask(unsigned char* data)
{
 int i;

 memcpy(bk_blk, data+64, 48);   //use 48 bytes of masked address
 memcpy(bk_blk+48, bk_psw, 16); //use password
 chacha20(bk_blk, bk_blk);  //permute
 for(i=0;i<64;i++) data[i]^=bk_blk[i]; //mask name, pubkey and first 16 bytes of addr
}




//load secret file data
int bk_load_secret(unsigned char* data)
{
 int plen;
 FILE* fl=0;

 //set filename
 plen=strlen(book_path); //len of book file path
 strcpy(book_path+plen, (char*)".sec"); //add extension for secret file

 if(!(fl = fopen(book_path, "rb" ))) //open file for read
 {
   //file not found
   if(!(fl = fopen(book_path, "wb" ))) //create new file (open for write)
   { //creating fail (some IO error)
    book_path[plen]=0;  //cut extension
    return 0; //return on error
   }
   fwrite(data,1,32,fl);  //write initial data to file
   fclose(fl);  //close
    book_path[plen]=0;  //cut extension
   return 2; //return initial data with code 2: creates new
 }
 fread(data,1,32,fl);  //read data from file
 fclose(fl); //close file
 book_path[plen]=0;  //cut extension
 return 1;  //return secret data with code 1 (OK)
}

// Implementation that should never be optimized out by the compiler 
void bk_memclr( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}
