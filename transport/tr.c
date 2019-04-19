//**************************************************************************
//ORFone project
//Core of transport module
//**************************************************************************
#define USE_TELNET 5000   //use telnet control for upper ports

#include <limits.h>
#include <stdio.h>

#ifdef _WIN32

#include <stddef.h>
#include <stdlib.h>
#include <basetsd.h>
#include <stdint.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>

#ifndef __BORLANDC__
 #include <ws2tcpip.h>
 #include <sys/time.h>
#endif

#define ioctl ioctlsocket
#define close closesocket
#define EWOULDBLOCK WSAEWOULDBLOCK  //no data for assync polling
#define ENOTCONN WSAENOTCONN        //wait for connection for assinc polling
#define ECONNRESET WSAECONNRESET    //no remote udp interface in local network



char sock_buf[32768];   //WSA sockets buffer
//------------------------------------------------------------------
//some Windows compilators not have gettimeofday and require own realization
//------------------------------------------------------------------
#ifndef gettimeofday
 int gettimeofday(struct timeval *tv, void* tz)
{

  FILETIME ft;
  const __int64 DELTA_EPOCH_IN_MICROSECS= 11644473600000000;
  unsigned __int64 tmpres = 0;
  unsigned __int64 tmpres_h = 0;
  //static int tzflag;

  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    //converting file time to unix epoch
    tmpres /= 10;  //convert into microseconds
    tmpres -= DELTA_EPOCH_IN_MICROSECS;

    tmpres_h=tmpres / 1000000UL; //sec
    tv->tv_sec = (long)(tmpres_h);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }
  return 0;

}
#endif


#else //linux
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/tcp.h>
#include <netdb.h>

#ifdef LINUX_FPU_FIX
#include <fpu_control.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "string.h"
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <ifaddrs.h>

#endif


#ifndef INVALID_SOCKET
 #define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
 #define SOCKET_ERROR -1
#endif



#include "jit.h"
#include "local_if.h"
#include "serpent.h"
#include "rnd.h"
#include "tr_if.h"
#include "tr.h"
#include "file.h"


tr_data tr; //internal data of transport module

char TR_DBG=0; //verbose switch



//******************************************************************
//                  INTERNAL PROCEDURES
//******************************************************************

void ct2bf(unsigned char* buf, unsigned int cnt)
{
 buf[0]=0;
 buf[1]=cnt&0xFF;
 buf[2]=(cnt>>8)&0xFF;
 buf[3]=(cnt>>16)&0xFF;
}

unsigned int bf2ct(unsigned char* buf)
{
 unsigned int ctr=((unsigned int)buf[3]<<16)+((unsigned int)buf[2]<<8) + buf[1];
 return ctr;
}

//------------------------------------------------------------------
//Compose in buf a control packet of type for remote party, returns length
//------------------------------------------------------------------
int make_control(unsigned char* buf, unsigned char type)
{
 //if(TR_DBG) printf("make_control: type=%d\r\n", (int)type);

  //ct2bf(buf+12, 0);
  (*(unsigned int*)(buf+12))=0;  //set MDC
  ct2bf(buf, tr.control_send_cnt);
  tr.control_send_cnt++;
  buf[4]=type; //set control type
  return CTRL_LEN; //return control len is 16 bytes
}

//------------------------------------------------------------------
//Check received packet is a control and returns control type or 0
//------------------------------------------------------------------
//check control packet is valid and returns type
int check_control(unsigned char* buf)
{
 
 unsigned int cnt;
 unsigned short l=buf[0];

 //if(TR_DBG) printf("check_control: l=%d\r\n", l);

 if(l<0x80) return CONTROL_VOICE; //voice, passing to queue

 if(l>0xEC) //file transfer packet
 {
  fl_rcvd(buf);
  return 0;
 }

 if(l>0x8B) //cr-to-cr packets, obtain type by len and pass to CR
 {
  if(l>0xAF) buf[0]=REMOTE_S; //SPEKE key
  else if(l>0xA3) buf[0]=REMOTE_K; //send key
  else if(l>0x97) buf[0]=REMOTE_Q; //IKE data
  else buf[0]=REMOTE_A; //reserv
  return CONTROL_CR;
 }
 //TR internal control packet (len=16-27)
 if(*(unsigned int*)(buf+12)) return CONTROL_ERROR; //check MDC must be zero
 buf[0]=0; //clear len
 cnt=bf2ct(buf); //get counter from packet
 buf[0]=l; //place back len
 if(cnt<tr.control_recv_cnt) return CONTROL_ERROR; //check is a new packet not replie
 tr.control_recv_cnt=cnt+1; //set packets counter to current packet numer
 //if(TR_DBG) printf("check_control: ret=%d\r\n", (int) buf[4]);
 return (int) buf[4]; //return control type

}
//------------------------------------------------------------------
//send NAT traverasal ping over UDP socket
//------------------------------------------------------------------
void direct_connect(void)
{

 int len;

 //check for tr.state_udp
 if(tr.state_udp==STATE_WAIT_ACK)
 {
  if(tr.their_lanport)
  {
   //if(TR_DBG) printf("direct_connect: their_lanport=%d\r\n", tr.their_lanport);
   memset(&tr.saddr, 0, sizeof(tr.saddr));
   tr.saddr.sin_family = AF_INET;
   tr.saddr.sin_port = htons(tr.their_lanport);
   tr.saddr.sin_addr.s_addr=tr.their_lanip;
   rn_getrnd(tr.workbuf, 16);
   make_control(tr.workbuf, CONTROL_CONNECT);
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_UDP;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   sendto(tr.sock_udp, (const char*)tr.outbuf, len, 0, (const struct sockaddr*)&tr.saddr, sizeof(tr.saddr));
  }
  if(tr.their_wanport)
  {
   //if(TR_DBG) printf("direct_connect: their_wanport=%d\r\n", tr.their_lanport);
   memset(&tr.saddr, 0, sizeof(tr.saddr));
   tr.saddr.sin_family = AF_INET;
   tr.saddr.sin_port = htons(tr.their_wanport);
   tr.saddr.sin_addr.s_addr=tr.their_wanip;
   rn_getrnd(tr.workbuf, 16);
   make_control(tr.workbuf, CONTROL_CONNECT);
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_UDP;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   sendto(tr.sock_udp, (const char*)tr.outbuf, len, 0, (const struct sockaddr*)&tr.saddr, sizeof(tr.saddr));
  }
 }

}


//------------------------------------------------------------------
//if not connected force outgoing TCP connecting to specified adress
//------------------------------------------------------------------
void recall(void)
{
 
 unsigned long opt = 1; //for ioctl
 int flag=1; //for setsockopt
 unsigned short port;


 if((!tr.their_port)||tr.state_out) return; //check for remote adress is specified

 //if(TR_DBG) printf("recall: their_port=%d\r\n", tr.their_port);

 //create new tcp socket for outgoing connecting
 if((tr.sock_tcp_out = socket(AF_INET, SOCK_STREAM, 0)) <0)
 {
  perror("tr.sock_tcp_out");
  tr.sock_tcp_out=INVALID_SOCKET;
  return;
 }

 //------------------------------------------------------------------------
 //set our local tcp port as a random
  rn_getrnd(tr.workbuf, 16); //generate random port
  port=tr.workbuf[1];
  port<<=8;
  port+=tr.workbuf[0];
  port|=1024;
  if(port==tr.tor_port) port^=8; //check port not the same of tor port
  if(port==tr.listen_port) port^=16; //check port not the same of listening port
  if(port==tr.tcp_port) port^=32; //check port not the same of old port

 //set remote adress structure for bind
  memset(&tr.saddr, 0, sizeof(tr.saddr));
  tr.saddr.sin_family = AF_INET;
  tr.saddr.sin_port = htons(port);
  tr.saddr.sin_addr.s_addr=INADDR_ANY;

  //bind udp socket to port
  if (bind(tr.sock_tcp_out, (struct sockaddr*)&tr.saddr, sizeof(tr.saddr)) < 0)
  {
   //if(TR_DBG) printf("UDP Socket bining error\r\n");
   close(tr.sock_tcp_out);
   tr.sock_tcp_out=INVALID_SOCKET;
   return;
  }

  tr.tcp_port=port; //save current tcp out port 
 //------------------------------------------------------------------------


 //disable nagle algo
 if (setsockopt(tr.sock_tcp_out, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)) < 0)
 {
  perror( "tr.sock_tcp_out TCP_NODELAY" );
  close(tr.sock_tcp_out);
  return;
 }

 //unblock socket
 opt=1;
 ioctl(tr.sock_tcp_out, FIONBIO, &opt);

 //set remote adress structure
 memset(&tr.saddr, 0, sizeof(tr.saddr));
 tr.saddr.sin_family = AF_INET;
 tr.saddr.sin_port = htons(tr.their_port);
 tr.saddr.sin_addr.s_addr=tr.their_ip;

 //start connecting procedure
 connect(tr.sock_tcp_out, (const struct sockaddr*)&tr.saddr, sizeof(tr.saddr));

 //set status of outgoing connecting.
 if(tr.tor_len) tr.state_out=STATE_WAIT_TOR; else tr.state_out=STATE_WAIT_TCP;
 return;

}


