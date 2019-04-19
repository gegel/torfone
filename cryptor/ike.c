
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "trng.h"   //true random number generator
#include "shake.h"  //shake800-128
#include "ecc.h"
#include "audio.h"
#include "if.h"
#include "cr.h"
#include "cr_if.h"
#include "pke.h"
#include "ike.h"

ct_data ctaa;  //cryptor data
ct_data* cc;  //pointer to cryptor data

#ifdef IKE_TEST
////--------------------TEST!!!--------------------------------
ct_data ctbb;

void setid(unsigned char id)
{
 if(id) cc=&ctbb; else cc=&ctaa;
}


void testinit(void)
{
 unsigned char key[32];

 setid(1); //acceptor
 memset(cc->sid, 0x55, 16);
 ike_rand(cc->a, 32); //set acceptor long term private key
 scalarmultbase(cc->A, cc->a); //compute public key
 memcpy(key, cc->A, 32); //copy as originator destination

 setid(0); //originator
 memset(cc->sid, 0xAA, 16);
 ike_rand(cc->a, 32); //set originator long term private key
 scalarmultbase(cc->A, cc->a); //compute public key
 memcpy(cc->B, key, 32); //copy as originator destination
}



void stemu(unsigned char* data)
{
 unsigned char* p=data+4; //their efemeral Y
 unsigned char tag[32];

 scalarmult(tag, cc->a, p); //compute tag Ta=Y*a
 memcpy(p, tag, 32); //output tag
}


void ikeres(void)
{
 unsigned char sea[32];
 unsigned char sdb[32];
 unsigned char sda[32];
 unsigned char seb[32];
 unsigned short sasa;
 unsigned short sasb;

 setid(0);
 memcpy(sea, cc->S, 16);
 memcpy(sda, cc->S+16, 16);
 sasa=cc->sas;

 setid(1);
 memcpy(seb, cc->S, 16);
 memcpy(sdb, cc->S+16, 16);
 sasb=cc->sas;

}


void testike(void)
{
 unsigned char buf[36]={0,};
 testinit();

 memset(buf, 0, 36);
 setid(1);
 ike_run_acceptor(buf);

 setid(0);
 ike_run_originator(buf);


 setid(1);
 ike_2(buf);
 setid(0);
 ike_3(buf);
 setid(1);
 ike_4(buf);
 stemu(buf);
 ike_6(buf);
 setid(0);
 ike_5(buf);
 setid(1);
 ike_8(buf);
 setid(0);
 ike_7(buf);
 stemu(buf);
 ike_9(buf);
 setid(1);
 ike_10(buf);
 setid(0);
 ike_11(buf);
 ikeres();

}




#endif






//--------------------HELPERS---------------------------------------

//safe clear memory
// Implementation that should never be optimized out by the compiler
void cr_memclr( void *v, short n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}


//comparing public keys not equal
unsigned char ike_cmp(unsigned char* keya, unsigned char* keyb)
{
 #define MINCMP 20
 #define MAXCMP 236
 short i;
 short r=0;
 unsigned char c;
 const static unsigned char bitset[16]={0,1,1,2, 1,2,2,3, 1,2,2,3, 2,3,3,4};

 for(i=0;i<32;i++)
 {
  c=keya[i]^keyb[i];
  r+=bitset[c&0x0F];
  r+=bitset[c>>4];
 }

 if((r<MINCMP)||(r>MAXCMP)) c=0xFF; //fail: keys are near the same
 else c=0;                       //OK: keys are different

 return c;
}

//generate len random bytes random to data
void ike_rand(unsigned char* data, short len)
{
  sh_ini();
  sh_upd(cc->sid, 16); //use seed
  sh_xof();
  sh_crp(cc->sid, 16); //update seed[16]
  sh_out(data, len);  //generate random a
  sh_clr();
}


