#include <stdio.h>
#include <stdlib.h>
#include <string.h>







#ifdef _WIN32

 #include <stddef.h>
 #include <basetsd.h>
 #include <stdint.h>
 #include <windows.h>
 #include <conio.h>

 #define close fclose
 #define usleep Sleep

#else  //Linux

 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 //#include <stdio.h>
 //#include <stdlib.h>
 #include <fcntl.h>
 #include <assert.h>
 //#include <string.h>
 #include <ctype.h>
 //#include <time.h>
 #include <sys/types.h>
 //#include <sys/time.h>
 #include <sys/wait.h>
 #include <sys/stat.h>
 #include <unistd.h>
 #include <errno.h>
 #include <signal.h>
 //#include <sys/socket.h>
 //#include <netinet/in.h>
 //#include <netinet/tcp.h>
 //#include <arpa/inet.h>
 //#include <netdb.h>
 #include <termios.h>
 #include <termio.h>

#endif



#include "if.h"
#include "ui_if.h"
#include "ui.h"



int jb;
int cn_cnt=0;


ui_data ui; //data
ui_conf cf; //configuration
gui_data gui; //gui data

//user notifications (interface UI -> GUI)
//get data
void gui_rdconf(void* conf); //get configuration from file
//set text
void gui_setinfo(char* text, unsigned char color); //set info text
void gui_setfgp(char* text, unsigned char color); //set fgp text
void gui_setsas(char* text, unsigned char color); //set sas text
void gui_setname(char* text, unsigned char color); //set name field
void gui_setaddr(char* text, unsigned char color); //set address field
//set icons
void gui_setnew(unsigned char result); //set result of creating contact
void gui_setchange(unsigned char result); //set rersult of changing contact
void gui_setspeke(unsigned char result); //set result of speke
void gui_setkey(unsigned char result); //set result of key receiving
void gui_setdir(unsigned char result); //set result of direct mode
void gui_setdbl(unsigned char result); //set result of dobling mode
//manage list
void gui_addlist(char* name);  //add name to contact list
void gui_dellist(char* name);  //delete name from contact list
//set state
void gui_setiddle(void); //set iddle state
void gui_setaccept(unsigned char in, unsigned char tor);  //set state after accepting (pre-IKE)
void gui_setcall(unsigned char in, unsigned char tor);  //set work state (in call)
void gui_setterm(void); //set call termination state

//user commands (interface GUI -> UI)
void ui_doinit(char* password);  //create UI
void ui_donew(char* newname);  //create new contact
void ui_dospeke(char* secret); //run SPEKE
void ui_doaddr(char* addr);  //set current address
void ui_dolist(char* mask); //request list output by mask
void ui_dochange(char* name, char* addr);  //change current contact
void ui_docall(char* name, char* addr); //outgoing call
void ui_domute(unsigned char on); //mute/talk
void ui_dodirect(unsigned char on); //allow/deny direct p2p mode
void ui_docancel(void); //terminate call
void ui_doselect(char* name);  //select contact
void ui_dokey(char* name);  //send key to remote



//=========================incoming data processing===========================

//----------------init start----------------------

//========================================================================
//   CR reports: ST inited, returns rand
//========================================================================
void ud_getpsw(unsigned char* data)
{
 //char* pp = (char*)(ui.out+4); //out ptr
 
 //can seed PRNG by random in data
 //password packewt already ready in ui.out
 if(ui.iscall) return;
 if(ui.out[0]!=CR_PSW) return; //check password still valid
 if_setgui(ui.out, 36); //send password to CR
 memset(ui.out, 0, 36);//clear password packet
}

//========================================================================
//   CR returns fingerprint of our public key
//========================================================================
void ud_setfgp(unsigned char* data)
{
 unsigned short tcpport=0;
 char* p=(char*)(data+4);
 char* pp=(char*)(ui.out+4);

 if(ui.iscall) return;


 if(!data[0])
 {
  strcpy(gui.info, "Storage init error!");
  ui_setcommand(GUI_INFO_TEXT);
  ui_setcommand(GUI_INFO_R);
 }

 //set fingerprint
 p[8]=0; //terminate FGP string

 //gui_setfgp(p, 0); //set fingerprint black
 memcpy(gui.fgp, p, sizeof(gui.fgp));
 ui_setcommand(GUI_FGP_TEXT);
 ui_setcommand(GUI_FGP_W);
 
 //output network user's setings
 ui.out[0]=TR_INIT; //TR inii packet type
 ui.out[1]=cf.wan; //listen WAN TCP or Local Tor only
 if(cf.tcp[0]) tcpport=atoi(cf.tcp); //tcp port from string
 ui.out[2]=tcpport&0xFF;
 ui.out[3]=tcpport>>8; //set Tor socks5 port for TR
 strncpy(pp, cf.stun, 32); //stun address
 if_setgui(ui.out, 36); //send
}

//========================================================================
//   TR reports initialization result
//========================================================================
void ud_setnet(unsigned char* data)
{
 if(ui.iscall) return;
 if(data[1])
 {
  ui_setcommand(GUI_FGP_W);
  strcpy(gui.info, "Network error!");
  ui_setcommand(GUI_INFO_TEXT);
  ui_setcommand(GUI_INFO_R);
 }
 else
 {
  ui_setcommand(GUI_FGP_G);
 }
 memset(ui.out, 0, 36);
 ui.out[0]=ST_REQ;  //request of adressbook name entrie
 if_setgui(ui.out, 36); //send
}


