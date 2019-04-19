#include <string.h>
#include "shake.h"
#include "audio.h"
#include "if.h"
#include "ike.h"
#include "pke.h"
#include "trng.h"
#include "cr_if.h"
#include "tr.h"
#include "cr.h"


extern ct_data* cc;  //cryptor data defined in ike.c
//----------------INTERFACE-----------------------------------

//creating Cryptor module and apply extern random sid
short ct_create(unsigned char* data)
{

 ike_create(); //create IKE module
 //update internal sid by random and optionally external sid
 sh_ini();  //init sha3
 sh_upd(data, 16);  //update with random from ST
 sh_xof(); //permute
 sh_crp(cc->sid, 16);  //output random for seeding PRNG
 sh_clr(); //clear

 ike_reset(); //reset data for initial state
 return 1;
}


//processing data sended to Cryptor
short cr_process(unsigned char* data, short inlen)
{
 unsigned char type=data[0];  //data type
 short len=0;

 switch(type) //check for packet type
 {
  //Initialization
  case CR_PTH:{len=cr_ui_pth(data); break;} //*Path to book from UI (intercepted for next)
  case CR_RND:{len=cr_st_R(data); break;} //Rand from ST
  case EVENT_SEED_OK:{len=cr_event_ROK(data); break;} //SEED_OK from TR
  case CR_PSW:{len=cr_ui_psw(data); break;} //*Password for book from UI (intercepted for next)
  case CR_PUB:{len=cr_st_A(data); break;} //Our pubkey from ST
  case EVENT_INIT_OK:{len=cr_event_net(data); break;}  //Init event from TR
  case CR_AUD:{len=cr_au(data); break;}
  case REMOTE_A:{len=cr_ans(data); break;}

  //Call setup
  case EVENT_CALL_TCP:
  case EVENT_ACCEPT_TCP:{len=cr_event_accept(data); break;} //Incoming call event event from TR
  case CR_KEY:{len=cr_st_B(data); break;} //Their key from ST
  //case EVENT_BUSY:{len=cr_event_note(data); break;} //busy event event from TR
  case EVENT_READY_OUT:{len=cr_event_run(data); break;}  //Connected event event from TR

  //obfuscation (pre-IKE)
  case EVENT_OBF_OUT:{len=ike_rep(data); break;}
  case REMOTE_R:{len=ike_obf(data); break;}

  //IKE
  case REMOTE_Q:  //ike data from remote
  case CR_SEC: //Tag from ST
  case EVENT_SEC:{len=ike_r(data); break;}  //event TR apply secret from TR

  //SPEKE
  case CR_SPEKE:{len=key_speke(data); break;} //init SPEKE (from UI)
  case REMOTE_S:{len=key_S(data); break;} //init SPEKE (from remote)

  //key receiving
  case REMOTE_K: {len=key_K(data); break;}//init of receive key (from remote)
  case CR_ADDK:{len=cr_add_key(data); break;} //allow saving receiving key (from UI)

  //key sending
  case CR_REQK:{len=cr_set_key(data); break;} //start key send (from UI)

  //Call control
  case CR_MUTE:{len=cr_ui_mute(data); break;} //Mute control from UI
  case EVENT_DIRECT_REQ:{len=cr_event_direct(data); break;}//  derect reques event from TR

  //call termination
  case CR_RST: {len=cr_rst(data); break;} //from local side
  case EVENT_TERMINATED: {len=cr_term(data); break;}//from remote side
 }

 //check Cryptor have answer after processing
 if(len) cr_send(data, len); //send to interface

 return len; //returns answer len 
}


//===============Initialization==========================
 
//process book file path or serial device for storage from UI, resend to ST
short cr_ui_pth(unsigned char* data)
{
 unsigned char* p=data+4;
 //initialize cryptor
 ct_create(p); //for ST uses keccack!!!!
 if(!ike_test()) return 0; //stop process on wrong crypto

 cr_set_ser(data); //check path is serial device, set mode
 data[0]=ST_PTH; //for local: resend data to TR
 return IKE_LEN; //resend path to ST
}

//process rand from ST, send passw request to UI
short cr_st_R(unsigned char* data)
{
 unsigned char* p=data+4;
 
 

 ct_create(p); //initialize cryptor and update sid

 ike_rand(p, 32); //generate random for TR
 data[0]=TR_SEED; //resend to TR for seeding PRNG
 return IKE_LEN;
}

//process SEED_OK event from TR, send password request to UI
short cr_event_ROK(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr

 ike_rand(p, 32); //generate random for GUI
 data[0]=UI_INI; //send random to UI and request password 
 return IKE_LEN;
 //for dongle: process Q, compute M, send OK1 to UI
}


//process book file access + storage pin from UI, resend to TR
short cr_ui_psw(unsigned char* data)
{
 data[0]=ST_PSW; //resend psw01 to st
 return IKE_LEN;
 //for dongle: set mac and encrypt before sending
}


//process our public key A from ST (at the start), send fingerprint to UI
short cr_st_A(unsigned char* data)
{
 unsigned char* p=data+4; //in/out ptr
 unsigned int fp; //fingerprint of our public key
 short i;

 //if(data[1]!=STORAGE_OK) return 0; //check storage is OK

 memcpy(cc->A, p, 32); //store out public key
 fp=cr_crc32(p, 32); //comute fingerprint
 memset(data, 0, 36); //clear output
 for(i=0;i<8;i++) //convert 4 bytes of fp to hex
 {
  p[i]='0'+(fp&0x0F); //4 bits to hex digit
  if(p[i]>'9') p[i]+=7; //correct for A-F
  fp>>=4; //move to next 4 bits
 }

 data[0]=UI_FGP;
 return IKE_LEN; //send fingerprint to UI
}

