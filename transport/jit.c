//**************************************************************************
//ORFone project
//Sound data queue for transport module
//**************************************************************************


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jit.h"

jit_data jt; //internal data of audio queue module

char JT_DBG=0;

//*************************************************************************
//                               INTERNAL PROCEDURES
//*************************************************************************

//-------------------------------------------------------------------------
//convert frame counter value to buffer
//-------------------------------------------------------------------------
void cnt2buf(unsigned char* buf, unsigned int cnt)
{
 buf[1]=cnt&0xFF;
 buf[2]=(cnt>>8)&0xFF;
 buf[3]=(cnt>>16)&0xFF;
}


//*************************************************************************
//                               INTERFACE
//*************************************************************************



//========================Setup============================================

//-------------------------------------------------------------------------
//Reset sound queue for new session
//-------------------------------------------------------------------------
void jt_reset_speech(void)
{
 //---------receiving - playing global values------------
 memset(jt.rbuf, 0, sizeof(jt.rbuf)); //anti-jitter ring receiving buffer
 jt.in_pointer=0;  //pointer to frame will be received
 jt.out_pointer=0; //pointer to frame will be played
 jt.frame_counter=0; //expected counter of next receiving frame
 jt.play_crate=31;   //playing rate for cryptor (0-31, nominal is 15)
 jt.requested_frames=0; //number of frames requested by player
 jt.ssln=0; //counter of played silency frames
 //----------grabbing - sending part----------------------
 memset(jt.sbuf, 0, sizeof(jt.sbuf)); //recording collect buffer
 jt.scnt=0; //number of recorded frames in buf
 jt.smax=FRAMESINPACKET_TOR; //maximal number of frames in sended packet
 jt.sbeep=BEEPINTERVAL; //silency tail after thet the beep signal will be played
 jt.sctr=0; //counter of first packet in buf
 jt.jtalk=1; //voice on
 jt.chn=CHANNEL_TOR;
 jt.defchn=CHANNEL_TOR; //clear flag of default channel
 jt.min_level=GUARD_LEVEL*K_LEVEL;
 jt.max_level=GUARD_LEVEL*K_LEVEL;
}

//-------------------------------------------------------------------------
//set maximal number of 20 mS voice frames in transport packet
//parameter 0 - default channel, 1-UDP channel, 2-TCP channel, 3-tor channel
//for 2 and 3 set this channel default
//-------------------------------------------------------------------------
void jt_set_channel_type(int channel)
{

 int defchn = jt.defchn; //default channel type for this session
 int talk = jt.jtalk; //current state

 jt_reset_speech(); //reset voice queue
 jt.jtalk = talk; //restore current state
 jt.defchn = defchn; //restore default channel

 if(!channel) channel=jt.defchn; //set sesiion default channel if not specified

 if(channel==CHANNEL_UDP)  //for fast UDP transport channel
 {
  jt.smax = FRAMESINPACKET_UDP;
  jt.chn = CHANNEL_UDP;
 }
 else if(channel==CHANNEL_TCP)  //for middle TCP transport channel
 {
  jt.smax = FRAMESINPACKET_TCP;
  jt.defchn = channel; //set default channel type
  jt.chn=CHANNEL_TCP;
 }
 else //for slow TOR transport channel
 {
  jt.smax = FRAMESINPACKET_TOR;
  jt.defchn = channel; //set default channel type
  jt.chn = CHANNEL_TOR;
 }
}



//-------------------------------------------------------------------------
//mute/unmute Mike (allowing transmitt outgoing packets from collector)
//-------------------------------------------------------------------------
void jt_set_talk(int talk)
{
 unsigned char smax = jt.smax; //save current value of channel type

 jt_reset_speech(); //reset speech
 jt.smax=smax; //restore current channel type
 jt.jtalk=talk; //set global un-mute flag

}


//========================Transmitt========================================


