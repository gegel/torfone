

#include <string.h>

#include "if.h"
#include "st_if.h"
#include "ui_if.h"
#include "tr_if.h"
#include "cr_if.h"

 #define MAX_GUI_LEN 68
unsigned char gui_out[MAX_GUI_LEN];
unsigned char gui_in[MAX_GUI_LEN];

volatile short guiin_flag=0;
volatile short guiout_flag=0;


//=============Thread safe for exchange with GUI=================

//call from GUI thread for get data for GUI
short if_getgui(unsigned char* data)
{
 short len=guiout_flag; //check data length
 if(len) //if some data ready for pass to GUI
 {
  memcpy(data, gui_out, guiout_flag); //output it
  guiout_flag=0; //clear length
 }
 return len; //return len of outputted data
}

//call from GUI for pass data to processing
void if_setgui(unsigned char* data, short len)
{
 if(guiin_flag) return; //check there are not-processed data from GUI
 memcpy(gui_in, data, len); //copy new data
 guiin_flag=len;  //set data length
}

//process GUI data passing to interconnect with GUI thread
void ui_process(unsigned char* data, short len)
{
 if(!guiout_flag) //check there is data not processed by GUI yet
 {
  memcpy(gui_out, data, len); //copy new data will be processed by GUI thread
  guiout_flag=len; //set flag is data len
 }
}


//=========================================================

//call by other if for output data to UI (for UI or transpass)
void ui_send_to_ui(unsigned char* data, short len)
{
 unsigned char type=data[0]&TYPE_MASK;

 switch(type)
 {  //UI cannot resent any data to TR
   case TYPE_UI:{ui_process(data, len);break;} //data will be processed in UI
   case TYPE_CR:{cr_send_to_cr(data, len);break;} //data resend to CR
   case TYPE_ST:{st_send_to_st(data, len);break;} //data resend to ST
 }
}



//call from main loop for resend answer of UI
short ui_loop(void)
{
 short len=guiin_flag; //answer len
 unsigned char type=0;

 if(len) //if having answer
 {
  type=gui_in[0]&TYPE_MASK; //destination
  switch(type)
  {
   case TYPE_CR:{cr_send_to_cr(gui_in, len);break;} //data will be passed to CR
   case TYPE_ST:{st_send_to_st(gui_in, len);break;} //data will be passed to ST
   case TYPE_TR:{tr_send_to_tr(gui_in, len);} //commands will be passed to TR
  }
  guiin_flag=0; //clqar answer len
  type=1; //set job flag
 }
 return (short)type; //return job flag
}

