//---------------------------------------------------------------------------
#ifdef _WIN32

//#pragma hdrstop

//---------------------------------------------------------------------------

//#pragma argsused

 #include <stdlib.h>
 #include <stdio.h>
 #include <stddef.h>
 #include <basetsd.h>
 #include <stdint.h>
 #include <windows.h>
 #include <time.h>
 #include <conio.h>
 #include <string.h>
 #include "memory.h"
 #include "math.h"
 #include <sys/stat.h>
 #include <sys/types.h>

 #include <tchar.h>
  
 #else
 
 #include <stdlib.h>
 #include <stdio.h>
 #include <string.h>
 #include <time.h>
 #include "memory.h"
 #include "math.h"
 
 #include <sys/time.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 
 #endif

 
 
#define APP_VER (char*)"Torfone_V4.1_http://torfone.org" 

#include <stdarg.h> 
#include "whereami.h"
#include "telnet.h"
#include "if.h"    //general sedinitions
#include "ui.h"   //interface GUI to UI


void ui_doinit(char* password);  //create UI
void ui_dotimer(void); //process data sended to UI in context of GUI thread
void ui_donew(char* newname);  //create new contact
void ui_dospeke(char* secret); //run SPEKE
void ui_dochange(char* name, char* addr);  //change current contact
void ui_docall(char* name, char* addr); //outgoing call
void ui_domute(unsigned char on); //mute/talk
void ui_dodirect(unsigned char on); //allow/deny direct p2p mode
void ui_docancel(void); //terminate call
void ui_doselect(char* name);  //select contact
void ui_dokey(char* name);  //send key to remote
void ui_dokeyrcvd(unsigned char on); //aloow receiving keys
unsigned char ui_getcommand(char* par); 
void ui_setconfig(void* cfg); //set GUI configuration to UI 
void tf_loop(void); //application main loop 


void tf_setname(char* name);
void tf_setaddr(char* addr);

int tf_cnt=0;
int is_run=1;
char tf_name[16]={0,};
char tf_addr[32]={0,};
char tlport[8]={0,};
unsigned short tport=0;
char tlbuf[128]={0,};
char pwd[512];
int pwdlen=0;
unsigned char rcvd=0;
unsigned char rtor=0;
ui_conf cf; //pointer to configuration structure
char* cf_ptr =(char*)&cf;


typedef union
{
 const unsigned char b[64];
 const unsigned int d[16];
}URC_data0;

URC_data0 URC =
{
{'C','R','E','A', //CREATE:name
'S','E','L','E', //SELECT:name
'S','E','T','A', //SETADDR:addr
'S','E','T','N', //SETNAME:name
'S','A','V','E', //SAVE
'C','A','L','L', //CALL
'D','I','R','E', //DIRECT
'P','I','N','G', //PING
'S','E','N','D', //SEND:name
'S','P','E','K', //SPEKE:secret
'R','E','S','E', //RESET
'E','X','I','T', //EXIT
'C','O','N','F', //CONFIG
'T','A','L','K', //TALK
'L','I','S','T', //LIST:mask
'A','D','D','R'} //ADDR:addr
};

//Helpers prototypes
void showconf(void);
void command(char* str);
void tf_printf(char* s, ...);
void tf_setname(char* name);
void tf_setaddr(char* addr);


