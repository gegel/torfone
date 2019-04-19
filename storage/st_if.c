#include <string.h>

#include "if.h"
#include "st_if.h"
#include "ui_if.h"
#include "tr_if.h"
#include "cr_if.h"
#include "ser.h"
#include "st.h"



unsigned char st_buf[68];  //ST data buffer
short st_len=0; //length of unprocessed data in buffer
short st_path=0; //type of using ST


//check serial device name is specified for Storage
short st_set_ser(unsigned char* data)
{
 if(data[4]=='*') st_path=1;   //use external storage
 else if(data[4]=='$') st_path=-1;  //use this torfone as a storage only
 else st_path=0; //use internal storage
 if(st_path) return 1; else return 0;
}

//call by other if for output data to CR (for CR or transpass)
void st_send_to_st(unsigned char* data, short dlen)
{
 if((!st_path)&&(!st_len))   //pass data to software ST
 {
  memcpy(st_buf, data, dlen);
  st_len=dlen;
 }
 else if(st_path>0) ser_send(data); //pass data to external ST
}

//call by ST procedures for output data to other if
void st_send(unsigned char* data, short dlen)
{
 unsigned char type=data[0]&TYPE_MASK;

 if(!st_path) //process data from software ST
 {
  switch(type)
  {  //ST cannot output any data to TR!!!
   case TYPE_UI:{ui_send_to_ui(data, dlen);break;} //data will be passed to UI
   case TYPE_CR:{cr_send_to_cr(data, dlen);} //data will be passed to CR
  }
 }
 else if(st_path<0) ser_send(data);  //pass data to external torfone from this ST
}

//call from main loop for processing data received from other if
short st_loop(void)
{
 unsigned char type=0;
 if(st_path) //both external_ST and run_as_ST modes
 {
  st_len=ser_read(st_buf); //get exactly 36 bytes of data from serial
  if(!st_len) return 0; //no data receiced from serial

  if(st_path>0)  //external_ST mode
  {
   type=st_buf[0]&TYPE_MASK; //check data type
   switch(type)
   {  //ST cannot output any data to TR!!!
    case TYPE_UI:{ui_send_to_ui(st_buf, st_len);break;} //data will be passed to UI
    case TYPE_CR:{cr_send_to_cr(st_buf, st_len);} //data will be passed to CR
   }
   st_len=0; //clear len
   return 1; //return job
  }
 }

 //for normal mode and _run_as_storage mode

 if(st_len) //check we have data from other if
 {
  type=st_buf[0]&TYPE_MASK;  //check destination
  switch(type)
  { //UI cannot transpass any data to other if!!!
   case TYPE_ST:{st_process(st_buf, st_len);break;} //data for Cryptor from UI and ST
  }
  st_len=0; //clear data length (data was processed0
  type=1; //set job flag
 }
 return (short)type; //return job flag
}




