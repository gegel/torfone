
#include <stdio.h> //fopen
#include <string.h> //memcpy

#include "if.h"
#include "ui.h"
#include "tr.h"
#include "file_if.h"
#include "file.h"


void fl_note(unsigned char type, char* par);


fl_data fl; //file transfer data
char fl_path[FILE_MAX_PATH]={0}; //preset path to files folder
int fl_pathlen=0; //length of path
char fl_str[128]={0};

CMD_data CMD =
{
{'F','I','L','E', //FILE:filename
'T','E','R','M', //TERM
'S','T','E','P', //STEP
'P','I','N','G',  //PING
'C','A','L','L', //CALL:address
'T','A','L','K', //TALK:mode
'R','E','S','E', //RESET
'E','X','I','T'}//EXIT
};

unsigned short fl_crc16(unsigned char const *p, int len, unsigned short crc)
{
 #define TELCRCPOLY 0xA001 
 int i;
 
  while(len--) {
    crc^=*p++;
    for(i=0; i<8; i++)
      crc=(crc&1) ? (crc>>1)^TELCRCPOLY : (crc>>1);
    }

  return crc;
}


void fl_init_path(char* path)
{
 strncpy(fl_path, path, sizeof(fl_path)-32);
 fl_pathlen=strlen(fl_path);
}

int fl_init(unsigned short telnet_port)
{
 int i;
 memset(&fl, 0, sizeof(fl));
 i=tc_init(telnet_port);
 return i;
}

//output stege of file transfer in percents
void fl_dostep(void)
{
 char str[4]={0};
 if(!fl.send_file) strcpy(str, (char*)"--");  //no send
 else if(!fl.send_len) strcpy(str, (char*)"-"); //unexpected
 else if(fl.sended<fl.send_len) //in process
 {
  int i=(int)fl.sended*100/fl.send_len; //percents were sended
  if(i>99) i=99; //restrict to 2 digits
  sprintf(str, "%d", i); //as string
 }
 fl_note(NOTE_SEND, str); //output to Telnet client

}

//send notification to Telnet client
void fl_note(unsigned char type, char* par)
{

 int l;
 char* p=fl_str+4;

 //set verbose type of notification
 switch(type)
 {
  case NOTE_STOP:{strcpy(fl_str, "STOP");break;} //transfer terminated
  case NOTE_SEND:{strcpy(fl_str, "SEND");break;} //state of transfer: percents
  case NOTE_CPLT:{strcpy(fl_str, "CPLT");break;} //transfer compleet: name
  case NOTE_RCVD:{strcpy(fl_str, "RCVD");break;} //file received: name
  case NOTE_PONG:{strcpy(fl_str, "PONG");break;} //ping
  case NOTE_IDDL:{strcpy(fl_str, "IDDL");break;} //iddle state (no call)
  case NOTE_RING:{strcpy(fl_str, "RING");break;} //incoming call
  case NOTE_ANSW:{strcpy(fl_str, "ANSW");break;} //remote party answered
 }

 //add optional parameter
 if(par>0) sprintf(p, ":%s\r\n", par);
 else sprintf(p, "\r\n");

 //check len of data to send
 l=strlen(fl_str);
 tc_write((unsigned char*)fl_str, l);  //send to Telnet client

}

//send control to remote: type and parameters
void fl_remote(unsigned char type, unsigned short par1, unsigned char* par2)
{
 unsigned char buf[144];

 //set packet header
 buf[0]=0x80; //torfone control type
 buf[1]=type; //file control type
 buf[2]=par1&0xFF; //len, offset, crc16
 buf[3]=par1>>8;

 if(type==FILE_DATA) memcpy(buf+4, par2, 120); //set data if specified
 else if(!par2) memset(buf+4, 0, 120); //or set zero
 else strcpy((char*)(buf+4), (char*)par2);

 tr_send_to_remote(buf, 124); //send to remote
}


void fl_start(char* name) //start send file
{
 int i;


 if((!name)||(!name[0]))  //terminate old send process
 {
  //close old file
  if(fl.send_file)
  {  //exiting send mode
   fclose(fl.send_file); //close old file
   fl.send_file=0; //clear file
   fl.send_len=0;
   fl.sended=0;
   fl_note(NOTE_STOP, "terminate");  //notify user for fail
  }
  else fl_note(NOTE_STOP, (char*)"already");  //notify user for fail


  return;
 }


 //try open file for read
 name[31]=0; //terminate filename for safe
 strncpy(fl_path+fl_pathlen, name, sizeof(fl_path)-fl_pathlen);  //add filename to documents path
 fl.send_file = fopen (fl_path,"rb"); //try open file for read
 if(!fl.send_file)  //not opened
 {
  fl_note(NOTE_STOP, (char*)"no file");  //notify user for fail
  return;
 }
 //get file len
 fseek(fl.send_file,0,SEEK_END);  //set pointer to end of file
 i=ftell(fl.send_file); //get pointer position is len of file
 if((i<1)||(i>65535)) //check len is acceptable
 {
  fclose(fl.send_file);  //close file
  fl.send_file=0; //clear file
  fl.send_len=0; //clear len
  fl_note(NOTE_STOP, (char*)"bad length"); //notify user for fail
  return;
 }
  fseek(fl.send_file,0,SEEK_SET); //set pointer to start of file
  fl.send_len=(unsigned short)i;
  fl.send_crc=fl_crc16((unsigned char*)name, strlen(name), 0); //initiale crc from name
  fl.sended=0; //clear already sended bytes counter
  fl_remote(FILE_SEND, fl.send_len,(unsigned char*) name); //send startup to remote
}

