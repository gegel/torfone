#include "if.h"

 //socket states
#define STATE_IDDLE    0  //inactive (not exist)
#define STATE_WAIT_OBF 1 //after first connect, wait REP
#define STATE_WAIT_TCP 2 //after creating, wait connect to TCP
#define STATE_WAIT_TOR 3 //after creating, wait connect to Tor
#define STATE_WAIT_SOC 4 //after was connected to Tor, wait socks5 hello[<10] or ack[>10]
#define STATE_WAIT_ACK 5 //after rand, wait hello or hungup (obfuscated)
#define STATE_WORK     6   //after hello: work is receive control/voice, can send
#define STATE_SEND     7 //was sended recently

//timings
#define TIMELOOP            5    //number of iteration of main loop for renew timestamp
#define PING_INTERVAL       90 //number of seconds for send keepalive hello
#define CONNECT_INTERVAL    2   //number of seconds for send direct UDP packets for traversal NAT
#define DOUBLE_INTERVAL     10  //number of seconds for recall double connecting

#define MAX_TCP_TIME 5  //number in seconds for connecting to remote TCP
#define MAX_TOR_TIME 2  //number in seconds for connecting to local TOR
#define MAX_ONION_TIME 120 //number in seconds for connecting to remote ONION
#define MAX_IKE_TIME 15  //number in seconds for connecting to IKE
#define OBF_PAUSE 2  //time between pre-ike session and main session
#define MAX_SESSION_TIME    1800 //number of seconds for talk (30 min max)

//channel doubling
#define STAT_PACKETS        300 //5 in 1 sec: total number of packets for measure channel statistic
#define STAT_DELTA          50  // 1/3 to 1/2: minimal difference of packets delevered by channels for initiate change

//default ports
#define DEFSTUNPORT         3478 //default port of STUN server for NAT traversal
#define DEFTORPORT          9150 //default port of SOCKS5 Tor interface
#define DEFTCPPORT          4444  //default TCP listener

//remote control packets
#define CTRL_LEN 0x10  //len of control

#define CONTROL_VOICE      0x90  //voice data
#define CONTROL_DISCONNECT 0x91  //session terminated
#define CONTROL_CONNECT    0x92  //session initiated
#define CONTROL_DOUBLE     0x93  //channel change request
#define CONTROL_CLOSE      0x94  //socket close notification
#define CONTROL_PING       0x95  //keep alive packet
#define CONTROL_LAN        0x96  //nat traversal invite for LAN
#define CONTROL_WAN        0x97  //nat traversal invite for WAN
#define CONTROL_DIR        0x98  //close direct connecting
#define CONTROL_SECRET     0x99  //update rx key
#define CONTROL_CR         0x9A  //packet for Cryptor
#define CONTROL_ERROR      0x9F  //ignored packets

//obfuscation masks
#define OBF_IN 0x0F
#define OBF_OUT 0xF0
#define OBF_UDP 0x00

//buffer sizes
#define MAXDATALENGTH 144   //maximal length of TCP data packet


#ifdef _WIN32

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <basetsd.h>
#include <stdint.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>

#define ioctl ioctlsocket
#define close closesocket
#define EWOULDBLOCK WSAEWOULDBLOCK //no data for assync polling
#define ENOTCONN WSAENOTCONN //wait for connection for assinc polling
#define ECONNRESET WSAECONNRESET //no remote udp interface in local network


#else //linux

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include <stdint.h>
#include <stdarg.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
//#include <ifaddrs.h>

#endif


//some socket definitions
#ifndef INVALID_SOCKET
 #define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
 #define SOCKET_ERROR -1
#endif

#ifndef INADDR_LOCAL
 #define INADDR_LOCAL 0x0100007F
#endif

#ifndef socklen_t
 #define socklen_t int
#endif

#ifndef MSG_NOSIGNAL
 #define MSG_NOSIGNAL 0
#endif