//========================================================================
//   ST returns next contact's name for List  (empty is last in book)
//========================================================================
void ud_addlst(unsigned char* data)
{
 char* p=(char*)(data+4); //input
 char* pp=(char*)(ui.out+4); //output par1
 char* ppp=(char*)(ui.out+20); //output par2
 
 if(ui.iscall) return;
 if(p[0]) //next name to list
 {
  //gui_addlist(p); //add contact to list
  memcpy(gui.list, p, sizeof(gui.list));
  ui_setcommand(GUI_LIST_ADD);
  
  if(!data[2]) memset(ui.out, 0, 36); //clear output
  else memcpy(p, ui.name, 16); //or set mask
  
  ui.out[0]=ST_REQ;  //request of adressbook name entrie
  ui.out[3]=0; //clear list restart flag
  if_setgui(ui.out, 36); //send
  cn_cnt=SLEEP_CMD; //restart sleep timer on GUI command 
 }
 else //there was a last contact in list
 {
  ui.name[0]=0;
  ui.addr[0]=0;
  if(!data[2]) //only in app nit
  {
   memset(ui.out, 0, 36); //clear output
   strncpy(pp, cf.mike, 16); //set audio input device
   strncpy(ppp, cf.spk, 16); //set audio output device
   ui.out[0]=CR_AUD; //setup audio interface
   if_setgui(ui.out, 36); //send
   strcpy(gui.info, "Ready!");
  }
  else strcpy(gui.info, "List success");
  ui_setcommand(GUI_STATE_IDDLE);
  ui_setcommand(GUI_INFO_TEXT);
  ui_setcommand(GUI_INFO_G);
  gui.note[0]=0;
 }  //finalize application initialize
}

//------init finish-------------------


//========================================================================
//   ST reports: new contact was created (result)
//========================================================================
void ud_addnew(unsigned char* data)
{
 char* p= (char*)(data+4); //name of contact was created

 if(ui.iscall) return;

 //check result
 if(data[1]== STORAGE_CREATE) //valid
 {
  memcpy(gui.list, p, sizeof(gui.list));
  ui_setcommand(GUI_LIST_ADD);
  //gui_addlist(p); //add new contact to list
  ui_setcommand(GUI_BTNNEW_G);  //notify creating OK
  //gui_setnew(2); //notify creating OK
 }
 else //book fail
 {
  if(data[1]==STORAGE_NOSPACE) strcpy(gui.info, "No space!");// gui_setinfo("No space!", 3);
  else if(data[1]==STORAGE_EXIST) strcpy(gui.info, "Already exist!"); //gui_setinfo("Already exist!", 3);
  else strcpy(gui.info, "Storage error!");// gui_setinfo("Storage error!", 3);
  ui_setcommand(GUI_INFO_TEXT);
  ui_setcommand(GUI_INFO_R);

  //gui_setnew(3); //notify creating fail
 }
}

//contact managment

//========================================================================
//   ST returns address: cases iddle, ike, call
//========================================================================
void ud_setadr(unsigned char* data)
{
 char* p=(char*)(data+4); //input
 char* pp=(char*)(ui.out+4); //output
 static const char suf[8]=".onion:";
 unsigned char fail=0;
 
 
 if(data[1]==STORAGE_MYSELF) strncpy(p, cf.myadr, 32); //set my onion from config
 else if(data[1]!=STORAGE_OK) 
 {
  p[0]=0; //or clear on adressbook error
  if(data[1]==STORAGE_UNINIT) strcpy(gui.info, "Storage not ready!");// gui_setinfo("No space!", 3);
  else if(data[1]==STORAGE_NONAME) strcpy(gui.info, "Empty name!"); //gui_setinfo("Already exist!", 3);
  else if(data[1]==STORAGE_RDFAIL) strcpy(gui.info, "Read error!");
  else if(data[1]==STORAGE_MACFAIL) strcpy(gui.info, "Password error!");
  else if(data[1]==STORAGE_NOTFIND) strcpy(gui.info, "Contact not found!");
  else strcpy(gui.info, "Storage error!");// gui_setinfo("Storage error!", 3);
  fail=1;
  if(!ui.iscall) //for iddle state: select contact only if it is existed
  {
   ui.name[0]=0;
   ui.addr[0]=0; //clear fields
   ui.isset=0;  //clear set flag
 
   gui.name[0]=0;
   ui_setcommand(GUI_NAME_TEXT);
   ui_setcommand(GUI_NAME_W);
   gui.addr[0]=0;
   ui_setcommand(GUI_ADDR_TEXT);
   ui_setcommand(GUI_ADDR_W);
  
 
   ui_setcommand(GUI_INFO_TEXT);
   ui_setcommand(GUI_INFO_R);
   ui_setcommand(GUI_BTNCHG_W);
   return;
  } 
 }


//----------------------DURING CALL: send contact to remote-----------------------------

 //Send selected contact to remote during call
 
 if(ui.istalk) //in call after IKE: send address to remote
 {
  if(!fail) //send only if contact exist
  {
   memcpy(ui.addr, p, 32); //save address of contact be sended
   ui.out[0]=CR_REQK;  //resend adress to CR for output
   memcpy(pp, p, 32);  //output adress of contact be sended
   if_setgui(ui.out, 36); //send
  }
 }

 //----------------------DURING IKE: make doubling call-----------------------------
 //Make outgoing doubling immediately after IKE of acceptor
 else if(ui.iscall) //in call on IKE: make doubling call
 {
  memcpy(gui.addr, p, sizeof(gui.addr));
  ui_setcommand(GUI_ADDR_TEXT);
  ui_setcommand(GUI_ADDR_W);
  
  strncpy(pp, p, 32); //copy adress to output
  ui.out[0]=TR_SET;
  //check is onion and make outgoing doubling call
  if(ui.iscall!=3) p[16]=0; //only if current call is incoming from Onion
  if(p[22] && (p[22]!=':')) p[16]=0; //check port delimiter
  if(!memcmp(p+16, suf, 6)) if_setgui(ui.out, 36); //chack suffix and send for doubling call
  ui.istalk=1; //set talk mode
 }
  //----------------------DURING IDDLE: update address field-----------------------------
 //Update address field in iddle mode
 else //not in call: update adress field
 {
  memcpy(gui.addr, p, sizeof(gui.addr));
  ui_setcommand(GUI_ADDR_TEXT);  //show adress
  ui_setcommand(GUI_ADDR_W);
  //gui_setaddr(p, GUI_COLOR_W); //show adress
  if(data[1]==STORAGE_MYSELF) //not myself!
  {
   ui.isset=0; //clear flag contact set
   //gui_setchange(GUI_SAVE_NO);   //cannt modify myself
   ui_setcommand(GUI_BTNCHG_W); //cannt modify myself
   memcpy(gui.addr, p, sizeof(gui.addr));
  }
  else
  {
   ui.isset=1; //set flag contact set
   //gui_setchange(GUI_SAVE_REQ);   //can modify current contact
   ui_setcommand(GUI_BTNCHG_B); //can modify current contac
  }
 }

}