//self-test of assemler cryptographic procedures (x25519_mult, x25519_elligator2, keccak-800 sponge)
//returns 1 is OK, 0 is failure
unsigned char ike_test(void)
{
 #define TST_VECTOR 0xAF91E972
 unsigned int i,j;
 unsigned char pub[32];
 unsigned char key[32];
 unsigned char sec[32];
 unsigned char c=0;

 //elligator2 test
 for(i=0;i<16;i++)
 {
  sh_ini();
  sh_upd(&i, 4); //absorb counter
  sh_xof(); //permute
  sh_out(sec, 32); //output secret key
  p2r(key, sec); //correct sec and output representation
  r2p(pub, key); //convert representation to point
  scalarmultbase(key, sec); //compute point in usual was: must be the same
  for(j=0;j<32;j++) c|=key[j]^pub[j];
 }
 if(c) return 0;

 //old test
 memset(pub, 0x55, 32); //set initial value
 for(i=0;i<16;i++) //provide sequence of computing
 {	//secret=H1(value); value^=H2(value); their_key=elligator2(value); value = their_key^secret; 
  sh_ini(); //init shake	
  sh_upd(&i, 4); //absorb counter
  sh_upd(pub, 32); //absorb value
  sh_xof(); //permute
  sh_crp(pub, 32); //update initial value	
  sh_out(sec, 32); //output secret key
  r2p(key, pub); //compute curve point into their_key
  scalarmult(pub, sec, key); //compute public key
 }
 //compare crc32 of resulting value with precomputed vector
 if(TST_VECTOR ^ cr_crc32(pub, 32)) return 0; //result fail
 else return 1; //result ok
}

//--------------------INIT---------------------------------------

//initialize IKE engine
void ike_create(void)
{
  cc=&ctaa;
  cr_memclr(cc, sizeof(ct_data)); //clear data
}

//reset IKE for new session
void ike_reset(void)
{
  //clear last session data
  cr_memclr(cc->x, 32);
  cr_memclr(cc->Y, 32);
  cr_memclr(cc->S, 32);
  cr_memclr(cc->C, 32);
  
  ike_rand(cc->B, 32); //set random their key B for new incoming session

  //set obfuscation for next session: this is not constatnt time those run on reset
  //leaks of session obf key is not critical because it is for IKE obfuscation only
  ike_rand(cc->U, 32); //set random obfuscation key
  p2r(cc->T, cc->U); //compute pseudorandom point representation (not constatnt time!)

  cc->cnt_in=0; //clear counters for new session
  cc->cnt_out=0;
  cc->keystep=0; //clear key step status
  cc->speke=0;  //clear speke init flag
  cc->sas=0; //clear SAS
  cc->ans=0; //answer flag
}


//-------------------------INTERFACE-----------------------------------

//call by crypto module for data packets for IKE
//change IKE state not back except ike_reset
short ike_r(unsigned char* data)
{
 short len=0;

 switch(cc->cnt_in) //depends IKE state
 {
  case 2: {len=ike_2(data);break;}  //acceptor rcvd U, send C
  case 3: {len=ike_3(data);break;}  //originator rcvd C, send X
  case 4: {len=ike_4(data);break;}  //acceptor rcvd X, send X to ST
  case 5: {len=ike_5(data);break;}  //originator rcvd V, send A
  case 6: {len=ike_6(data);break;}  //acceptor rcvd Zb from ST, send V
  case 7: {len=ike_7(data);break;}  //originator rcvd Y, send Y to ST
  case 8: {len=ike_8(data);break;}  //acceptor rcvd A, send Y
  case 9: {len=ike_9(data);break;}  //originator rcvd Za from ST, send Ma
  case 10: {len=ike_10(data);break;} //acceptor rcvd Ma, send Mb
  case 11: {len=ike_11(data);break;} //originator rcvd Mb, send S to TR
  case 12: {len=ike_12(data);break;} //acceptot rcvd EVENT_REQ from TR, send S to TR
  case 13: {len=ike_13(data);break;} //originator rcvd EVENT_SET from TR
  case 14: {len=ike_14(data);break;} //acceptor rcvd EVENT_SEC from TR, send au result + their B to ST, stop IKE
  case 15: {len=ike_15(data);break;} //originator rcvd EVENT_SEC from TR, send au result to UI, stop IKE
  default:    //during call (after IKE)
  {
   switch(cc->keystep)
   {
    case 1: {len=key_Q1(data);break;}  //originator rcvd Kr, send Kk
    case 2: {len=key_Q2(data);break;}  //acceptor receive Kk, send Adr to ST
    case 3: {len=key_Q3(data);break;}  //receive SPEKE Aut
   }
  }
 }
 return len;
}


//send representation to remote  on pre-IKE stage
short ike_rep(unsigned char* data)
{
 memcpy(data+4, cc->T, 32); //output representation
 data[0]=TR_REP; //to remote over TR
 return IKE_LEN;
}

