#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "shake.h"  
#include "ecc.h"
#include "if.h"
#include "cr.h"
#include "ike.h"
#include "pke.h"

extern ct_data* cc; //pointer to Cryptor's data

//=====================key sending===========================

//sender init key pass to remote
//calls from cr_st_b() during call (flag is cc->keystep)
//receive key from ST
//process and send to remote as TR_Q
//adress already in U
//sender compute Ta, store Ta in T, send Ta to receiver
//sending long Ta to remote
short key_run(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr
 cc->keystep=1; //set initial value
 data[0]=TR_K;  //send long Ta to remote

 if(data[1]!=STORAGE_OK) return 0;//check storage send key OK
 memcpy(cc->T, data+4, 32); //store contact's key

 sh_ini();
 sh_upd(cc->S, 16); //sender's Enc session key
 sh_upd(cc->T, 32); //contact key
 sh_xof();
 sh_crp(cc->U, 32); //Ta=ADR^H(Enc|KEY)

 memcpy(p, cc->U, 32); //sender output Ta
 return IKE_LEN;
}

//processing long Ta from remote
//receicer receive Ta, compute Tr
//store Ta in T, Tr in U, send Tr back to sender
short key_K(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr
 if((cc->keystep==2)||(cc->keystep==1)) return 0; //preventing double init


 cc->keystep=2;
 data[0]=TR_Q; //send Tr back to originator

 memcpy(cc->T, p, 32); //store Ta

 ike_rand(cc->U+16, 16); //get rand
 sh_ini();
 sh_upd(cc->U+16, 16); //Rnd
 sh_upd(cc->S, 16); //receiver's Enc session key
 sh_upd(cc->T, 32); //Ta
 sh_xof();
 sh_out(cc->U, 16); //Tr=H(Rnd|Enc|Ta) + Rnd

 memcpy(p, cc->U, 32); //receiver output Tr
 return IKE_LEN;
}

//processing short Tr from remote (originator in key send procedure)
//sender receive Tr, compute Tk, clear U and T
//send Tk to receiver
short key_Q1(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr
 unsigned char au_err=0;
 short i;


 cc->keystep=0;
 data[0]=TR_Q;

 sh_ini();
 sh_upd(p+16, 16); //Rnd
 sh_upd(cc->S+16, 16); //sender's Dec session key
 sh_upd(cc->U, 32); //Ta
 sh_xof();
 memcpy(cc->U, p, 16); //store R
 sh_crp(cc->U, 16); //Tr?=H(Rnd|Dec|Ta)

 for(i=0;i<16;i++) au_err|=cc->U[i]; //check Tr
 au_err=(!au_err)-1;  //0 is OK, 0xFF is fail

 sh_ini();
 sh_upd(p, 32); //R
 sh_upd(cc->S, 16); //sender's Enc session key
 sh_xof();
 sh_crp(cc->T, 32); //Tk=KEY^H(R|Enc)

 ike_rand(p, 32); //get random
 for(i=0;i<32;i++) p[i]=(p[i]&au_err)^cc->T[i]; //mask Tk if Tr is fail
 memset(cc->U, 0, 32);
 memset(cc->T, 0, 32); //clear data
 return IKE_LEN;
}

//processing short Tk from remote (acceptor in key send procedure)
//receiver receive Tk, compute KEY and ADDR
//save key in U, send ADDR to ST
//Ta in T, Tr in U
short key_Q2(unsigned char* data)
{
 short i,j;
 short ret=IKE_LEN;
 unsigned char* p=data+4; //in/out ptr


 cc->keystep=0;
 data[0]=ST_ADR;

 sh_ini();
 sh_upd(cc->U, 32); //R
 sh_upd(cc->S+16, 16); //receiver's Dec session key
 sh_xof();
 sh_crp(p, 32); //KEY=Tk^H(R|Dec)

 memcpy(cc->U, p, 32); //store KEY

 sh_ini();
 sh_upd(cc->S+16, 16); //receiver's Dec session key
 sh_upd(cc->U, 32); //KEY
 sh_xof();
 sh_crp(cc->T, 32); //ADR=Ta^H(Dec|KEY)

 memcpy(p, cc->T, 32); //output ADR
 memset(cc->T, 0, 32); //clear data

 //check ADR is valid?
 //this is not constant time because result is stop 
 for(i=0;i<32;i++) if(!p[i]) break; //adress len
 if(i==32) ret=0; //too long, must be up to 31 chars with terminating zero
 for(j=0;j<i;j++) if( (p[j]<46)||(p[j]>122) ) ret=0; //illegal chars
 if(!ret) //clear wrong data
 {
  memset(p, 0, 32);
  memset(cc->T, 0, 32);
 }
 return ret; //output only if received data valid
}

