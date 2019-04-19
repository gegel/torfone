//**************************************************************************
//ORFone project
//Audio processing for cryptor module
//**************************************************************************

//include system-depend audio interface


//amr codec
#include "amr/interf_enc.h" //AMR encoder
#include "amr/interf_dec.h" //AMR decoder
#include "npp/npp.h"  //noise supressor
#include "ike.h" //encrypting
#include "audio.h"  //this
#include "voice.h" //voice test AMR pattern 298 frames 12 bytes each
#include "ringwave.h" //ring PCM pattern 3296 unsigned short (20*160)
int cccc=0;

au_data au;  //data of audio module

int ddd;
char AU_DBG=0;


int cnt1=0;
int cnt0=0;

//wave tables
const short beep1000[8]={0, 11314, 16000, 11314, 0, -11314, -16000, -11314};
const short beep500[16]={0, 6123, 11314, 14782, 16000, 14782, 11314, 6123,
                  0, -6123, -11314, -14782, -16000, -14782, -11314, -6123};
//volume tabs in Q8.8 (256 is x1.0)
const unsigned short au_tx_tab[10]={26,51,77,128,179,256,333,384,435,512};
const unsigned short au_rx_tab[10]={26,51,77,128,179,256,333,384,435,512};

//internal procedures
void setmksens(short* spc);
void getsplevel(const short* spc);
int resampler(short* src, short* dest, int srclen, int rate);



//******************************************************************************
//INTERNAL PROCEDURES FOR AUDIO PROCESSING
//******************************************************************************


void volume(short* sp, unsigned short level)
{
 short i;
 int k;

 for(i=0;i<AMRFRAMELEN;i++) //process 160 samples
 {
  k=sp[i]*level;  //level in Q8.8
  k>>=8; //  /=256
  if(k>32767) k=32767;  //saturate
  if(k<-32767) k=-32767;
  sp[i]=k; //modify sample level
 }
}

//------------------------------------------------------------------------------
//Playing resampler 
//------------------------------------------------------------------------------
int resampler(short* src, short* dest, int srclen, int rate)
{


 short* sptr=src; //source
 short* dptr=dest; //destination
 int step=(rate*ONE_POS)/NOMINAL_RATE; //ratio between specified and default rates
 int diff=0;
 short i;

 //process samples
 for(i=0;i<srclen;i++) //process melpe frame
 {
  diff = *sptr-au.left_sample; //computes difference beetwen current and basic samples
  while(au.pos <= ONE_POS) //while position not crosses a boundary
  {
   *dptr++ = au.left_sample + ((diff * au.pos)>>14); //set destination by basic, difference and position
    au.pos += step; //move position forward to fractional step
  }
  au.left_sample = *sptr++; //set current sample as a  basic
  au.pos -= ONE_POS; //move position back to one outputted sample
 }
 return dptr-dest;  //number of outputted samples

}



//------------------------------------------------------------------------------
//check level of playing speech for smoothly adjusting mike sensitivity value
//------------------------------------------------------------------------------
void getsplevel(const short* spc)
{
 int i;
 unsigned int e=0;

 
 for(i=0;i<AMRFRAMELEN;i++) e+=abs(spc[i]); //summ of amplitudes
 e>>=7; // /=128: mean amplitude
 au.splevel -= (au.splevel>>ECHOSMOOTH);  //smooth mike level
 au.splevel += e;             //adjust mike level
}

//------------------------------------------------------------------------------
//decrease mike level depends current sensitivity value
//------------------------------------------------------------------------------
void setmksens(short* spc)
{
 
 int i;

 i=au.splevel>>ECHOSMOOTH; //0-32767  average amplitude
 if(i>MAXLEVEL) i=MAXLEVEL; //up trashold
 if(i>MINLEVEL) //down trashold
 {
  i=256-(au.echo*(i-MINLEVEL))/(MAXLEVEL-MINLEVEL); //value 0-255 for decrese mike sensitivity
  if(i>256) i=256;
  if(i<0) i=0;
  volume(spc, (unsigned short)i); //apply
 }
}