//-------------------------------------------------------------------------
//input: collect voice packet from cryptor to collector, returns 0
//if collector compleet return packets length and transport data for sending in buf
//-------------------------------------------------------------------------
int jt_add_cr_speech(unsigned char* buf)
{
  int i;
  unsigned int ctr;


//We will send only voiced frames and skip all unvoiced. Packet can
//contain from one to fixed maximal value of subsequent voiced frames
//For new voice frame we collect it in buffer and send comleet packet if
//we have maximal number of collected frames.

//If next frame is unvoiced, we skip it but send all previosly collected
//voiced frame from buffer (if exist). This packet can be incompleet
//(have the number of frames less then maximal down to one).

//The maximal number of frames (20 mS each) in packet is pre-set depens channel type.
//For Tor value is 10, for direct TCP is 4, for direct UDP is 2.

  //SET NUMBER OF FRAMES REQUSTED BY CRYPTOR FOR PLAYING

  //printf("req=%d to %d\r\n", jt.requested_frames, buf[0]&0x3F);

  jt.requested_frames=buf[0]&0x3F;  //set number of frames for playing requested by cryptor

  //RESTORE 24 BITS PACKET COUNTER FROM 16BITS VALUE FROM CRYPTOR PACKET HEADER
  //24 bit counter from packet
  ctr=((unsigned int)buf[3]<<16)+((unsigned int)buf[2]<<8) + buf[1]; //number of current voice frame

  if(!jt.jtalk) //check muting flag
  {
   jt.scnt=0;  //clear number of frames collected in buffer (collector is emty now)
   return 0;   //not collect new voice packets from cryptor
  }

  //check collector is empty now
  if(!jt.scnt) jt.sctr=ctr; //save cnumber of first collected frame in jt.sbuf

  //PROCESS UNVOICEED PACKET  FROM CRYPTOR
  if(buf[0]&0x40) //check  packet is unvoiced
  {
   if(jt.scnt) //there are some voiced packets in buf
   {  //make transport packet from existed voice packets, clear collector
    for(i=0;i<jt.scnt;i++) memcpy(buf+4+i*12, jt.sbuf[i], 12); //output all frames from record buf
    cnt2buf(buf, jt.sctr); //output 24 bits counter is a number of first frame in the packet
    buf[0]=jt.scnt*12+4;   //output packet length in bytes
    jt.scnt=0; //clear collector (all collected frames were outputted)
    return (int)buf[0];  //return packet length in bytes
   }
  }
  //ADD VOICED PACKET FROM CRYPTOR
  else //packet is voiced
  {
   memcpy(jt.sbuf[jt.scnt], buf+4, 12); //add frame to collector
   jt.scnt++; //check number of frames in collector now
   if((jt.scnt>=jt.smax)||(jt.scnt>=JTMAXCNT)) //bcollector full fill and ready for sending
   {  //make transport packet
    for(i=0;i<jt.scnt;i++) memcpy(buf+4+i*12, jt.sbuf[i], 12); //output all frames from collector
    cnt2buf(buf, jt.sctr); //add first frame number to packet's header
    buf[0]=jt.scnt*12+4;  //add packets length to header
    jt.scnt=0; //clear collector
    return (int)buf[0];  //return packet length in bytes
   }
  }
 return 0; //haven't collected frames for compose packet now
}


//========================Receive========================================