//======================SPEKE=========================

//initiate speke by user
short key_speke(unsigned char* data)
{
 //salt for derive X25519 point from password
 #define PKE_SALT (char*)"ORPhone_SPEKE_SALT"
 //number of PKDF iterations
 #define PKE_PKDF 1024

 unsigned char* p=data+4; //in/out ptr
 short i;

 ike_rand(cc->x, 32); //random to x: our private key
 p[31]=0; //truncate password string
 for(i=strlen((char*)p);i<31;i++) p[i]=0;  //zero pad password

 //compute X25519 point P[32] by password
 sh_ini();
 sh_upd(PKE_SALT, strlen(PKE_SALT)); //hash salt
 sh_upd(p, 32); //hash password
 for(i=0;i<PKE_PKDF; i++) sh_xof();  //PKDF
 sh_out(p, 32); //output r
 r2p(cc->C, p); //Hash2Point:  BasePoint BP
 scalarmult(p,cc->x, cc->C); //output ourQ=BP^x
 memcpy(cc->C, p, 32); //store our speke key for comparing

 cc->speke=1; //set SPEKE init flag
 data[0]=TR_S; //send Sq to remote
 return IKE_LEN;
}


//initiate speke by remote
//receive very long Sq
short key_S(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr
 if(cc->keystep==3) return 0; //preventing double remote init

 memcpy(cc->Y, p, 32); //store their Q
 cc->keystep=3; //expected Sm from remote
 data[0]=UI_SPREQ; //send remote SPEKE request to GUI
 if(!cc->speke) return 36; //user not init speke yet

 //if user already init speke
 speke(); //compute speke
 cc->speke=0; //clear SPEKE init flag
 memcpy(p, cc->C, 32); //output our autentificator
 data[0]=TR_Q;  //send Sm to remote
 return IKE_LEN;
}

//receive short Sm
//process speke authentificator
short key_Q3(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr
 unsigned char au_err=0; //speke result
 short i;

 if(cc->speke==1) speke();  //compute speke if don't yet

 //check theit authenticator is equal too expected, set au
 for(i=0;i<32;i++) au_err|=(p[i]^cc->x[i]); //0-OK
 au_err=(!au_err)-1;  //0 is OK, 0xFF is fail

 memcpy(p, cc->C, 32); //output our authenticator Se
 data[0]=UI_SPEKE; //send to ui (notify result and resend to remote)
 data[1]=au_err; //speke result (0-OK)
 data[2]=cc->speke; //need to send our autheticator to remote (1-send)

 memset(cc->x, 0, 32); //clear data
 memset(cc->Y, 0, 32);
 memset(cc->C, 0, 32);
 cc->keystep=0; //clear step: no expexted more
 cc->speke=0; //clear speke flag
 return IKE_LEN;
}

//compute SPEKE secret and derive authenticators
//input: our private in x, our public in C, their public in Y
//output: our authenticator in C, their expected authenticator in x
void speke(void)
{
  short i;
  unsigned char cp=0; //comparing value

  //check our and their public not the same and compute DH secret
  cp=ike_cmp(cc->C, cc->Y); //check is invalid their Q; the same as our Q
  ike_rand(cc->C, 32); //get random
  for(i=0;i<32;i++) cc->x[i]^=(cc->C[i]&cp); //mask our secret if their key invalid
  scalarmult(cc->C,cc->x,cc->Y);  //speke_secret = their_public ^ our_private

  //derive their expected authentificator
  sh_ini();
  sh_upd(cc->C, 32); //Secret
  sh_upd(cc->S+16, 16); //Dec
  sh_xof(); //hash
  sh_out(cc->x, 32); //Sd

  //derive our authenticator
  sh_ini();
  sh_upd(cc->C, 32); //Secret
  sh_upd(cc->S, 16); //Enc
  sh_xof(); //hash
  sh_out(cc->C, 32); //Se
}
