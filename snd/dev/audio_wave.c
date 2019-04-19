
//**************************************************************************
//ORFone project
//Low level audio playing/recording module for Windows OS (wave)
//**************************************************************************


//This file contains low-level procedure for wave audio
//Win32 system for X-Phone (OnionPhone) project

//FOR WIN32 ONLY!
#ifdef _WIN32

#include <basetsd.h>
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>



#include "audio_wave.h"
#define UNREF(x) (void)(x)

#define DEFCONF "conf.txt"  //configuration filename
#define SampleRate 8000  //Samle Rate
#define BitsPerSample 16  //PCM16 mode
#define Channels 1     // mono
#define CHSIZ 320 //chunk size in bytes (2 * sample125us)
#define CHNUMS 32  //number of chunks for buffer size 2400 samples (300 mS) can be buffered
#define ROLLMASK (CHNUMS-1) //and-mask for roll buffers while pointers incremented


//#define SPEEX_ECHO
#define WEBRTC_ECHO

#ifdef SPEEX_ECHO
 #undef WEBRTC_ECHO
#endif 



//=======================================================

HWAVEIN In=0;                   // Wave input device
HWAVEOUT Out=0;                 // Wave output device
WAVEFORMATEX Format;          // Audio format structure
UINT BytesPerSample;          // Bytes in sample (2 for PCM16)
HANDLE WorkerThreadHandle=0;    // Wave thread
DWORD WorkerThreadId=0;         // ID of wave thread

int IsSound=0;   //flag: sound OK
int IsGo=0;     //flag: input runs
int DevInN=0;    //system input numer devicce specified in donfig file
int DevOutN=0;   //system output numer devicce specified in donfig file
int ChSize=CHSIZ; //chunk size
int ChNums=CHNUMS; //number of chunks

int IsGrab=0;    //flags for grab/play devices was initialized
int IsPlay=0;
int PlayPath=0; //default path

//audio input
WAVEHDR *Hdr_in[2]; //headers for 2 chunks
unsigned char wave_in[CHNUMS][CHSIZ+40];  //output buffers for 16 frames 20 mS each
volatile unsigned char p_in=0; //pointer to chunk will be returned by input device
unsigned char g_in=0; //pointer to chunk will be readed by application
int n_in=0; //number of bytes ready in this frame

//audio output
WAVEHDR *Hdr_out[CHNUMS]; //headers for each chunk
unsigned char wave_out[CHNUMS][CHSIZ+40]; //output buffers for 16 frames 20 mS each
unsigned char p_out=0;  //pointer to chunk will be passed to output device
int n_out=0;  //number of bytes already exist in this frame
volatile unsigned char g_out=0;  //pointer to chunk will be returned by output device

//Internal procedures
int OpenDevices (void); //open in & out devices
void StopDevices (void); //paused
void CloseDevices (void); //closed devised (finalize)
void ReleaseBuffers(void); //de - allocate memory

void dlg_init(void); //ini audio
int dlg_start(void); //create tread

//-------------------------------
#ifdef WEBRTC_ECHO
#include "aecm.h"
void *aecmInst = 0;    //aec instance pointer
short out_buffer[160]; //aec output buffer
#endif

#ifdef SPEEX_ECHO
#include "speex_echo.h"
SpeexEchoState *echo_state = 0; //speex echo state
short out_buffer[160]; //aec output buffer
#endif

//-----------------------------------------

