
//#define USE_SYSTEM_SEED  //Use system random generator for initialisation SPRNG
#define USE_HAVEGE  //Use Havage entropy  for initialisation SPRNG



#ifdef USE_SYSTEM_SEED
 #ifdef _WIN32
   #include <sys/stat.h>
#include <sys/types.h>
//#include <winnt.h>
#include <stddef.h>
#include <stdlib.h>
#include <basetsd.h>
#include <stdint.h>
//#include <fixedint.h>
#include "basetsd.h"
//#include <windef.h>

#include <stdint.h>
//#include <fixedint.h>
#include <basetsd.h>




  #include <windows.h>
  #include <wincrypt.h>
 #else
  #include <stdio.h>
 #endif
 #endif

 

 
#ifdef USE_HAVEGE
 #include "havege.h"
 havege_state hv; 
#endif



#include "trng.h"


int getSid(unsigned char* sysrand, int len)
{

 int ret=0;
 int i, j;
 unsigned int r=0;
 unsigned short hi[16]={0};

 #ifdef USE_SYSTEM_SEED
  do {
  #ifdef _WIN32
    HCRYPTPROV prov;

    if (!CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))  {
        break;
    }

    if (!CryptGenRandom(prov, len, sysrand))  {
        CryptReleaseContext(prov, 0);
        break;
    }
    CryptReleaseContext(prov, 0);
    ret=1;
  #else
    FILE *f = fopen("/dev/urandom", "rb");

    if (f == NULL) {
        break;
    }

    fread(sysrand, 1, RNDLEN, f);
    fclose(f);
    ret=1;
  #endif
  } while(0);
 #endif
 
 
 //Use Havege
 #ifdef USE_HAVEGE
  havege_init(&hv);  //init havage

 for(i=0;i<len;i++)  //fill all bytes in keccak state
 {
  for(j=0;j<8;j++)  //fill 8 bits in each byte
  {
   sysrand[i]<<=1;  //shift byte
   havege_random(&hv, (unsigned char*)&r, 4); //get random
   if(r&1) sysrand[i]^=1;  //use only one LSB of random
  }
 }

  ret+=2;
 #endif

  //estimite randomnity of the sid
  for(i=0;i<len;i++)  //check probability of each tetrade value in random bytes
  {
   hi[sysrand[i]&0x0F]++;
   hi[sysrand[i]>>4]++;
  }

  //check probability of each value is not too small or too big
  for(i=0;i<16;i++)
  {
   if((hi[i]<(len/16))||(hi[i]>(len/4))) break;
  }
  if(i!=16) ret=0; //check all values are in range
  return ret;  //return 0 if TRNG error
 }