//------------------------------------------------------------------
//send keep alive packet over awaliable connectings were inactive last time
//------------------------------------------------------------------
void ping_remote(void)
{
 
 int len;

 if(tr.state_udp==STATE_SEND) tr.state_udp=STATE_WORK;
 else if(tr.state_udp==STATE_WORK)
 {
  make_control(tr.workbuf, CONTROL_PING);
  memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
  tr.outbuf[1]^=OBF_UDP;
  len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
  sendto(tr.sock_udp, (const char*)tr.outbuf, len, 0, (const struct sockaddr*)&tr.saddrUDPTo, sizeof(tr.saddrUDPTo));
  //if(TR_DBG) printf("ping_remote udp len=%d\r\n", len);
 }

 if(tr.state_in==STATE_SEND) tr.state_in=STATE_WORK;
 else if(tr.state_in==STATE_WORK)
 {
  make_control(tr.workbuf, CONTROL_PING);
  memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
  tr.outbuf[1]^=OBF_IN;
  len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
  if(len!=send(tr.sock_tcp_in, (char*)tr.outbuf, len, MSG_NOSIGNAL))  call_terminate(1); //terminate call
  //if(TR_DBG) printf("ping_remote tcp_in len=%d\r\n", len);
 }

 if(tr.state_out==STATE_SEND) tr.state_out=STATE_WORK;
 else if(tr.state_out==STATE_WORK)
 {
  make_control(tr.workbuf, CONTROL_PING);
  memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
  tr.outbuf[1]^=OBF_OUT;
  len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
  if(len!=send(tr.sock_tcp_out, (char*)tr.outbuf, len, MSG_NOSIGNAL))  close_out(); //recall
  //if(TR_DBG) printf("ping_remote tcp_out len=%d\r\n", len);
 }

}

//------------------------------------------------------------------
//measure statistic for incoming and outgoing TCP connectings
//request remote reconnecting if our outgoing is significantly worse
//------------------------------------------------------------------
void set_channel_statistic(int receive_channel)
{
  int len;

  //check for both channels avaliable for comparing and UDP in not active
  if((tr.state_out<STATE_WORK)||(tr.state_in<STATE_WORK)||(receive_channel<0)||(tr.state_udp>=STATE_WORK))
  {  //else clear comparing statistic
   tr.data_recv_in=0;
   tr.data_recv_out=0;
   return;
  }
  if(receive_channel) tr.data_recv_in++; else tr.data_recv_out++; //outgoing or incoming was fast
  //compare statistic of incoming and outgoing channals
  if((tr.data_recv_in+tr.data_recv_out)>STAT_PACKETS) //1 min comparing interval
  {
   //if(TR_DBG) printf("set_channel_statistic: in=%d, out=%d\r\n", tr.data_recv_in, tr.data_recv_out);
   tr.dbl_request=0; //clear flag of worse event
   if((tr.data_recv_in-tr.data_recv_out)>STAT_DELTA) //outgoing channel is worse
   {
    make_control(tr.workbuf, CONTROL_DOUBLE); //make request for close this channel remotely
    memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
    tr.outbuf[1]^=OBF_IN;
    len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
    send(tr.sock_tcp_in, (char*)tr.outbuf, len, MSG_NOSIGNAL); //send request over incoming channel (it is the best)
    tr.dbl_request=1; //set flag request was sended and wait remotely closing
   }               //after this their requests will be ignored up to new measurement
   tr.data_recv_in=0; //clear statistic for new measure interval
   tr.data_recv_out=0;
  }
}


//------------------------------------------------------------------
//renew timestamp and process events
//------------------------------------------------------------------
int wait_timer(unsigned char* buf)
{

 if(tr.soc_event) //check for event was set
 {
  buf[0]=0xFF&(tr.soc_event); //set event flag and event type
  buf[1]=tr.soc_event>>8; //extension
  buf[2]=0;
  buf[3]=0;
  tr.soc_event=0; //clear event
  return 4; //return event notification
 }

 if(tr.time_counter) tr.time_counter--; //check for loop iterations
 else  //get timestamp with system call
 {
  struct timeval tt1;
  gettimeofday(&tt1, NULL); //get timestamp
  tr.time_now=(unsigned int) tt1.tv_sec; //set global value
  tr.time_counter=TIMELOOP; //set counter for next loop
  if((tt1.tv_usec>>FILE_SEND_DEL)!=tr.time_for_file) //every 32 mS
  {
   tr.time_for_file = tt1.tv_usec>>FILE_SEND_DEL; //set current time for next
   fl_send(); //process file transfering
  }
  //send keepalive ping
  if(tr.time_now>tr.time_for_ping)
  {
   if(tr.time_for_ping) ping_remote();
   tr.time_for_ping=tr.time_now+PING_INTERVAL;  //180 sec
  }
  //connect timer
  if(tr.time_now>tr.time_for_connect)
  {
   if(tr.time_for_connect)
   {
    direct_connect();
    recall();
   }
   tr.time_for_connect=tr.time_now + CONNECT_INTERVAL;  //2 sec
  }
  //disconnect
  if(tr.time_now>tr.time_for_close)
  {
   if(tr.time_for_close) call_terminate(1);
   tr.time_for_close=0xFFFFFFFF;
  }

 }
 return 0;

}


//------------------------------------------------------------------
//poll listener for new incoming connecting, accept or reject
//------------------------------------------------------------------
int wait_accept(void)
{

  int tor_flag=0; //incoming from tor
  int flag=1; 
  unsigned long opt=1;
  int sTemp=INVALID_SOCKET; //accepted socket
  int ll=sizeof(tr.saddr);
  int len=0;

  sTemp  = accept(tr.sock_listener, (struct sockaddr *) &tr.saddr, (socklen_t*)&ll);
  if(sTemp<=0) return 0; //no incoming connections accepted

  if(tr.saddr.sin_addr.s_addr==INADDR_LOCAL) tor_flag=1;//if incoming from localhost (Tor)


  //if(TR_DBG) printf("wait_accept from %s:%d\r\n", inet_ntoa(tr.saddr.sin_addr), htons(tr.saddr.sin_port));  //noti

  //disable nagle on sTemp ->Err
  if (setsockopt(sTemp, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)) < 0)
  {
   perror( "TCPin TCP_NODELAY" );
   close(sTemp);
   return -1;
  }

  //unblock socket
  opt=1;
  ioctl(sTemp, FIONBIO, &opt);

  //incoming during existing call
  if(tr.isready)
  {
   //check for incoming connecting already active, or current or incoming  is not tor
   if(tr.state_in||(!tor_flag)||(!tr.istor))
   {
    ll=0;
    if(len!=send(sTemp, (char*)(&ll), 1, MSG_NOSIGNAL))  //send 1 byte is zero for notify busy
    {
     //if(TR_DBG) printf(" wait_accept: reject, sock=%d\r\n", tr.sock_tcp_in);//send busy nitification
    }
    close(sTemp); //close unexpected incoming
    tr.soc_event=EVENT_BUSY;
    return -2;
   }
  }
  else //no call established (iddle or in IKE)
  {
   //check for any connecting already exist
   if(tr.state_in||tr.state_out)
   {
    ll=0;
    if(len!=send(sTemp, (char*)(&ll), 1, MSG_NOSIGNAL))  //send 1 byte is zero for notify busy
    {
     //if(TR_DBG) printf(" wait_accept: reject, sock=%d\r\n", tr.sock_tcp_in);//send busy nitification
    }
    close(sTemp); //close unexpected incoming
    tr.soc_event=EVENT_BUSY;
    return -2;
   }
  }

  //check old incoming still exist and close it
  if(tr.sock_tcp_in!=INVALID_SOCKET) close(tr.sock_tcp_in);

  //set new incomin connecting
  tr.sock_tcp_in=sTemp;   //sen new incoming

//------------flag==0-------------------------------------  
  //start obfuscation session
  if(!tr.obf)
  {
   tr.state_in=STATE_WAIT_OBF; //set state
   tr.time_for_close=tr.time_now+MAX_IKE_TIME; //set timeout for IKE process for new incoming
   tr.time_for_ping=tr.time_now+PING_INTERVAL;  //180 sec
   return 3;
  }


//-------------flag==1-------------------------------------
  tr.state_in=STATE_WAIT_ACK; //set state

  //check conncting already exist: doubling
  if(tr.isready)
  {
   tr.soc_event=EVENT_DOUBLING_IN; //during existed call: incoming doubling
   return 2;
  }

  //new incoming connecting
  if(tor_flag)
  {
   tr.soc_event=EVENT_ACCEPT_TOR; //set event: new incoming over tor
   tr.istor=1; //set tor flag
   jt_reset_speech(); //reset speech jitter buffer
   jt_set_channel_type(CHANNEL_TOR);
  }
  else
  {
   tr.soc_event=EVENT_ACCEPT_TCP; //set event of new incoming over tcp
   jt_reset_speech(); //reset speech jitter buffer
   jt_set_channel_type(CHANNEL_TCP);
  }
  tr.time_for_close=tr.time_now+MAX_IKE_TIME; //set timeout for IKE process for new incoming
  tr.time_for_ping=tr.time_now+PING_INTERVAL;  //180 sec
  return 1;
}