//******************************************************************************
//INTRFACE FOR AUDIO PROCESSING
//******************************************************************************

//------------------------------------------------------------------------------
//Open audio grabbing/playing
//------------------------------------------------------------------------------
//input is text strings from "0" to "9" are numbers of recording and playing devices in system
//in more cases "0" is default sound card
int au_create(char* audio_in, char* audio_out)
{
  strncpy(au.grabber, audio_in, sizeof(au.grabber)); //set grabbing souncard
  strncpy(au.player, audio_out, sizeof(au.player));  //set playing soundcard
  npp_ini();  //initialize noise supressor

  //initialize amr codec
  au.amrenstate = Encoder_Interface_init(1);  //dtx on
  au.amrdestate = Decoder_Interface_init();

  au.snddev=0;   //descryptor of sound device
  memset(au.inbuf, 0, sizeof(au.inbuf));  //one frame sample buffer
  memset(au.outbuf, 0, sizeof(au.outbuf));//resumpler output for play
  au.pos=ONE_POS; //resamplers position
  au.left_sample=0; //resamplers base
  au.splevel=0; //energy of played sound
  au.mode=0; //mode of audio
  au.mute = 0; //mute grabbing
  au.tone = 0; //local tone modes
  au.npp=0; //npp
  au.echo=0; //echo supression off
  au.tx=256; //speaker volume
  au.rx=256; //mike sensitivity
  return 0;
}

//------------------------------------------------------------------------------
//Close audio grabbing/playing
//------------------------------------------------------------------------------
void au_close(void)
{
  au_stop();  //stop audio process

  //close codec
  if(au.amrenstate)
  {
   Encoder_Interface_exit(au.amrenstate);
   au.amrenstate=0;
  }
  if(au.amrdestate)
  {
   Decoder_Interface_exit(au.amrdestate);
   au.amrdestate=0;
  }
}


//set output path
void au_dev(unsigned char dev)
{
 if(dev!=au.dev)
 {	
  au_stop();
  au.dev=dev;
  sounddev(dev);		
 }
}

//------------------------------------------------------------------------------
//START: Start audio grabbing/playing
//------------------------------------------------------------------------------
int au_start(void)
{
   if(AU_DBG) printf("au_start  snddev=%d\r\n", au.snddev);
  //initialize audio play&capture device
  if(!au.snddev) //if audio device was specified
  {  //run sound thread and set flag if success
     if(soundinit(au.grabber, au.player)) au.snddev=1;
  }

  if(au.snddev) soundrec(1);  //run recording process

  memset(au.inbuf, 0, sizeof(au.inbuf));  //one frame sample buffer
  memset(au.outbuf, 0, sizeof(au.outbuf));//resumpler output for play
  au.pos=ONE_POS; //resamplers position
  au.left_sample=0; //resamplers base
  au.splevel=0; //energy of played sound

  if(au.snddev) return 1; else return 0; //returns flag of audio  enables
}

//------------------------------------------------------------------------------
//STOP: Stop audio grabbing/playing
//------------------------------------------------------------------------------
void au_stop(void)
{
  //close audio device
  if(au.snddev) //if audio enables
  {
   soundrec(0); //stop recording process
   soundterm(); //terminate sound thread
   au.snddev=0; //clear flag of audio enabled
  }
  au.mode=0; //clear audio mode state 
}