//========================================================================
//   ST returns current contact's name as result: new adress was set
//          cases: iddle, call
//========================================================================
void ud_addadr(unsigned char* data)
{
 char* p=(char*)(data+4); //input
 char* pp=(char*)(ui.out+4); //output

 if(data[1]!=STORAGE_OK) return;
//------------------DURING CALL: rerquest of allow key saving--------------
 //in Call: on sending contact to remote 
 if(ui.istalk) //in call: request for allow to add new key
 {
  if(!ui.iskey)
  {
   strcpy(gui.info, "Key receiving deny!"); //gui_setinfo("Storage error!", 3);
   ui_setcommand(GUI_INFO_TEXT);
   ui_setcommand(GUI_INFO_B);
   ui_setcommand(GUI_BTNKEY_B);// gui_setkey(GUI_KEY_REQ); //key saving denied
  }
  else ui_setcommand(GUI_BTNKEY_W); //gui_setkey(GUI_KEY_NO); //key saving allowed

  ui.out[0]=CR_ADDK; //allow add new contact from remote
  ui.out[1]=ui.iskey; //allow flag
  if_setgui(ui.out, 36); //send
  return;
 }

 //----------------------DURING IDDLE: change contact--------------------
 if(!ui.iscall)
 {
  //request for change contact alrerady in ui.out
  if(strncmp(pp, p, 16)) return; //compare old name in packet with returned by ST
  if(strncmp(pp, ui.name, 16)) return; //compare old name in packet with saved in UI
  if_setgui(ui.out, 36); //send  request for change contatc
 }
}


//========================================================================
//   ST reports: contact was changed
//========================================================================
void ud_setupd(unsigned char* data)
{

 char* oldname =(char*)(data+4);
 char* newname =(char*)(data+20);

 if(ui.iscall) return;

 gui.name[0]=0;
 ui_setcommand(GUI_NAME_TEXT);
 ui_setcommand(GUI_NAME_W);
 gui.addr[0]=0;
 ui_setcommand(GUI_ADDR_TEXT);
 ui_setcommand(GUI_ADDR_W);

 //gui_setname(0, GUI_COLOR_W); //set name field not yellow
 //gui_setaddr(0, GUI_COLOR_W); //set address field not yellow

 //check result
 if(data[1]!=STORAGE_UPDATE)  //ST errors
 {
   if(data[1]==STORAGE_NOSPACE) strcpy(gui.info, "No space!"); //gui_setinfo("No space!", 3);
   else if(data[1]==STORAGE_EXIST) strcpy(gui.info, "Already exist!"); //gui_setinfo("Already exist!", 3);
   else strcpy(gui.info, "Storage error!"); //gui_setinfo("Storage error!", 3);
   ui_setcommand(GUI_INFO_TEXT);
   ui_setcommand(GUI_INFO_R);
   ui.name[0]=0;
   ui.addr[0]=0; //clear fields
   ui.isset=0;  //clear set flag
   ui_setcommand(GUI_BTNCHG_R); //set result red
   //gui_setchange(GUI_SAVE_FAIL);
   return;
 }
 //ST change OK
 strcpy(gui.info, "Contact changed");
 ui_setcommand(GUI_INFO_TEXT);
 ui_setcommand(GUI_INFO_W);
 //gui_setinfo("Contact changed", 0);
 if(strcmp(oldname, newname)) //check p and p+16 is different
 {    //edit list: remove old contact and add new
  memcpy(gui.del, oldname, sizeof(gui.del));
  ui_setcommand(GUI_LIST_DEL);
  memcpy(gui.list, newname, sizeof(gui.list));
  ui_setcommand(GUI_LIST_ADD);
  //gui_dellist(oldname); //delete old name from list
  //gui_addlist(newname); //add new name to list
  strncpy(ui.name, newname, 16); //save new name
 }
 memcpy(gui.name, ui.name, sizeof(gui.name));
 memcpy(gui.addr, ui.addr, sizeof(gui.addr));
 ui_setcommand(GUI_NAME_TEXT);
 ui_setcommand(GUI_NAME_W);
 ui_setcommand(GUI_ADDR_TEXT);
 ui_setcommand(GUI_ADDR_W);
 ui_setcommand(GUI_BTNCHG_G);
 //gui_setname(ui.name, GUI_COLOR_W);
 //gui_setaddr(ui.addr, GUI_COLOR_W);
 //gui_setchange(GUI_SAVE_OK);
 ui.isset=1;

}