//------------------------------------------------------------------
//poll outgoing socket is connected during TCP connecting process
//for over Tor connecting also provide SOCK5 connecting procedure
//------------------------------------------------------------------
int wait_connect(unsigned char* buf)
{
  
  int len=0;
  //check for connect to remote TCP
  if(tr.state_out==STATE_WAIT_TCP)
  {
   if(!send(tr.sock_tcp_out, (char*)&len, 0, MSG_NOSIGNAL))
   {

//-------------flag==1-------------------------------------
    if(tr.obf)
    {
     tr.state_out=STATE_WAIT_ACK; //set state to tcp connected
     rn_getrnd(tr.workbuf, 16);
     make_control(tr.workbuf, CONTROL_CONNECT); //make connecting control
     memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
     tr.outbuf[1]^=OBF_OUT;
     len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
     send(tr.sock_tcp_out, (char*)tr.outbuf, len, MSG_NOSIGNAL); //send connectin event
     //if(TR_DBG) printf("wait_connect: Connected to TCP, send %d bytes\r\n", len);
     tr.time_for_close=tr.time_now+MAX_IKE_TIME; //connect to remote TCP, set timeout for IKE
     tr.time_for_ping=tr.time_now+PING_INTERVAL;  //180 sec
    }
    else  //obfuscation session
    {
     tr.state_out=STATE_WAIT_OBF;
     tr.time_for_close=tr.time_now+MAX_IKE_TIME; //set timeout for IKE process for new incoming
     tr.time_for_ping=tr.time_now+PING_INTERVAL;  //180 sec
     tr.soc_event=EVENT_OBF_OUT; //send event
    }
//----------------------------------------------------------
    return 0;
   }
  }
  //check to connect to Tor SOCK5 interface
  if(tr.state_out==STATE_WAIT_TOR)
  {
   if(!send(tr.sock_tcp_out, (char*)&len, 0, MSG_NOSIGNAL))
   {
    //if(TR_DBG) printf("wait_connect: Connected to Tor, send 3 bytes\r\n");
    send(tr.sock_tcp_out, (const char*)tr.torbuf, 3, MSG_NOSIGNAL); //Client hello: 05 01 00
    tr.state_out=STATE_WAIT_SOC;
    if(!tr.state_in) tr.time_for_close=tr.time_now+MAX_ONION_TIME; //connect to local Tor, set timeout for connet to remote Onion
    return 0;
   }
  }

  //after connected to Tor perform to SOCK5 connecting
  if(tr.state_out==STATE_WAIT_SOC)
  {
   len=recv(tr.sock_tcp_out, (char*)tr.workbuf, sizeof(tr.workbuf), 0);
   if(!len)
   {
    //if(TR_DBG) printf("wait_connect: Out connect close remotely by recv\r\n");
    //close_out(); //check for read disconnect event
    close(tr.sock_tcp_out);
    tr.sock_tcp_out=INVALID_SOCKET;
    tr.state_out=0;
    return 0;
   }
   else if(len==1)
   {
    //if(TR_DBG) printf("wait_connect: Remote is busy\r\n");
    call_terminate(1);
    return 0;
   }
   else if(len>1)
   { //SOCK5 protocol:  result 05 00 00 01 AA AA AA AA PH PL
    //if(TR_DBG) printf("wait_connect: recv len=%d\r\n", len);
    if( (len>9) && (tr.workbuf[0]==5) && (tr.workbuf[1]==0) )
    {

//-------------flag==1-------------------------------------
     if(tr.obf)
     {
      tr.state_out=STATE_WAIT_ACK;  //check for SOCK5 HELLO
      rn_getrnd(tr.workbuf, 16);
      make_control(tr.workbuf, CONTROL_CONNECT); //make connecting control
      memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
      tr.outbuf[1]^=OBF_OUT;
      len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
      send(tr.sock_tcp_out, (char*)tr.outbuf, len, MSG_NOSIGNAL); //send connectin event
      //if(TR_DBG) printf("wait_connect: SOCK5 connected, send CONTROL_CONNECT len=%d\r\n", len);
      if(!tr.state_in) tr.time_for_close=tr.time_now+MAX_IKE_TIME; //connected to remote Onion, set timeout for IKE
     }
     else   //obfuscation session
     {
      tr.state_out=STATE_WAIT_OBF;
      tr.time_for_close=tr.time_now+MAX_IKE_TIME; //set timeout for IKE process for new incoming
      tr.time_for_ping=tr.time_now+PING_INTERVAL;  //180 sec
      tr.soc_event=EVENT_OBF_OUT; //send event
     }
//---------------------------------------------------------
     return 0;
    }

    //SOCK5: answer to Hello: 05 00
    if( (len<10) && (tr.workbuf[0]==5) && (tr.workbuf[1]==0) ) //check for SOCK5 ACK
    {
     send(tr.sock_tcp_out, (const char*)tr.torbuf, tr.tor_len, MSG_NOSIGNAL); //send sock5 HS-request to Tor
     //if(TR_DBG) printf("wait_connect: SOCK5 discovered, send %d from torbuf\r\n", tr.tor_len);
     return 0;
    }

   }
  }

  return 0;
}



//------------------------------------------------------------------
//poll TCP incoming socket for received data
//------------------------------------------------------------------
int wait_tcpin(unsigned char* buf)
{
  
  int i, len;

//----------------OBF--------------------
  if(tr.sock_tcp_in==INVALID_SOCKET) return 0; //check socket is valid

  if(tr.state_in==STATE_WAIT_OBF) //check is in pre-IKE stage
  {  //receive representation: first 32 bytes is carry out
   len=recv(tr.sock_tcp_in, (char*)tr.readbuf_in+tr.readptr_in, 32, 0);
   if(len<0) return 0;
   if(!len) //check for read disconnect
   {
    //if(TR_DBG) printf("wait_tcpin: In close remotely by recv\r\n");
    tr.state_in=0;
    call_terminate(1); //check for read disconnect event
    return 0;
   }
   tr.readptr_in+=len; //total bytes received
   if(tr.readptr_in<32) return 0; //check we have representation
   tr.readptr_in=0;  //clear pointer
   memcpy(buf+4, tr.readbuf_in, 32); //output rep
   buf[0]=REMOTE_R;  //set type is rep
   return 36;
  }
//----------------------------------------

  //read data from incoming tcp connecting
  len=recv(tr.sock_tcp_in, (char*)tr.readbuf_in+tr.readptr_in, tr.readlen_in, 0);
  if(!len) //check for read disconnect
  {
   //if(TR_DBG) printf("wait_tcpin: In close remotely by recv\r\n");
   tr.state_in=0;
   call_terminate(1); //check for read disconnect event
   return 0;
  }
  else if(len<0) return 0; //no data
  //if(TR_DBG) printf("wait_tcpin: rcvd %d\r\n", len);
  tr.readlen_in-=len; //byte to read left
  tr.readptr_in+=len; //move data pointer
  if(tr.readptr_in==16)
  {
   tr.readlen_in=rn_unhide(tr.readbuf_in); //first block received: unhide
   if((tr.readlen_in>119)||(tr.readlen_in<0))
   {
    call_terminate(1);
    return 0;
   }
   //if(TR_DBG) printf("wait_tcpin: rn_unhide=%d\r\n", tr.readlen_in);
  }
  if(tr.readlen_in) return 0; //full packet received? receive tail or process packet
  len=tr.readptr_in; //total received data length
  tr.readptr_in=0; //clear data pointer for next
  tr.readlen_in=16;  //clear data length for next
  memcpy(buf, tr.readbuf_in, len); //output compleet received packet
  //if(TR_DBG) printf("wait_tcpin: pkt len=%d\r\n", len);
  len=rn_decrypt(buf, len); //check mac and decrypt data
  if(!len) return 0;
  buf[1]^=OBF_OUT;
  i=check_control(buf); //check received packet is control
  //if(TR_DBG) printf("wait_tcpin: control=%d\r\n", i);

  if(i==CONTROL_VOICE)
  {
   i=jt_add_tf_speech(buf, len);
   if(i) set_channel_statistic(1);
  }
  else if(i==CONTROL_CR) return len;
  else if(i==CONTROL_DISCONNECT)
  { //remote close in socket is terminate call
   //if(TR_DBG) printf("wait_tcpin: recv terminate control over TCPIN\r\n");
   call_terminate(1); //terminate call if term control received
   return 0;
  }
  else if((i==CONTROL_CONNECT)&&(tr.state_in==STATE_WAIT_ACK))
  { //first connecting or authentication in reconnecting
   tr.state_in=STATE_WORK;
   if(tr.istor) tr.soc_event=EVENT_READY_TOR;
   else tr.soc_event=EVENT_READY_TCP; //set event incoming connecting ready
   //send CONTROL_CONNECT to initiator
   rn_getrnd(tr.workbuf, 16);
   make_control(tr.workbuf, CONTROL_CONNECT); //make connecting control
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_IN;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   send(tr.sock_tcp_in, (char*)tr.outbuf, len, MSG_NOSIGNAL); //send connectin event
   //if(TR_DBG) printf("wait_tcpin: ACK received over in-socket, send ACK. TCPIn in work\r\n");
  }
  else if((i==CONTROL_WAN)&&(tr.state_in>=STATE_WORK))
  {
   tr.their_wanip=(*(unsigned int*)(buf+8));
   tr.their_wanport=(*(unsigned short*)(buf+6));
   tr.saddr.sin_addr.s_addr = tr.their_wanip;
   tr.soc_event=EVENT_DIRECT_REQ; //set event remote request direct connecting
   //if(TR_DBG) printf("wait_tcpin: rcvd WAN invite %s:%d\r\n", inet_ntoa(tr.saddr.sin_addr), tr.their_wanport);
  }
  else if((i==CONTROL_LAN)&&(tr.state_in>=STATE_WORK))
  {
   tr.their_lanip=(*(unsigned int*)(buf+8));
   tr.their_lanport=(*(unsigned short*)(buf+6));
   tr.saddr.sin_addr.s_addr = tr.their_lanip;
   tr.soc_event=EVENT_DIRECT_REQ; //set event remote request direct connecting
   //if(TR_DBG) printf("wait_tcpin: rcvd LAN invite %s:%d\r\n", inet_ntoa(tr.saddr.sin_addr), tr.their_lanport);
  }
  else if(i==CONTROL_DIR)
  {
   //if(TR_DBG) printf("wait_udp: recv close control over TCPout\r\n");
   close_udp(); //close udp if close control received
   return 0;
  }
  else if(i==CONTROL_SECRET) //only can be on acceptor side
  {
   return (tr_set_remote(buf)); //return event immediately
  }
  else if(i==CONTROL_DOUBLE)  //back doubling answer
  {
   //processing doubling answer only if both in and out in work state
   if((tr.state_in<STATE_WORK)||(tr.state_out<STATE_WORK)) return 0;

   //if(TR_DBG) printf("wait_tcpin: recv doubleansw control over TCPIN, OUT closed\r\n");

   //close outgoing socket
   if(tr.sock_tcp_out!=INVALID_SOCKET)  close(tr.sock_tcp_out); //close outgoing socket
   tr.sock_tcp_out=INVALID_SOCKET; //set socket invalid
   tr.state_out=0; //clear state
   tr.time_for_connect=tr.time_now + DOUBLE_INTERVAL;  //set reconnecting timeout
   tr.soc_event=EVENT_CLOSE_OUT; //output event closing outgoing socket
  }

 return 0;
}


