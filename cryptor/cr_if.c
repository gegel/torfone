#define CI_DBG 0


#include <string.h>
#include "cr.h"

#include "if.h"
#include "st_if.h"
#include "ui_if.h"
#include "tr_if.h"
#include "cr_if.h"
#include "ser.h"
#include "audio.h"


unsigned char cr_buf[68];  //CR data buffer
unsigned char au_buf[16];
short cr_len=0; //length of unprocessed data in buffer
short au_len=0; //length of unprocessed voice from TR


short cr_set_ser(unsigned char* data)
{
 short ser=0;
 
 ser+=st_set_ser(data); //check path is for external_ST or run_as_ST
 ser+=tr_set_ser(data); //check path is for external_TR or run_as_TR

 ser_close(); //close serial device
 if(ser) //if serial device will be used
 {
  ser=ser_open((char*)data+5);  //open serial device by specified name
  return ser; //return result
 }
 return 0; //serial device not used
}


//call by other if for output data to CR (for CR or transpass)
void cr_send_to_cr(unsigned char* data, short dlen)
{
  if(!(data[0]&0x80)) //voice packet from TR
  {  //can overvrite previous
   memcpy(au_buf, data, 16);
   au_len=16;
  }
  else if(!cr_len)
  {  //cannot overwrite previous
   memcpy(cr_buf, data, dlen); //data packet
   cr_len=dlen;
  }
}

//call by CR procedures for output data to other if
void cr_send(unsigned char* data, short dlen)
{
 unsigned char type=data[0]&TYPE_MASK;
 if(!(type&0x80)) tr_send_to_tr(data, dlen); //outgoing voice data will be sent to TR
 else switch(type)
 {
   case TYPE_UI:{ui_send_to_ui(data, dlen);break;} //data will be passed to UI
   case TYPE_ST:{st_send_to_st(data, dlen);break;} //data will be passed to ST
   case TYPE_TR:{tr_send_to_tr(data, dlen);} //data will be passed to TR
 }
}


short au_loop(void)
{
 unsigned char job=0;
 
  //audio playing
 if(au_len) //check we have voice from TR fro playing
 {
  au_play(au_buf); //play data
  au_len=0; //clear flag
  job=1; //set job flag and process further
 }

 //audio grabbing
 if(au_grab(au_buf)) //chack audio data are avaliable for TR
 {
  tr_send_to_tr(au_buf, 16);  //send to TR
  job=1; //return: TR must process voice
 }

 return job;
}


//call from main loop for processing data received from other if
short cr_loop(void)
{
 unsigned char type=0;
 unsigned char job=0;

 /*
 //audio playing
 if(au_len) //check we have voice from TR fro playing
 {
  au_play(au_buf); //play data
  au_len=0; //clear flag
  job=1; //set job flag and process further
 }

 //audio grabbing
 if(au_grab(au_buf)) //chack audio data are avaliable for TR
 {
  tr_send_to_tr(au_buf, 16);  //send to TR
  return 1; //return: TR must process voice
 }
 */

 if(cr_len) //check we have data from other if
 {
  type=cr_buf[0]&TYPE_MASK;  //check destination
  switch(type)
  {
   case TYPE_CR:{cr_process(cr_buf, cr_len);break;} //data for Cryptor from UI and ST
   case TYPE_UI:{ui_send_to_ui(cr_buf, cr_len);break;} //data will be passed to UI
   case TYPE_ST:{st_send_to_st(cr_buf, cr_len);break;} //data will be passed to ST
   case TYPE_TR:
   {
    if(cr_buf[0]&TYPE_TRCMD) cr_process(cr_buf, cr_len); //events from TR
    else tr_send_to_tr(cr_buf, cr_len); //command for TR and data will be pass to remote
   }
  }
  cr_len=0; //clear data length (data was processed0
  job=1; //set job flag
 }
 return (short)job; //return job flag
}