//========================================================================
//   ST reports: contact was copied
//========================================================================
void ud_setcpy(unsigned char* data)
{
 //char* oldname =(char*)(data+4);
 char* newname =(char*)(data+20);

 if(ui.iscall) return;

 gui.name[0]=0;
 ui_setcommand(GUI_NAME_TEXT);
 ui_setcommand(GUI_NAME_W);
 gui.addr[0]=0;
 ui_setcommand(GUI_ADDR_TEXT);
 ui_setcommand(GUI_ADDR_W);

 //gui_setname(0, GUI_COLOR_W); //set name field not yellow
 //gui_setaddr(0, GUI_COLOR_W); //set address field not yellow

 if(data[1]!=STORAGE_COPY)  //ST errors
 {
  if(data[1]==STORAGE_NOSPACE) strcpy(gui.info, "No space!"); //gui_setinfo("No space!", 3);
  else if(data[1]==STORAGE_EXIST) strcpy(gui.info, "Already exist!"); //gui_setinfo("Already exist!", 3);
  else strcpy(gui.info, "Storage error!"); //gui_setinfo("Storage error!", 3);
  ui_setcommand(GUI_INFO_TEXT);
  ui_setcommand(GUI_INFO_R);
  ui.name[0]=0;
  ui.addr[0]=0; //clear fields
  ui.isset=0;  //clear set flag
  ui_setcommand(GUI_BTNCHG_R);
  //gui_setchange(GUI_SAVE_FAIL);  //set result red
  return;
 }
 //changed OK
  strcpy(gui.info, "Contact copied");
 ui_setcommand(GUI_INFO_TEXT);
 ui_setcommand(GUI_INFO_W);
 //gui_setinfo("Contact copied", 0);

 memcpy(gui.list, newname, sizeof(gui.list));
 ui_setcommand(GUI_LIST_ADD);
 //gui_addlist(newname); //add new name to list
 strncpy(ui.name, newname, 16); //save new name

 memcpy(gui.name, ui.name, sizeof(gui.name));
 memcpy(gui.addr, ui.addr, sizeof(gui.addr));
 ui_setcommand(GUI_NAME_TEXT);
 ui_setcommand(GUI_NAME_W);
 ui_setcommand(GUI_ADDR_TEXT);
 ui_setcommand(GUI_ADDR_W);
 ui_setcommand(GUI_BTNCHG_G);
 //gui_setname(ui.name, GUI_COLOR_W);
 //gui_setaddr(ui.addr, GUI_COLOR_W);
 //gui_setchange(GUI_SAVE_OK);
 ui.isset=1;
}

//========================================================================
//   ST reports: contact was deleted
//========================================================================
void ud_setdel(unsigned char* data)
{
 char* oldname =(char*)(data+4);
 //char* newname =(char*)(data+20);

 if(ui.iscall) return;


 gui.name[0]=0;
 ui_setcommand(GUI_NAME_TEXT);
 ui_setcommand(GUI_NAME_W);
 gui.addr[0]=0;
 ui_setcommand(GUI_ADDR_TEXT);
 ui_setcommand(GUI_ADDR_W);
 //gui_setname(0, GUI_COLOR_W); //set name field not yellow
 //gui_setaddr(0, GUI_COLOR_W); //set address field not yellow
 ui.name[0]=0;
 ui.addr[0]=0; //clear fields
 ui.isset=0;  //clear set flag



 if(data[1]!=STORAGE_DELETE)
 {
  strcpy(gui.info, "Storage error!"); //gui_setinfo("Storage error!", 3);
  ui_setcommand(GUI_INFO_TEXT);
  ui_setcommand(GUI_INFO_R);
  ui_setcommand(GUI_BTNCHG_R);
  //gui_setinfo("Storage error!", 3);
  //gui_setchange(GUI_SAVE_FAIL);  //set result red
 }
 else
 {
  memcpy(gui.del, oldname, sizeof(gui.del));
  ui_setcommand(GUI_LIST_DEL);
  ui_setcommand(GUI_BTNCHG_W);
  strcpy(gui.info, "Contact deleted");
  ui_setcommand(GUI_INFO_TEXT);
  ui_setcommand(GUI_INFO_W);

  //gui_dellist(oldname); //delete old name from list
  //gui_setchange(GUI_SAVE_NO);  //set result black
  //gui_setinfo("Contact deleted", 0);
 }

}



//call
//========================================================================
//   CR reports: their public key was set, can start call now
//            Only outgoing call
//========================================================================
void ud_setout(unsigned char* data)
{
 char* pp=(char*)(ui.out+4); //output

 if( (ui.iscall!=1) || ui.istalk) return; //on outgoing call before IKE comleet

 //call to destination address
  strncpy(pp, ui.addr, 32); //use contact address from field
  ui.out[0]=TR_SET; //start outgoing call
  if_setgui(ui.out, 36);//send
}

//========================================================================
//   TR reports: connecting, start preIKE
//========================================================================
void ud_setacc(unsigned char* data)
{
 ui.in=0;
 ui.tor=data[1];  //parameter1: 0-tcp, 1-tor
 if(data[2]==EVENT_ACCEPT_TCP) ui.in=1; //parameter2: accept or connect
 if(ui.in && (!ui.iscall))
 {
  if(ui.tor) ui.iscall=3; else ui.iscall=2; //set call flag (1-outg, 2-inc tcp, 3-inc tor)
  strcpy(gui.note, "Incoming call");
  ui_setcommand(GUI_NOTE_TEXT);
 }

 ui_setcommand(GUI_STATE_IKE); //gui set IKE state
 if(ui.in) //incoming call
 {
  if(ui.tor) strcpy(gui.sas, "<===");  //set text of SAS label depends call type
  else strcpy(gui.sas, "<---");
  gui.name[0]=0;        //clear name and address fields
  ui_setcommand(GUI_NAME_TEXT);
  gui.addr[0]=0;
  ui_setcommand(GUI_ADDR_TEXT);
 }
 else   //outgoing call
 {
  if(ui.tor) strcpy(gui.sas, "===>");  //set text of SAS label depends call type
  else strcpy(gui.sas, "--->");
 }
 ui_setcommand(GUI_SAS_TEXT);  //set SAS label

}