//compute obfuscation shared secret and set to TR on pre-IKE stage
short ike_obf(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr

 if(cc->cnt_in) return 0; //only in iddle state!
 cc->cnt_in=1; //set state!
 r2p(cc->Y, p); //compute point form their representation
 scalarmult(p, cc->U, cc->Y); //compute shared secret
 sh_ini();
 sh_upd(p, 32); //hash secret
 sh_xof();
 sh_out(p, 32);  //output material
 sh_clr();
 data[0]=TR_OBF; //output to TR
 return IKE_LEN;
}

//connect of originator,  send X'
short ike_run_originator(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr
 cc->cnt_in=3; //start IKE as originator

 //destination public key is preset in CC->B
 ike_rand(cc->x, 32); //originator's private x  in cc->x
 ike_rand(cc->U, 32); //originator's private u  in cc->U
 
 scalarmult(cc->T,cc->x, cc->B); //their tag Tb = B*x in cc->T

 sh_ini();
 sh_upd(cc->T, 32); //Tb
 sh_xof();
 sh_out(cc->C, 32); //H(Tb)
 r2p(cc->S, cc->C); //speke base P = E(Tb)
 scalarmult(cc->C, cc->U, cc->S); //our speke key U=u*P: our U in cc->C

 scalarmultbase(p, cc->x); //our X=G*x
 sh_ini();
 sh_upd(cc->C, 32); //hash our U
 sh_xof();
 sh_crp(p, 32); //mask our X (for commitment reason): X'=X^H(U)
 //output X'
 data[0]=TR_Q;
 return IKE_LEN;
}

//connect of acceptor, send note to UI
short ike_run_acceptor(unsigned char* data)
{
 cc->cnt_in=2; //start IKE as acceptor

 ike_rand(cc->x, 32);  //acceptor's private y in cc->C
 ike_rand(cc->U, 32);  //acceptor's private v in cc->U
 ike_rand(cc->B, 32);  //destination is unknown in cc->B

 return 0;
}



//acceptor rcvd X', send Y
short ike_2(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr

 cc->cnt_in+=2;

 //receive X'
 memcpy(cc->Y, p, 32); //store their masked efemeral X' in cc->Y
 scalarmultbase(p, cc->x); //output our efemeral  Y=G*y

 //output Y
 data[0]=TR_Q;
 return IKE_LEN;
}

//originator rcvd Y, send U
short ike_3(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr

 cc->cnt_in+=2;

 //receive Y
 memcpy(cc->Y, p, 32); //store their efemeral Y in cc->Y
 scalarmult(cc->S, cc->x, cc->Y); //compute DH secret = Y*x   to cc->S

 memcpy(p, cc->C, 32); //output our speke key U from cc->C
 //output U
 data[0]=TR_Q;
 return IKE_LEN;
}

//acceptor rcvd U, unmask X and send to ST
short ike_4(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr

 cc->cnt_in+=2;
 //receive U
 scalarmult(cc->C, cc->U, p); //compute SPEKE secret S=U^v to cc->C

 //unmask their X (was masked for commitment reason)
 sh_ini();
 sh_upd(p, 32); //hash their speke key U
 sh_xof();
 sh_crp(cc->Y, 32); //unmask X: X=X'^H(U)   store their efemeral X in cc->Y

 scalarmult(cc->S, cc->x, cc->Y); //compute DH secret = X*y to cc->S

 //output X to ST
 memcpy(p, cc->Y, 32); //output their efemeral X to ST for safely compute tag Tb=X*b
 data[0]=ST_SEC;
 return IKE_LEN;
}


//acceptor rcvd  tag X*b from ST, send V
short ike_6(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr

 cc->cnt_in+=2;
 //receive tag from ST
 //if ST fail input is random generated by ST
 memcpy(cc->T, p, 32); //save our  tag X*b

 //compute speke base point using Tb
 sh_ini();
 sh_upd(cc->T, 32); //Tb=X*b
 sh_xof();
 sh_out(p, 32); //H(Tb)
 r2p(cc->Y, p); //P=E(Tb) //SPEKE point
 scalarmult(p,cc->U, cc->Y); //V=v*P

 //output V
 data[0]=TR_Q;
 return IKE_LEN;
}