//------------------------------------------------------------------------------
//PLAY: decrypt, decode packet, resample and play speech
//------------------------------------------------------------------------------
//always return 0
int au_play(unsigned char* pkt)
{
 int i, j;
 int Rs_out_rate;
 int playdelay;
 
 //printf("Play\r\n");

 if(!au.snddev) return 0; //check audio is enabled
 decrypt(pkt);  //decrypt voice packet

 if(au.tone==9) memcpy(pkt+4, au.loopamr, 12); //amr loopback

 if(au.tone==7) //test3: AMR local playing Replace received pkt by table pkt marked as a voice
 {
  //printf("autest %d\r\n", au.tone_cnt);
  memcpy((unsigned char*)(pkt+4), speech_tbl[au.tone_cnt], 12);//copy next AMR frame by index tone_cnt to pkt+4
  au.tone_cnt++;  //increment tone_cnt up to test table volume
  if(au.tone_cnt>=AMR_SPEECH) au.tone_cnt=0;
  pkt[0]&=(~0x40); //mark pkt as a voice
 }
 
 if(au.tone==6) memcpy(au.inbuf, au.loopbuf, AMRFRAMELEN*sizeof(short)); //test2: loopback test: use PCM from own Mike
 else if(au.tone==5) //test replacing by beep
 { 
   if(pkt[0]& 0x40) for(i=0;i<AMRFRAMELEN/16;i++) //silency on test1: play beep 500
   {
    for(j=0;j<16;j++) au.inbuf[i*16+j]=beep500[j];
   }
   else for(i=0;i<AMRFRAMELEN/8;i++) //incoming voice in test1: play beep 100
   {
    for(j=0;j<8;j++) au.inbuf[i*8+j]=beep1000[j];
   }
 }
 else if(au.tone==1) memset(au.inbuf, 0, 2*AMRFRAMELEN); //force silency
 else if(au.tone==2) //connecting signal
 {
  if(!au.tone_cnt) for(i=0;i<AMRFRAMELEN/8;i++) //connect beep 1000Hz
                           {
                            for(j=0;j<8;j++) au.inbuf[i*8+j]=beep1000[j];
                           }
  else memset(au.inbuf, 0, 2*AMRFRAMELEN);
  au.tone_cnt++;
  if(au.tone_cnt>CONNECT_SIGNAL_PAUSE) au.tone_cnt=0;
 }
 else if(au.tone==3) //call signal
 {
  if(au.tone_cnt<CALL_SIGNAL) for(i=0;i<AMRFRAMELEN/16;i++) //call signal 500Hz
                                      {
                                       for(j=0;j<16;j++) au.inbuf[i*16+j]=beep500[j];
                                      }
  else memset(au.inbuf, 0, 2*AMRFRAMELEN); //call pause
  au.tone_cnt++;
  if(au.tone_cnt>CALL_SIGNAL_PAUSE) au.tone_cnt=0;
 }
 else if(au.tone==4) //ring signal
 {
  if(au.tone_cnt<RING_SIGNAL)
  memcpy(au.inbuf, ringwave+AMRFRAMELEN*au.tone_cnt, 2*AMRFRAMELEN); //ring
  else memset(au.inbuf, 0, 2*AMRFRAMELEN); //ring pause
  au.tone_cnt++;
  if(au.tone_cnt>RING_SIGNAL_PAUSE) au.tone_cnt=0;
 }
 else if( (!(pkt[0]& 0x40)) && au.amrdestate ) //check voice flag
 {  //this is a voice packet 
  //check VAD decission flag
  if(pkt[4]&1) 
  {
   memset(au.inbuf, 0, 2*AMRFRAMELEN); //set silency
  }
  else 
  {
   AMR_decode(au.amrdestate, AMRMODE, pkt+4, au.inbuf, 0); //or decode voice frame
   volume(au.inbuf, au.tx); //manually set volume 
  }
 }  //this is a silency or beep packet
#ifdef BEEP_ROGER 
 else if(pkt[4]==1) for(i=0;i<160;i++) au.inbuf[i]=beep500[i&15];  //500Hz 20 mS
 else if(pkt[4]==2) for(i=0;i<160;i++) au.inbuf[i]=beep1000[i&7];  //1000Hz 20mS
#endif //BEEP_ROGER 
 else memset(au.inbuf, 0, 2*AMRFRAMELEN); //silency
 
 getsplevel(au.inbuf);  //set output level for echo supressing
 i=pkt[0]&0x3F; //set playing rate by value from packet 0-63
 Rs_out_rate=AMRSAMPLERATE + DRATE*(i-0x1F); //6450-9550 samoles per sec
 //resample audio for adjusting palying time
 i=resampler(au.inbuf, au.outbuf, AMRFRAMELEN,  Rs_out_rate);
 //get number of unplayed audio samples in low level buffer at the moment
 playdelay=getdelay();
 //printf("Rate=%d smpl=%d del=%d\r\n", Rs_out_rate, i, playdelay);
 //add new audio to low level buffer
 if(playdelay<1500) //check the low-level buffer have space
 {
  //play speech frame
  j=soundplay(i, (unsigned char *)au.outbuf); //add i samples to low level buffer
  if(j<0) //underrun was occured
  {
   memset(au.inbuf, 0, 2*AMRFRAMELEN); //set silency
   soundplay(AMRFRAMELEN, (unsigned char *)au.outbuf);  //play one extra frame
   soundplay(i, (unsigned char *)au.outbuf); //play buffer once more
  }
 }
 return 0;
}