//========================================================================
//   TR reports: incoming party finish IKE
//========================================================================
void ud_setname(unsigned char* data)
{
 unsigned short ss;
 char sas[8];
 short i;
 char* p=(char*)(data+4); //name
 unsigned char au=0;

 if(!ui.iscall) return;
 if(ui.istalk) return;

 //check Diffie-Hellman key exchange is OK
 if(data[1]==CR_DH_FAIL)  //commitment error
 {
  strcpy(gui.info, "Security fail!");
  ui_setcommand(GUI_INFO_TEXT);
  ui_setcommand(GUI_INFO_R);
  ui_setcommand(GUI_INFO_R);
  ui_setcommand(GUI_STATE_TERM);

  //gui_setinfo("Security fail!", 3);
  //gui_setterm();
  ui.out[0]=CR_RST;  //terminate call
  if_setgui(ui.out, 36);//send
 }

 //set SAS
 ss=data[2];
 ss=ss<<8;
 ss+=data[3];
 for(i=0;i<4;i++) //convert 2 bytes of ss to hex
 {
  sas[i]='0'+(ss&0x0F); //4 bits to hex digit
  if(sas[i]>'9') sas[i]+=7; //correct for A-F
  ss>>=4; //move to next 4 bits
 }
 sas[4]=0;
 if(!data[1]) au=1;

 ui_setcommand(GUI_STATE_CALL);
 memcpy(gui.sas, sas, sizeof(gui.sas));
 ui_setcommand(GUI_SAS_TEXT);

 if(ui.in)
 {
  if(ui.tor) ui_setcommand(GUI_BTNCLL_G);
  else ui_setcommand(GUI_BTNCLL_B);
  ui_setcommand(GUI_BTNTRM_B);
 }
 else
 {
  ui_setcommand(GUI_BTNCLL_R);
  ui_setcommand(GUI_BTNTRM_R);
 }

 memcpy(gui.name, p, sizeof(gui.name));
 ui_setcommand(GUI_NAME_TEXT);
 if(au) ui_setcommand(GUI_NAME_G);
 else ui_setcommand(GUI_NAME_W);
 //gui_setcall(ui.in, ui.tor);   //set name
 //gui_setsas(sas, GUI_COLOR_W);
 //if(au) gui_setname(p, GUI_COLOR_G); else gui_setname(p, GUI_COLOR_W);

 //check incoming call in progress and authentication OK
 //and request adreess of incoming contact
 if( au && (ui.iscall>1)) //au ok and this is incoming call
 {
  memcpy(ui.out, data, 36); //incoming name
  ui.out[0]=ST_GETADR; //request address of incoming contact
  if_setgui(ui.out, 36);//send
 }
 else ui.istalk=1; //or send talk mode now

}

//========================================================================
//   TR reports: outgoing party finish IKE
//========================================================================
void ud_setcall(unsigned char* data)
{
 unsigned short ss;
 char sas[8];
 short i;
 unsigned char au=0;

 if(!ui.iscall) return;
 if(ui.istalk) return;

 //check Diffie-Hellman key exchange is OK
 if(data[1]==CR_DH_FAIL)  //commitment error
 {
  strcpy(gui.info, "Security fail!");
  ui_setcommand(GUI_INFO_TEXT);
  ui_setcommand(GUI_INFO_R);
  ui_setcommand(GUI_INFO_R);
  ui_setcommand(GUI_STATE_TERM);
  //gui_setinfo("Security fail!", 3);
  //gui_setterm();
  ui.out[0]=CR_RST;  //terminate call
  if_setgui(ui.out, 36);//send
 }

 //Check current adress is onion and set field green:
 //You are shure you actually connected to this adress
 //(while you trust Tor)
 if(strstr(ui.addr, (char*)".onion")) ui_setcommand(GUI_ADDR_G); //  gui_setaddr(0, GUI_COLOR_G);

 //set SAS
 ss=data[2];
 ss=ss<<8;
 ss+=data[3];
 for(i=0;i<4;i++) //convert 2 bytes of ss to hex
 {
  sas[i]='0'+(ss&0x0F); //4 bits to hex digit
  if(sas[i]>'9') sas[i]+=7; //correct for A-F
  ss>>=4; //move to next 4 bits
 }
 sas[4]=0;
 if(!data[1]) au=1;

 ui_setcommand(GUI_STATE_CALL);
 memcpy(gui.sas, sas, sizeof(gui.sas));
 ui_setcommand(GUI_SAS_TEXT);

 if(ui.in)
 {
  if(ui.tor) ui_setcommand(GUI_BTNCLL_G);
  else ui_setcommand(GUI_BTNCLL_B);
  ui_setcommand(GUI_BTNTRM_B);
 }
 else
 {
  ui_setcommand(GUI_BTNCLL_R);
  ui_setcommand(GUI_BTNTRM_R);
 }
 
 if(au) ui_setcommand(GUI_NAME_G);
 //gui_setcall(ui.in, ui.tor);
 //gui_setsas(sas, GUI_COLOR_W);
 //if(au) gui_setname(0, GUI_COLOR_G);
 ui.istalk=1; //set call mode after IKE
}

//in call

//**************************speke***************************************
//========================================================================
//   TR reports remote SPEKE request
//========================================================================
void ud_getsec(unsigned char* data)
{
 if(!ui.istalk) return;
 ui_setcommand(GUI_BTNSPK_B); //speke request
 //gui_setspeke(GUI_SPEKE_REQ);
}

//========================================================================
//   CR reports SPEKE result
//========================================================================
void ud_setsec(unsigned char* data)
{
 if(!ui.istalk) return;
 //show result
 if(data[1]) ui_setcommand(GUI_BTNSPK_R); //speke fail  gui_setspeke(GUI_SPEKE_FAIL);
 else ui_setcommand(GUI_BTNSPK_G); //speke OK  //gui_setspeke(GUI_SPEKE_OK);
 //check need answer
 if(data[2]) //check need answer to remote
 {
  memcpy(ui.out, data, 36);
  ui.out[0]=TR_Q;
  ui.out[1]=0;    //clear result flag
  if_setgui(ui.out, 36);//resend to remote
 }

}

//key send
//========================================================================
//   CR reports address is redy for sending, request key
//========================================================================
void ud_setkey(unsigned char* data)
{
 char* pp=(char*)(ui.out+4); //output
 if(!ui.istalk) return;

 memset(ui.out, 0, 36);
 strncpy(pp, ui.name, 16);  //contact name
 ui.out[0]=ST_GETKEY; //request key
 ui.out[2]=1; //flag of send key on call
 if_setgui(ui.out, 36); //send
}