//------------------------------------------------------------------
//poll TCP outgoing socket for received data
//------------------------------------------------------------------
int wait_tcpout(unsigned char* buf)
{
  
  int i, len;


//----------------OBF--------------------
  if(tr.sock_tcp_out==INVALID_SOCKET) return 0; //check socket is valid

  if(tr.state_out==STATE_WAIT_OBF) //check is in pre-IKE stage
  {  //receive representation: first 32 bytes is carry out
   len=recv(tr.sock_tcp_out, (char*)tr.readbuf_out+tr.readptr_out, 32, 0);
   if(len<0) return 0;
   if(!len) //check for read disconnect
   {
    //if(TR_DBG) printf("wait_tcpout: Out close remotely by recv\r\n");
    tr.state_out=0;
    call_terminate(1); //check for read disconnect event
    return 0;
   }
   tr.readptr_out+=len; //total bytes received
   if(tr.readptr_out<32) return 0; //check we have representation
   tr.readptr_out=0;  //clear pointer
   memcpy(buf+4, tr.readbuf_out, 32); //output rep
   buf[0]=REMOTE_R;  //set type is rep
   return 36;
  }
//----------------------------------------


  //HERE CHECK STATE < wait_ack and proceed to wait_connect
  if(tr.state_out<STATE_WAIT_ACK)
  {
   return(wait_connect(buf));
  }

  //read data from incoming tcp connecting
  len=recv(tr.sock_tcp_out, (char*)tr.readbuf_out+tr.readptr_out, tr.readlen_out, 0);
  if(!len) //check for read disconnect
  {
   //if(TR_DBG) printf("wait_tcpout: In close remotely by recv\r\n");
   tr.state_out=0;
   call_terminate(1); //check for read disconnect event
   return 0;
  }
  else if(len<0) return 0; //no data


   //if(TR_DBG) printf("wait_tcpout: rcvd %d\r\n", len);
  tr.readlen_out-=len; //byte to read left
  tr.readptr_out+=len; //move data pointer
  if(tr.readptr_out==16)
  {
   tr.readlen_out=rn_unhide(tr.readbuf_out); //first block received: unhide
   if((tr.readlen_out>119)||(tr.readlen_out<0))
   {
    call_terminate(1);
    return 0;
   }
   //if(TR_DBG) printf("wait_tcpin: rn_unhide=%d\r\n", tr.readlen_out);
  }
  if(tr.readlen_out) return 0; //full packet received: process it

  //if(TR_DBG) printf("wait_tcpout: pkt len=%d\r\n", len);

  len=tr.readptr_out; //received data length
  tr.readptr_out=0; //clear data pointer for next
  tr.readlen_out=16;  //clear data length for next
  memcpy(buf, tr.readbuf_out, len); //output compleet received packet
  len=rn_decrypt(buf, len); //check mac and decrypt data
  if(!len) return 0;
  buf[1]^=OBF_IN;
  i=check_control(buf); //check received packet is control
  //if(TR_DBG) printf("wait_tcpout: control=%d\r\n", i);
  if(i==CONTROL_VOICE)
  {
   i=jt_add_tf_speech(buf, len);
   if(i) set_channel_statistic(0);
  }
  else if(i==CONTROL_CR) return len;
  else if(i==CONTROL_DISCONNECT)
  {
   //if(TR_DBG) printf("wait_tcpout: recv terminate control\r\n");
   call_terminate(1); //terminate call if term control received
   return 0;
  }
  else if((i==CONTROL_CONNECT)&&(tr.state_out==STATE_WAIT_ACK))
  { //first connecting or authentication in reconnecting
   //if(TR_DBG) printf("wait_tcpout: ACK received. TCPOUT in work\r\n");
   tr.state_out=STATE_WORK;
   tr.soc_event=EVENT_READY_OUT; //set event outgoing connecting ready
  }
  else if((i==CONTROL_WAN)&&(tr.state_out>=STATE_WORK))
  {
   tr.their_wanip=(*(unsigned int*)(buf+8));
   tr.their_wanport=(*(unsigned short*)(buf+6));
   tr.saddr.sin_addr.s_addr = tr.their_wanip;
   tr.soc_event=EVENT_DIRECT_REQ; //set event remote request direct connecting
   if(TR_DBG) printf("wait_tcpout: rcvd WAN invite %s:%d\r\n", inet_ntoa(tr.saddr.sin_addr), tr.their_wanport);
  }
  else if((i==CONTROL_LAN)&&(tr.state_out>=STATE_WORK))
  {
   tr.their_lanip=(*(unsigned int*)(buf+8));
   tr.their_lanport=(*(unsigned short*)(buf+6));
   tr.saddr.sin_addr.s_addr = tr.their_lanip;
   tr.soc_event=EVENT_DIRECT_REQ; //set event remote request direct connecting
   if(TR_DBG) printf("wait_tcpout: rcvd LAN invite %s:%d\r\n", inet_ntoa(tr.saddr.sin_addr), tr.their_lanport);
  }
  else if(i==CONTROL_DIR)
  {
   //if(TR_DBG) printf("wait_udp: recv close control over TCPout\r\n");
   close_udp(); //close udp if close control received
   return 0;
  }
  else if(i==CONTROL_DOUBLE)
  {
   //processing doubling request only if both in and out in work state and no our request on time
   if((tr.state_in<STATE_WORK)||(tr.state_out<STATE_WORK)||tr.dbl_request) return 0;

   //send back notfication of doubling accepted
   make_control(tr.workbuf, CONTROL_DOUBLE); //doubler contro packet
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_OUT;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   send(tr.sock_tcp_out, (char*)tr.outbuf, len, MSG_NOSIGNAL);
   //if(TR_DBG) printf("wait_tcpout: recv doublereq control over TCPOUT, IN closed\r\n");

  //only set iddle state for current incoming socket (will be not monitored for remote close)
  //not close in_sock yet!
   tr.state_in=0;
   tr.soc_event=EVENT_CLOSE_IN;
  }
  else if(i==CONTROL_SECRET) //only can be on acceptor side
  {
   return (tr_set_remote(buf)); //return event immediately
  }
  return 0;

}