//Transcode cp866(ru) to latin
void r2tru(char* in, char* out)
{
 static char tbl_win[64]={
     -32, -31, -30, -29, -28, -27, -26, -25,
     -24, -23, -22, -21, -20, -19, -18, -17,
     -16, -15, -14, -13, -12, -11, -10, -9,
     -8, -7, -4, -5, -6, -3, -2, -1,
     -64, -63, -62, -61, -60, -59, -58, -57,
     -56, -55, -54, -53, -52, -51, -50, -49,
     -48, -47, -46, -45, -44, -43, -42, -41,
     -40, -39, -36, -37, -38, -35, -34, -33
 };
 static char* tbl_trn="abvgdezzijklmnoprstufhccss'y'ejjABVGDEZZIIKLMNOPRSTUFHCCSS'I'EJJ";
 int i,j;
 char*p=out;

 for(i=0;i<1+(int)strlen(in); i++)
 {
  if(in[i]>=0) (*p++)=in[i];
  else
  {
   for(j=0; j<64; j++)
   {
    if(in[i]==tbl_win[j])
    {
     (*p++)=tbl_trn[j];
     if((j==6)||(j==23)||(j==24)||(j==38)||(j==55)||(j==56)) (*p++)='h'; //zh, ch, sh
     if((j==25)||(j==57)) (*p++)='c'; //sc
     if((j==30)||(j==62)) (*p++)='u'; //ju
     if((j==31)||(j==63)) (*p++)='a'; //sc
     break;
    }
    if(j==64) (*p++)='?';
   }
  }
 }
}


//=============================Level 0==================

//*****************************************************************************
//Parse config file for param and copy value to param, return length of value
//zero if not found and error code if no config file
int  parseconfa(char* param)
{
 FILE *fpp;
 char buf[256];
 char* p=NULL;
 int i;

 //open configuration file
 fpp = fopen(DEFCONF, "rt");
 if (fpp == NULL)
 {
  perror("Cannot open config file");
  return -1;
 }
  //read it sting-by-string
  while( fgets(buf, sizeof(buf), fpp) )
  {
   if((buf[0]=='#')||(buf[0]==0)) continue; //skip comments and emty stings
   p=strstr(buf, param); //search specified parameter in string
   if(!p) continue; //process next string up to eof
   p=strchr(buf, '='); //search separator
   if(!p) continue; //no value
   p++; //set pointer to value
   break;
  }
  fclose(fpp);
  param[0]=0; //clear input string
  if(p) //if parameter found
  {  //truncate value string to first space or end of string
   for(i=0;i<(int)strlen(p);i++)
   if( (p[i]=='\r')||(p[i]=='\n')||(p[i]==' ') ) break;
   p[i]=0;
   strncpy(param, p, 31); //replace input parameter by it's value
   param[31]=0;
  }
  return (strlen(param)); //length of value's string or null
}



//*****************************************************************************
//discower wave input and output in system and print devices list
void dlg_init(void)
{
 int NumWaveDevs;
 int i;
 char str[256];

 //get number of wave input devices in system
 NumWaveDevs = waveInGetNumDevs ();
 for (i = 0; i < NumWaveDevs + 1; i++)
 {
  WAVEINCAPS DevCaps;
  waveInGetDevCaps (i - 1, &DevCaps, sizeof (DevCaps)); //get devices name
  r2tru(DevCaps.szPname, str);
  printf("WaveInDevice %d: %s\r\n", i, str);

 }
 //get number of wave output devices in system
 NumWaveDevs = waveOutGetNumDevs ();
 for (i = 0; i < NumWaveDevs + 1; i++)
 {
  WAVEOUTCAPS DevCaps;
  waveOutGetDevCaps (i - 1, &DevCaps, sizeof (DevCaps)); //get devices name
  r2tru(DevCaps.szPname, str);
  printf("WaveOutDevice %d: %s\r\n", i, str); //print it
 }
}

//*****************************************************************************
//Stor audio input/autput
void StopDevices (void)
{
  waveInReset (In);
  waveOutReset (Out);
}

//*****************************************************************************
//Close audio input/output
void CloseDevices (void)
{
 if (In) waveInClose (In);
 In = NULL;
 if (Out) waveOutClose (Out);
 Out = NULL;

}

//*****************************************************************************
//release dynamically allocated wave header
void ReleaseBuffers(void)
{
 volatile int i;
 if(WorkerThreadId) PostThreadMessage (WorkerThreadId, WM_QUIT, 0, 0);
 for(i=0;i<CHNUMS; i++) if(Hdr_out[i]) LocalFree ((HLOCAL)Hdr_out[i]);
 for(i=0;i<2; i++) if(Hdr_in[i]) LocalFree ((HLOCAL)Hdr_in[i]);
}