//========================================================================
//   ST returns name of new contact created for receiving key + address
//========================================================================
void ud_getkey(unsigned char* data)
{
 char* p = (char*)(data+4);

 if(!ui.istalk) return;
 if(data[1]==STORAGE_CREATE)  //new contact add sucess
 {
  strcpy(gui.info, "New key received!");
  ui_setcommand(GUI_INFO_TEXT);
  ui_setcommand(GUI_INFO_G);
  //gui_setinfo("New key received!", 2);
  memcpy(gui.list, p, sizeof(gui.list));
  ui_setcommand(GUI_LIST_ADD);
  ui_setcommand(GUI_BTNKEY_G);

  //gui_addlist(p);
  //gui_setkey(GUI_KEY_OK);
 }
 else
 {
  if(data[1]==STORAGE_NOSPACE) strcpy(gui.info, "No space!"); //gui_setinfo("No space!", 3);
  else if(data[1]==STORAGE_EXIST) strcpy(gui.info, "Already exist!"); //gui_setinfo("Already exist!", 3);
  else if(data[1]==STORAGE_NONAME) strcpy(gui.info, "Name already exist!"); //gui_setinfo("Name already exist!", 3);
  else if(data[1]==STORAGE_WRFAIL) strcpy(gui.info, "ST+ write error!"); //gui_setinfo("ST+ write error!", 3);
  ui_setcommand(GUI_INFO_TEXT);
  ui_setcommand(GUI_INFO_R);
  ui_setcommand(GUI_BTNKEY_R);
  
  //gui_setkey(GUI_KEY_FAIL);
 }
}



//========================================================================
//   TR reports calle anwser
//========================================================================
void ud_setans(unsigned char* data)
{
  if(!ui.istalk) return;
  ui_setcommand(GUI_NOTE_ANS); //notify GUI for callee accept
}

//========================================================================
//   TR reports change of double state
//========================================================================
//data[1] is 0 for originator, 1 for accept ovet TCP, 2 for accept over Tor
//3-close out, 4-xlose in
void ud_setdbl(unsigned char* data)
{
 if(!ui.istalk) return;
 if(!data[1]) ui_setcommand(GUI_ADDR_G); //gui_setaddr(0, GUI_COLOR_G); //set their onion adress valid for connect outgoing doble

 //set doubling state

 if(data[1]==3) ui_setcommand(GUI_BTNTRM_B);// gui_setdbl(GUI_DBL_IN); //doubling disabled: only incoming work now icon=Blue
 else if(data[1]==4) ui_setcommand(GUI_BTNTRM_R); //gui_setdbl(GUI_DBL_OUT); //doubling disabled: only outgoing work now: icon=Black
 else ui_setcommand(GUI_BTNTRM_G); //gui_setdbl(GUI_DBL_BOTH);//0 or 2: new outgoing or Tor incoming ready: doubling enabled now: icon=Green
}


//========================================================================
//   TR reports change of direct call state
//========================================================================
void ud_setdir(unsigned char* data)
{
 if(!ui.istalk) return;

 //buf[1]: 0-req, 1-deny, 2-active : set button color
 if(data[1]==2) ui_setcommand(GUI_BTNDIR_G);// gui_setdir(GUI_DIR_OK);//double active
 else if(data[1]==1) ui_setcommand(GUI_BTNDIR_W); //gui_setdir(GUI_DIR_DENY); //double denied
 else ui_setcommand(GUI_BTNDIR_B); //gui_setdir(GUI_DIR_REQ); //double remote request
}

//========================================================================
//   CR reports reset
//========================================================================
void ud_setrst(unsigned char* data)
{
 memset(&ui, 0, sizeof(ui)); //clear UI data
 memcpy(ui.sid, data+4, 32); //set new seed
 ui_setcommand(GUI_STATE_IDDLE);
 gui.note[0]=0;
 //gui_setiddle(); //reset GUI to iddle state
}


//*******************************************************************
//User commands (interface GUI -> UI) calls from GUI (in GUI context)
//*******************************************************************

//===================================================================
//Start UI
//==================================================================
void ui_doinit(char* password)  //init UI
{
 unsigned short torport=0;
 char* pp = (char*)(ui.out+4); //out ptr
 
 memset(&ui, 0, sizeof(ui)); //clear UI data
 memset(&gui, 0, sizeof(gui)); //clear GUI data
 //gui_rdconf(&cf); //read config data
 if(cf.tor[0]) torport=atoi(cf.tor); //tor port from string
 //compose PTH packet send now
 ui.out[2]=torport&0xFF;
 ui.out[3]=torport>>8; //set Tor socks5 port for TR
 strncpy(pp, cf.com, 32); //output book path
 ui.out[0]=CR_PTH; //set packet type is book path
 if_setgui(ui.out, 36); //send book path for TF thread
 //compose PSW packet for next
 memset(pp, 0, 32);
 strncpy(pp, password, 32); //save password for next packet
 ui.out[0]=CR_PSW; //set packet type password
 memset(password, 0, 32); //clear password from GUI
 cn_cnt=SLEEP_CMD; //restart sleep timer on GUI command
}

//===================================================================
//Create new contact by name
//==================================================================
void ui_donew(char* newname)  //create new contact
{
 char* pp=(char*)(ui.out+4); //output
 if(ui.iscall) return; //only in iddle mode

 if((newname[0]<='*')||(newname[0]=='=')) return;
 memset(ui.out, 0, 36);
 ui.out[0]=ST_NEW;
 strncpy(pp, newname, 16);
 if_setgui(ui.out, 36); //send
 cn_cnt=SLEEP_CMD; //restart sleep timer on GUI command
}

//===================================================================
//Select contact by name
//==================================================================
void ui_doselect(char* name)  //select contact
{
 char* pp=(char*)(ui.out+4); //output

 if(ui.iscall) return; //only in iddle mode

 strncpy(ui.name, name, 16); //save name of selected contact
 memset(ui.addr, 0, 32); //clear address of selected contact

 memcpy(gui.name, ui.name, sizeof(gui.name));
 ui_setcommand(GUI_NAME_TEXT);
 ui_setcommand(GUI_NAME_W);
 //memcpy(gui.addr, ui.addr, sizeof(gui.addr));
 //ui_setcommand(GUI_ADDR_TEXT);
 //ui_setcommand(GUI_ADDR_W);
 //ui_setcommand(GUI_BTNNEW_B);

 //request address of selected contact
 memset(ui.out, 0, 36);
 strncpy(pp, name, 16);
 ui.out[0]=ST_GETADR;  //request address
 if_setgui(ui.out, 36); //send
 cn_cnt=SLEEP_CMD; //restart sleep timer on GUI command
}