//originator rcvd V, send A
short ike_5(unsigned char* data)
{
 unsigned char cp;
 short i;
 unsigned char* p=data+4; //in/out ptr

 cc->cnt_in+=2;

 //receive V

 //check speke keys not same
 cp=ike_cmp(cc->C, p); //compare our and their SPEKE public keys
 ike_rand(cc->T, 32); //get random
 for(i=0;i<32;i++) cc->U[i]^=(cc->T[i]&cp); //mask our SPEKE secret if their key invalid

 scalarmult(cc->C,cc->U, p); //compute speke secret x1*y1*E(X*b)
 scalarmult(cc->U,cc->x, cc->B);  //compute their tag Tb=B*x

 //mask A
 memcpy(p, cc->A, 32); //our A
 sh_ini();
 sh_upd(cc->C, 32); //speke secret
 sh_xof();
 sh_out(cc->C, 32); //replace SPEKE secret by it's hash 
 sh_crp(p, 32); //encrypt A to A'

 //send A'
 data[0]=TR_Q;
 return IKE_LEN;
}

//acceptor rcvd A', send DH fingerprint C
short ike_8(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr

 cc->cnt_in+=2;

 //receive A'

 //unmask A
 sh_ini();
 sh_upd(cc->C, 32); //use speke secret
 sh_xof();
 sh_out(cc->C, 32); //replace SPEKE secret by it's hash
 sh_crp(p, 32); //decrypt A
 memcpy(cc->B, p, 32); //A

 scalarmult(cc->U,cc->x, cc->B); //Ta=A*y  (their tag)

 //compute fingerprint of DH secret
 sh_ini();
 sh_upd(cc->S, 32);  //DH secret
 sh_upd((char*)"$DH:SALT", 8);  //salt
 sh_xof();
 sh_out(p, 32); //C

 //send C
 data[0]=TR_Q;
 return IKE_LEN;
}

//originator rcvd C, send Y to ST
short ike_7(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr
 short i;
 unsigned char cp=0;
 cc->cnt_in+=2;

 //check fingerprint of DH secret
 sh_ini();
 sh_upd(cc->S, 32);  //DH secret
 sh_upd((char*)"$DH:SALT", 8);  //salt
 sh_xof();
 sh_crp(p, 32); //must be zero
 
 for(i=0;i<32;i++) cp|=p[i]; //zero (OK) or nonzero (fail)
 cp=(!cp)-1; //zero (OK) or FF (fail)
 ike_rand(cc->T, 32); //get random
 for(i=0;i<32;i++) cc->Y[i]^=(cc->T[i]&cp); //mask Y if fingerprint invalid

 memcpy(p, cc->Y, 32); //output Y to ST
  //output Y to ST
 data[0]=ST_SEC;
 return IKE_LEN;
}

//originator rcvd our tag Ta=Y*a from ST, send Ma+Mx
short ike_9(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr

 cc->cnt_in+=2;

 //receive Ta

 memcpy(cc->T, p, 32);//save our tag Ta=Y*a

 //extended TripleDH

 sh_ini();
 sh_upd(cc->C, 32); //SPEKE secret
 sh_upd(cc->S, 32); //DH secret
 sh_upd(cc->T, 32); //our tag
 sh_upd(cc->U, 32); //their tag
 sh_xof();
 sh_out(p, 16); //Ma

 sh_ini();
 sh_upd(cc->C, 32); //SPEKE secret
 sh_upd(cc->S, 32); //DH secret
 sh_upd(cc->U, 32); //their tag
 sh_upd(cc->T, 32); //our tag
 sh_xof();
 sh_out(cc->x, 16); //Mb

 sh_ini();
 sh_upd(cc->S, 32); //DH secret
 sh_xof();
 
 sh_out(cc->S, 16); //Enc key
 sh_out(p+16, 16); //output Mx

 sh_out(cc->S+16, 16); //Dec key
 sh_out(cc->x+16, 16); //output expected My

 sh_out(cc->T, 32); //TR secret
 sh_out(&cc->sas, sizeof(short)); //SAS

 //S=Enc/Dec keys
 //T=TR secret
 //U=empty
 //x=expected Mb+My
 
 data[0]=TR_Q;
 return IKE_LEN;
}


