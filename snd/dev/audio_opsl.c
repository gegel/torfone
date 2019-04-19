

/*
  opensl_io.c:
  Android OpenSL input/output module
  Copyright (c) 2012, Victor Lazzarini
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
  * Neither the name of the <organization> nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#if defined(__ANDROID__)

#include "audio_opsl.h"
#define CONV16BIT 32768
#define CONVMYFLT (1./32768.)
#define GRAIN 4


//we have an internal audio queue is double buffers CHSIZE each.  
//When buffer is ready (empty for playing or full for recording) the callback is occures
//in this callback ready buffer process to/from circular buffer
//circular buffer contain space for BUFFERS number of chunks CHSIZE each.
//So only CHSIZE can be grabbing one time  or can be playing one time by internal audio queue.
//But user application can add/grab any samples in each one portion to/from circullarbuffer
//This is unblocking, so if you want to grab 160 samples, but in circullar buffer is only 10 samples
//we will grab 10 samples (returned value).
//And the same for playing: we can play 10 sample the 170 samples but if internal engine will require 
//next portion of CHSIZE samples and there are no this count in the cirecullar buffer in time 
//the flag "underrun" will be set.
//after first playing after underrun occures playsound will returns -1 and play one chunk of silency
//in this case we must replay actual samples ones more

   


//-----globals for torfone interface-------
#define OPSR 8000  //sampling rate
#define CHSIZE 320 //chunk size in short
#define BUFFERS 8  //number of chunks in circullar buffers

#include <stdio.h>

//===================================================
#include "aecm.h"
void *aecmInst = 0;    //aec instance pointer
short out_buffer[320]; //aec output buffer

//==================================================




OPENSL_STREAM  *op_stream=NULL;  //pointer to used IO stream
int PlayPath=0; //default path
int IsGo=1;     //flag: input runs
volatile int pldelay=0; //number of unplayed samples in the play buffer (2*CHSIZE internal double-buffers not in this count)
//-----------------------------------------


static void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
static void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
circular_buffer* create_circular_buffer(int bytes);
int checkspace_circular_buffer(circular_buffer *p, int writeCheck);
int read_circular_buffer_bytes(circular_buffer *p, char *out, int bytes);
int write_circular_buffer_bytes(circular_buffer *p, const char *in, int bytes);
void free_circular_buffer (circular_buffer *p);

// creates the OpenSL ES audio engine
static SLresult openSLCreateEngine(OPENSL_STREAM *p)
{
  SLresult result;
  // create engine
  result = slCreateEngine(&(p->engineObject), 0, NULL, 0, NULL, NULL);
  if(result != SL_RESULT_SUCCESS) goto  engine_end;

  // realize the engine 
  result = (*p->engineObject)->Realize(p->engineObject, SL_BOOLEAN_FALSE);
  if(result != SL_RESULT_SUCCESS) goto engine_end;

  // get the engine interface, which is needed in order to create other objects
  result = (*p->engineObject)->GetInterface(p->engineObject, SL_IID_ENGINE, &(p->engineEngine));
  if(result != SL_RESULT_SUCCESS) goto  engine_end;

 engine_end:
  return result;
}

// opens the OpenSL ES device for output
static SLresult openSLPlayOpen(OPENSL_STREAM *p)
{
  SLresult result;
  SLuint32 sr = p->sr;
  SLuint32  channels = p->outchannels;

  if(channels){
    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};

    switch(sr){

    case 8000:
      sr = SL_SAMPLINGRATE_8;
      break;
    case 11025:
      sr = SL_SAMPLINGRATE_11_025;
      break;
    case 16000:
      sr = SL_SAMPLINGRATE_16;
      break;
    case 22050:
      sr = SL_SAMPLINGRATE_22_05;
      break;
    case 24000:
      sr = SL_SAMPLINGRATE_24;
      break;
    case 32000:
      sr = SL_SAMPLINGRATE_32;
      break;
    case 44100:
      sr = SL_SAMPLINGRATE_44_1;
      break;
    case 48000:
      sr = SL_SAMPLINGRATE_48;
      break;
    case 64000:
      sr = SL_SAMPLINGRATE_64;
      break;
    case 88200:
      sr = SL_SAMPLINGRATE_88_2;
      break;
    case 96000:
      sr = SL_SAMPLINGRATE_96;
      break;
    case 192000:
      sr = SL_SAMPLINGRATE_192;
      break;
    default:
      return -1;
    }
   
    const SLInterfaceID ids[] = {SL_IID_VOLUME};
    const SLboolean req[] = {SL_BOOLEAN_FALSE};
    result = (*p->engineEngine)->CreateOutputMix(p->engineEngine, &(p->outputMixObject), 1, ids, req);
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;

    // realize the output mix
    result = (*p->outputMixObject)->Realize(p->outputMixObject, SL_BOOLEAN_FALSE);


    //SLVolumeItf outputMixVolume;
    //result = (*p->outputMixObject)->GetInterface(p->outputMixObject, SL_IID_VOLUME,
    //        &outputMixVolume);
    //if(result != SL_RESULT_SUCCESS) goto end_openaudio;


    int speakers;
    if(channels > 1) 
      speakers = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    else speakers = SL_SPEAKER_FRONT_CENTER;
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,channels, sr,
				   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
				   speakers, SL_BYTEORDER_LITTLEENDIAN};

    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, p->outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    // create audio player
    const SLInterfaceID ids1[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION};
    const SLboolean req1[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*p->engineEngine)->CreateAudioPlayer(p->engineEngine, &(p->bqPlayerObject), &audioSrc, &audioSnk,
						   2, ids1, req1);
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;


    SLAndroidConfigurationItf playerConfig;
    result = (*p->bqPlayerObject)->GetInterface(p->bqPlayerObject, SL_IID_ANDROIDCONFIGURATION, &playerConfig);

    if(result==SL_RESULT_SUCCESS)
    {
     SLint32 streamType;
     if(PlayPath==2) streamType = SL_ANDROID_STREAM_VOICE;
     else if(PlayPath==3) streamType = SL_ANDROID_STREAM_RING;
     else streamType = SL_ANDROID_STREAM_MEDIA;
     result = (*playerConfig)->SetConfiguration(playerConfig, SL_ANDROID_KEY_STREAM_TYPE, &streamType, sizeof(SLint32));
    }


    // realize the player
    result = (*p->bqPlayerObject)->Realize(p->bqPlayerObject, SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;

    // get the play interface
    result = (*p->bqPlayerObject)->GetInterface(p->bqPlayerObject, SL_IID_PLAY, &(p->bqPlayerPlay));
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;

    //-----------------------------------------------------------------
    // Code for working with ear speaker by setting stream type to STREAM_VOICE

    //SLAndroidConfigurationItf playerConfig;
    //result = (*p->bqPlayerObject)->GetInterface(p->bqPlayerObject, SL_IID_ANDROIDCONFIGURATION, &playerConfig);
    //if(result != SL_RESULT_SUCCESS) goto end_openaudio;

    //SLint32 streamType = SL_ANDROID_STREAM_VOICE;
    //result = (*playerConfig)->SetConfiguration(playerConfig, SL_ANDROID_KEY_STREAM_TYPE, &streamType, sizeof(SLint32));
    //if(result != SL_RESULT_SUCCESS) goto end_openaudio;
    //-----------------------------------------------------------------

    // get the buffer queue interface
    result = (*p->bqPlayerObject)->GetInterface(p->bqPlayerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
						&(p->bqPlayerBufferQueue));
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;

    // register callback on the buffer queue
    result = (*p->bqPlayerBufferQueue)->RegisterCallback(p->bqPlayerBufferQueue, bqPlayerCallback, p);
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;

    // set the player's state to playing
    result = (*p->bqPlayerPlay)->SetPlayState(p->bqPlayerPlay, SL_PLAYSTATE_PLAYING);

    if((p->playBuffer = (short *) calloc(p->outBufSamples, sizeof(short))) == NULL) {
      return -1;
    }

    (*p->bqPlayerBufferQueue)->Enqueue(p->bqPlayerBufferQueue, 
				       p->playBuffer,p->outBufSamples*sizeof(short));
 
  end_openaudio:
    return result;
  }
  return SL_RESULT_SUCCESS;
}

// Open the OpenSL ES device for input
static SLresult openSLRecOpen(OPENSL_STREAM *p){

  SLresult result;
  SLuint32 sr = p->sr;
  SLuint32 channels = p->inchannels;

  if(channels){

    switch(sr){

    case 8000:
      sr = SL_SAMPLINGRATE_8;
      break;
    case 11025:
      sr = SL_SAMPLINGRATE_11_025;
      break;
    case 16000:
      sr = SL_SAMPLINGRATE_16;
      break;
    case 22050:
      sr = SL_SAMPLINGRATE_22_05;
      break;
    case 24000:
      sr = SL_SAMPLINGRATE_24;
      break;
    case 32000:
      sr = SL_SAMPLINGRATE_32;
      break;
    case 44100:
      sr = SL_SAMPLINGRATE_44_1;
      break;
    case 48000:
      sr = SL_SAMPLINGRATE_48;
      break;
    case 64000:
      sr = SL_SAMPLINGRATE_64;
      break;
    case 88200:
      sr = SL_SAMPLINGRATE_88_2;
      break;
    case 96000:
      sr = SL_SAMPLINGRATE_96;
      break;
    case 192000:
      sr = SL_SAMPLINGRATE_192;
      break;
    default:
      return -1;
    }
    
    // configure audio source
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
				      SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataSource audioSrc = {&loc_dev, NULL};

    // configure audio sink
    int speakers;
    if(channels > 1) 
      speakers = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    else speakers = SL_SPEAKER_FRONT_CENTER;
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, channels, sr,
				   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
				   speakers, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    // create audio recorder
    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*p->engineEngine)->CreateAudioRecorder(p->engineEngine, &(p->recorderObject), &audioSrc,
						     &audioSnk, 1, id, req);
    if (SL_RESULT_SUCCESS != result) goto end_recopen;

    // realize the audio recorder
    result = (*p->recorderObject)->Realize(p->recorderObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) goto end_recopen;

    // get the record interface
    result = (*p->recorderObject)->GetInterface(p->recorderObject, SL_IID_RECORD, &(p->recorderRecord));
    if (SL_RESULT_SUCCESS != result) goto end_recopen;
 
    // get the buffer queue interface
    result = (*p->recorderObject)->GetInterface(p->recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
						&(p->recorderBufferQueue));
    if (SL_RESULT_SUCCESS != result) goto end_recopen;

    // register callback on the buffer queue
    result = (*p->recorderBufferQueue)->RegisterCallback(p->recorderBufferQueue, bqRecorderCallback,
							 p);
    if (SL_RESULT_SUCCESS != result) goto end_recopen;
    result = (*p->recorderRecord)->SetRecordState(p->recorderRecord, SL_RECORDSTATE_RECORDING);

    if((p->recBuffer = (short *) calloc(p->inBufSamples, sizeof(short))) == NULL) {
      return -1;
    }

    (*p->recorderBufferQueue)->Enqueue(p->recorderBufferQueue, 
				       p->recBuffer, p->inBufSamples*sizeof(short));
     
  end_recopen: 
    return result;
  }
  else return SL_RESULT_SUCCESS;


}

// close the OpenSL IO and destroy the audio engine
static void openSLDestroyEngine(OPENSL_STREAM *p){

  // destroy buffer queue audio player object, and invalidate all associated interfaces
  if (p->bqPlayerObject != NULL) {
    SLuint32 state = SL_PLAYSTATE_PLAYING;
    (*p->bqPlayerPlay)->SetPlayState(p->bqPlayerPlay, SL_PLAYSTATE_STOPPED);
    while(state != SL_PLAYSTATE_STOPPED)
      (*p->bqPlayerPlay)->GetPlayState(p->bqPlayerPlay, &state);
    (*p->bqPlayerObject)->Destroy(p->bqPlayerObject);
    p->bqPlayerObject = NULL;
    p->bqPlayerPlay = NULL;
    p->bqPlayerBufferQueue = NULL;
    p->bqPlayerEffectSend = NULL;
  }

  // destroy audio recorder object, and invalidate all associated interfaces
  if (p->recorderObject != NULL) {
   SLuint32 state = SL_PLAYSTATE_PLAYING;
    (*p->recorderRecord)->SetRecordState(p->recorderRecord, SL_RECORDSTATE_STOPPED);
    while(state != SL_RECORDSTATE_STOPPED)
      (*p->recorderRecord)->GetRecordState(p->recorderRecord, &state);
    (*p->recorderObject)->Destroy(p->recorderObject);
    p->recorderObject = NULL;
    p->recorderRecord = NULL;
    p->recorderBufferQueue = NULL;
  }

  // destroy output mix object, and invalidate all associated interfaces
  if (p->outputMixObject != NULL) {
    (*p->outputMixObject)->Destroy(p->outputMixObject);
    p->outputMixObject = NULL;
  }

  // destroy engine object, and invalidate all associated interfaces
  if (p->engineObject != NULL) {
    (*p->engineObject)->Destroy(p->engineObject);
    p->engineObject = NULL;
    p->engineEngine = NULL;
  }

}


// open the android audio device for input and/or output
OPENSL_STREAM *android_OpenAudioDevice(int sr, int inchannels, int outchannels, int bufferframes){
  
  OPENSL_STREAM *p;
  p = (OPENSL_STREAM *) malloc(sizeof(OPENSL_STREAM));
  memset(p, 0, sizeof(OPENSL_STREAM));
  p->inchannels = inchannels;
  p->outchannels = outchannels;
  p->sr = sr;
 
  if((p->outBufSamples  =  bufferframes*outchannels) != 0) {
    if((p->outputBuffer = (short *) calloc(p->outBufSamples, sizeof(short))) == NULL) {
      android_CloseAudioDevice(p);
      return NULL;
    }
  }

  if((p->inBufSamples  =  bufferframes*inchannels) != 0){
    if((p->inputBuffer = (short *) calloc(p->inBufSamples, sizeof(short))) == NULL){
      android_CloseAudioDevice(p);
      return NULL; 
    }
  }

  if((p->outrb = create_circular_buffer(p->outBufSamples*sizeof(short)*BUFFERS)) == NULL) {
      android_CloseAudioDevice(p);
      return NULL; 
  }
 if((p->inrb = create_circular_buffer(p->inBufSamples*sizeof(short)*BUFFERS)) == NULL) {
      android_CloseAudioDevice(p);
      return NULL; 
  }

  if(openSLCreateEngine(p) != SL_RESULT_SUCCESS) {
    android_CloseAudioDevice(p);
    return NULL;
  }

  if(openSLRecOpen(p) != SL_RESULT_SUCCESS) {
    android_CloseAudioDevice(p);
    return NULL;
  } 

  if(openSLPlayOpen(p) != SL_RESULT_SUCCESS) {
    android_CloseAudioDevice(p);
    return NULL;
  }  

  p->time = 0.;
  return p;
}

// close the android audio device
void android_CloseAudioDevice(OPENSL_STREAM *p){

  if (p == NULL)
    return;

  openSLDestroyEngine(p);

  if (p->outputBuffer != NULL) {
    free(p->outputBuffer);
    p->outputBuffer= NULL;
  }

  if (p->inputBuffer != NULL) {
    free(p->inputBuffer);
    p->inputBuffer = NULL;
  }

  if (p->playBuffer != NULL) {
    free(p->playBuffer);
    p->playBuffer = NULL;
  }

  if (p->recBuffer != NULL) {
    free(p->recBuffer);
    p->recBuffer = NULL;
  }

  free_circular_buffer(p->inrb);
  free_circular_buffer(p->outrb);

  free(p);
}

// returns timestamp of the processed stream
double android_GetTimestamp(OPENSL_STREAM *p){
  return p->time;
}


// this callback handler is called every time a buffer finishes recording
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
  OPENSL_STREAM *p = (OPENSL_STREAM *) context;
  int bytes = p->inBufSamples*sizeof(short);

  //==================================================
  WebRtcAecm_Process(aecmInst, p->recBuffer, NULL, out_buffer, 160, 80);
  //memcpy(p->recBuffer, out_buffer, 320);
  WebRtcAecm_Process(aecmInst, 160+(p->recBuffer), NULL, out_buffer+160, 160, 80);
  //memcpy(160+(p->recBuffer), out_buffer, 320);
  //===================================================
  write_circular_buffer_bytes(p->inrb, (char *) out_buffer,bytes);
  //write_circular_buffer_bytes(p->inrb, (char *) p->recBuffer,bytes);
  (*p->recorderBufferQueue)->Enqueue(p->recorderBufferQueue,p->recBuffer,bytes);
}
 
// gets a buffer of size samples from the device
int android_AudioIn(OPENSL_STREAM *p,float *buffer,int size){
  short *inBuffer;
  int i, bytes = size*sizeof(short);
  if(p == NULL ||  p->inBufSamples ==  0) return 0;
  bytes = read_circular_buffer_bytes(p->inrb,(char *)p->inputBuffer,bytes);
  size = bytes/sizeof(short);
  for(i=0; i < size; i++){
    buffer[i] = (float) p->inputBuffer[i]*CONVMYFLT;
  }
  //if(p->outchannels == 0) p->time += (double) size/(p->sr*p->inchannels);
  return size;
}

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
  OPENSL_STREAM *p = (OPENSL_STREAM *) context;
  int bytes = p->outBufSamples*sizeof(short);
  if(bytes!=read_circular_buffer_bytes(p->outrb, (char *) p->playBuffer,bytes)) pldelay=-1;
  else pldelay-=bytes;

  //===========================================================
  WebRtcAecm_BufferFarend(aecmInst, p->playBuffer, 160);
  WebRtcAecm_BufferFarend(aecmInst, 160+(p->playBuffer), 160);

  //===========================================================

  (*p->bqPlayerBufferQueue)->Enqueue(p->bqPlayerBufferQueue,p->playBuffer,bytes);
}

// puts a buffer of size samples to the device
int android_AudioOut(OPENSL_STREAM *p, float *buffer,int size){

  short *outBuffer, *inBuffer;
  int i, bytes = size*sizeof(short);
  if(p == NULL  ||  p->outBufSamples ==  0)  return 0;
  for(i=0; i < size; i++){
    p->outputBuffer[i] = (short) (buffer[i]*CONV16BIT);
  }
  bytes = write_circular_buffer_bytes(p->outrb, (char *) p->outputBuffer,bytes);
  p->time += (double) size/(p->sr*p->outchannels);
  return bytes/sizeof(short);
}

circular_buffer* create_circular_buffer(int bytes){
  circular_buffer *p;
  if ((p = calloc(1, sizeof(circular_buffer))) == NULL) {
    return NULL;
  }
  p->size = bytes;
  p->wp = p->rp = 0;
   
  if ((p->buffer = calloc(bytes, sizeof(char))) == NULL) {
    free (p);
    return NULL;
  }
  return p;
}

int checkspace_circular_buffer(circular_buffer *p, int writeCheck){
  int wp = p->wp, rp = p->rp, size = p->size;
  if(writeCheck){
    if (wp > rp) return rp - wp + size - 1;
    else if (wp < rp) return rp - wp - 1;
    else return size - 1;
  }
  else {
    if (wp > rp) return wp - rp;
    else if (wp < rp) return wp - rp + size;
    else return 0;
  }	
}

int read_circular_buffer_bytes(circular_buffer *p, char *out, int bytes){
  int remaining;
  int bytesread, size = p->size;
  int i=0, rp = p->rp;
  char *buffer = p->buffer;
  if ((remaining = checkspace_circular_buffer(p, 0)) == 0) {
    return 0;
  }
  bytesread = bytes > remaining ? remaining : bytes;
  for(i=0; i < bytesread; i++){
    out[i] = buffer[rp++];
    if(rp == size) rp = 0;
  }
  p->rp = rp;
  return bytesread;
}

int write_circular_buffer_bytes(circular_buffer *p, const char *in, int bytes){
  int remaining;
  int byteswrite, size = p->size;
  int i=0, wp = p->wp;
  char *buffer = p->buffer;
  if ((remaining = checkspace_circular_buffer(p, 1)) == 0) {
    return 0;
  }
  byteswrite = bytes > remaining ? remaining : bytes;
  for(i=0; i < byteswrite; i++){
    buffer[wp++] = in[i];
    if(wp == size) wp = 0;
  }
  p->wp = wp;
  return byteswrite;
}

void
free_circular_buffer (circular_buffer *p){
  if(p == NULL) return;
  free(p->buffer);
  free(p);
}


//====================Torfone interface============================

//open audio device
int soundinit(char* device_in, char* device_out)
{
 //=============================================================
    AecmConfig config;
    int status;



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


 //============================================================
 op_stream = android_OpenAudioDevice(OPSR,1,1,CHSIZE);
 if(op_stream == NULL) return 0;

 return 1;
} 

//close audio device
void soundterm(void)
{
 if(op_stream != NULL) 
 {
  android_CloseAudioDevice(op_stream);
  op_stream = NULL;
 }

 if(aecmInst)
 {
  WebRtcAecm_Free(aecmInst);
  aecmInst=0;
 }

}

//record audio
int soundgrab(char *buf, int size)
{
  short *sbuf = (short*) buf;
  OPENSL_STREAM *p = op_stream;

  //------------------------------------
  short *inBuffer;
  int i, bytes = size*sizeof(short);
  if(p == NULL ||  p->inBufSamples ==  0) return 0;
  bytes = read_circular_buffer_bytes(p->inrb,(char *)p->inputBuffer,bytes);
  size = bytes/sizeof(short);
  
  if(IsGo)   
  for(i=0; i < size; i++) sbuf[i]=p->inputBuffer[i];
  else for(i=0; i < size; i++) sbuf[i]=0;
 
  return size; 
}

//grab at least len PCM samples to buf
//
int soundplay(int size, unsigned char *buf)
{
  short *sbuf = (short*) buf;
  OPENSL_STREAM *p = op_stream;
  
  short *outBuffer, *inBuffer;
  int i, bytes = size*sizeof(short);
  if(p == NULL  ||  p->outBufSamples ==  0)  return 0;
  
  //check for underrun was occured
  if(pldelay>=0) //no underrun
  {
   for(i=0; i < size; i++) p->outputBuffer[i] = sbuf[i]; //copy played samples to buffer
   bytes = write_circular_buffer_bytes(p->outrb, (char *) p->outputBuffer,bytes); //write samples to circurrar buffer
   p->time += (double) size/(p->sr*p->outchannels); //correct timestamp
   pldelay+=bytes; //add played samples to delay count
   return bytes/sizeof(short); //return number of actually played samples
  }
  else //resume underrun
  {
   size=CHSIZE; //bytes in one chunk
   bytes = size*sizeof(short);
   for(i=0; i < size; i++) p->outputBuffer[i] = 0; //set buffer to silency
   bytes = write_circular_buffer_bytes(p->outrb, (char *) p->outputBuffer,bytes); //write one chunk of silency to circullar buffer
   p->time += (double) size/(p->sr*p->outchannels); //correct timestamp
   pldelay=0; //reset playing delay
   return -1; //return -1 for indicate underrun resume
  }
}

//get number of unplayed PCM samples in the internal play buffer
int getdelay(void)
{
  signed int ret= pldelay/2;
  return ret;
}

//get number of size of one chunk (in PCM samples)
int getchunksize(void)
{
 return CHSIZE;
}

//get total size of buffers (in PCM samples)
int getbufsize(void)
{
 return BUFFERS*CHSIZE;
}

//flush output sound buffer
void soundflush(void)
{
 //not realized yet
}

//mute audio capturing (for secure reason)
int soundrec(int on)
{   
 IsGo=on;
 return IsGo;
}


//set audio output path (2-voice, 3-ring, default-media)
void sounddev(unsigned char dev)
{
 PlayPath=(int)dev; //up level stop audio before path change, path can be use in next start
}

#endif //#if defined(__ANDROID__)