
#include <string.h>
#include "tr.h"

#include "if.h"
#include "st_if.h"
#include "ui_if.h"
#include "tr_if.h"
#include "cr_if.h"
#include "ser.h"


unsigned char tr_buf[256];  //TR data buffer
short tr_path=0; //type of using TR


//check serial device name is specified for Storage
short tr_set_ser(unsigned char* data)
{
 if(data[4]=='&') tr_path=1;   //use external TR
 else if(data[4]=='#') tr_path=-1;  //use this torfone as a TR only
 else tr_path=0; //use internal TR
 if(tr_path) return 1; else return 0;
}

//call by other if for output data to TR (for TR or transpass)
void tr_send_to_tr(unsigned char* data, short len)
{
 unsigned char type=data[0]&TYPE_MASK;

 memcpy(tr_buf, data, len);
 memset(tr_buf+len, 0, sizeof(tr_buf)-len); //clear unused data
 if(!(type&0x80)) tr_process(tr_buf, len); //outgoing voice data
 else switch(type)
 {  //UI cannot resent any data to TR
  case TYPE_TR:
  {
   if(!tr_path) tr_process(tr_buf, len); //use internal TR
   else if(tr_path>0) ser_send(data); //use external TR
   break;  //ignore data for using this Torfone only_as_TR
  } //data will be process in TR
  case TYPE_UI:{ui_send_to_ui(tr_buf, len);break;} //data resend to UI
  case TYPE_CR:{cr_send_to_cr(tr_buf, len);break;} //data resend to CR
  case TYPE_ST:{cr_send_to_cr(tr_buf, len);break;} //data resend to ST over CR
 }
}

 
//call from main loop for processing TR data
short tr_loop(void)
{
 short len;

 if(!tr_path) //runs as Torfone with internal TR
 {  //get data from TR
  len=tr_read_from_remote(tr_buf);
  if(len>36) len=36;
  if(len) //check we have data
  {
   memset(tr_buf+len, 0, sizeof(tr_buf)-len); //clear unused data
   cr_send_to_cr(tr_buf, len); //pass to CR
   len=1; //set job flag
  }
 }
 else if(tr_path<0) //runs as only_as_TR
 {
  //get data from TR
  len=tr_read_from_remote(tr_buf);
  if(len>0)
  {
   ser_send(tr_buf); //send to other modules over serial
   len=1; //set job flag
  }

  //get data from serial for TR
  if(0<ser_read(tr_buf))
  {
   tr_process(tr_buf, 36);//process in TR
   len=1;  //set job flag
  }
 }
 else  //runs as Torfone with external TR
 {
  len=ser_read(tr_buf); //get data from TR
  if(len>0) //check we have data
  {
   cr_send_to_cr(tr_buf, 36); //pass to CR
   len=1; //set job flag
  }
 }


 return len; //return job flag
}