//------------------------------------------------------------------
//poll UDP socket for received data
//------------------------------------------------------------------
int wait_udp(unsigned char* buf)
{
 
 int len=0;
 int i;

 //try read data from udp
 if(!tr.state_udp) return 0;  //check for socket readable
 i=sizeof(tr.saddr);
 len=recvfrom(tr.sock_udp, (char*)buf, 136, 0, (struct sockaddr*)&tr.saddr, (int*)&i);
 if(len<=0) return 0;

 //if(TR_DBG) printf("wait_udp: rcvd %d\r\n", len);

 //check for stun answer
 if(tr.saddr.sin_port==htons(tr.stun_port)) //check remote port is stun port
 {
  if((tr.state_udp!=STATE_WAIT_ACK)||(buf[0]!=1)||(buf[1]!=1)||(len<32)) return 0;
  tr.our_wanport=htons(*(unsigned short*)(buf+26)); //our external PORT reported by STUN
  tr.our_wanip=(*(unsigned int*)(buf+28)); //our external IP reported by STUN
  tr.saddr.sin_addr.s_addr=tr.our_wanip;
  //if(TR_DBG) printf("wait_udp: STUNanswer wan IP is %s:%d\r\n", inet_ntoa(tr.saddr.sin_addr),tr.our_wanport);  //notify
  //make control with our remote UDP adress
  (*(unsigned int*)(tr.workbuf+8))=tr.our_wanip;
  (*(unsigned short*)(tr.workbuf+6))=tr.our_wanport;
  make_control(tr.workbuf, CONTROL_WAN);
  if(tr.state_in>=STATE_WORK)
  {
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_IN;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   send(tr.sock_tcp_in, (char*)tr.outbuf, len, MSG_NOSIGNAL); //send invite for direct connecting to remote part
  }
  if(tr.state_out>=STATE_WORK)
  {
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_OUT;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   send(tr.sock_tcp_out, (char*)tr.outbuf, len, MSG_NOSIGNAL);
  }
  return 0;
 }

 //process received data
 len=rn_unhide(buf);  //unhide received data
 if((len>119)||(len<0))
 {
  call_terminate(1);
  return 0;
 }
 len+=0x10; //in UDP body and tail reseived as whole packet
 //if(TR_DBG) printf("wait_udp: rn_unhide=%d\r\n", len);

 len=rn_decrypt(buf, len); //check mac and decrypt data
 if(!len) return 0;
 buf[1]^=OBF_UDP;
 i=check_control(buf); //check received packet is control
 //if(TR_DBG) printf("wait_udp: control=%d\r\n", len);

 if((i==CONTROL_CONNECT)&&(tr.state_udp==STATE_WAIT_ACK))
 {  //process received control
  tr.state_udp=STATE_WORK; //set work status if control is valid
  //set remote UDP adress is sender adress
  memset(&tr.saddrUDPTo, 0, sizeof(tr.saddrUDPTo));
  tr.saddrUDPTo.sin_family = AF_INET;
  tr.saddrUDPTo.sin_port = tr.saddr.sin_port;
  tr.saddrUDPTo.sin_addr.s_addr = tr.saddr.sin_addr.s_addr;
  tr.soc_event=EVENT_DIRECT_ACTIVE; //set event direct connecting is active
  jt_set_channel_type(CHANNEL_UDP); //set udp voice buffer type
  //if(TR_DBG) printf("wait_udp: Direct ACK received from %s:%d, UDP in work\r\n", inet_ntoa(tr.saddr.sin_addr), htons(tr.saddr.sin_port));
  //send CONTROL_CONNECT back to sender
  rn_getrnd(tr.workbuf, 16);
  make_control(tr.workbuf, CONTROL_CONNECT);
  memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
  tr.outbuf[1]^=OBF_UDP;
  len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
  sendto(tr.sock_udp, (const char*)tr.outbuf, len, 0, (const struct sockaddr*)&tr.saddrUDPTo, sizeof(tr.saddrUDPTo));
  return 0;
 }

 if(tr.state_udp<STATE_WORK) return 0;

 if(i==CONTROL_VOICE) //incoming voice packet
 {
  jt_add_tf_speech(buf, len); //queue voice
  set_channel_statistic(-1);  //measure statistic for obtaing slowest channel
 }
 else if(i==CONTROL_CR) //data for CR module
 {
  return len;
 }
 else if(i==CONTROL_DIR)
 {
  //if(TR_DBG) printf("wait_udp: recv close control over UDP\r\n");
  close_udp(); //close udp if close control received
  return 0;
 }

 if(i==CONTROL_DISCONNECT)
 {
  //if(TR_DBG) printf("wait_udp: recv terminate control over UDP\r\n");
  call_terminate(1); //terminate call if term control received
 }
 return 0;

}

//------------------------------------------------------------------
//Close TCP incoming connecting
//------------------------------------------------------------------
void close_in(void)
{
 
 int len;

  //if(TR_DBG) printf("close_in: state=%d\r\n", tr.state_in);

  if(tr.sock_tcp_in!=INVALID_SOCKET)
  {
   if(tr.state_in>=STATE_WORK)
   {
    make_control(tr.workbuf, CONTROL_DISCONNECT);
    memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
    tr.outbuf[1]^=OBF_IN;
    len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
    send(tr.sock_tcp_in, (char*)tr.outbuf, len, MSG_NOSIGNAL);
   }
   close(tr.sock_tcp_in);
   tr.sock_tcp_in=INVALID_SOCKET;
  }
  tr.state_in=0;
}

//------------------------------------------------------------------
//Close TCP outgoing connecting
//------------------------------------------------------------------
void close_out(void)
{
 
 int len;

 //if(TR_DBG) printf("close_out: state=%d\r\n", tr.state_out);

 if(tr.sock_tcp_out!=INVALID_SOCKET)
 {
   if(tr.state_out>=STATE_WORK)
   {
    make_control(tr.workbuf, CONTROL_DISCONNECT);
    memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
    tr.outbuf[1]^=OBF_OUT;
    len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
    send(tr.sock_tcp_out, (char*)tr.outbuf, len, MSG_NOSIGNAL);
   }
   close(tr.sock_tcp_out);
   tr.sock_tcp_out=INVALID_SOCKET;
 }
 tr.state_out=0;
}

//------------------------------------------------------------------
//Close UDP direct connecting
//------------------------------------------------------------------
void close_udp(void)
{
 int len;
 //if(TR_DBG) printf("close_udp: state=%d\r\n", tr.state_udp);

 if(tr.state_udp) //check is our direct active in any state
 {  //send DENY control to remote over all avaliable channel
  make_control(tr.workbuf, CONTROL_DIR);
  if(tr.sock_udp!=INVALID_SOCKET)
  {
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_UDP;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   sendto(tr.sock_udp, (const char*)tr.outbuf, len, 0, (const struct sockaddr*)&tr.saddrUDPTo, sizeof(tr.saddrUDPTo));
  }
  if(tr.state_out>=STATE_WORK)
  {
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_OUT;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   send(tr.sock_tcp_out, (char*)tr.outbuf, len, MSG_NOSIGNAL);
  }
  if(tr.state_in>=STATE_WORK)
  {
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_IN;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   send(tr.sock_tcp_in, (char*)tr.outbuf, len, MSG_NOSIGNAL);
  }
 }
 if(tr.sock_udp!=INVALID_SOCKET) close(tr.sock_udp);  //close UDP socket if exist
 tr.sock_udp=INVALID_SOCKET;
 tr.state_udp=0;  //clear UDP state
 tr.their_lanip=0;  //clear their UDP IP and ports (LAN and WAN)
 tr.their_lanport=0;
 tr.their_wanip=0;
 tr.their_wanport=0;
 tr.soc_event=EVENT_DIRECT_DENY;
 jt_set_channel_type(CHANNEL_DEF); //back to default voice buffer type
}

//------------------------------------------------------------------
//Terminate all connectings and set iddle state
//------------------------------------------------------------------
void call_terminate(int back)
{
  
  int len;
  //if(TR_DBG) printf("call_terminate\r\n");

  make_control(tr.workbuf, CONTROL_DISCONNECT);  //make terminator, send over all connectings
  if(tr.state_in>=STATE_WAIT_ACK)
  {
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_IN;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   send(tr.sock_tcp_in, (char*)tr.outbuf, len, MSG_NOSIGNAL);
  }
  if(tr.state_out>=STATE_WAIT_ACK)
  {
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_OUT;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   send(tr.sock_tcp_out, (char*)tr.outbuf, len, MSG_NOSIGNAL);
  }
  if(tr.state_udp>=STATE_WAIT_ACK)
  {
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_UDP;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   sendto(tr.sock_udp, (char*)tr.outbuf, len, 0, (const struct sockaddr*)&tr.saddrUDPTo, sizeof(tr.saddrUDPTo));
  }


  close_in(); //close tcp incoming
  close_out(); //close tcp outgoing
  close_udp(); //close udp


  rn_rstkeys(); //after make_control and close !!!

  //clear data buffers
  memset(tr.workbuf, 0, MAXDATALENGTH);  //work data buffer

  memset(tr.readbuf_in, 0, MAXDATALENGTH);  //read buffer for icoming TCP connecting
  tr.readlen_in=16; //byte to read over incoming TCP connecting
  tr.readptr_in=0;  //pointer to read for incoming TCP connecting

  memset(tr.readbuf_out, 0, MAXDATALENGTH); //read buffer for outgoing TCP connecting
  tr.readlen_out=16; //byte to read over outgoing TCP connecting
  tr.readptr_out=0;  //pointer to read for ougoing TCP connecting


  memset(tr.torbuf, 0, sizeof(tr.torbuf)); //SOCK5 adress request for Tor
  tr.tor_len=0;  //length of SOCK5 adress request

  //clear socket adress structures
  memset(&tr.saddrUDPTo, 0, sizeof(struct sockaddr_in));  //udp destination address structure
  memset(&tr.saddr, 0, sizeof(struct sockaddr_in));  //work address structure

  //clear their inrefaces
  tr.their_ip=0; //their interface for tcp/tor connecting
  tr.their_port=0;

  //clear their interfaces for direct udp connecting
  tr.their_lanip=0;
  tr.their_lanport=0;
  tr.their_wanip=0;
  tr.their_wanport=0;

  //clear our interfaces for direct udp connecting
  tr.our_lanport=0;
  tr.our_wanip=0; //obtained by STUN
  tr.our_wanport=0;

  //clear timers
  tr.time_for_close=0xFFFFFFFF;  //time for call terminate
  tr.time_for_connect=0; //time for send direct invite
  tr.time_for_ping=0; //time for keepalive

  //clear counters
  tr.control_recv_cnt=0;  //counter of incoming control packets
  tr.control_send_cnt=0; //counter of outgoing control packets
  tr.data_recv_cnt=0;     //counter of incoming data packets
  tr.data_recv_in=0;  //incoming channel statistic
  tr.data_recv_out=0; //outgoing channel statistic

  //clear states
  tr.state_in=0;  //status of tcp incoming connection
  tr.state_out=0; //status of tcp outgoing connection
  tr.state_udp=0;    //status of udp outgoing

  //clear flags and set event
  tr.dbl_request=0; //flag of request was sended
  tr.istor=0;
  tr.isready=0;
  tr.obf=0; 

  if(back) tr.soc_event=EVENT_TERMINATED; //set event call was terminated
}




//*************************************************************************
//                               INTERFACE
//*************************************************************************



