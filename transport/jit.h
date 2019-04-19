//mode
#define MUTE_OFF    4  //set mike mute off
#define MUTE_ON     8  //set mike mute on

//channel types
#define CHANNEL_DEF 0  //set default channel type (can be TOR or TCP)
#define CHANNEL_UDP 1  //set channel type is UDP
#define CHANNEL_TCP 2  //set channel type is TCP
#define CHANNEL_TOR 3  //set channel type is TOR

//maximal number of voice frames in data packet for different channel types
#define JTMAXCNT 10 //maxila number of frame in packet
#define FRAMESINPACKET_TOR 10
#define FRAMESINPACKET_TCP 5
#define FRAMESINPACKET_UDP 2

//PTT notification
#define BEEPINTERVAL 25 //number of silency frames before "Roger" tone

//Tor jitter compensation
#define GUARD_LEVEL 5  //number of 20 mS frames must always be in queue
#define K_LEVEL 8      //smoothing koefficient to adjusting optimal buffer fill depend acrual jitter
#define K_RATE 10 //40      //smoothig koefficient for tuning playing sampling rate depends difference of actual and optimal buffer fill

//TCP jitter compensation
#define TCP_GUARD_LEVEL 3
#define K_RATE_TCP 5

//UDP jitter compensation
#define UDP_GUARD_LEVEL 1
#define K_RATE_UDP 3

typedef struct {
//---------receiving - playing global values------------
unsigned char rbuf[128][16]; //anti-jitter ring receiving buffer
unsigned char in_pointer;  //pointer to frame will be received
unsigned char out_pointer; //pointer to frame will be played
unsigned int frame_counter; //expected counter of next receiving frame
int play_crate;   //playing rate for cryptor (0-31, nominal is 15)
int requested_frames; //number of frames requested by player
int ssln; //counter of played silency frames
//----------grabbing - sending part----------------------
unsigned char sbuf[10][12]; //recording collect buffer
unsigned char scnt; //number of recorded frames in buf
unsigned char smax; //maximal number of frames in sended packet
unsigned char sbeep; //silency tail after thet the beep signal will be played
unsigned int sctr; //counter of first packet in buf
int jtalk; //mute flag for grabbing voice
int min_level;
int max_level;
int defchn;
int chn;
} jit_data;



//Interface:
int jt_add_cr_speech(unsigned char* buf); //add new speech frame from cryptor, if ready returns packet and length
int jt_ext_cr_speech(unsigned char* buf); //extract next speech frame from queue for cryptor
int jt_add_tf_speech(unsigned char* buf, int len); //process received packet and update queue
void jt_reset_speech(void);                   //reset sound queue for next session
void jt_set_channel_type(int channel); //set channel type: Tor, TCP or Direct
void jt_set_talk(int talk); //allow grabbing