//acceptor rcvd Ma, save au result, send Mb
short ike_10(unsigned char* data)
{
 short i;
 volatile unsigned char dh_err=0;  //0 is DH is OK, 0xFF is fail
 volatile unsigned char au_err=0;  //0 is tDH aut is OK, 0xFF is fail
 unsigned char* p=data+4; //in/out ptr
 //unsigned char* pp=data+20; //in/out ptr
 //unsigned char* ppx=cc->x+16; //in/out ptr

 cc->cnt_in+=2;

 //Extended Triple DH

 sh_ini();
 sh_upd(cc->C, 32); //SPEKE secret
 sh_upd(cc->S, 32); //DH secret
 sh_upd(cc->T, 32); //our tag
 sh_upd(cc->U, 32); //their tag
 sh_xof();
 sh_out(cc->x, 16); //compute Mb

 sh_ini();
 sh_upd(cc->C, 32); //SPEKE secret
 sh_upd(cc->S, 32); //DH secret
 sh_upd(cc->U, 32); //their tag
 sh_upd(cc->T, 32); //our tag
 sh_xof();
 sh_crp(p, 16); //check Ma

 sh_ini();
 sh_upd(cc->S, 32); //use DH secret
 sh_xof();
 
 sh_out(cc->S+16, 16); //compute Dec key
 sh_crp(p+16, 16); //check Mx

 sh_out(cc->S, 16); //compute Enc key
 sh_out(cc->x+16, 16); //output My

 sh_out(cc->T, 32); //compute TR secret
 sh_out(&cc->sas, sizeof(short)); //SAS

 for(i=0;i<16;i++) au_err|=p[i];  //0 is OK, other is fail
 for(i=16;i<32;i++) dh_err|=p[i]; //0 is OK, other is fail
 au_err=(!au_err)-1;  //0 is OK, 0xFF is fail
 dh_err=(!dh_err)-1;  //0 is OK, 0xFF is fail
 au_err|=dh_err;    //AU always fail id DH is fail

 //mask values on authentication error
 ike_rand(p, 32); //generate random
 for(i=0;i<32;i++) cc->S[i]^=(p[i]&dh_err); //overwrite Ke+Kd
 ike_rand(p, 32); //generate random
 for(i=0;i<32;i++) cc->T[i]^=(p[i]&dh_err); //overwrite TR secret
 ike_rand(p, 32); //generate random
 for(i=0;i<16;i++) cc->x[i]^=(p[i]&au_err); //overwrite Mb
 for(i=16;i<32;i++) cc->x[i]^=(p[i]&dh_err); //overwrite Mc

 au_err&=CR_AU_FAIL; //set error codes 0 or FE
 dh_err&=CR_DH_FAIL; //0 or FF
 cc->cnt_out=au_err | dh_err; //save authentication results 0 or FE or FF
 au_err=0; //clear values
 dh_err=0;

 memcpy(p, cc->x, 32);  //output Mb+My
 data[0]=TR_Q;
 return IKE_LEN;
}


 //originator rcvd Mb, send secret to TR
short ike_11(unsigned char* data)
{
 short i;
 volatile unsigned char dh_err=0;
 volatile unsigned char au_err=0;
 unsigned char* p=data+4; //in/out ptr
 //unsigned char* pp=data+20; //in/out ptr
 //unsigned char* ppx=cc->x+16; //in/out ptr

 cc->cnt_in+=2;

 //check common secret validity
 for(i=0;i<16;i++) au_err|=p[i]^cc->x[i];
 au_err=(!au_err)-1;

 //check DH fingerprint validity
 for(i=16;i<32;i++) dh_err|=p[i]^cc->x[i];
 dh_err=(!dh_err)-1;

 //mask values on authentication error
 ike_rand(p, 32); //generate random
 for(i=0;i<32;i++) cc->S[i]^=(p[i]&dh_err); //overwrite Ke+Kd
 ike_rand(p, 32); //generate random
 for(i=0;i<32;i++) cc->T[i]^=(p[i]&dh_err); //overwrite TR secret

 au_err&=CR_AU_FAIL; //set authentication result codes
 dh_err&=CR_DH_FAIL;
 cc->cnt_out = dh_err | au_err; //save au result  0 or FE or FF

 memcpy(p, cc->T, 32); //output TR secret

 au_err=0; //clear values
 dh_err=0;

 data[0]=TR_SEC;
 data[1]=1; //need remote notification
 return IKE_LEN;
}

//originator rcvd EVENT_SET from TR, no to do
short ike_13(unsigned char* data)
{
 cc->cnt_in+=2;
 return 0;
}


//originator rcvd EVENT_SEC from TR, send au result to UI, stop IKE
short ike_15(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr

 cc->cnt_in=IKE_END; //finish of IKE, start work
 
 memcpy(p, cc->A, 32); //our public key (for test only)
 au_set(AUDIO_WAIT); //play wait tone

 data[0]=UI_CALL;
 data[1]=0xFF&cc->cnt_out; //set au result
 data[2]=cc->sas>>8;
 data[3]=0xFF&cc->sas;  //output SAS

 //clear data after IKE
 cr_memclr(cc->x, 32);
 cr_memclr(cc->Y, 32);
 cr_memclr(cc->C, 32);
 cr_memclr(cc->T, 32);
 cr_memclr(cc->U, 32);

 cc->cnt_out=IKE_END;
 return IKE_LEN;
}