//*****************************************************************************
//Open and init wave input and output
int OpenDevices (void)
{
  MMRESULT Res;
  volatile int i;
  WAVEHDR *Hdr;

  InitCommonControls (); //init system controls
  In = NULL; //clear devices pointers
  Out = NULL;
  //set audio format
  BytesPerSample = (BitsPerSample + 7) / 8;
  Format.wFormatTag = WAVE_FORMAT_PCM;
  Format.nChannels = Channels;
  Format.wBitsPerSample = BitsPerSample;
  Format.nBlockAlign = (WORD)(Channels * BytesPerSample);
  Format.nSamplesPerSec = SampleRate;
  Format.nAvgBytesPerSec = Format.nSamplesPerSec * Format.nBlockAlign;
  Format.cbSize = 0;

  //---------------------Open Devices----------------
  //Output
  Res = waveOutOpen (
      (LPHWAVEOUT)&Out,
      DevOutN - 1,
      &Format,
      (DWORD)WorkerThreadId,
      0,
      CALLBACK_THREAD
    );
  if (Res != MMSYSERR_NOERROR) Out=0;


  //Input
  Res = waveInOpen (
      (LPHWAVEIN)&In,
      DevInN - 1,
      &Format,
      (DWORD)WorkerThreadId,
      0,
      CALLBACK_THREAD
    );
  if (Res != MMSYSERR_NOERROR) In=0;

  //------------Allocates wave headers----------------
 if(Out)
 {
  waveOutPause (Out); //stop output device;
  //For output device allocate header for each frame in buffer
  for(i=0; i<CHNUMS; i++)
  {
   Hdr = (WAVEHDR *)LocalAlloc (LMEM_FIXED, sizeof (*Hdr));
   if (Hdr)
   {
    Hdr->lpData = (char*)wave_out[i];
    Hdr->dwBufferLength = ChSize;
    Hdr->dwFlags = 0;
    Hdr->dwLoops = 0;
    Hdr->dwUser = 0;
    Hdr_out[i]=Hdr;
   }
   else
   {
    for(i=0;i<CHNUMS; i++) if(Hdr_out[i]) LocalFree ((HLOCAL)Hdr_out[i]);
    return 0;
   }
  }
  //init output pointers
   p_out=0;
   g_out=0;
   n_out=0;
   printf("Headset ready\r\n");
 }

 if(In)
 {
  //-------------------------init wave input---------------
  //Allocates 2 headers
  for(i=0; i<2; i++)
  {
   Hdr = (WAVEHDR *)LocalAlloc (LMEM_FIXED, sizeof (*Hdr));
   if (Hdr)
   {
    Hdr->lpData = (char*)wave_in[i];
    Hdr->dwBufferLength = ChSize;
    Hdr->dwFlags = 0;
    Hdr->dwLoops = 0;
    Hdr->dwUser = 0;
    Hdr_in[i]=Hdr;
    waveInPrepareHeader (In, Hdr, sizeof (WAVEHDR));
    Res = waveInAddBuffer (In, Hdr, sizeof (WAVEHDR));
   }
   if( (!Hdr) || (Res != MMSYSERR_NOERROR) )
   {
    for(i=0;i<CHNUMS; i++) if(Hdr_out[i]) LocalFree ((HLOCAL)Hdr_out[i]);
    for(i=0;i<CHNUMS; i++) if(Hdr_in[i]) LocalFree ((HLOCAL)Hdr_in[i]);
    return 0;
   }
  }
  //init input pointers
  p_in=0;
  g_in=0;
  n_in=0;
  printf("Mike ready\r\n");
 }
  return 1;
}

//========================Top level=================