//------------------------------------------------------------------
//Initialize TCP listening interface and resolve STUN adress
//------------------------------------------------------------------
int tr_init(unsigned char* data)
{
  //data format: header[1], wan_flag[1], tcp_port[2], tor_port[2], stun_address[64]
  unsigned long opt = 1; //for ioctl
  int flag=1; //for setsockopt
  

  char str[256];
  char* p;
  struct hostent *hh; //for resolving

  char direct;    //flag of listening WAN or localhost only
  unsigned short tcpprt;  //listening tcp port
  char* stun_address=(char*)data+4; //domain of stun-server

  //if(TR_DBG) printf("tr_init: d=%d, tcp=%d, tor=%d, stun=%s\r\n", direct, tcpprt, torprt, stun_address);


  //convert data to parameters
  direct=(char)data[1]&1; //flag of TCP if
  tcpprt=data[3];       //TCP listening port
  tcpprt=(tcpprt<<8)+data[2];
  if(tcpprt<1) tcpprt=DEFTCPPORT;

  //CHECK LISTENER ALREADY EXIST AND CLOSE IT OR INITIALISE SOCKETS FIRST
  //create tcp listener if not exist: only once on application start
  if(tr.sock_listener==INVALID_SOCKET) //check for tcp listener already exist
  {

  }
  else //listener already exist
  {
   close(tr.sock_listener);   //close listener
   tr.sock_listener=INVALID_SOCKET;   //set socket invalid
  }


  //CREATE TCP LISTENER AND BIND TO SPECIFIED INTERFACE

   //create tcp listener
   if((tr.sock_listener = socket(AF_INET, SOCK_STREAM, 0)) <0)
   {
    perror("Listener");
    tr.sock_listener=INVALID_SOCKET;
    tr.soc_event=EVENT_INIT_FAIL;
    return -1;
   }

   //set reuseable adressing
   if(setsockopt(tr.sock_listener, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag)))
   {
    perror("Listener SO_REUSEADDR");
    close(tr.sock_listener);
    tr.sock_listener=INVALID_SOCKET;
    tr.soc_event=EVENT_INIT_FAIL;
    return -2;
   }

   //set address structure to listener interface
   memset(&tr.saddr, 0, sizeof(tr.saddr));
   tr.saddr.sin_family = AF_INET;
   tr.saddr.sin_port = htons(tcpprt);
   if(direct) tr.saddr.sin_addr.s_addr=INADDR_ANY;
   else tr.saddr.sin_addr.s_addr=INADDR_LOCAL;

   //bind listener
   if (bind(tr.sock_listener, (struct sockaddr*)&tr.saddr, sizeof(tr.saddr)) < 0)
   {
    perror("Listener bind");
    close(tr.sock_listener);
    tr.sock_listener=INVALID_SOCKET;
    tr.soc_event=EVENT_INIT_FAIL;
    return -3;
   }

   tr.listen_port=tcpprt; //our tcp lsitener

    //unblock listener socket
   opt=1;
   ioctl(tr.sock_listener, FIONBIO, &opt);

   //set lestening mode for waiting 2 incoming at a time
   listen(tr.sock_listener, 2);

   //if(TR_DBG) printf("tr_init: Listen on %s:%d\r\n", inet_ntoa(tr.saddr.sin_addr), htons(tr.saddr.sin_port));

//----------------------------------------------------------------------
//GET LOCAL IP FOR UDP DIRECT CONNECTING INTO LAN 
  tr.our_lanip=get_local_if(0); 
  tr.our_lanport=0; 
//----------------------------------------------------------------------
//SET ADRESS OF USED STUN SERVER
  //set adress of stun server for obtaining external IP/port
  tr.stun_port=0;
  if(stun_address[0])
  {
   stun_address[31]=0;
   strncpy(str, stun_address, sizeof(str)); //safe copy remote adress string
   //search port delimeter, obtain port
   p=strchr(str, ':');
   if(p)
   {
    p[0]=0;
    p++;
    tr.stun_port=atoi(p);
   } else tr.stun_port=DEFSTUNPORT;
   if(tr.stun_port<1) tr.stun_port=DEFSTUNPORT;

   //check stun address is IP, resolve it
   tr.stun_ip=inet_addr(str);
   if(tr.stun_ip==INADDR_NONE) //check for adress string is not IP
   {
    hh = gethostbyname(str); //resolve domain name
    if (hh == 0)
    {
     //if(TR_DBG) printf("unknown stun %s\r\n", str); //if no DNS reported
    }
    else
    {
     memcpy((char *) &tr.stun_ip, (char *) hh->h_addr, sizeof(tr.stun_ip));
    }
    if(tr.stun_ip==INADDR_NONE) tr.stun_port=0;
    tr.saddr.sin_addr.s_addr=tr.stun_ip;
    //if(TR_DBG) printf("tr_init: STUN IP: %s:%d\r\n", inet_ntoa(tr.saddr.sin_addr), tr.stun_port);  //notify
   } //if(tr.stun_ip==INADDR_NONE)
  } //if(stun_address[0])

  if(tr.listen_port > USE_TELNET) fl_init(tr.listen_port+1); //run EKG telnet server telten on tcpport+1

  tr.soc_event=EVENT_INIT_OK;
  return 0;

}



//------------------------------------------------------------------
//Setup outgoing conneting to specified adress IP or domain format
//Custom port can be specified too.
//If .onion suffix is specified Tor will be used
//Terminate call if address not specified.
//rerturns 1 for start Tor call, -1 for start TCP call or 0 for skip/error
//------------------------------------------------------------------
int tr_call_setup(char* address)
{
 
 char str[256];
 char* p;
 int i;
 unsigned long naddr;
 struct hostent *hh; //for resolving
 unsigned char tor_flag=0;



 //check for empty adress and terminate active call
 if((!address)||(!address[0]))
 {
  if(address) rn_setrnd((unsigned char*)address+16); //reseed PRNG
  //if(TR_DBG) printf("tr_call_setup: 0\r\n");
  call_terminate(1);
  return 0;
 }
 //if(TR_DBG) printf("tr_call_setup to %s\r\n", address);


 //check for outgoing call is active now
 if(tr.state_out) return 0; //ignored, not terminted active call!!!

 //Check for port is specified in address string
 address[32]=0;
 strncpy(str, address, sizeof(str)); //safe copy remote adress string
 p=strchr(str,':'); //search port delimiter
 if(p) //port is specified
 {
  p[0]=0; //separate port from adress
  p++; //set pointer to port string
  i=atoi(p); //convert port string to integer
 } else i=0;
 if((i>0)&&(i<65536)) tr.their_port=i; else tr.their_port=DEFTCPPORT; //set specified or sefault port

 //if(TR_DBG) printf("tr_call_setup: port is %d\r\n", tr.their_port);

 //check for adress will be processed by Tor or by direct TCP
 p=strstr(str, ".onion"); //check adress is for Tor
 if(p)
 {
  p[0]=0; //skip .onion suffix
  tor_flag=1; //and set Tor flag
 }

 //check for call in progress now
 if(tr.isready)
 {  //only can be doubling
  if((!tr.istor)||(!tor_flag)) return 0; //not call tor or not call call
  tr.soc_event=EVENT_DOUBLING_OUT;
 }
 else if(!tr.state_in) //new outgoing call
 {
  if(tor_flag) //check tor
  {       //new outgoing connecting over tor
   tr.istor=1;
   tr.soc_event=EVENT_CALL_TOR;
   jt_reset_speech(); //reset speech jitter buffer
   jt_set_channel_type(CHANNEL_TOR);
   tr.time_for_close=tr.time_now+MAX_TOR_TIME; //set timeout for connect to local TOR
   tr.time_for_connect=tr.time_now-1; //connect immediately
   tr.time_for_ping=tr.time_now+PING_INTERVAL;  //180 sec
  }
  else
  {
   tr.soc_event=EVENT_CALL_TCP; //new outgoing over tcp
   jt_reset_speech(); //reset speech jitter buffer
   jt_set_channel_type(CHANNEL_TCP);
   tr.time_for_close=tr.time_now+MAX_TCP_TIME; //set timeout for connect to remote TCP
   tr.time_for_connect=tr.time_now-1; //connect immediately
   tr.time_for_ping=tr.time_now+PING_INTERVAL;  //180 sec
  }
 }
 else return 0;  //try outgoing call during ike

 //check the torified environment
 if((!tr.tor_port) && tor_flag)
 {
  //check for no dots in address string and append by .onoin suffix
  if(!strchr(str, '.'))  strcpy(str+strlen(str), (char*)".onion");
  //if(TR_DBG) printf("tr_call_setup: Setup Tor call to %s over torified TCP\r\n", str);
  tor_flag=0; //clear tor_flag: TCP will be used
 }

 naddr=inet_addr(str); //check for specified address is IP-address

 //setup Tor adress
 if(tor_flag)
 {
  //check for no dots in address string and append by .onoin suffix
  if(!strchr(str, '.'))  strcpy(str+strlen(str), (char*)".onion");

  //if(TR_DBG) printf("tr_call_setup: Setup Tor call to %s\r\n", str);
  //Make socks5 request in tr.torbuf
  strcpy((char*)tr.torbuf+5, str); //hostname or IP string
  i=4; //IPv4 Len
  tr.torbuf[3]=0x01; //for IPv4 socks request
  //check for adress string is IP, replace string by integer
  if(naddr!= INADDR_NONE) (*(unsigned int *)(tr.torbuf+4)) = (unsigned int)naddr;
  else //or use string as a hostname for Tor
  {
   i=strlen((const char*)tr.torbuf+5); //length of hostname string
   tr.torbuf[3]=0x03; //for hostname socks request
  }
  tr.torbuf[4]=i;  //length of hostname or integer IP
  tr.torbuf[0]=0x05; //socks5 ver
  tr.torbuf[1]=0x01; //socks request type: connect
  tr.torbuf[2]=0x00; //reserved
  tr.torbuf[i+5]=(tr.their_port>>8);
  tr.torbuf[i+6]=(tr.their_port&0xFF); //remote port
  tr.tor_len=i+7; //total length of request packet

  tr.their_ip=INADDR_LOCAL; //set connetctin target is a Tor SOCKS5 interface
  tr.their_port=tr.tor_port;
 }

 //setup TCP adress
 else
 {
  //if(TR_DBG) printf("tr_call_setup: TCP call to %s\r\n", str);
  if(naddr==INADDR_NONE) //check for adress string is not IP
  {
   hh = gethostbyname(str); //resolve domain name
   if (hh == 0) //if no DNS reported
   {
    //if(TR_DBG) printf("unknown host %s\r\n", str);
    tr.their_port=0; //clear their interface
    call_terminate(1);
    return 0;
   }
   memcpy((char *) &naddr, (char *) hh->h_addr, sizeof naddr);
   tr.saddr.sin_addr.s_addr=naddr;
   //if(TR_DBG) printf("tr_call_setup: Resolved: %s\r\n", inet_ntoa(tr.saddr.sin_addr));  //notify
  }
  tr.their_ip=naddr; //set their IP
 }

 if(tor_flag) return 1; else return -1;

}