//acceptot rcvd EVENT_REQ from TR, send setkey to ST
short ike_12(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr

 cc->cnt_in+=2;

 memcpy(p, cc->T, 32); //output TR secret

 data[0]=TR_SEC;
 data[1]=0; //not need remote notification
 return IKE_LEN;
}


//acceptor rcvd EVENT_SET from TR, send au result to UI, stop IKE
short ike_14(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr

 cc->cnt_in=IKE_END;
 au_set(AUDIU_RING); //play ring tone
 memcpy(p, cc->B, 32); //output their key
 data[0]=ST_GETNAME;
 data[1]=0xFF&cc->cnt_out; //set au result  00 or FE or FH
 data[2]=cc->sas>>8;
 data[3]=0xFF&cc->sas;  //output SAS
 //clear data after IKE
 cr_memclr(cc->x, 32);
 cr_memclr(cc->Y, 32);
 cr_memclr(cc->C, 32);
 cr_memclr(cc->T, 32);
 cr_memclr(cc->U, 32);
 cc->cnt_out=IKE_END; //finish of IKE, start work
 return IKE_LEN;
}



//*************************************************************************
//                        SYMMETRIC CRYPTOGRAPHY
//*************************************************************************


//==========================CTR ENCRYPT=====================================
//encrypt voice packet
//input: plane voice packet: header[4]+data[12]
//output: encrypted voice packet header[1]+ctr[3]+data'[12]
//==========================================================================
short encrypt(unsigned char* pkt)
{
 unsigned char* p=pkt+4;

 if(cc->cnt_in < IKE_END) return 0;

 //INCREMENT OUR OUTGOING PACKETS COUNTER, CHECK OVERFLOW
 cc->cnt_out++;
 if(0x1000000 & cc->cnt_out)
 {
  cr_rst(pkt);
  return 0;
 }

 if(!(pkt[0]&0x40))
 {
  //CTR ENCRYPT USING GAMMA
  //data'[12] = data[12] ^ H( Kd[16] | ctr[4] )
  sh_ini();  //init sha3
  sh_upd(cc->S, 16);  //hash encryption key
  sh_upd(&(cc->cnt_out), 4);  //hash output packets counter
  sh_xof(); //permute
  sh_crp(p, 12);    //encrypt voice data
 }
 //set 24 bit packet counter value to header
 pkt[1]=cc->cnt_out&0xFF;
 pkt[2]=(cc->cnt_out>>8)&0xFF;
 pkt[3]=(cc->cnt_out>>16)&0xFF;

 return 16;
}


//==========================CTR DECRYPT=====================================
//decrypt voice packet
//input: encrypted voice packet: header[2]+ctr[2]+data'[12]
//output: plane voice packet: header[4]+data[12]
//==========================================================================
short decrypt(unsigned char* pkt)
{
 unsigned char* p=pkt+4;
 unsigned int cnt=((unsigned int)pkt[3]<<16)+((unsigned int)pkt[2]<<8)+pkt[1]; //16 bit counter from packet
 
 if(!(pkt[0]&0x40))
 {
  //CHECK WE HAVE A KEY
 if(cc->cnt_in < IKE_END) return 0; //TEST ONLY!!!!!!!!!!!!!!!!!!1

  if(0x1000000 & cc->cnt_in) //restrict number of packet in one session
  {
   cr_rst(pkt); //session too long - reset it
   return 0;
  }

  //INCREMENT INTERNAL PACKET COUNTER, CHECK OVERFLOW
  if(cnt <= cc->cnt_in)
  {
   return 0;
  }
  cc->cnt_in=cnt;

  //CTR DECRYPT USING GAMMA
  //data[12] = data'[12] ^ H( Kd[16] | ctr[4] )
  sh_ini();  //init sha3
  sh_upd(16 + cc->S, 16);  //hash decryption key
  sh_upd(&(cc->cnt_in), 4);  //hash input packets counter
  sh_xof(); //permute
  sh_crp(p, 12);    //decrypt voice frame
 }

 return 16;
}
//=========================================================================