//------------------------------------------------------------------------------
//GRAB: grab speech frame, denoize, encode and encrypt
//------------------------------------------------------------------------------
 //returns 0 if no new frames and 16 is a new frame length in pkt 
 int au_grab(unsigned char* pkt)
 {
  int i;
  int playdelay;
         
  if(!au.snddev) return 0;  //check audio is enabled

  //check the number of unplayed samples in play buffer is critical
  playdelay=getdelay(); //get actual playing delay in samples
  
  if(au.tone) i=NOMPLAYDELAY; else i=NOTPLAYDELAY;
  
  if(playdelay<=i)
  {
   au.playamr[0]=0x40;
   au.playamr[4]=0;
   au_play(au.playamr); //play silency or tone
  }

  i=soundgrab((char*) au.inbuf, AMRFRAMELEN); //grab one frame of PCM speech

  //if(i) printf("grab %d\r\n", i);

  if( (i!=AMRFRAMELEN) || (!au.mode) ) return 0;  //haven't yet
  volume(au.inbuf, au.rx); //manual set of mike sensetivity
  //loopback and mute
  if(au.tone==6) memcpy(au.loopbuf, au.inbuf, AMRFRAMELEN*sizeof(short)); //test2: loopback
  if(au.mute) memset(au.inbuf, 0, AMRFRAMELEN*sizeof(short)); //mute grabbing
  //decrease sensitivity on echo
  if(au.echo) setmksens(au.inbuf);  //dunamically decrease mike sensitivity for echo supressing
  if(au.npp) npp(au.inbuf, au.inbuf); //denoise
  //amr encoding
  pkt[0]=0x40; //set silency flag
  if(au.amrenstate)  //check amr codec is initialized
  {
   if(au.tone==8) //test4: send test AMR frames
   {
     if(au.tone_cnt<AMR_SPEECH) memcpy((unsigned char*)(pkt+4), speech_tbl[au.tone_cnt], 12);//copy next AMR frame by index tone_cnt to (unsigned char*)(pkt+4)
     else memcpy((unsigned char*)(pkt+4), speech_tbl[0], 12);//copy first AMR frame (silency)
     au.tone_cnt++; //increment tone_cnt up to AMR table volume
     if(au.tone_cnt>(AMR_SPEECH+TEST_SIGNAL_PAUSE)) au.tone_cnt=0; //in loop
   }
   else AMR_encode( au.amrenstate, AMRMODE, au.inbuf, (unsigned char*)(pkt+4), AMRCBR); //encode PCM to AMR frame



   //pkt[4]&=0xFE; //force voice
   //pkt[4]|=0x01; //force silency

   if(!(pkt[4]&1))
   {
    pkt[0]=0;   //check no frame silency flag and clear pkt silency flag
    //printf("Voice\r\n");
   }
   //else printf("Silency\r\n");

   if(au.tone==9) memcpy(au.loopamr, pkt+4, 12);  //loopback with amr encoding

   encrypt(pkt); //encrypt packet
  }

  //request 0,1 or 2 new frames for next playing depend actual number of unplayed samples
  if(playdelay<MINPLAYDELAY) pkt[0]+=2; //request 2 frames for next playing
  else if(playdelay<MAXPLAYDELAY) pkt[0]++; //or request one frame for playing
  //printf("Delay: %d, req:%d\r\n", playdelay, (int)pkt[0]&0x0F);
  if(AU_DBG) printf("playdelay=%d, header=%d\r\n", playdelay, 0x3F&pkt[0]);
  return 16; //returns length of grabbed audio packet
 }