//===================================================================
//Set address
//==================================================================
void ui_doaddr(char* addr)  //set current address
{
 memcpy(ui.addr, addr, sizeof(ui.addr));
}

//===================================================================
//Request output of contact list by mask
//==================================================================
void ui_dolist(char* mask) //request list output by mask 
{
 memcpy(ui.name, mask, sizeof(ui.name)); //save mask to name field
 memcpy(ui.out+4, ui.name, sizeof(ui.name)); //set mask for first request
 ui.out[0]=ST_REQ;  //request of adressbook contact list
 ui.out[1]=0; //clear error code
 ui.out[2]=1; //set flag: no init sequence after list (for all request)
 ui.out[3]=1; //set flag: require book init (only for first request) 
 if_setgui(ui.out, 36); //send
 cn_cnt=SLEEP_CMD; //restart sleep timer on GUI command
}


//===================================================================
//Change selected contact to new name and address
//==================================================================
void ui_dochange(char* name, char* addr)  //change current contact
{
  char* pp=(char*)(ui.out+4); //output old name
  char* ppp=(char*)(ui.out+20); //output new name

  //set address of current contact
  if(ui.iscall) return; //only in iddle mode
  if(!ui.isset) return; //check contact is set
  ui.isset=0; //clear flag: contact in work now
  //gui_setchange(0); //deny changes (for result)
  ui_setcommand(GUI_BTNCHG_W);
  ui_setcommand(GUI_BTNNEW_B); 
  if(addr) strncpy(ui.addr, addr, 32); //save new address
  strncpy(pp, ui.addr, 32); //output new address
  ui.out[0]=ST_ADR;
  if_setgui(ui.out, 36); //send address first

  //prepare next packet with ol name/new name
  memset(ui.out, 0, 36);
  strncpy(pp, ui.name, 16); //old name
  strncpy(ppp, name, 16); //new name
  if(!name[0]) ui.out[0]=ST_DEL; //new name is empty: delete
  else if(name[0]=='=') ui.out[0]=ST_COPY; //new name is alias: copy
  else ui.out[0]=ST_SAVE; //new name specified: change name or adress
  cn_cnt=SLEEP_CMD; //restart sleep timer on GUI command
}

//===================================================================
//Start outgoing call
//==================================================================
void ui_docall(char* name, char* addr) //outgoing call
{
  char* pp=(char*)(ui.out+4); //output

  if(ui.iscall) return; //only in iddle mode
 //initiate outgoing call
  ui.iscall=1; //set call mode: outgoing
  ui.istalk=0;  //flag talk is inactive
  ui_setcommand(GUI_STATE_TERM);
  //gui_setterm(); //set GUI state: init outgoing call
  //save name and address for call
  strncpy(ui.name, name, 16);
  if(addr) strncpy(ui.addr, addr, 32);
  //send contact name from field to ST
  memset(ui.out, 0, 36);
  strncpy(pp, name, 16);
  ui.out[0]=ST_GETKEY;  //request key for setup outgoing call
  ui.out[2]=0; //no-call mode: initiate outgoing call
  if_setgui(ui.out, 36);//send
  cn_cnt=SLEEP_CMD; //restart sleep timer on GUI command
}

//===================================================================
//Change talk|mute in call
//==================================================================
void ui_domute(unsigned char on) //mute/talk
{
 if(!ui.istalk) return;
 //if(on) strcpy(gui.info, "Talk");
 //else strcpy(gui.info, "Mute");
 //ui_setcommand(GUI_INFO_TEXT);
 //ui_setcommand(GUI_INFO_W);
 ui.out[0]=CR_MUTE; //type
 ui.out[1]=on; //on/off voice
 if_setgui(ui.out, 36); //send
 cn_cnt=SLEEP_CMD; //restart sleep timer on GUI command
}

//===================================================================
//Initiale SPEKE in call
//==================================================================
void ui_dospeke(char* secret) //run SPEKE
{
 char* pp=(char*)(ui.out+4); //output

 if(!ui.istalk) return;
 if(secret[0])
 {
  strncpy(pp, secret, 32);
  ui.out[0]=CR_SPEKE;
  if_setgui(ui.out, 36); //send
 }
 cn_cnt=SLEEP_CMD; //restart sleep timer on GUI command
}


//===================================================================
//Change direct call status
//==================================================================
void ui_dodirect(unsigned char on) //allow/deny direct p2p mode
{
 if(!ui.istalk) return;
 if(on) ui_setcommand(GUI_BTNDIR_R);// gui_setdir(GUI_DIR_ALLOW);
 else ui_setcommand(GUI_BTNDIR_W); //gui_setdir(GUI_DIR_DENY);
 memset(ui.out, 0, 36);
 ui.out[0]=TR_DIR;
 ui.out[1]=on; //direct allow/deny flag
 if_setgui(ui.out, 36);//send
 cn_cnt=SLEEP_CMD; //restart sleep timer on GUI command
}

//===================================================================
//Send contact to remote
//==================================================================
void ui_dokey(char* name)  //send key to remote
{
 char* pp=(char*)(ui.out+4); //output
 if(!ui.istalk) return;
 
 //request address of selected contact
 memset(ui.out, 0, 36);
 strncpy(ui.name, name, 16);  //save name of sended contact
 strncpy(pp, name, 16);
 ui.out[0]=ST_GETADR;  //request address
 if_setgui(ui.out, 36); //send
 cn_cnt=SLEEP_CMD; //restart sleep timer on GUI command
}

//===================================================================
//Terminate call by user
//==================================================================
void ui_docancel(void) //terminate call
{
 //gui_setterm();
 ui_setcommand(GUI_STATE_TERM);
 memset(ui.out, 0, 4);
 memcpy(ui.out+4, ui.sid, 32); //send seed to CR
 ui.out[0]=CR_RST;
 if_setgui(ui.out, 36);//send
 cn_cnt=SLEEP_CMD; //restart sleep timer on GUI command
}


//===================================================================
//Receive keys allowing
//==================================================================
void ui_dokeyrcvd(unsigned char on) 
{
 ui.iskey=on;
}