//*****************************************************************************
//init wave devices
int soundinit(char* device_in, char* device_out)
{
  //int16_t echoMode = 1;// 0, 1, 2, 3 (default), 4
  //int16_t msInSndCardBuf = 40;
  AecmConfig config;
  int status;


  unsigned int i;
  IsSound=0; //set sound flag to OK
  //rdcfg();  //read sound devices numbers from config file

  //======================================================================

#ifdef SPEEX_ECHO
  echo_state = speex_echo_state_init(160, 160*10);
  if(!echo_state)
  {
   printf("cannot create speex echo!\r\n");
   return 0;
  }
#endif

  //========================================================================
  //                   Create AEC

#ifdef WEBRTC_ECHO
    config.cngMode = AecmTrue;
    config.echoMode = 1;      //mode
    aecmInst = WebRtcAecm_Create();
    if(!aecmInst)
    {
     printf("cannot create aec!\r\n");
     return 0;
    }

    status = WebRtcAecm_Init(aecmInst, 8000);//8000 or 16000 Sample rate
    if (status != 0) {
        printf("WebRtcAecm_Init fail\r\n");
        WebRtcAecm_Free(aecmInst);
        return 0;
    }

    status = WebRtcAecm_set_config(aecmInst, config);
    if (status != 0) {
        printf("WebRtcAecm_set_config fail\n");
        WebRtcAecm_Free(aecmInst);
        return 0;
    }

#endif


  //=========================================================================



  //set input device for use
  if(device_in[0]!='#') for(i=0;i<strlen(device_in); i++)
  {
   if((device_in[i]>='0')&&(device_in[i]<='9')) //search first number in the string
   {
    DevInN=device_in[i]-'0';  //set recording wave device by this number
    break;
   }
  }

  //set input device for use
  if(device_out[0]!='#') for(i=0;i<strlen(device_out); i++)
  {
   if((device_out[i]>='0')&&(device_out[i]<='9'))   //search first number in the string
   {
    DevOutN=device_out[i]-'0';  //set playing wave device by this number
    break;
   }
  }


 //ChNums=2; //defaults
 //ChSize=80; //defaults
  printf("In/out wave devices %d/%d will be used\r\n", DevInN, DevOutN);


  dlg_init(); //print list of avaliable sound devices
  IsSound=dlg_start(); //open wave input and wave output
  return IsSound;
}

//*****************************************************************************
//terminate wave devices
void soundterm(void)
{
 CloseHandle (WorkerThreadHandle); //stop audio thread
 WorkerThreadId = 0;
 WorkerThreadHandle = NULL;
 //StopDevices ();  //paused
 CloseDevices (); //close devices
 Sleep(500); //some time for close compleet
 ReleaseBuffers(); //release memory
 //SetPriorityClass (GetCurrentProcess (), DefPrioClass); //restore priority of main procedure
 printf("Wave Thread stopped!\r\n");
 IsSound=0;

 #ifdef WEBRTC_ECHO
 if(aecmInst)
 {
  WebRtcAecm_Free(aecmInst);
  aecmInst=0;
 }
 #endif

 #ifdef SPEEX_ECHO
 if(echo_state)
 {
  speex_echo_state_destroy(echo_state);
  echo_state = 0;
 }
 #endif

}

//*****************************************************************************
//Get number of samples in output queue
int getdelay(void)
{
 int i=(int)g_out; //read volatile value: pointer to next buffer will be released
 i=-i;
 i=i+p_out; //buffers in queue now
 if(i<0) i=CHNUMS+i; //correct roll
 i=i-2; //skip two work buffers
 if(i<0) i=0; //correct
 i=i*ChSize; //bytes in queue
 i=i+n_out; //add tail in current buffer
 i=i/2; //samples in queue
 return i;
}

//*****************************************************************************
//get number of samples in chunk (frame)
int getchunksize(void)
{
 return ChSize/2;
}

//*****************************************************************************
//get total buffers size in samples
int getbufsize(void)
{
 return ((ChSize*(CHNUMS-1))/2);
}

//*****************************************************************************
//skip all samples grabbed before
void soundflush(void)
{
 //in: not released yet
}

//*****************************************************************************
//skip all unplayed samples
void soundflush1(void)
{
 //out: not released yet
}