//process command received from Telnet client
void command(char* str)
{
 unsigned int u;
 char* p=0;
 char buf[32];
 
 u=(*(unsigned int*)str); //command type
 printf("\r\nProcess command: %s\r\n", str);
 p=strchr(str, ' '); //search first space separated command from parameter
 buf[0]=0; //clear parameter
 if(p) //if space is in staring
 {
  p[0]=0; //separate command from parameter
  strncpy(buf, ++p, 32); //copy parameter string (up to 31 chars) to buffer
 }
  //check command type and process it
  if(u==URC.d[0])ui_donew(buf); //CREATE:name
  else if(u==URC.d[1])ui_doselect(buf); //SELECT:name
  else if(u==URC.d[2])tf_setaddr(buf); //SETADDR:adr
  else if(u==URC.d[3])tf_setname(buf); //SETNMAME:name
  else if(u==URC.d[4])ui_dochange(tf_name, tf_addr); //SAVE
  else if(u==URC.d[5])ui_docall(tf_name, tf_addr); //CALL
  else if(u==URC.d[6])ui_dodirect(buf[0]!='0');  //DIRECT
  else if(u==URC.d[7])tf_printf("PONG\r\n");//PING
  else if(u==URC.d[8])ui_dokey(buf); //SEND:name
  else if(u==URC.d[9])ui_dospeke(buf); //SPEKE:secret
  else if(u==URC.d[10])ui_docancel(); //RESET
  else if(u==URC.d[11])is_run=0; //EXIT
  else if(u==URC.d[12])showconf(); //CONFIG
  else if(u==URC.d[13])ui_domute(buf[0]!='0');
  else if(u==URC.d[14])ui_dolist(buf); //LIST:mask
  else if(u==URC.d[15])ui_doaddr(buf); //ADDR:addr
}


//output like printf to stdout and to telnet port
void tf_printf(char* s, ...)
{
    char st[256];
    va_list ap;

    va_start(ap, s);  //parce arguments
    vsprintf(st, s, ap); //printf arg list to array
    printf("%s", st);  //out to stdout
    tn_write((unsigned char*)st, strlen(st)); //out to control port
    va_end(ap);  //clear arg list
    return;
}

//set name field by user
void tf_setname(char* name)
{
 strncpy(tf_name, name, sizeof(tf_name));
 tf_printf("Set new name: %s\r\n", tf_name);
}

//set address field by user
void tf_setaddr(char* addr)
{
 strncpy(tf_addr, addr, sizeof(tf_addr));
 tf_printf("Set new addr: %s\r\n", tf_addr);
}

/*
#ifndef __WIN32
int kbhit()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

	//poll keyboard for user commands
	if(kbhit()) //check char was inputted
	{
	 c = getc(stdin); //get char
	 if((c==0x0D)||(c==0x0A)) //check is the Enter
	 {	  
	  str[ptr]=0;
	  if(ptr) command(str);
	  ptr=0;
	  str[ptr]=0;
	 }
	 else if((ptr<127)&&(c>=0x20)) str[ptr++]=c;//collect up to 127 chars in strings 
	}


#endif
*/

//show current configuration
void showconf(void)
{
 pwd[pwdlen]=0;
 tf_printf("app ver=%s\r\n", APP_VER);
 tf_printf("app dir=%s\r\n", pwd);
 tf_printf("my_address=%s\r\n", cf.myadr);
 tf_printf("tcp port=%s wan=%d\r\n", cf.tcp, cf.wan);
 tf_printf("tor port=%s\r\n", cf.tor);
 tf_printf("stun=%s\r\n", cf.stun);
 tf_printf("storage=%s\r\n", cf.com);
 tf_printf("mike=%s\r\n", cf.mike);
 tf_printf("speaker=%s\r\n", cf.spk);
 tf_printf("rcvd keys=%d\r\n", rcvd);
 tf_printf("run Tor=%d\r\n", rtor);
 tf_printf("telnet listen port=%s\r\n", tlport);
}