//===================================================================
//Set GUI configuration to UI
//==================================================================
void ui_setconfig(void* cfg)
{ //called from GUI to pass configuration to UI
 if(cfg) memcpy(&cf, cfg, sizeof(cf));
 else //or set default config
 {
  strcpy(cf.myadr, (char*)"?"); //our onion address (will be loaded from HS)
  strcpy(cf.tcp, (char*)"4444"); //TCP listener (part of our address)
  cf.wan=1; //allow listen World Internet
  strcpy(cf.tor, (char*)"9055"); //Tor SOCKS5 port
  strcpy(cf.stun, (char*)"stun.ekiga.net"); //STUN server (only for optional NAT traversal)
  strcpy(cf.com, (char*)"bb"); //Storage file
  strcpy(cf.mike, (char*)"plughw:0,0"); //audio input device
  strcpy(cf.spk, (char*)"plughw:0,0"); //oudio output device 
 }
}



//===================================================================
//Add new command for GUI to pipe
//==================================================================
void ui_setcommand(unsigned char cmd)
{  //called from internal ui.c routines 
 gui.cmd[gui.in]=cmd; //add new command to pipe
 gui.in++;  //move pointer to unprocessed commands
 gui.in&=0x0F; //ring for pipe size for 16 commads
}


//===================================================================
//Loop of packets from TF  and commands for GUI processing
//==================================================================
unsigned char ui_getcommand(char* par)
{  //called from GUI C++ code
 short len;
 unsigned char type;
 unsigned char cmd=0;

 len=if_getgui(ui.buf);  //receive data from torfone level over UI interface
 if(len)  //check data packet was received
 {
  type=ui.buf[0]; //check packet type
  switch(type)  //process packet by it's type
  {
   case(UI_INI):{ud_getpsw(ui.buf); break;} //rand from CR
   case(UI_FGP):{ud_setfgp(ui.buf); break;} //our fingerprint from CR
   case(UI_TCP):{ud_setnet(ui.buf); break;} //result from TR of TCP binding
   case(UI_LST):{ud_addlst(ui.buf); break;} //next contact from ST for list
   //contact managment
   case(UI_ADR):{ud_setadr(ui.buf); break;}   //Adress from ST: set adress field or call doubling or send to remote or
   case(UI_NEWOK):{ud_addnew(ui.buf); break;} //new contact crested from ST
   case(UI_ADROK):{ud_addadr(ui.buf); break;} //adress saved in ST: save contact or save remote key
   case(UI_SAVEOK):{ud_setupd(ui.buf); break;} //change contact notification
   case(UI_COPYOK):{ud_setcpy(ui.buf); break;} //coping contact notification
   case(UI_DELOK):{ud_setdel(ui.buf); break;} //deleting contact notification

   //key send
   case(UI_REQ):{ud_setkey(ui.buf); break;} //set name for sending contact to remote
   case(UI_KEY):{ud_getkey(ui.buf); break;} //notify received contact name
   //speke
   case(UI_SPREQ):{ud_getsec(ui.buf); break;} //notify SPEKE request
   case(UI_SPEKE):{ud_setsec(ui.buf); break;} //notify SPEKE result, optionally send auth to remote
   //call
   case(UI_ACCEPT):{ud_setacc(ui.buf); break;} //notify in-call state
   case(UI_OUT):{ud_setout(ui.buf); break;}  //start outgoing call
   case(UI_NAME):{ud_setname(ui.buf); break;} //set incoming name and au result, start doubling
   case(UI_CALL):{ud_setcall(ui.buf); break;} //set outgoing au result
   //in call
   case(UI_ANSWER):{ud_setans(ui.buf); break;}  //notify onion authentication
   case(UI_DBL):{ud_setdbl(ui.buf); break;}  //notify onion authentication
   case(UI_DIR):{ud_setdir(ui.buf); break;}  //notify direct connection
   case(UI_RST):{ud_setrst(ui.buf); break;} //reset
  } //switch
 }  //if(len)

 //check we have unprocessed commands for GUI level in pipe
 if(gui.in!=gui.out)
 {
  cmd=gui.cmd[gui.out]; //extract command type
  switch(cmd)
  {  //copy text of field for some commands
   case GUI_NOTE_TEXT:{memcpy(par, gui.note, sizeof(gui.note));break;} //name edit
   case GUI_NAME_TEXT:{memcpy(par, gui.name, sizeof(gui.name));break;} //name edit
   case GUI_ADDR_TEXT:{memcpy(par, gui.addr, sizeof(gui.addr));break;} //address edit
   case GUI_FGP_TEXT:{memcpy(par, gui.fgp, sizeof(gui.fgp));break;} //fingerprint label
   case GUI_SAS_TEXT:{memcpy(par, gui.sas, sizeof(gui.sas));break;} //sas label
   case GUI_INFO_TEXT:{memcpy(par, gui.info, sizeof(gui.info));break;} //info label
   case GUI_LIST_ADD:{memcpy(par, gui.list, sizeof(gui.list));break;} //list add field
   case GUI_LIST_DEL:{memcpy(par, gui.del, sizeof(gui.del));break;} //list del field
  }
  gui.out++; //set output pointer to next command
  gui.out&=0x0F; //ring for pipe size for 16 commads
 }
 return cmd;
}


//suspend excuting of the thread  for paus msec
void psleep(int paus)
{
 #ifdef _WIN32
    Sleep(paus);
 #else
    usleep(paus*1000);
 #endif
}


void tf_loop(void)
{
       //run modules in loop
      jb=ui_loop();  //GUI
      jb+=cr_loop(); //Cryptor
      jb+=st_loop(); //Storage
      jb+=tr_loop(); //Transport
      jb+=au_loop(); //Voice

       //check job and sleep thread
      if(!jb) //if no job
      {
       if(cn_cnt) cn_cnt--;  //downcount timer
       else if(ui.iscall) psleep(SLEEP_TIME); //or suspend thread on 10 mS
       else psleep(SLEEP_IDDLE);
      } else cn_cnt=SLEEP_DELAY; //restart sleep timer on any job
}