//*****************************************************************************
//------------------------------------------------
//grab up to len samples from wave input device
//return number of actually getted samples
int soundgrab(char *buf, int len)
{
 int i, l, d=0;
 unsigned char cpp, cp=p_in; //read volatile value
 //p_in pointes to first frame in work now
 //p_in+1 frame also in work
 //p_in-1 frame was last returned by input device

 //g_in pointes to the most oldest unread frame
 //n_in is number of unreaded bytes in it (tail)

 if((!In)||(!IsSound)) return 0;

 if(IsGo)//check for device opened
 {
  cpp=cp+1;  //pointer to buffer passed to input device
  cpp&=ROLLMASK;
  l=len*2; //length in bytes (for 16 bit audio samples)
  while(l>0) //process up to length
  { //check for pointed buffer not uses by input device now
   if((g_in==cp)||(g_in==cpp)) break; //2 chunks uses by input device at time
   i=ChSize-n_in; //ready bytes in this frame
   if(i>l) i=l;  //if we need less then exist
   memcpy(buf, &wave_in[g_in][n_in], i); //copy to output
   d+=i; //bytes outed
   l-=i; //bytes remains
   n_in+=i; //bytes processed in current frame
   if(n_in>=ChSize) //if all bytes of current frame processed
   {
    g_in++;   //pointer to next frame
    g_in=g_in&ROLLMASK; //roll mask (16 frames total)
    n_in=0;  //no byte of this frame has not yet been read
   }
  }
 }
 //if(!d) Sleep(20);
 return (d/2); //returns number of outputted frames
}


//*****************************************************************************
//pass up to len samples from buf to wave output device for playing
//returns number of samples actually passed to wave output device
int soundplay(int len, unsigned char *buf)
{
 #define STARTDELAY 2 //number of sylency chunk passed to output before playing
 int d=0; //bytes passed
 int i, l;
 unsigned char cp, cg=g_out; //read volatile value
 //g_out pointes to last played (and empty now) frame
 //p_out points to frame for writing data now
 //at-least one frame must be played at time (normally two and more)
 //if no frames played at time (g_out==p_out) that is underrun occured
 //and we must push some frames of silency (for prevention next underrun)
 //and restart playing

 if((!IsSound)||(!Out)) return len;  //check for device ready
 if(p_out==cg) //underrun occured
 {
  for(i=0;i<STARTDELAY;i++) //pass to device some silency frames
  {
   memset(wave_out[i], 0, ChSize); //put silency to first frame
   waveOutPrepareHeader (Out, Hdr_out[i], sizeof (WAVEHDR)); //prepare header
   waveOutWrite (Out, Hdr_out[i], sizeof (WAVEHDR));    //pass it to wave output device
  }
  g_out=0; //reset pointer of returned headers
  p_out=i; //set pointer to next header to pass to device
  n_out=0; //it is empty
  waveOutRestart (Out); //restart wave output process
  return -1; //return error code
 }

 l=len*2; //length in bytes (for 16 bit audio samples)
 while(l) //process all input data
 {
  cp=p_out+1; //pointer to next frame
  cp=cp&ROLLMASK; //roll mask
  if(cp==cg) break; //if next frame not returned yet (buffer full)
  i=ChSize-n_out; //number of empty bytes in this frame
  if(i>l) i=l; //if we have less bytes then empty
  memcpy(&wave_out[p_out][n_out], buf+d, i); //copy input data to frame
  l-=i; //remain data bytes
  d+=i; //processed bytes
  n_out+=i; //empty bytes remains in current frame
  if(n_out>=ChSize) //if chunk full
  {
   waveOutPrepareHeader (Out, Hdr_out[p_out], sizeof (WAVEHDR)); //prepare header
   waveOutWrite (Out, Hdr_out[p_out], sizeof (WAVEHDR));    //pass it to wave output device
   p_out=cp; //pointer to next chunk
   n_out=0;  //all bytes in this frame are empty
  }
 } //while(l)
 d=d/2;
 return (d);  //returns bumber of accepted samples
}


//*****************************************************************************
//===================Wave task========================

