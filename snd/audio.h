
//AMR codec settingth
#define AMRFRAMELEN 160    //samples in 20 mS AMR frame
#define AMRSAMPLERATE 8000 //samplerate, Hz
#define AMRMODE 0  // (0-7) 0=mode 4.75 kbps
#define AMRCBR 0   //not replace unvoiced frames by comfort noise descriptors

//simple echo supressor settings
#define ECHOSMOOTH 3 //for 8: average frames measuring speaker level
#define MAXLEVEL 2048  //up trashold of speaker level for maximal de-echo
#define MINLEVEL 256 //down trashold of speaker level for start de-echo

//resampler for playing rate tuning
#define DRATE 50 //koefficient of tuning playing rate by -32 + 32 value
#define MAX_RESAMPL_BUF AMRFRAMELEN+AMRFRAMELEN/2 //resampling buffer length: 1.5 * max_frame_length (160 samples for amr)

//resampler values
#define ONE_POS  0x4000  //1.0 in Q17.14
#define NOMINAL_RATE 8000 //base sampling rate


//trashold of playing buffer level
#define NOTPLAYDELAY 320 //critical number of samples for play silency ot tone 
#define MINPLAYDELAY 480 //mininam number of samples in playing buffer  (2 new frames will be requested)
#define NOMPLAYDELAY 720 //nominal number of samples in playing buffer  (1 new frames will be requested)
#define MAXPLAYDELAY 960 //maximal number of samples in playing buffer  (0 new frames will be requested)

#define CONNECT_SIGNAL_PAUSE  50 //duration of pauses beetwen short 1000Hz beeps on connect
#define CALL_SIGNAL 50 //duration of long 500Hz tone on wait answer
#define CALL_SIGNAL_PAUSE 200 //duration of pauses beetwen long 500Hz tones on wait answer
#define RING_SIGNAL 20 //duration of ring signal
#define RING_SIGNAL_PAUSE 30 //duration of pauses beetwen ring signals
#define TEST_SIGNAL_PAUSE 100 //duration of pause beetwen test signals to remote

//audio path
#define AUDIO_PATH_MEDIA 1    //loudspeaker
#define AUDIO_PATH_VOICE 2    //airphone
#define AUDIO_PATH_RING 3     //ring

//interface for set
#define AUDIO_OFF 0
#define AUDIO_MUTE 1
#define AUDIO_CONNECT 2
#define AUDIO_WAIT 3
#define AUDIU_RING 4
#define AUDIO_TALK 5
#define AUDIO_BEEP 6
#define AUDIO_LOOP 7
#define AUDIO_SPK 8
#define AUDIO_MIKE 9
#define AUDIO_COD 10

//auidio engine data structure
typedef struct {
char grabber[32]; //audio recording device system name
char player[32];  //audio playing device system name
int snddev;   //descryptor of sound device
int *amrenstate;  //descryptor of amr encoder
int *amrdestate;  //descryptor of amr decoder
short inbuf[AMRFRAMELEN];  //one frame sample buffer
short loopbuf[AMRFRAMELEN];  //test loopback buffer
short outbuf[MAX_RESAMPL_BUF]; //resumpler output for play
unsigned char loopamr[32];
unsigned char playamr[32];
int pos; //resamplers position
short left_sample; //resamplers base
unsigned int splevel; //energy of played sound
unsigned char mode; //mode of audio
unsigned char npp; //flags of using nnp
unsigned short tx; //specher volume code
unsigned short rx; //mike sensitivity code;
unsigned char echo; //echo supression level
unsigned char test; //test modes
unsigned char dev; //output audio path
short tone_cnt; //counter of test AMR frames decoded and played over specker
short tone;  //playing mode: voice, silency, connect, call, ring
short mute; //1- replace grabbed voice by silency
}au_data;


//interface (see C file for details)
int au_grab(unsigned char* pkt);
int au_play(unsigned char* pkt);

int au_create(char* audioin, char* audioout);
void au_close(void);
int au_start(void);
void au_stop(void);

void au_mode(unsigned char mode);
void au_npp(unsigned char val);
void au_tx(unsigned char val);
void au_rx(unsigned char val);
void au_echo(unsigned char val);
void au_tone(unsigned char val);
void au_dev(unsigned char dev);
void au_mute(unsigned char val);
void au_set(unsigned char val);


 //interface to platfrom-depend low level audio procedures
 int soundinit(char* device_in, char* device_out);
 int soundgrab(char *buf, int len);
 int soundplay(int len, unsigned char *buf);
 void soundterm(void);
 int getdelay(void);
 int getchunksize(void);
 int getbufsize(void);
 void soundflush(void);
 int soundrec(int on);
 void sounddev(unsigned char dev);
 
