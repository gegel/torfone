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


#define EXTERN_TELNET 7000 //bind wan access to upper telnet ports
#define MAXTCPSIZE 512 //maximal size of internet packet
#define MAXDATA 128    //maximal length of command


#include "file_if.h"


unsigned long  tc_addr=INADDR_NONE; //work address  
struct sockaddr_in tc_saddr;  //work address structure   
int tc_sock=INVALID_SOCKET; //listener socket
int tc_work=INVALID_SOCKET; //work socket
unsigned char tc_tcpinbuf[MAXTCPSIZE]; //tcp receiving buffer
unsigned char tc_tcpdata[MAXDATA]; //data buffer
int tc_tcplen=0;  //length of data string

int tc_listen=0;  //flag of listener enabled
int tc_connect=0; //flag of connecting active
 

//open/close listener on specified port
int tc_init(unsigned short port)
{
  unsigned long opt = 1; //for ioctl
  int flag=1; //for setsockopt
  struct linger struct_linger;
   
  struct_linger.l_onoff  = 1;
  struct_linger.l_linger = 0;
 
  //close old connecting
  tc_connect=0; //clear flags
  tc_listen=0;
  if(tc_work!=INVALID_SOCKET) //check for connecting active
  {
   close(tc_work);   //close connecting
   tc_work=INVALID_SOCKET;   //set socket invalid
  }
  if(tc_sock!=INVALID_SOCKET) //check for tcp listener already exist
  {
   close(tc_sock);   //close listener
   tc_sock=INVALID_SOCKET;   //set socket invalid
  }
  
 
  //convert port from string to integer
  if(!port) return 0;
  
 
  //create tcp listener
   if((tc_sock = socket(AF_INET, SOCK_STREAM, 0)) <0)
   {
    perror("Listener");
    tc_sock=INVALID_SOCKET;
    return -2; //error creating listener
   }

   //set reuseable adressing
   if(setsockopt(tc_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag)))
   {
    perror("Listener SO_REUSEADDR");
    close(tc_sock);
    tc_sock=INVALID_SOCKET;
    return -3;
   }
   
   //set address structure to listener interface
   memset(&tc_saddr, 0, sizeof(tc_saddr));
   tc_saddr.sin_family = AF_INET;
   tc_saddr.sin_port = htons(port);
   if(port>=EXTERN_TELNET) tc_saddr.sin_addr.s_addr=INADDR_ANY;
   else tc_saddr.sin_addr.s_addr=INADDR_LOCAL;
   
   //bind listener
   if (bind(tc_sock, (struct sockaddr*)&tc_saddr, sizeof(tc_saddr)) < 0)
   {
    perror("Listener bind");
    close(tc_sock);
    tc_sock=INVALID_SOCKET;
    return -4;
   } 
   
     //disable Nagle algo
   if (setsockopt(tc_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)) < 0)
   {
    printf("disable Nagle for accepted socket error\r\n" );
    close(tc_sock);
    tc_sock=INVALID_SOCKET;
    return -5;
   }

 //check connect periodically usign keepalive
  if(setsockopt(tc_sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(flag))<0)
  {
   printf("set keepalive mode error\r\n" );
   close(tc_sock);
   tc_sock=INVALID_SOCKET;
   return -6;
  }

//set linger option: socket will be locally closed immediately with lost of any data               
  if(setsockopt(tc_sock, SOL_SOCKET, SO_LINGER, (char* ) &struct_linger, sizeof(struct_linger))<0)
  {
   printf("set keepalive mode error\r\n" );
   close(tc_sock);
   tc_sock=INVALID_SOCKET;
   return -7;
  }    
  
    //unblock listener socket
  opt=1;
  ioctl(tc_sock, FIONBIO, &opt);

  //set lestening mode for waiting 2 incoming at a time
  listen(tc_sock, 2);
  tc_listen=1; 
  
  printf("Telnet server run on %s:%d\r\n", inet_ntoa(tc_saddr.sin_addr), htons(tc_saddr.sin_port));
  
  return 0; 
   
}

//wsend data to connected client
int tc_write(unsigned char* data, int len)
{ //ckeck connecting ready
 if( (!tc_connect) || (tc_work==INVALID_SOCKET) ) return 0;
 //try send data
 if(len!=send(tc_work, (char*)data, len, MSG_NOSIGNAL))
 {  //send error: 
  tc_connect=0; //clear ready flag
  close(tc_work);   //close work
  tc_work=INVALID_SOCKET;   //set socket invalid
  len=-1; //return error
  printf("Telnet client unaccessible\r\n");
 }
 
 return len;
}


//try read data from connected client and try accept new client
int tc_read(unsigned char* data)
{
 //listen
 if(tc_listen && (tc_sock!=INVALID_SOCKET)) //check listener is valid
 {
  int sTemp; //accepted socket
  int ll=sizeof(tc_saddr);
  //try accept for new connecting
  sTemp  = accept(tc_sock, (struct sockaddr *) &tc_saddr, (socklen_t*)&ll);
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
	
	if(tc_work!=INVALID_SOCKET) close(tc_work); //close old connecting
	tc_work = sTemp; //set new socket
    tc_connect=1; //set connecting flag
	
	//print incoming address:port from
	printf("Telnet client connected from %s:%d\r\n", inet_ntoa(tc_saddr.sin_addr), htons(tc_saddr.sin_port));
   } 
  }
 }
 
 //read
 if(tc_connect && (tc_work!=INVALID_SOCKET)) //check connecting is valid
 {
  int len;
  
  len=recv(tc_work, (char*)tc_tcpinbuf, sizeof(tc_tcpinbuf), 0); //try read socket
  if(len<0) return 0; //no data
  if(len>0) //data received
  {
   int i;
   unsigned char c;
   for(i=0;i<len;i++) //process byte-by-byte
   {
    c=tc_tcpinbuf[i]; //get next byte
	if((c==0x0D)||(c==0x0A)||(c==0)) //check end of string
	{	 
	 if(len>0) //check we have a string in buffer
	 {
	  len=tc_tcplen; //len of having string
	  tc_tcplen=0; //clear buffer for next
	  memcpy(data, tc_tcpdata, len); //output data
	  data[len]=0; //terminate string
      return len;//return len of received message	  
	 }
	}
    else if((c>=0x20)&&(c<0xFF)) //only for non-control chars
    {	
	 tc_tcpdata[tc_tcplen++]=c; //add next char to buffer and move pointer
	 if(tc_tcplen>=sizeof(tc_tcpdata)) tc_tcplen=0; //skip buffer if overflow
	}
   }
  }
  else //len==0: remote side close connecting
  {
   close(tc_work);   //close work
   tc_work=INVALID_SOCKET;   //set socket invalid
   printf("Telnet client disconnected\r\n");
  }
 }
 return 0;
}