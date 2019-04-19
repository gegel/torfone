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


//some socket definitions
#ifndef INVALID_SOCKET
 #define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
 #define SOCKET_ERROR -1
#endif

#ifndef MSG_NOSIGNAL
 #define MSG_NOSIGNAL 0
#endif

//some socket definitions
#ifndef INVALID_SOCKET
 #define INVALID_SOCKET -1
#endif

#ifndef INADDR_LOCAL
 #define INADDR_LOCAL 0x0100007F
#endif

#ifndef socklen_t
 #define socklen_t int
#endif


 
#define MAXTCPSIZE 512 //maximal size of internet packet
#define MAXDATA 128    //maximal length of command


#include "telnet.h"


unsigned long  tn_addr=INADDR_NONE; //work address  
struct sockaddr_in tn_saddr;  //work address structure   
int tn_sock=INVALID_SOCKET; //listener socket
int tn_work=INVALID_SOCKET; //work socket
unsigned char tn_tcpinbuf[MAXTCPSIZE]; //tcp receiving buffer
unsigned char tn_tcpdata[MAXDATA]; //data buffer
int tn_tcplen=0;  //length of data string

int tn_listen=0;  //flag of listener enabled
int tn_connect=0; //flag of connecting active
 

//open/close listener on specified port
int tn_init(unsigned short port)
{
  unsigned long opt = 1; //for ioctl
  int flag=1; //for setsockopt
  struct linger struct_linger;
   
  struct_linger.l_onoff  = 1;
  struct_linger.l_linger = 0;
 
  //close old connecting
  tn_connect=0; //clear flags
  tn_listen=0;
  if(tn_work!=INVALID_SOCKET) //check for connecting active
  {
   close(tn_work);   //close connecting
   tn_work=INVALID_SOCKET;   //set socket invalid
  }
  if(tn_sock!=INVALID_SOCKET) //check for tcp listener already exist
  {
   close(tn_sock);   //close listener
   tn_sock=INVALID_SOCKET;   //set socket invalid
  }
  
 
  //convert port from string to integer
  if(!port) return 0;
  
 
  //create tcp listener
   if((tn_sock = socket(AF_INET, SOCK_STREAM, 0)) <0)
   {
    perror("Listener");
    tn_sock=INVALID_SOCKET;
    return -2; //error creating listener
   }

   //set reuseable adressing
   if(setsockopt(tn_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag)))
   {
    perror("Listener SO_REUSEADDR");
    close(tn_sock);
    tn_sock=INVALID_SOCKET;
    return -3;
   }
   
   //set address structure to listener interface
   memset(&tn_saddr, 0, sizeof(tn_saddr));
   tn_saddr.sin_family = AF_INET;
   tn_saddr.sin_port = htons(port);
   if(port>=5000) tn_saddr.sin_addr.s_addr=INADDR_ANY;
   else tn_saddr.sin_addr.s_addr=INADDR_LOCAL;
   
   //bind listener
   if (bind(tn_sock, (struct sockaddr*)&tn_saddr, sizeof(tn_saddr)) < 0)
   {
    perror("Listener bind");
    close(tn_sock);
    tn_sock=INVALID_SOCKET;
    return -4;
   } 
   
     //disable Nagle algo
   if (setsockopt(tn_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)) < 0)
   {
    printf("disable Nagle for accepted socket error\r\n" );
    close(tn_sock);
    tn_sock=INVALID_SOCKET;
    return -5;
   }

 //check connect periodically usign keepalive
  if(setsockopt(tn_sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(flag))<0)
  {
   printf("set keepalive mode error\r\n" );
   close(tn_sock);
   tn_sock=INVALID_SOCKET;
   return -6;
  }

//set linger option: socket will be locally closed immediately with lost of any data               
  if(setsockopt(tn_sock, SOL_SOCKET, SO_LINGER, (char* ) &struct_linger, sizeof(struct_linger))<0)
  {
   printf("set keepalive mode error\r\n" );
   close(tn_sock);
   tn_sock=INVALID_SOCKET;
   return -7;
  }    
  
    //unblock listener socket
  opt=1;
  ioctl(tn_sock, FIONBIO, &opt);

  //set lestening mode for waiting 2 incoming at a time
  listen(tn_sock, 2);
  tn_listen=1; 
  
  printf("Telnet server run on %s:%d\r\n", inet_ntoa(tn_saddr.sin_addr), htons(tn_saddr.sin_port));
  
  return 0; 
   
}

//wsend data to connected client
int tn_write(unsigned char* data, int len)
{ //ckeck connecting ready
 if( (!tn_connect) || (tn_work==INVALID_SOCKET) ) return 0;
 //try send data
 if(len!=send(tn_work, (char*)data, len, MSG_NOSIGNAL))
 {  //send error: 
  tn_connect=0; //clear ready flag
  close(tn_work);   //close work
  tn_work=INVALID_SOCKET;   //set socket invalid
  len=-1; //return error
  printf("Telnet client unaccessible\r\n");
 }
 
 return len;
}


//try read data from connected client and try accept new client
int tn_read(unsigned char* data)
{
 //listen
 if(tn_listen && (tn_sock!=INVALID_SOCKET)) //check listener is valid
 {
  int sTemp; //accepted socket
  int ll=sizeof(tn_saddr);
  //try accept for new connecting
  sTemp  = accept(tn_sock, (struct sockaddr *) &tn_saddr, (socklen_t*)&ll);
  if(sTemp>0)// incoming connections accepted
  {
   unsigned long opt = 1; //for ioctl
   int flag=1; //for setsockopt
   //disable nagle on sTemp ->Err
   if (setsockopt(sTemp, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)) < 0)
   {
    perror( "accept TCP_NODELAY error" );
    close(sTemp);
   }
   else //all OK
   {
    opt=1;
    ioctl(sTemp, FIONBIO, &opt); //unblock socket
	
	if(tn_work!=INVALID_SOCKET) close(tn_work); //close old connecting
	tn_work = sTemp; //set new socket
    tn_connect=1; //set connecting flag
	
	//print incoming address:port from
	printf("Telnet client connected from %s:%d\r\n", inet_ntoa(tn_saddr.sin_addr), htons(tn_saddr.sin_port));
   } 
  }
 }
 
 //read
 if(tn_connect && (tn_work!=INVALID_SOCKET)) //check connecting is valid
 {
  int len;
  
  len=recv(tn_work, (char*)tn_tcpinbuf, sizeof(tn_tcpinbuf), 0); //try read socket
  if(len<0) return 0; //no data
  if(len>0) //data received
  {
   int i;
   unsigned char c;
   for(i=0;i<len;i++) //process byte-by-byte
   {
    c=tn_tcpinbuf[i]; //get next byte
	if((c==0x0D)||(c==0x0A)||(c==0)) //check end of string
	{	 
	 if(len>0) //check we have a string in buffer
	 {
	  len=tn_tcplen; //len of having string
	  tn_tcplen=0; //clear buffer for next
	  memcpy(data, tn_tcpdata, len); //output data
	  data[len]=0; //terminate string
      return len;//return len of received message	  
	 }
	}
    else if((c>=0x20)&&(c<0xFF)) //only for non-control chars
    {	
	 tn_tcpdata[tn_tcplen++]=c; //add next char to buffer and move pointer
	 if(tn_tcplen>=sizeof(tn_tcpdata)) tn_tcplen=0; //skip buffer if overflow
	}
   }
  }
  else //len==0: remote side close connecting
  {
   close(tn_work);   //close work
   tn_work=INVALID_SOCKET;   //set socket invalid
   printf("Telnet client disconnected\r\n");
  }
 }
 return 0;
}