//------------------------------------------------------------------
//Allowing or deny UDP direct connecting after TCP was connect
//For start NAT traversal remote party must allowed too
//------------------------------------------------------------------
void tr_direct_setup(int on)
{
  
  unsigned long opt = 1; //for ioctl
  int len;

  //if(TR_DBG) printf("tr_direct_setup: %d\r\n", on);

  if(!on)  //closing UDP
  {
   //force close old UDP direct connecting
   tr.state_udp=1; //force closing
   close_udp();  //close UDP
   tr.soc_event=0; //mask EVENT_DIRECT_DENY for local closing
   return;
  }

  //clear old UDP values
  if(tr.sock_udp!=INVALID_SOCKET) close(tr.sock_udp);  //close UDP socket if exist
  tr.sock_udp=INVALID_SOCKET;
  tr.state_udp=0;  //clear UDP state



  //at least one tcp channel is must be work
  if((tr.state_in<STATE_WORK) && (tr.state_out<STATE_WORK)) return;

  //create new UDP socket
  if ((tr.sock_udp = socket(AF_INET, SOCK_DGRAM, 0)) <0)
  {
    //if(TR_DBG) printf("UDP Socket creating error\r\n");
    return;
  }

  //maybe set reuseable address here

  //set our local port as a random
  rn_getrnd(tr.workbuf, 16); //generate random port
  tr.our_lanport=tr.workbuf[1];
  tr.our_lanport<<=8;
  tr.our_lanport+=tr.workbuf[0];
  tr.our_lanport|=1024;
  if(tr.our_lanport==tr.stun_port) tr.our_lanport^=8;

  //set remote adress structure for bind
  memset(&tr.saddr, 0, sizeof(tr.saddr));
  tr.saddr.sin_family = AF_INET;
  tr.saddr.sin_port = htons(tr.our_lanport);
  tr.saddr.sin_addr.s_addr=INADDR_ANY;

  //bind udp socket to port
  if (bind(tr.sock_udp, (struct sockaddr*)&tr.saddr, sizeof(tr.saddr)) < 0)
  {
   //if(TR_DBG) printf("UDP Socket bining error\r\n");
   close(tr.sock_udp);
   tr.sock_udp=INVALID_SOCKET;
   tr.our_lanport=0;
   return;
  }

  //unblock udp socket
  opt=1;
  ioctl(tr.sock_udp, FIONBIO, &opt);

  tr.state_udp=STATE_WAIT_ACK;
  //if(TR_DBG) printf("tr_direct_setup: LAN port=%d\r\n", tr.our_lanport);
 //-------------------------------------------------------------------

   //if address of STUN server is specified send request of our WAN interface
   if(tr.stun_port)
   {
    //prepare STUN request
    memset(tr.workbuf, 0, 28);
    tr.workbuf[1]=1; //type: request
    tr.workbuf[3]=8; //len of atributes
    tr.workbuf[21]=3; //Change
    tr.workbuf[23]=4; //len of value
    rn_getrnd(tr.workbuf+4, 16);//16 bytes of random data

    //prepare adress structure of STUN server address
    memset(&tr.saddr, 0, sizeof(tr.saddr));
    tr.saddr.sin_family = AF_INET;
    tr.saddr.sin_port = htons(tr.stun_port); //port of used STUN server
    tr.saddr.sin_addr.s_addr=tr.stun_ip; //IP adress of used STUN server

    //send STUN request
    sendto(tr.sock_udp, (char*)tr.workbuf, 28, 0, (const struct sockaddr*)&tr.saddr, sizeof(tr.saddr));
    //if(TR_DBG) printf("tr_direct_setup: send STUN req\r\n");
   }


   //make control with our local UDP adress
  (*(unsigned int*)(tr.workbuf+8))=tr.our_lanip;
  (*(unsigned short*)(tr.workbuf+6))=tr.our_lanport;
  make_control(tr.workbuf, CONTROL_LAN);
  //if(TR_DBG) printf("tr_direct_setup: send LAN invite: in:%d out:%d\r\n", tr.state_in, tr.state_out);
  if(tr.state_in>=STATE_WORK)
  {
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_IN;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   send(tr.sock_tcp_in, (char*)tr.outbuf, len, MSG_NOSIGNAL); //send invite for direct connecting to remote part
  }
  if(tr.state_out>=STATE_WORK)
  {
   memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
   tr.outbuf[1]^=OBF_OUT;
   len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
   send(tr.sock_tcp_out, (char*)tr.outbuf, len, MSG_NOSIGNAL);
  }

  return;
}

//wrapper for processign data from tf module
//bit7 - control flag, bit6 - fragmentation flag
//bit5 - gui destination flag (bit4=0: from cryptor, bit4=1 : from tf)
//bit4 - tr destination flag: 0-commands for remote cryptor, 1 - commands for local tr

void tr_process(unsigned char* buf, int len)
{

 if(!(buf[0]&0x80)) //check control flag is 0: this is speech packet
 {
  len=jt_add_cr_speech(buf); //16 bytes speech packet from cryptor: compose
  if(len)
  {
   //jt_add_tf_speech(buf, len); //for loopback test
   tr_send_to_remote(buf, len); //composed voice data send to remote
  }
 }
 else switch(buf[0])
 {
  case TR_A:{tr_send_to_remote(buf, 28);break;} //cr-to-remote_cr transpass SPEKE A pkt
  case TR_Q:{tr_send_to_remote(buf, 36);break;} //cr-to-remote_cr transpass IKE and SPEKE Q pkt
  case TR_K:{tr_send_to_remote(buf, 52);break;} //cr-to-remote_cr transpass PUBKEY pkt
  case TR_S:{tr_send_to_remote(buf, 64);break;} //cr-to-remote_cr transpass ADRESS pkt
  case TR_SET:{tr_call_setup((char*)buf+4);break;} //start/term call
  case TR_DIR:{tr_direct_setup(buf[1]);break;} //start/term direct
  case TR_SEC:{tr_set_secret(buf);break;} //store common secret
  case TR_INIT:{tr_init(buf);;break;} //bind transport interface
  case TR_SEED:{tr_create(buf);break;} //seed TR by rand
  case TR_RST:{call_terminate(1);break;} //reset call
  case TR_MUTE:{jt_set_talk(1);break;} //mute voice queue
  case TR_REP:{tr_rep(buf);break;} //send representation to remote
  case TR_OBF:{tr_obf(buf);break;} //set keymaterial for session obfuscation
 }
}


//------------------------------------------------------------------
// send len bytes in buf to remote party over all avaliable channels
//------------------------------------------------------------------
void tr_send_to_remote(unsigned char* buf, int len)
{
 
 int plen;
 int ret=0;
 int ctrl=0;

 if(tr.rcvblk || (len>124)) return;  //check block flag is reset and len is valid
 if(buf[0]&0x80) ctrl=0x80; //check for remote control/data packets



 //check for UDP is avaliable
 if(tr.state_udp>=STATE_WORK)
 {
  //if(TR_DBG) printf("tr_send_to_remote1: udp len=%d\r\n", len);
  tr.state_udp=STATE_SEND;
  memcpy(tr.outbuf, buf, len);
  tr.outbuf[1]^=OBF_UDP;
  plen=rn_encrypt(tr.outbuf, len+ctrl);
  sendto(tr.sock_udp, (char*)tr.outbuf, plen, 0, (const struct sockaddr*)&tr.saddrUDPTo, sizeof(tr.saddrUDPTo));
  ret+=4;
 }
 else  //if UDP is unavaliable, send over tcp
 {

  if(tr.state_in>=STATE_WORK) //check for incoming TCP is avaliable
  {  //send and terminate call on disonnect event
   tr.state_in=STATE_SEND;
   //if(TR_DBG) printf("tr_send_to_remote1: in len=%d\r\n", len);
   memcpy(tr.outbuf, buf, len);
   tr.outbuf[1]^=OBF_IN;
   plen=rn_encrypt(tr.outbuf, len+ctrl);
   if(plen!=send(tr.sock_tcp_in, (char*)tr.outbuf, plen, MSG_NOSIGNAL))  call_terminate(1);  //terminate call
   else ret+=2;
  }

  if(tr.state_out>=STATE_WORK)//also chech for outgoing TCP is avaliable
  {  //send and recall on disonnect event
   tr.state_out=STATE_SEND;
   //if(TR_DBG) printf("tr_send_to_remote1: out len=%d\r\n", len);
   memcpy(tr.outbuf, buf, len);
   tr.outbuf[1]^=OBF_OUT;
   plen=rn_encrypt(tr.outbuf, len+ctrl);
   if(plen!=send(tr.sock_tcp_out, (char*)tr.outbuf, plen, MSG_NOSIGNAL)) 
   {
    if(tr.state_in>=STATE_WORK) close_out(); //recall
   }
   else ret+=1;
  }

 }
  return;

}



