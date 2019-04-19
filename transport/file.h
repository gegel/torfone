//interface
void fl_init_path(char* path);
int fl_init(unsigned short telnet_port);
void fl_start(char* name); //start send file
void fl_send(void); //sending loop
void fl_rcvd(unsigned char* data); //process part of file data
void fl_ctrl(unsigned char* data); //process file control

//remote data packet types
#define FILE_DATA 0  //payload: u16 offset + u8[120] file data
#define FILE_SEND 1  //start: u16 len +s8[32] name (len=0 for stop)
#define FILE_CPLT 2  //finish: u16 crc
#define FILE_RCVD 3  //acc: u16 len (0 is error)

//local notification codes:
#define NOTE_STOP 0 //transfer aborted
#define NOTE_CPLT 1 //:name transfer compleet
#define NOTE_RCVD 2 //:name receiving compleet
#define NOTE_SEND 3 //:progress in percent
#define NOTE_PONG 4 //answer to ping
#define NOTE_IDDL 5 //call iddle
#define NOTE_RING 6 //incoming call
#define NOTE_ANSW 7 //remote party answer

#define FILE_SEND_DEL 16   //120 bytes every  2^n uS

//local commands
//FILE:name[32] :start
//STEP  :request state of transfer
//TERM  :stop current transfer
//PING  :for testing inteface



/*
//type of control data and it's parameters
#define FILE_SEND_BREAK 0  //no parameters
#define FILE_SEND_START 1  //parameters: u16 len, char* name
#define FILE_SEND_OK 2 //parameter: u32 crc
#define FILE_RCVD_OK 3 //parameter: u32 crc
#define FILE_RCVD_FAIL 4 //no parameters
#define FILE_DATA 5 //part (1-120 bytes) of file's data
#define FILE_PING 6
 */

 
#define FILE_MAX_PATH 512

typedef union
{
 const uint8_t b[32];
 const uint32_t d[8];
}CMD_data;

//file tranfering data
typedef struct {
 //general
 unsigned char buf[144]; //work buffer
 char name[32]; //received file name
 //sending
 FILE * send_file; //file for reading
 unsigned short send_len; //len of file will be sended
 unsigned short sended; //bytes already sended
 unsigned short send_crc; //crc of sended bytes
 //receiving
 FILE * rcvd_file; //file for writing
 unsigned short rcvd_len; //len of file will be received
 unsigned short received; //bytes already received
 unsigned short rcvd_crc; //crc of received bytes
} fl_data;