DWORD WINAPI WorkerThreadProc (void *Arg)
{
  MSG Msg; //system message
  //Set hight priority
  SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL);
  //work cicle: wait for Message
  while (GetMessage (&Msg, NULL, 0, 0) == TRUE)
  {
   switch (Msg.message) //process system message from HW wave devices
   {
    case MM_WIM_DATA: //wave input buffer compleet
    {
     WAVEHDR *Hdr = (WAVEHDR *)Msg.lParam;
     unsigned char bc;

     waveInUnprepareHeader (In, Hdr, sizeof (*Hdr)); //unprepare returned wave header
     //p_in is a pointer to returned frame
     bc=p_in; //computes pointer to next frame (normally in use now)

  #ifdef WEBRTC_ECHO
   //---------------------------------------------
     WebRtcAecm_Process(aecmInst, (short*)(wave_in[bc]), NULL, out_buffer, 160, 40);
     memcpy(wave_in[bc], out_buffer, 320);
  //----------------------------------------------
  #endif

  #ifdef SPEEX_ECHO
    speex_echo_capture(echo_state, (short*)(wave_in[bc]), out_buffer);
    memcpy(wave_in[bc], out_buffer, 320);
  #endif

     bc++;
     bc&=ROLLMASK;  //roll mask
     p_in=bc;  //set it as a pointer to next frame will be returns
     bc++;     //computes pointer to next frame for passes to input device
     bc&=ROLLMASK; //roll mask
     //returns header with next frame to input device
     Hdr->lpData=(char*)wave_in[bc]; //attach next buffer to this header
     waveInPrepareHeader (In, Hdr, sizeof (*Hdr)); //prepare header
     waveInAddBuffer (In, Hdr, sizeof (*Hdr)); //back to input device
     break;
    }

    case MM_WOM_DONE:  //wave output buffer played
    {
     WAVEHDR *Hdr = (WAVEHDR *)Msg.lParam;
     unsigned char bc;

     waveOutUnprepareHeader (Out, Hdr, sizeof (*Hdr)); //unprepare returned wave header
     bc=g_out; //pointer to returned header

  #ifdef WEBRTC_ECHO
     //--------------------------------------------------------------
     WebRtcAecm_BufferFarend(aecmInst, (short*)(wave_out[bc]), 160);
  #endif

  #ifdef SPEEX_ECHO
     speex_echo_playback(echo_state, (short*)(wave_out[bc]));
  #endif
     //---------------------------------------------------------------

     bc++; //pointer to next frame: normally it was early passed to device
     g_out=bc&ROLLMASK; //roll mask
     break;
    }//if(f_out[g_out]==1)
   } //switch (Msg.message)
  }  //while (GetMessage (&Msg, NULL, 0, 0) == TRUE)
  UNREF (Arg);
  return 0;
}
//===============end of wave task=======================


//*****************************************************************************
//start audio
int dlg_start(void)
{
   int Success=0;
   //up priority of main task WARNING: there are some problems of Win32 priority!!!
   //SetPriorityClass (GetCurrentProcess (), HIGH_PRIORITY_CLASS);
   //creates wave thread
   WorkerThreadHandle = CreateThread (
              NULL,
              0,
              WorkerThreadProc,
              NULL,
              0,
              &WorkerThreadId
            );
   //open wave devices
   Success = OpenDevices ();
/*   if (Success)
   {  //start audio input
    if (waveInStart(In) != MMSYSERR_NOERROR)
    {  //if no inputs
     CloseDevices ();
     IsSound=0;
     printf("Starting audio input failed\r\n");
    }
   } */
   return Success;
}

//start/stop audio input
int soundrec(int on)
{
 
 printf("Soundrec %d\r\n", on);
 if(on!=IsGo)
 {
  MMRESULT Res;

  if(on) Res = waveInStart(In); //start audio input
  else Res = waveInStop(In);   //stop audio input
  if(Res != MMSYSERR_NOERROR) IsGo=0; //error
  else IsGo=on;                       //set flag
  //printf("Rec=%d\r\n", IsGo);
 }
 return IsGo;                        //return status
}


//set audio output path (2-voice, 3-ring, default-media)
void sounddev(unsigned char dev)
{
 PlayPath=(int)dev; //up level stop audio before path change, path can be use in next start
}

#endif //#ifdef _WIN32