//command line parameter can be password
int main(int argc, char* argv[])
{
  //char str[128];
  //int ptr=0;
  char buf[32]; //work buffer
  char par[4];
  unsigned char cmd=0; //command type
  char torcmd[1024];
  FILE * pFile;
  int i, len;
  //char c;
  tf_cnt=0;
  
 
 printf("----------------------------------------------------------------------\r\n");
 printf("   TorFone v4.1a  Van Gegel, 2019  MailTo: torfone@ukr.net\r\n");
 printf("   VOIP over Tor tool with own encrypting and authentication\r\n");
 printf("TorFone is developed independently from the Tor anonymity software\r\n");
 printf("        and carries no guarantee from The Tor Project\r\n");
 printf("TorFone uses onion network as a transport and inherits Tor security\r\n");
 printf("while also adding an independent layer of encryption and authentication\r\n"); 
 printf("-----------------------------------------------------------------------\r\n");
     
  //get current dir  
  //get path of Torfone executable:
  //////getcwd(pwd, sizeof(pwd)); //get over getcwd 
  //more accurace use special crossplatform interface
  i=wai_getModulePath(NULL, 0, NULL); //get len of path
  if(i<(sizeof(pwd)-64)) wai_getModulePath(pwd, i, &len); //get path+exec_name
  if(!len) pwd[0]=0;
  else for(i=len; i>=0; i--) if((pwd[i]==92)||(pwd[i]==47)) break; //search last path delimiter
  if(i) pwd[i]=0; //skip name (we need path_to_directory only)
  pwdlen=strlen(pwd); //len of actual path
 
 //create tf_data directory
  strncpy(pwd+pwdlen, (char*)"/tf_data", sizeof(pwd)-pwdlen); //set directory path
#ifdef __WIN32
  mkdir(pwd);
#else  
  mkdir(pwd, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);  
#endif


//set default config
  strcpy(cf.myadr, (char*)"?"); //our onion address (will be loaded from HS)
  rcvd=1; //allow receive contacts
  rtor=1; //run Tor
  strcpy(cf.tcp, (char*)"4444"); //TCP listener (part of our address)
  cf.wan=1; //allow listen World Internet
  strcpy(cf.tor, (char*)"9055"); //Tor SOCKS5 port
  strcpy(cf.stun, (char*)"stun.ekiga.net"); //STUN server (only for optional NAT traversal)
  strcpy(cf.com, (char*)"bb"); //Storage file
  strcpy(cf.mike, (char*)"plughw:0,0"); //audio input device
  strcpy(cf.spk, (char*)"plughw:0,0"); //oudio output device
  strcpy(tlport, (char*)"8023"); //telnet port

//try load config from file
  strncpy(pwd+pwdlen, (char*)"/tf_data/tf.ini", sizeof(pwd)-pwdlen); //set config path+file
  pFile = fopen (pwd,"rt"); //try open for read
  if(pFile) //opened 
  {
   //read from file
   fgets(cf.myadr, sizeof(cf.myadr), pFile);
   fgets(par, sizeof(par), pFile); if(par[0]=='1') rcvd=1; else rcvd=0; 
   fgets(par, sizeof(par), pFile); if(par[0]=='1') rtor=1; else rtor=0;
   fgets(cf.tcp, sizeof(cf.tcp), pFile);
   fgets(par, sizeof(par), pFile); if(par[0]=='1') cf.wan='1'; else cf.wan='0';
   fgets(cf.tor, sizeof(cf.tor), pFile);
   fgets(cf.stun, sizeof(cf.stun), pFile);
   fgets(cf.com, sizeof(cf.com), pFile);
   fgets(cf.mike, sizeof(cf.mike), pFile);
   fgets(cf.spk, sizeof(cf.spk), pFile);
   fgets(tlport, sizeof(tlport), pFile);
   fclose (pFile);
   pFile=0;
   printf("Config file read OK\r\n");
   
   //eleminate non-printable
   for(i=0;i<sizeof(cf);i++) if(cf_ptr[i]<' ') cf_ptr[i]=0;
   cf.wan&=1; //set flag binary instead digit
  }
  else //save default configuration
  {
   pFile = fopen (pwd,"wt"); //try open config file for write
   if(pFile)
   {
    //save config
    fprintf(pFile, "%s\n", cf.myadr); 
    if(rcvd) fprintf(pFile, "1\n"); else fprintf(pFile, "0\n");
    if(rtor) fprintf(pFile, "1\n"); else fprintf(pFile, "0\n");
    fprintf(pFile, "%s\n", cf.tcp);
    if(cf.wan=='1') fprintf(pFile, "1\n"); else fprintf(pFile, "0\n");
    fprintf(pFile, "%s\n", cf.tor);
    fprintf(pFile, "%s\n", cf.stun);
    fprintf(pFile, "%s\n", cf.com);
    fprintf(pFile, "%s\n", cf.mike);
    fprintf(pFile, "%s\n", cf.spk);
	fprintf(pFile, "%s\n", tlport);
   
    fclose (pFile);
	pFile=0;
	
	printf("Config file created as default\r\n");
   }
  }
  

  
//kill old Tor process
printf("Try to kill old Tor\r\n");
#ifdef __WIN32
  strcpy(torcmd, (char*)"taskkill /IM tf_tor.exe /F");
#else
  strcpy(torcmd, (char*)"killall -9 tf_tor"); 
#endif
  system(torcmd); 

  
//check we need run Tor
  i=atoi(cf.tor);  //specified SOCKS5 port
  if((i<1)||(i>65535)) i=0; //zeroes if Tor not need
  if(rtor && i) //run Tor if required
  { 
   strncpy(pwd+pwdlen, (char*)"/tor/torrc", sizeof(pwd)-pwdlen); //set path to torrc file
   pFile = fopen(pwd, "w" ); //try write to torrc
   if(pFile)
   {
    strcpy(torcmd, (char*)"RunAsDaemon 1"); //demonize Tor after run
 #ifndef __WIN32   
	fprintf(pFile, "%s\r\n", torcmd); //this options only for Linux
 #endif  
    strcpy(torcmd, (char*)"SocksPort "); //SOCK5 port binded by Tor
    strcpy(torcmd+strlen(torcmd), cf.tor);
    fprintf(pFile, "%s\r\n", torcmd);
   
    strcpy(torcmd, (char*)"DataDirectory "); //Work directory will be created by Tor
    strncpy(pwd+pwdlen, (char*)"/tor_data", sizeof(pwd)-pwdlen);
    strcpy(torcmd+strlen(torcmd), pwd);
    fprintf(pFile, "%s\r\n", torcmd);
  
    strcpy(torcmd, (char*)"HiddenServiceDir "); //Hidden service directore will be create by Tor 
    strncpy(pwd+pwdlen, (char*)"/hidden_service", sizeof(pwd)-pwdlen);
    strcpy(torcmd+strlen(torcmd), pwd);
    fprintf(pFile, "%s\r\n", torcmd);
   
    strcpy(torcmd, (char*)"HiddenServiceVersion 2"); //Version 2 of HS: short onion address with RCA key
    fprintf(pFile, "%s\r\n", torcmd);
  
    strcpy(torcmd, (char*)"HiddenServicePort "); //TCP port accessable over our HiddenService
    strcpy(torcmd+strlen(torcmd), cf.tcp);
    fprintf(pFile, "%s\r\n", torcmd);
   
    strcpy(torcmd, (char*)"LongLivedPorts "); //set this port as long lived (for stable connecting during call)
    strcpy(torcmd+strlen(torcmd), cf.tcp);
    fprintf(pFile, "%s\r\n", torcmd);
   
    fclose (pFile);
    pFile=0;
	printf("'torrc' file updated, run Tor...\r\n");
   }
  
   //set path to Tor executable
   strncpy(pwd+pwdlen, (char*)"/tor/tf_tor -f ", sizeof(pwd)-pwdlen);
   strcpy(torcmd, pwd);
   //add path to torrc as parameter
   strncpy(pwd+pwdlen, (char*)"/tor/torrc", sizeof(pwd)-pwdlen);
   strncpy(torcmd+strlen(torcmd), pwd, sizeof(torcmd)-strlen(torcmd));

   //run Tor in separated thread
#ifdef __WIN32
if(1) //use CreateProcess and wait 2 sec for Tor will be stable before next steps
{
   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   memset(&si, 0, sizeof(si));
   si.cb = sizeof(si);
   si.wShowWindow=SW_HIDE;  //SW_SHOWNORMAL;
   memset(&pi, 0, sizeof(pi));
   if( !CreateProcess( NULL,   // No module name (use command line)
        torcmd,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        0,              // Set handle inheritance to FALSE
        CREATE_NEW_CONSOLE,              // Creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
    printf( "CreateProcess failed (%d)\r\n", (int)GetLastError() ); 
    else WaitForSingleObject( pi.hProcess, 2000 );
  
}
#else   
   system(torcmd); //Linux: start Tor, wait for demonize
#endif
 
  
  } 



//set path to storage files
   strncpy(pwd+pwdlen, (char*)"/tf_data/", sizeof(pwd)-pwdlen);
   bk_init_path(pwd);  //use direct access to local ST instead using command if: this is need only for local ST under OS


//input password or load from file / command line
   buf[0]=0;
  
  //check password from command line
  if(argc>1)
  {
   strncpy(buf, argv[1], sizeof(buf));
   for(i=0;i<sizeof(buf);i++) if(buf[i]<=0x0D) buf[i]=0; 
   if(buf[0]) printf("Use password from command line\r\n");   
  }
  
  //try load password from file
  if(!buf[0])
  {
   strncpy(pwd+pwdlen, (char*)"/tf_data/psw.txt", sizeof(pwd)-pwdlen); 
   pFile = fopen (pwd,"rt");
   if(pFile) //opened 
   {
    fgets(buf, sizeof(buf), pFile); 	
    for(i=0;i<sizeof(buf);i++) if(buf[i]<=0x0D) buf[i]=0;
    fclose (pFile);
    pFile=0;
   }
   if(buf[0]) printf("Use password from file\r\n"); 
  }
  
  //input password manually
  if(!buf[0])
  {
   printf("\r\nInput password:\r\n");
   fgets(buf, sizeof(buf), stdin);
   for(i=0;i<sizeof(buf);i++) if(buf[i]<=0x0D) buf[i]=0;
   //optionally save password to file
   printf("\r\nSave? (Y/n)\r\n");
   par[0]=0;
   fgets(par, sizeof(par), stdin);
   if((par[0]=='Y')||(par[0]=='y'))
   { 
	pFile = fopen (pwd,"wt");
    if(pFile) //opened 
    {
     fprintf(pFile, "%s\r\n", buf);
     fclose (pFile);
     pFile=0;
	 printf("\r\nPassword saved to file ./tf_data/pass.txt\r\n");
	 printf("and will be load on next run. Note: this is not safe!\r\n");
    }
   }
  }
  

 //try to load our onion address from HS (already created by Tor)
   if((cf.myadr[0]=='?')&&(cf.myadr[1]==0)) //check request
   { //set path to hostname generated by Tor
	strncpy(pwd+pwdlen, (char*)"/hidden_service/hostname", sizeof(pwd)-pwdlen);   
	pFile = fopen (pwd,"r"); //try open file
	if(pFile)
	{   
     cf.myadr[0]=0; //clear onion from config
     fgets(cf.myadr, sizeof(cf.myadr), pFile); //read hostname
	 len=strlen(cf.myadr); //len
     for(i=0;i<len;i++) if(cf.myadr[i]<=0x20) cf.myadr[i]=0;  //skip <CR><LF>	 
	 len=strlen(cf.myadr); //len without unprintable
	 if(len==22) //check len is valid onion
	 {
	  strcpy(cf.myadr+strlen(cf.myadr), (char*)":"); //add delimiter
	  strncpy(cf.myadr+strlen(cf.myadr), cf.tcp, 6); //add
	 }
	 else strcpy(cf.myadr, (char*)"?"); //restore ? for invalid
	 fclose(pFile);
	 pFile=0;
    }
   }

  //convert Telnet port from string
  if(tlport[0])
  {	   
   //open telnet port
   tlport[7]=0;
   i=atoi(tlport);
   if((i>0)&&(i<65535)) tport=(unsigned short)i;	
  }
  
  //apply configuration and output current parameters
  ui_setconfig(&cf); //set config to UI
  
  printf("\r\n-----------------------Configuration:-------------------------\r\n");
  showconf(); //output current configuration
  printf("\r\n-----------------------Initilization:-------------------------\r\n");
  
  //set password and start Torfone initilisation
  ui_doinit(buf); //set password to UI 
  memset(buf, 0, sizeof(buf)); //clear password
  
  //main loop 
  while(is_run) 
  {
	//poll telnet for user commands
	if(0<tn_read((unsigned char*)tlbuf)) command(tlbuf);
	
	//tf loop: all data processed in one thread for safe
	tf_loop(); 
	
	//get TF-to-GUI commands from pipe
	while(cmd=ui_getcommand(buf)) 
	{
	 switch(cmd) //process cmd
     {  
      //states
      case GUI_STATE_IDDLE:
	  {
	   tf_printf("state=IDDLE\r\n");
	   tf_name[0]=0;
	   tf_addr[0]=0;
       if(tport) //check telnet port is specified
	   {
	    tn_init(tport); //open telnet server
		tport=0; //once
	   }
	   break;
	  }
      case GUI_STATE_IKE:{tf_printf("state=IKE\r\n");break;}
      case GUI_STATE_CALL:{tf_printf("state=CALL\r\n");break;}
      case GUI_STATE_TERM:{tf_printf("state=TERM\r\n");break;} 
      //fields
      case GUI_NAME_TEXT:{tf_printf("name=%s\r\n", buf);strncpy(tf_name, buf, 16);break;}
      case GUI_ADDR_TEXT:{tf_printf("addr=%s\r\n", buf);strncpy(tf_addr, buf, 32);break;}
      //labels
      case GUI_SAS_TEXT:{tf_printf("sas: %s\r\n", buf);break;}
      case GUI_INFO_TEXT:{tf_printf("info: %s\r\n", buf);break;}
      case GUI_FGP_TEXT:{tf_printf("fgp(of my key): %s\r\n", buf);break;}
      //list
      case GUI_LIST_ADD:{tf_printf("list+=%s\r\n", buf);break;}
      case GUI_LIST_DEL:{tf_printf("list-=%s\r\n", buf);break;} 
      //set text color
      case GUI_NAME_W:{tf_printf("name_au=0\r\n");break;}
      case GUI_NAME_G:{tf_printf("name_au=1 (Contact verified OK)\r\n");break;}
      case GUI_ADDR_W:{tf_printf("addr_au=0\r\n");break;}
      case GUI_ADDR_G:{tf_printf("addr_au=1 (Addr verified) OK\r\n");break;}
      case GUI_FGP_W:{tf_printf("fgp_clr=W\r\n");break;}
      case GUI_FGP_G:{tf_printf("fgp_clr=G (Network is OK)\r\n");break;}
      case GUI_INFO_W:{tf_printf("info_clr=W\r\n");break;}
      case GUI_INFO_B:{tf_printf("info_clr=B\r\n");break;}
      case GUI_INFO_G:{tf_printf("info_clr=G\r\n");break;}
      case GUI_INFO_R:{tf_printf("info_clr=R\r\n");break;}
      //bunnon new color
      case GUI_BTNNEW_W:{tf_printf("btn_new=W\r\n");break;}
      case GUI_BTNNEW_B:{tf_printf("btn_new=B (Book is ready)\r\n");break;}
      case GUI_BTNNEW_G:{tf_printf("btn_new=G (Contact created OK)\r\n");break;}
      case GUI_BTNNEW_R:{tf_printf("btn_new=R (Contact not created)\r\n");break;}
      //bunnon change color
      case GUI_BTNSPK_W:{tf_printf("btn_spk=W\r\n");break;}
      case GUI_BTNSPK_B:{tf_printf("btn_spk=B (SPEKE request)\r\n");break;}
      case GUI_BTNSPK_G:{tf_printf("btn_spk=G (SPEKE OK)\r\n");break;}
      case GUI_BTNSPK_R:{tf_printf("btn_spk=R (SPEKE FAIL!)\r\n");break;}
      //button change/key color
      case GUI_BTNCHG_W:{tf_printf("btn_chg=W\r\n");break;}
      case GUI_BTNCHG_B:{tf_printf("btn_chg=B (Contact selected)\r\n");break;}
      case GUI_BTNCHG_G:{tf_printf("btn_chg=G (Contact changed OK)\r\n");break;}
      case GUI_BTNCHG_R:{tf_printf("btn_chg=R (Contact not changed!)\r\n");break;}
      //button change/key color
      case GUI_BTNKEY_W:{tf_printf("btn_key=W\r\n");break;}
      case GUI_BTNKEY_B:{tf_printf("btn_key=B (Receiving key request)\r\n");break;}
      case GUI_BTNKEY_G:{tf_printf("btn_key=G (Key received OK) \r\n");break;}
      case GUI_BTNKEY_R:{tf_printf("btn_key=R (Key not received) \r\n");break;}
      //button call/mute color
      case GUI_BTNCLL_W:{tf_printf("btn_cll=W\r\n");break;}
      case GUI_BTNCLL_B:{tf_printf("btn_cll=B (Incoming call from TCP)\r\n");break;}
      case GUI_BTNCLL_G:{tf_printf("btn_cll=G (Incoming call from Tor)\r\n");break;}
      case GUI_BTNCLL_R:{tf_printf("btn_cll=R (Outgoing call) \r\n");break;}
      //button list/direct color
      case GUI_BTNDIR_W:{tf_printf("btn_dir=W (p2p denied)\r\n");break;}
      case GUI_BTNDIR_B:{tf_printf("btn_dir=B (p2p request)\r\n");break;}
      case GUI_BTNDIR_G:{tf_printf("btn_dir=G (p2p active)\r\n");break;}
      case GUI_BTNDIR_R:{tf_printf("btn_dir=R (p2p allowed)\r\n");break;}
      //button menu/cancel color
      case GUI_BTNTRM_W:{tf_printf("btn_trm=W\r\n");break;}
      case GUI_BTNTRM_B:{tf_printf("btn_trm=B (In)\r\n");break;}
      case GUI_BTNTRM_G:{tf_printf("btn_trm=G (In&Out)\r\n");break;}
      case GUI_BTNTRM_R:{tf_printf("btn_trm=R (Out)\r\n");break;}
	  //notification
	  case GUI_NOTE_TEXT:{tf_printf("Notify(Incoming call)\r\n");break;}
      case GUI_NOTE_ANS:{tf_printf("Remote answer: TALK mode\r\n");ui_domute(1);}
     } //switch
	} //while	
  } //loop 

  tf_printf("Exit Torfone\r\n");
  
  //exit application
  ui_docancel(); //terminate all calls
  for(i=0;i<20;i++) tf_loop(); //for success termination
  tn_init(0); //close telnet port
  //kill Tor process
  printf("Try to kill old Tor\r\n");
#ifdef __WIN32
  strcpy(torcmd, (char*)"taskkill /IM tf_tor.exe /F");
#else
  strcpy(torcmd, (char*)"killall -9 tf_tor"); 
#endif
  system(torcmd);

  
   return 0;
} //main
//---------------------------------------------------------------------------