//------------------------------------------------------------------------------
//controlling of audio grabbing
//------------------------------------------------------------------------------
void au_mode(unsigned char mode)
{
 au.mode=mode; //enabling audio output
}

//------------------------------------------------------------------------------
//controlling of noise protecting and acho cancellation
//------------------------------------------------------------------------------
//noise preprocessing on/off (MSB) and vocoder type 
void au_npp(unsigned char val)
{
 au.npp=val; //set new for non-negative value
}

//speacker volume
void au_tx(unsigned char val)
{
 if(val<=9) au.tx=au_tx_tab[val]; //set new for non-negative value
}

//mike sensityvity
void au_rx(unsigned char val)
{
 if(val<=9) au.rx=au_rx_tab[val];//set new for non-negative value
}

//echo supression level
void au_echo(unsigned char val)
{
 au.echo=val;//set new for non-negative value
}


//local playing tone: 0-voice, 1-silency, 2-connect, 3-call, 4-ring
//test modes: 5-beep, 6-loop, 7-speaker, 8-mike 
void au_tone(unsigned char val)
{
 au.tone=val; //set new for non-negative value
}

//1-mute grabbing
void au_mute(unsigned char val)
{
 au.mute=val; //set new for non-negative value
}


//interface: set mode of audio module
//0-stop, 1-mute, 2-connect, 3-wait, 4-ring,
//5-talk, 6-beep, 7-loop, 8-spk, 9-mike
void au_set(unsigned char val)
{
 switch(val)
 {
  //iddle mode
  case AUDIO_OFF:     {au_mode(0);au_mute(1);au_tone(1);au_stop();break;}   //stop audio
  //no grab, play tones only
  case AUDIO_MUTE:    {au_mode(0);au_mute(1);au_tone(1);au_start();break;}  //global muting during call
  case AUDIO_CONNECT: {au_mode(0);au_mute(1);au_tone(2);au_start();break;}  //play connect tone during IKE
  case AUDIO_WAIT:    {au_mode(0);au_mute(1);au_tone(3);au_start();break;}  //play wait tone before answer
  case AUDIU_RING:    {au_mode(0);au_mute(1);au_tone(4);au_start();break;}  //play ring tone before answer
  //talk
  case AUDIO_TALK:    {au_mode(1);au_mute(0);au_tone(0);au_start();break;}  //talk
  //test
  case AUDIO_BEEP:    {au_mode(1);au_mute(0);au_tone(5);au_start();break;}  //beep test
  case AUDIO_LOOP:    {au_mode(1);au_mute(0);au_tone(6);au_start();break;}  //loop back test
  case AUDIO_SPK:     {au_mode(1);au_mute(0);au_tone(7);au_start();break;}  //local playing test
  case AUDIO_MIKE:    {au_mode(1);au_mute(0);au_tone(8);au_start();break;}  //remote playing test
  case AUDIO_COD:     {au_mode(1);au_mute(0);au_tone(9);au_start();break;} //loopback over AMR 
 }
 au.tone_cnt=0; //clear tone frames counter
}