void fl_send(void) //sending loop
{
 int i;

 i=tc_read(fl.buf); //check for command from Telnet client
 if(i) //parse it
 {
  unsigned int u;
  char* p=0;



  u=(*(unsigned int*)fl.buf); //command type
  p=strchr((char*)fl.buf, ':'); //search first space separated command from parameter
  if(p) p++; //if space is in staring

  //check command type and process it
  if(u==CMD.d[0])fl_start(p);//FILE:name
  else if(u==CMD.d[1])fl_start(0);//TERM
  else if(u==CMD.d[2])fl_dostep(); //STEP
  else if(u==CMD.d[3])fl_note(NOTE_PONG, 0);//PING
  else if(u==CMD.d[4])ui_docall((char*)"", p);//CALL:address
  else if(u==CMD.d[5])ui_setcommand(GUI_NOTE_ANS); //TALK:mode
  else if(u==CMD.d[6])ui_docancel(); //RESET
  else if(u==CMD.d[7])ui_setcommand(GUI_NOTE_TEXT); //EXIT
 }

 if(fl.send_file && fl.send_len) //check for having file for send
 {
  i=fl.send_len; //total len of file
  i-=fl.sended; //rest bytes to send
  if(i>120) i=120; //restrict up to payload size
  if(i>0) //if there are some bytes to send
  {
   if(i==fread(fl.buf, 1, i, fl.send_file)) //read data from file
   {  //read OK
    fl_remote(FILE_DATA, fl.sended, fl.buf); //send data to remote
    fl.send_crc=fl_crc16(fl.buf, i, fl.send_crc); //update crc of sended data
    fl.sended+=i; //count sended data
   }
   else
   {
    fl.send_len=0;  //read file error: clear total len for force stop sending
   }
  }
  else //all data were sent
  {
   fl.send_len=0; //once
   fl_remote(FILE_CPLT, fl.send_crc, 0); //send stop control
  }
 }
}


void fl_rcvd(unsigned char* data) //process file control
{
 unsigned char type=data[1];
 unsigned char* p=data+4;
 unsigned short d=(((unsigned short)data[3])<<8)+data[2];
 int i;

 switch(type)
 {
 //-----sender-----
 //file receive: success or fail
  case FILE_RCVD:
  {
   //notify remote result and exiting send mode
   if(fl.send_file)
   {
    p[31]=0;
    if(d) fl_note(NOTE_CPLT, (char*)p); //notify "delivered sucess" with name of sended file
    else fl_note(NOTE_STOP, (char*)p);  //notify "delivered fail"
    fclose(fl.send_file); //close old file
    fl.send_file=0; //clear file
    fl.send_len=0; //once
    fl.sended=0;
   }
   break;
  }


//-------receiver-------
   //sender start send
  case FILE_SEND:
  {
   //file len is zero: terminate receiving
   if(!d) //terminate old receiving remotely
   { //exit receiving state
    if(fl.rcvd_file)
    {
     fclose(fl.rcvd_file);
     fl.rcvd_file=0;
     fl.rcvd_len=0;
     fl.received=0;
     fl_remote(FILE_RCVD, 0, (unsigned char*)"rcvd terminate");
    }
    else fl_remote(FILE_RCVD, 0, (unsigned char*)"rcvd ready");
    break;
   }

   //file len is specified: start receiving
   if(!fl.rcvd_file) //check no already receive now 
   {
    fl.rcvd_len=d; //set len of file will be received
    fl.received=0; //clear number of received bites
    fl.name[0]=0;
    p[31]=0; //terminate filename for safe
    fl.rcvd_crc=fl_crc16((unsigned char*)p, strlen((char*)p), 0); //initiale crc from name
    strncpy(fl_path+fl_pathlen, (char*)p, sizeof(fl_path)-fl_pathlen);  //add filename to documents path
    fl.rcvd_file = fopen (fl_path,"wb"); //try open file for write
    if(!fl.rcvd_file)
    {
     fl_remote(FILE_RCVD, 0, (unsigned char*)"remote open error"); //send to remote: error of receiving file
     fl.rcvd_len=0; //exit receiving state
    }
    else strncpy(fl.name, (char*)p, sizeof(fl.name)); //copy name of file wiil be received
   }
   break;
  }


   //part of file data
  case FILE_DATA:
  {
   if(fl.rcvd_file && (d==fl.received)) //check this is a new data
   {
    i=fl.rcvd_len; //len of whole file
    i-=fl.received; //required data len
    if(i>120) i=120;  //set up to payload len
    if((i>0)&&(fl.rcvd_file)) //check we need data and output file is redy
    {
     if(i!=fwrite(p, 1, i, fl.rcvd_file)) //save received data to file
     { //write error
       fl_remote(FILE_RCVD, 0, (unsigned char*)"remote write error"); //send to remote: write error
       fclose(fl.rcvd_file);  //exit receiving state
       fl.rcvd_file=0;
       fl.rcvd_len=0;
       fl.received=0;
     }
     fl.rcvd_crc=fl_crc16(p, i, fl.rcvd_crc); //update crc of received data
     fl.received+=i;  //count received data
    }
   }
   break;
  }



   //sender finish send
  case FILE_CPLT:
  {
   if(fl.rcvd_file) //check in receiving state
   {
    if((fl.received==fl.rcvd_len)&&(d==fl.rcvd_crc)) //check len and crc of receiving file
    {  //ok
     fl_note(NOTE_RCVD, fl.name); //notify: received file name
     fl_remote(FILE_RCVD, fl.received, (unsigned char*)fl.name); //send to remote: received file name
    } //errro
    else fl_remote(FILE_RCVD, 0, (unsigned char*)"crc error"); //send to remote: error of receiving file

    fclose(fl.rcvd_file);  //exit receiving state
    fl.rcvd_file=0;
    fl.rcvd_len=0;
    fl.received=0;
   }
   break;
  }

 } //switch
}