//TCP listener setup result
short cr_event_net(unsigned char* data)
{
 //send result in data[1] to UI
 data[0]=UI_TCP;
 return EVENT_LEN;
}

//set audio devices
short cr_au(unsigned char* data)
{
 char* pmike=(char*)data+4; //name of input device
 char* pspk=(char*)data+20; //name of output device

 pmike[15]=0; //terminate strings
 pspk[15]=0;
 au_create(pmike, pspk); //initialize audio module
 au_set(AUDIO_OFF); //close audio
 return 0; //no to answer: initialization finished
}



//===========Key add========================


//their public key B from ST (initiate outgoing call or send key to remote)
short cr_st_B(unsigned char* data)
{
  unsigned char* p=data+4; //in/out ptr

  if(!data[2]) //iddle mode: setup outgoing call
  {
   memcpy(cc->B, p, 32);  //save their B to cc->B
   data[0]=UI_OUT; //start outgoing call
   return IKE_LEN;
  }
  else  return (key_run(data)); //in call; send to remote
}

 //receive address and start contact output sequence
 //input: adress (CR_REQK)
 //output: request (UI_REQK)
 short cr_set_key(unsigned char* data)
 {
  unsigned char* p=data+4; //in/out ptr

  //store address to U
  memcpy(cc->U, p, 32); //store contact address for output
  cc->keystep=1; //set flag of key sending process
  data[0]=UI_REQ; //request UI for contact key
  return IKE_LEN;
 }

 //allow add received key to book
 short cr_add_key(unsigned char* data)
 {
  unsigned char* p=data+4; //in/out ptr
  short ret=0;

  if(data[1])  //check allow flag
  {
   memcpy(p, cc->U, 32); //output received key to ST
   data[0]=ST_ADD;
   data[1]=0;
   data[2]=cc->sas>>8;
   data[3]=0xFF&cc->sas;  //output SAS
   ret=IKE_LEN;
  }
  memset(cc->U, 0, 32); //clear data
  return ret;
 }



//=====================CALL=========================

//accept/call
short cr_event_accept(unsigned char* data)
{
 if(cc->cnt_in>1) return 0;
 
 //set audio path
 if(data[0]==EVENT_ACCEPT_TCP)
 {
  au_dev(AUDIO_PATH_RING); //ring for incoming
  cc->ans=2; //set flag of acceptor
 }
 else au_dev(AUDIO_PATH_MEDIA); //media for outgoing

 au_set(AUDIO_CONNECT); //play connect tone during IKE
 data[2]=data[0];  //accept or call (incoming or outgoing)
 data[0]=UI_ACCEPT;
 return EVENT_LEN;
}

//handshake OK
//data[1] is 0 for originator, 1 for accept ovet TCP, 2 for accept over Tor
//3-close out, 4-close in
short cr_event_run(unsigned char* data)
{
 short ret=0;
 data[0]=UI_DBL;

 if(cc->cnt_in==1)  //state after obfuscation: run key exchange for new call
 {
  //check EVENT_READY_OUT (caller flag)
  if(!data[1]) ret=ike_run_originator(data); //run IKE as originator, return TR_Q
  else ret=ike_run_acceptor(data);     //run IKE as acceptor, return UI_INC
 }   //call state: check for outgoing doublimg call connected success
 else ret=IKE_LEN; //notify UI on call
 return ret;
}

//direct connecting remote notification
short cr_event_direct(unsigned char* data)
{
  data[0]=UI_DIR;   //resent to UI
  return EVENT_LEN;
}

//callee answer to caller
short cr_ans(unsigned char* data)
{ //originator only!
 if(!cc->ans)
 {
  cc->ans=1;
  //notify GUI for calle answer
  data[0]=UI_ANSWER;
  return IKE_LEN;
 }
 else return 0;
}

 //voice control from UI
short cr_ui_mute(unsigned char* data)
{
  if(cc->cnt_in < IKE_END) return 0;

  if(data[1]) //talk
  {
   au_dev(data[1]); //set audio path by user (1-media, 2-voice)
   au_echo(240); //set echo canceler
   au_npp(1); //set noise preprocessor
   au_rx(5); //set Mike sensitivity
   au_tx(5); //set Speaker volume
   au_set(AUDIO_TALK); //set audio mode for talk
  }

  else au_set(AUDIO_MUTE); //mute audio
  if(cc->ans==2) //acceptor only
  {
   cc->ans=1;
   data[0]=TR_A; //send answer to originator
  }
  else data[0]=TR_MUTE; //send talk/mute signal to TR for reset voice queue
  return IKE_LEN;
}

//reset: from cr and ui
short cr_rst(unsigned char* data)
{
 if(data)
 {
  //resid CR sid
  sh_ini();  //init sha3
  sh_upd(data+4, 32);  //hash sid from ui
  sh_xof(); //permute
  sh_crp(cc->sid, 16);    //update cr sid
  memset(data, 0, 36); //clear sid from ui
 }
 ike_rand(data+4, 32); //generate new radnom for reseeding TF
 data[4]=0; //clear string for terminate call
 data[0]=TR_SET; //reset call in TR module
 return IKE_LEN;
}

//call termination event from TR
short cr_term(unsigned char* data)
{
 memset(data, 0, 36); //clear data
 au_set(AUDIO_OFF); //close audio
 ike_reset(); //reset cryptor module
 data[0]=ST_RST; //send reset to ST
 ike_rand(data+4, 32); //set rand for ST
 return IKE_LEN;
}

unsigned int cr_crc32(unsigned char const *p, unsigned int len)
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