//-------------------------------------------------------------------------
//process data packet from remote, update ring buffer
//return 0 for doubles packets and 1 for actual processed packets
//-------------------------------------------------------------------------
int jt_add_tf_speech(unsigned char* buf, int len)
{
 unsigned int ctr;
 int frames_in_packet, frames_in_buffer, frames_of_silency, jitter, level;
 int i;

//We can receive packets over three channels: two Tor connections and
//direct UDP connecting. So we can have three same copy of voice packets.
//After first copy was processed we must reject other copies by check
//frame counter in received packet.

//Received packet can contain from one to fixed maximal value
//of subsequent voice frames with one counter value for first frame.
//Note: we not send silency, only voiced frames. But we can obtain
//how many silency frames was skipped by comparing counters in the
//subsequent voice packets. So we can add skipped silency frames to buffer.

//Now we can adjust anti-jitter buffer filling in two ways:

//If we have at least one skipped silency frames before, we can
//add more or less silency to buffer depends actual fill level
//so we can adjust fill level of the buffer to required one.

//If we have subsequented voice without silency inserts we still can
//set player rate and adjust fill level of buffer this way too.

//And also we can smoothle adjust required value of buffer's fill level
//depend mesured jitter of namber of frames in buffer (tail) at the moment
//of receiving next full voice packet (for this both compared packets must be full
//so contains equal maximal number of voice frames). Jitter is the difference of
//frames in buffer in two measurements. Further we can obtain the expected buttom
//of buffer is expected fill level minus a half of jitter. This value must be
//positive for preventing underruns. But it must be no so large for minimize
//latency. The way is comparing the buttom value with muximum number of frames
//in the packet pre-setted for specific voice transport (Tor, TCP, UDP).
//Empirically the buttom value must be between a maximal frames value and half of it.
//So we can smoothle adjust expected fill value based on jitter measurement.

//And also we have additional way for urgent preventing of underruns:
//if tail number of frame in buffer in the moment of next packet was received
//is less then some fixed value (for example, one or two) we must increase
//expected fill value rapidly for adequate adjusting of actual fill of buffer.


//CHECK PACKET DATA AND REJECT DOUBLES
//----------------------------------------------------
 if(len<16) return 0;  //packet must containg 4 bytes of header and at least one 12 bytes voice frame
 frames_in_packet=(len-4)/12; //number of frames in packet
 ctr = ((unsigned int)buf[3]<<16) + ((unsigned int)buf[2]<<8) + buf[1]; //counter of this frame
 frames_of_silency=ctr - jt.frame_counter; //number of unvoiced frames skipped before
 if(frames_of_silency<0) return 0; //this is old packet (double), skip it
 frames_in_buffer = jt.in_pointer - jt.out_pointer;  //actual number of frames in buffer now
 if(frames_in_buffer<0) frames_in_buffer+=128; //ring buffer!
 level = frames_in_buffer; //buffer guard level
 //----------------------------------------------------


//ADD SOME SILENCY TO BUFFER
//----------------------------------------------------
 if(frames_of_silency) //some silency was before: add silency frames to buffer
 {
  jitter=(jt.min_level+jt.max_level)/(2*K_LEVEL); //actual jitter (frames)

  //compute optimal buffer level for different channel types
  if(jt.chn==CHANNEL_UDP) i=UDP_GUARD_LEVEL;  //fixed guard buffer level for UDP
  else if(jt.chn==CHANNEL_TCP) i=TCP_GUARD_LEVEL; //fixed guard buffer level for TCP
  else i=jitter+GUARD_LEVEL; //optimal buffer fill level for Tor (number of frames in buffer)

  frames_of_silency = i - frames_in_buffer; //optimal length of inserted silency (frames)
  if(frames_of_silency<1) frames_of_silency=1; //at least one (a few of silecy must be added anyway)
  i=frames_in_buffer+frames_of_silency; //current buffer fill level (frames)
  //printf("jt=%d, silw=%d, min=%d\r\n", jitter, frames_of_silency, jt.min_level);
  jt.min_level=(i-jitter)*K_LEVEL; //esimated minimal buffer level depends jitter
  //printf("jt=%d, silw=%d, min=%d\r\n", jitter, frames_of_silency, jt.min_level);
  if(jt.min_level<0) jt.min_level=0; //restrict
  jt.max_level=jt.min_level; //set max level equal to min level (clear jitter value)
  //add silency to buffer
  for(i=0;i<frames_of_silency;i++)
  {
   jt.rbuf[jt.in_pointer][0]=0x40; //set unvoiced flag
   jt.rbuf[jt.in_pointer][4]=0; //no beep
   jt.in_pointer++; //move pointer
   jt.in_pointer&=0x7F; //ring buffer!
   frames_in_buffer++; //number of frames in buffers now
  }
  jt.frame_counter=ctr; //set counter as a value of first voiced frame
 }
 else //thre are subsequent voice stream: obtain jitter
 { //set maximal and minimal buffer levels
  //printf("Test: min=%d fr=%d\r\n", jt.min_level, frames_in_buffer);
  if(frames_in_buffer > (jt.max_level/K_LEVEL)) jt.max_level=K_LEVEL*frames_in_buffer;
  else if(jt.max_level) jt.max_level--; //slowly decrease
  if(frames_in_buffer < (jt.min_level/K_LEVEL)) jt.min_level=K_LEVEL*frames_in_buffer;
  else jt.min_level++; //slowly increase
 }

 //set optimal playing rate for different channels

 //compare starting frames_in_buffer with GUARD
 if(jt.chn==CHANNEL_UDP)
 {
  i=level-UDP_GUARD_LEVEL; //can be -1, 0, or >0
  if(i>UDP_GUARD_LEVEL) i=UDP_GUARD_LEVEL;
  i*=K_RATE_UDP; //-5, 0, 5
  i+=31;
 }
 else if(jt.chn==CHANNEL_TCP)//TCP
 {
  //compute optimal playing rate
  i=jt.min_level - TCP_GUARD_LEVEL*K_LEVEL;   //positive or negative, proportional crate delta
  i*=K_RATE_TCP; //apply smothing koefficient
  i/=(TCP_GUARD_LEVEL*K_LEVEL); //compute ratio
  i+=31; //conver signed to unsigned
 }
 else //TOR
 {
  //compute optimal playing rate
  i=jt.min_level - GUARD_LEVEL*K_LEVEL;   //positive or negative, proportional crate delta
  i*=K_RATE; //apply smothing koefficient
  i/=(GUARD_LEVEL*K_LEVEL); //compute ratio
  i+=31; //conver signed to unsigned
 }


 jt.play_crate=i; //set playing rate tuning value recommended  for playing
 if(jt.play_crate<0) jt.play_crate=0; //restrict rate value
 else if(jt.play_crate>63) jt.play_crate=63; //only 5 bits uses
 //if(JT_DBG) printf("sile=%d, rate=%d min=%d max=%d fr=%d\r\n", frames_of_silency, jt.play_crate, jt.min_level/K_LEVEL, jt.max_level/K_LEVEL,frames_in_buffer);

//ADD RECEIVED VOICE FRAMES TO BUFFER
//----------------------------------------------------
 for(i=0;i<frames_in_packet;i++) //add received voice frames to buffer
 {
  memcpy(jt.rbuf[jt.in_pointer]+4, buf+4+i*12, 12);  //add voice frame
  cnt2buf(jt.rbuf[jt.in_pointer], jt.frame_counter); //add frame counter
  jt.rbuf[jt.in_pointer][0]=0; //voice flag
  jt.in_pointer++;   //move buffer pointer
  jt.in_pointer&=0x7F;  //ring buffer!
  jt.frame_counter++;  //next frame counter
  frames_in_buffer++; //increase number of frames in buffer
 }
 //CHECK GENERAL PACKET VALIDITY
 //----------------------------------------------------
 len-=4; //skip header length
 if(len%12) return 0;  //must be for some application but not for Torfone (padding!!!)
 else return 1;
}