//------------------------------------------------------------------
// receive data from remote party, return length or 0
// must call periodically as a heartbeat for transport module
//------------------------------------------------------------------
int tr_read_from_remote(unsigned char* buf)
{
 
 int ret=0;

 ret=jt_ext_cr_speech(buf); //check for next speech packet is ready
 if(ret) return ret;
 ret=wait_timer(buf);  //check for events
 if(ret) return ret;
 if(tr.state_udp) ret=wait_udp(buf); //poll udp
 if(ret) return ret;
 if(tr.state_in) ret=wait_tcpin(buf); //poll tcpin
 if(ret) return ret;
 if(tr.state_out) ret=wait_tcpout(buf); //poll tcpout
 if(ret) return ret;
 wait_accept(); //poll listener for new incoming

 return ret;

}

//-----------------------------------------------------------------
// First initialization of transport module
//-----------------------------------------------------------------
void tr_create(unsigned char* data)
{
  unsigned short torprt;
  unsigned char* p=data+4; //rand

  //get port of Tor sock5 interface
  torprt=data[3];
  torprt=(torprt<<8)+data[2];

  rn_create(p); //init obfuscation module and seed PRNG
  jt_reset_speech(); //reset speech jitter buffer

  tr.sock_listener=INVALID_SOCKET; //tcp listening socket
  tr.sock_tcp_in=INVALID_SOCKET;   //tcp accepting socket (incoming)
  tr.sock_tcp_out=INVALID_SOCKET;  //tcp connected socket (outgoing)
  tr.sock_udp=INVALID_SOCKET;      //udp direct socket


  //if(TR_DBG) printf("tr_create\r\n");

 //clear data buffers
  memset(tr.workbuf, 0, MAXDATALENGTH);  //work data buffer

  memset(tr.readbuf_in, 0, MAXDATALENGTH);  //read buffer for icoming TCP connecting
  tr.readlen_in=16; //byte to read over incoming TCP connecting
  tr.readptr_in=0;  //pointer to read for incoming TCP connecting

  memset(tr.readbuf_out, 0, MAXDATALENGTH); //read buffer for outgoing TCP connecting
  tr.readlen_out=16; //byte to read over outgoing TCP connecting
  tr.readptr_out=0;  //pointer to read for ougoing TCP connecting


  memset(tr.torbuf, 0, sizeof(tr.torbuf)); //SOCK5 adress request for Tor
  tr.tor_len=0;  //length of SOCK5 adress request

  //clear socket adress structures
  memset(&tr.saddrUDPTo, 0, sizeof(struct sockaddr_in));  //udp destination address structure
  memset(&tr.saddr, 0, sizeof(struct sockaddr_in));  //work address structure

  //clear their inrefaces
  tr.their_ip=0; //their interface for tcp/tor connecting
  tr.their_port=0;

  //clear their interfaces for direct udp connecting
  tr.their_lanip=0;
  tr.their_lanport=0;
  tr.their_wanip=0;
  tr.their_wanport=0;

  //clear our interfaces for direct udp connecting
  tr.our_lanport=0;
  tr.our_wanip=0; //obtained by STUN
  tr.our_wanport=0;

  //clear timers
  tr.time_for_close=0xFFFFFFFF;  //time for call terminate
  tr.time_for_connect=0; //time for send direct invite
  tr.time_for_ping=0; //time for keepalive

  //clear counters
  tr.control_recv_cnt=0;  //counter of incoming control packets
  tr.control_send_cnt=0; //counter of outgoing control packets
  tr.data_recv_cnt=0;     //counter of incoming data packets
  tr.data_recv_in=0;  //incoming channel statistic
  tr.data_recv_out=0; //outgoing channel statistic

  //clear states
  tr.state_in=0;  //status of tcp incoming connection
  tr.state_out=0; //status of tcp outgoing connection
  tr.state_udp=0;    //status of udp outgoing

  //clear flags and set event
  tr.dbl_request=0; //flag of request was sended
  tr.istor=0;
  tr.isready=0;
  tr.rcvblk=0;
  tr.obf=0;
  tr.soc_event=0; //set event call was terminated

  tr.tor_port = torprt; //can be o for always tcp connecl (in torificated environment)


   //check for first start and inialize WinSocket (windows only)

  #ifdef _WIN32
    //Initializing WinSocks
   if (WSAStartup(0x202, (WSADATA *)&sock_buf[0]))
   {
    //if(TR_DBG) printf("ti_init: WSAStartup error: %d\n", WSAGetLastError());
   }
  #endif
  
  tr.soc_event=EVENT_SEED_OK; //set event: TR created OK

}


//-----------------------------------------------------------------
//update keys using secret with optional remote notification BEFORE
//-----------------------------------------------------------------
void tr_set_secret(unsigned char* data)
{
 int len;

 //send control packet to remote
 make_control(tr.workbuf, CONTROL_SECRET); //make control for notifyremote side
 if(tr.state_out>=STATE_WORK) //only originator send remote notification
 {
  memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
  tr.outbuf[1]^=OBF_OUT;
  len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
  send(tr.sock_tcp_out, (char*)tr.outbuf, len, MSG_NOSIGNAL);
 }
 if(tr.state_in>=STATE_WORK) //only originator send remote notification
 {
  memcpy(tr.outbuf, tr.workbuf, CTRL_LEN);
  tr.outbuf[1]^=OBF_IN;
  len=rn_encrypt(tr.outbuf, CTRL_LEN + CTRL_FLAG);
  send(tr.sock_tcp_in, (char*)tr.outbuf, len, MSG_NOSIGNAL);
 }

 if(tr.rcvblk==2) //check secret was requested by remote
 {
  //apply secret
  rn_set_secret(data+4); //save secret for updating keys
  rn_upd_ekey(); //update tx  key on our side
  rn_upd_dkey(); //update rx  key on our side
  tr.isready=1;  //set IKE compleet flag
  tr.time_for_close=tr.time_now+MAX_SESSION_TIME; //set timeout for session
  tr.rcvblk=0;
  tr.soc_event=EVENT_SEC;
 }
 else  //initilize secret
 {  //store secret
  rn_set_secret(data+4); //save secret for updating keys
  tr.rcvblk=1;
  tr.soc_event=EVENT_REQ;
 }
}

//-----------------------------------------------------------------
//CONTROL_SECRET received: update secrets
//-----------------------------------------------------------------
short tr_set_remote(unsigned char* data)
{
 short ret=0;

 if(tr.rcvblk==1) //secret already initialized by TR_SEC
 {
  rn_upd_ekey(); //update tx  key on our side
  rn_upd_dkey(); //update rx  key on our side
  tr.isready=1;  //set IKE compleet flag
  tr.time_for_close=tr.time_now+MAX_SESSION_TIME; //set timeout for session
  tr.rcvblk=0;   //clear flag
  data[0]=EVENT_SEC; //generate event: secret was applied
  ret=4; //answer len
 }
 else //not initialized: request for secret
 {
  tr.rcvblk=2;   //set request flag
  data[0]=EVENT_SEC;//generate event: request secret
  ret=4; //answer len
 }

 return ret;
}

//-----------------------------------------------------------------
//send our representation to remote
//-----------------------------------------------------------------
short tr_rep(unsigned char* data)
{
 unsigned char buf[544];
 short len;
 short* lenptr = (short*)buf;

 rn_getrnd(buf, sizeof(buf)); //fill random
 len=(*lenptr);
 len&=0x1FF;
 len+=32;//len 32 - 544
 memcpy(buf, data+4, 32); //representation

 if(tr.sock_tcp_out!=INVALID_SOCKET) //if out_socket exist (originator)
 {  //send representation to acceptor
  if(len!=send(tr.sock_tcp_out, (char*)buf, len, MSG_NOSIGNAL))  call_terminate(1);
 }

 if(tr.sock_tcp_in!=INVALID_SOCKET) //if in_socket exist (acceptor)
 {  //send representation to originator
  if(len!=send(tr.sock_tcp_in, (char*)buf, len, MSG_NOSIGNAL))  call_terminate(1);
 }

 return 0;
}

//-----------------------------------------------------------------
//set keymaterial for obfuscation
//-----------------------------------------------------------------
short tr_obf(unsigned char* data)
{
 rn_setobf(data+4); //set 16 bytes of keymaterial for obfuscation
 tr.obf=1; //set flag keymaterial ready
 if(tr.istor) tr.time_for_close=tr.time_now+MAX_TOR_TIME+OBF_PAUSE;  //set timeout for reconnect
 else tr.time_for_close=tr.time_now+MAX_TCP_TIME+OBF_PAUSE;

 //close firest connecting
 if(tr.sock_tcp_out!=INVALID_SOCKET) //originator side
 {
  close(tr.sock_tcp_out); //close out socket
  tr.sock_tcp_out=INVALID_SOCKET;
  tr.state_out=0; //clear out state
  tr.time_for_connect=tr.time_now+OBF_PAUSE; //reconnect immediately
 }
 else //acceptor state
 {
  tr.state_in=0; //clear in state
  tr.soc_event=EVENT_OBF_IN; //send event
 }

 return 0;
}