//===================data of Transport module=========================
typedef struct {

 unsigned char workbuf[MAXDATALENGTH];  //work data buffer
 unsigned char outbuf[MAXDATALENGTH];  //send data buffer
 unsigned char readbuf_in[MAXDATALENGTH];  //read buffer for icoming TCP connecting
 int readlen_in; //byte to read over incoming TCP connecting
 int readptr_in;  //pointer to read for incoming TCP connecting

 unsigned char readbuf_out[MAXDATALENGTH]; //read buffer for outgoing TCP connecting
 int readlen_out; //byte to read over outgoing TCP connecting
 int readptr_out;  //pointer to read for ougoing TCP connecting


 unsigned char torbuf[48]; //SOCK5 adress request for Tor
 int tor_len;  //length of SOCK5 adress request

 //socket adress structures
 struct sockaddr_in saddrUDPTo;  //udp destination address structure
 struct sockaddr_in saddr;  //work address structure

 //sockets
 int sock_listener; //tcp listening socket
 int sock_tcp_in;   //tcp accepting socket (incoming)
 int sock_tcp_out;  //tcp connected socket (outgoing)
 int sock_udp;     //udp created socket (outgoing)

 //our inrefaces
 unsigned short tor_port; //tor interface on localhost
 unsigned long stun_ip;  //STUN interface
 unsigned short stun_port;
 unsigned short listen_port; //TCP listening port
 unsigned short tcp_port;  //our last outgoing tcp port 

 //their interfaces
 unsigned long their_ip; //their interface for tcp/tor connecting
 unsigned short their_port;

 //their interfaces for direct udp connecting
 unsigned long their_lanip;
 unsigned short their_lanport;
 unsigned long their_wanip;
 unsigned short their_wanport;

 //our interfaces for direct udp connecting
 unsigned long our_lanip; //obtained by if
 unsigned short our_lanport;
 unsigned long our_wanip; //obtained by STUN
 unsigned short our_wanport;

 //timing
 unsigned int time_counter; //counter for call gettimeofday
 unsigned int time_now; //actual timestamp
 unsigned int time_for_close;  //time for call terminate
 unsigned int time_for_connect; //time for send direct invite
 unsigned int time_for_ping; //time for keepalive
 unsigned int time_for_file; //time for process file transfer

 //counters
 unsigned int control_recv_cnt;  //counter of incoming control packets
 unsigned int control_send_cnt; //counter of outgoing control packets
 unsigned int data_recv_cnt;     //counter of incoming data packets
 int data_recv_in;  //incoming channel statistic
 int data_recv_out; //outgoing channel statistic

 //states
 char state_in;  //status of tcp incoming connection
 char state_out; //status of tcp outgoing connection
 char state_udp;    //status of udp outgoing

 //flags
 unsigned char dbl_request; //flag of request was sended
 unsigned char istor;   //flag of connecting over tor
 unsigned char isready;  //flag is IKE compleet
 unsigned short soc_event;  //user notify event
 unsigned char rcvblk; //block receiving before set key
 unsigned char obf; //obfuscation ready
} tr_data;

//=============Procedures of Transport module======================

//helpers
void ct2bf(unsigned char* buf, unsigned int cnt); //convert 24-bits counter to buf
unsigned int bf2ct(unsigned char* buf); //convert but to 24 bits counter

//internal control
int make_control(unsigned char* buf, unsigned char type); //make remote control packet
int check_control(unsigned char* buf);  //check remote control packet type
void direct_connect(void); //provide NAT traversal for direct UDP connect
void recall(void);  //recall to specified address
void ping_remote(void); //ping remote
void set_channel_statistic(int receive_channel); //check slowes channel and initiate recall
void close_in(void);  //close incoming TCP connecting
void close_out(void); //close outgoing TCP connecting
void close_udp(void); //close UDP connecting

//polling
int wait_timer(unsigned char* buf); //poll time and events
int wait_accept(void);  //poll new incoming TCP connecting
int wait_connect(unsigned char* buf); //poll outgoing connecting process
int wait_tcpin(unsigned char* buf); //poll data received over incoming tcp
int wait_tcpout(unsigned char* buf); //poll data received over outgoing TCP
int wait_udp(unsigned char* buf);  //poll data received over UDP

//internal commands
void tr_create(unsigned char* data); //initialize transport module, seed PRNG
int tr_init(unsigned char* data); //initialize TCP listener
int tr_call_setup(char* address); //start outgoing call
void tr_direct_setup(int on); //start NAT traversal
void tr_send_to_remote(unsigned char* buf, int len); //send data to remote over avaliable channels
void call_terminate(int back); //terminate call

//obfuscation key managment
short tr_rep(unsigned char* data); //send our representation to remote
short tr_obf(unsigned char* data); //set material derived by secret shared  in pre-IKE
void tr_set_secret(unsigned char* data); //update material by secret shared in IKE
short tr_set_remote(unsigned char* data); //remote request for update material by secret shared in IKE

//==============INTERFACE====================

//send  len bytes of data in buf to remote
//len must be 16-124 bytes
//first byte will be updated before sending!
void tr_process(unsigned char* buf, int len);

//read incoming data from remote in buf
//return data length or zero if no data was received
//must call periodically as a heartbeat for socket module
int tr_read_from_remote(unsigned char* buf);