//-------------------------------------------------------------------------
//extract from ring buffer one voice or silency frame
//or silency on underrun, make packet for cryptor in buf
//return frame length == 16  or 0 if no packets were requested by cryptor
//-------------------------------------------------------------------------
int jt_ext_cr_speech(unsigned char* buf)
{

 //Player periodically extracts next frame from buffer by own timer based on
 //audio device playin rate.  If there are no frames in buffer (underrun)
 //buffer still outputs silency frames with beep mark in one silency
 //frame (not first!) aftre underrun. This mark can be used as "Roger" signal after
 //remote party stop talk and wait answer.

 //CHECK NUMBER OF FRAMES REQUESTED BY PLAYER 
 if(!jt.requested_frames) return 0; //check some packet were requested by cryptor
 jt.requested_frames--; //decrese number of requested packet (one will be sended now)

 //if(JT_DBG) printf("out=%d in=%d\r\n", jt.out_pointer, jt.in_pointer);
 //check ring buffer have voice frame
 if(jt.out_pointer!=jt.in_pointer) //ring buffer is not empty
 {
  memcpy(buf, jt.rbuf[jt.out_pointer], 16); //voice or sillency frame from queue
  jt.out_pointer++; //move output pointer
  jt.out_pointer&=0x7F; //wrap ring buffer
  jt.ssln=0; //clear number of subsequenting silency frames
 }
 else //no frames in buffer: underrun
 {
  jt.ssln++; //count silency frames
  buf[0]=0x40; //silency type
  if(jt.ssln==jt.sbeep) buf[4]=1;
  else buf[4]=0; //silency or beep500 once
 }

 buf[0]+=jt.play_crate;  //set optimal play rate for cryptor depends queue fill level
 return 16; //always return a frame
}





