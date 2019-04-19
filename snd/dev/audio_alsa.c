///////////////////////////////////////////////
//
// **************************
//
// Project/Software name: X-Phone
// Author: "Van Gegel" <gegelcopy@ukr.net>
//
// THIS IS A FREE SOFTWARE  AND FOR TEST ONLY!!!
// Please do not use it in the case of life and death
// This software is released under GNU LGPL:
//
// * LGPL 3.0 <http://www.gnu.org/licenses/lgpl.html>
//
// You’re free to copy, distribute and make commercial use
// of this software under the following conditions:
//
// * You have to cite the author (and copyright owner): Van Gegel
// * You have to provide a link to the author’s Homepage: <http://torfone.org>
//
///////////////////////////////////////////////

#ifndef _WIN32


#define WEBRTC_ECHO

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include "audio_alsa.h"
//#include "cntrls.h"
#define SAMPLE_RATE 	8000
#define DEFBUFSIZE 6400
#define DEFPERIODS 4

#define DEF_devAudioInput "plughw:0,0"
#define DEF_devAudioOutput "plughw:0,0"
#define DEF_devAudioControl "default"
#define DEF_capture_mixer_elem "Capture"
#define DEF_playback_mixer_elem "PCM"

static char devAudioInput[32];
static char devAudioOutput[32];
static char devAudioControl[32];
static char capture_mixer_elem[32];
static char playback_mixer_elem[32];

/* - setup - */
static int snd_rate = SAMPLE_RATE;
static int snd_format = SND_PCM_FORMAT_S16_LE;// SND_PCM_FORMAT_MU_LAW;
static int snd_channels = 1;
static int verbose = 0;		/* DEBUG! */
static int quiet_mode = 0;	/* Show info when suspending.  Not relevant as
				   this application doesn't suspend. */

static unsigned buffer_time = 300 * 1000;	/* Total size of buffer: 900 ms */
static unsigned period_time = 5 * 1000;	/* a.k.a. fragment size: 30 ms */
static unsigned bufsize = DEFBUFSIZE;
static unsigned periods = DEFPERIODS;
static int showparam=0;

static int sleep_min = 0;
static int avail_min = -1;
static int start_delay = 0;
static int stop_delay = 0;

/* - record/playback - */
static snd_pcm_t *pcm_handle_in = NULL; //record
static snd_pcm_t *pcm_handle_out = NULL; //playback
static snd_pcm_uframes_t chunk_size, buffer_size;
static size_t bits_per_sample, bits_per_frame;
static size_t chunk_bytes;

/* - mixer - */
static int mixer_failed = 0;
static snd_mixer_t *mixer = NULL;
static snd_mixer_elem_t *capture_elem = NULL;
static snd_mixer_elem_t *playback_elem = NULL;
static int mixer_capture_failed=0, mixer_playback_failed=0;
static long reclevel_min, reclevel_max;
static long playlevel_min, playlevel_max;

/* - misc - */
static snd_output_t *tlog;

//VAD
int Vad_c=0;

int IsGo=0;     //flag: input runs
int PlayPath=0; //play path: not used yet

#define MAX_AUD_LEN 160  //frame for AMR codec
static short aud_buf[MAX_AUD_LEN]; //audio buffer
short aud_ptr=0; //number of samples in buffer 


#ifdef WEBRTC_ECHO
#include "aecm.h"
#define MAX_RBUF 2560 //160*16
short echobuf[160];
short outbuf[160];
short rbuf[MAX_RBUF];
short in_rptr=0;
short out_rptr=0;
void *aecmInst = 0;    //aec instance pointer
#endif


//read specified alsa buffer parameters from config file
static int rdcfg(char* d_in, char* d_out)
{
 char buf[256];
 char* p=NULL;
 
 //set defaults
 periods=DEFPERIODS;
 bufsize=DEFBUFSIZE;
 strcpy(devAudioInput,DEF_devAudioInput);
 strcpy(devAudioOutput,DEF_devAudioOutput);
 strcpy(devAudioControl,DEF_devAudioControl);
 strcpy(capture_mixer_elem,DEF_capture_mixer_elem);
 strcpy(playback_mixer_elem,DEF_playback_mixer_elem);

 //read from config file:

 //chunk size and cunks in buffer
 strcpy(buf, "AudioChunks");

 // if(0>=parseconf(buf)) strcpy(buf, "#");
 strcpy(buf, "default");

 p=strchr(buf, '*');
 if((buf[0]=='#')||(!p)) showparam=1;
 else
 { 
   showparam=0;
   periods=0;
   p[0]=0;
   periods=atoi(++p);
   printf("PPPeriods=%u\r\n", periods);
   if(!periods) periods=DEFPERIODS;
   bufsize=periods*atoi(buf);
   printf("BBBufsize=%u\r\n", bufsize);
   if(!bufsize) bufsize=DEFBUFSIZE; 
 }
 
 // printf("RDCFG: %s %s\r\n", d_in, d_out);


//device names
 strcpy(buf, "AudioInput");

 //if(0>=parseconf(buf)) strcpy(buf, "#");
 if(d_in[0]) strcpy(buf, d_in);
 else strcpy(buf, "plug:default");

 if(buf[0]!='#') strcpy(devAudioInput,buf);
 
 strcpy(buf, "AudioOutput");

 //if(0>=parseconf(buf)) strcpy(buf, "#");
 if(d_out[0]) strcpy(buf, d_out);
 else strcpy(buf, "plug:default");

 if(buf[0]!='#') strcpy(devAudioOutput,buf);
 
 strcpy(buf, "AudioControl");
// if(0>=parseconf(buf))
 strcpy(buf, "#");
 if(buf[0]!='#') strcpy(devAudioControl,buf);
 
 strcpy(buf, "CaptureMixer");
// if(0>=parseconf(buf)) 
strcpy(buf, "#");
 if(buf[0]!='#') strcpy(capture_mixer_elem, buf);

 strcpy(buf, "PlaybackMixer");
// if(0>=parseconf(buf)) 
strcpy(buf, "#");
 if(buf[0]!='#') strcpy(playback_mixer_elem, buf);
 
 printf("AUDIO: %s, %s  per=%d, buf=%d\r\n",devAudioInput, devAudioOutput, periods, bufsize);

 return 0; 
}

//returns chunk size
int getchunksize(void)
{
 return (int) chunk_size;
}

//returns buffer size
int getbufsize(void)
{
 return (int) buffer_size;
}

/*
//returns current count of samples in output buffers 
int getdelay(void)
{
 int rc;
 snd_pcm_sframes_t frames;
 rc=snd_pcm_delay(pcm_handle_out, &frames);
 if(rc) 
 {
  fprintf(stderr, "GetDelay error=%d", rc);
  return 0;
 }
 return frames;
}
*/

int getdelay(void)
{
 int i=buffer_size;
 int j=snd_pcm_avail_update(pcm_handle_out);
 if(j<0) j=buffer_size;
 return (i-j);
}



/*
 * Open the sound peripheral and initialise for access.  Returns TRUE if
 * successful, FALSE otherwise.
 *
 * iomode: O_RDONLY (sfmike) or O_WRONLY (sfspeaker).
 */
static int soundinit_2(int iomode)
{
	snd_pcm_stream_t stream;
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_uframes_t xfer_align, start_threshold, stop_threshold;
	size_t n;
	char *device_file;
	
	snd_pcm_t *pcm_handle=NULL;
        
        int ret;

        //apply users config
      //  rdcfg();

	// might be used in case of error even without verbose.
	snd_output_stdio_attach(&tlog, stderr, 0);

	if (iomode != 0) {
		stream = SND_PCM_STREAM_CAPTURE;
		start_delay = 1;
		device_file = devAudioInput;
		
	} else {
		stream = SND_PCM_STREAM_PLAYBACK;
		device_file = devAudioOutput;
		
	}

	if (snd_pcm_open(&pcm_handle, device_file, stream, SND_PCM_NONBLOCK) < 0) {
		fprintf(stderr, "Error opening PCM device %s\n", device_file);
		return FALSE;
	}

	snd_pcm_hw_params_alloca(&hwparams);
	if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
		fprintf(stderr, "Can't configure the PCM device %s\n",
			devAudioInput);
		return FALSE;
	}


	/* now try to configure the device's hardware parameters */
	if (snd_pcm_hw_params_set_access(pcm_handle, hwparams,
		SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		fprintf(stderr, "Error setting interleaved access mode.\n");
		return FALSE;
	}

	/* Here we request mu-law sound format.  ALSA can handle the
	 * conversion to linear PCM internally, if the device used is a
	 * "plughw" and not "hw". */
	if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, snd_format) < 0) {
		fprintf(stderr, "Error setting PCM format\n");
		return FALSE;
	}

	if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, snd_channels) < 0) {
		fprintf(stderr, "Error setting channels to %d\n",
			snd_channels);
		return FALSE;
	}

	if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, (unsigned int*)&snd_rate, NULL) < 0) {
		fprintf(stderr, "The rate %d Hz is not supported.  "
			"Try a plughw device.\n", snd_rate);
		return FALSE;
	}

// buffer and period size can be set in bytes, or also in time.  


	if (snd_pcm_hw_params_set_buffer_time_near(pcm_handle, hwparams, &buffer_time, 0) < 0) {
		fprintf(stderr, "Error setting buffer time to %u\n", buffer_time);
		return FALSE;
	}

	if (snd_pcm_hw_params_set_period_time_near(pcm_handle, hwparams, &period_time, 0) < 0) {
		fprintf(stderr, "Error setting period time to %u\n", period_time);
		return FALSE;
	}

 if(!showparam) //read config file
 {      //sets specified buffer parameters
	if (snd_pcm_hw_params_set_buffer_size(pcm_handle, hwparams, bufsize) < 0) {
		fprintf(stderr, "Error setting buffer size to %u\n", bufsize);
		return FALSE;
	}
	if (snd_pcm_hw_params_set_periods(pcm_handle, hwparams, periods, 0) < 0) {
		fprintf(stderr, "Error setting periods.\n");
		return FALSE;
	}
 }
 
	if ((ret=snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
		fprintf(stderr, "Error setting hardware parameters=%d\n", ret);
		return FALSE;
	}

	/* check the hw setup */
	snd_pcm_hw_params_get_period_size(hwparams, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
	if (chunk_size == buffer_size) {
		fprintf(stderr, "Can't use period equal to buffer size (%lu)\n",
			chunk_size);
		return FALSE;
	}
	if (verbose||showparam)
		printf("Period size %lu, Buffer size %lu\n\r",
			chunk_size, buffer_size);

	/* now the software setup */
	/* This is from aplay, and I don't really understand what it's good
	 * for... */
	snd_pcm_sw_params_alloca(&swparams);
	snd_pcm_sw_params_current(pcm_handle, swparams);

	if (snd_pcm_sw_params_get_xfer_align(swparams, &xfer_align) < 0) {
		fprintf(stderr, "Unable to obtain xfer align\n");
		return FALSE;
	}
	if (sleep_min)
		xfer_align = 1;
	if (snd_pcm_sw_params_set_sleep_min(pcm_handle, swparams, sleep_min) < 0) {
		fprintf(stderr, "Unable to set sleep_min to %d\n", sleep_min);
		return FALSE;
	}

	if (avail_min < 0)
		n = chunk_size;
	else
		n = (double) snd_rate * avail_min / 1000000;
	if (snd_pcm_sw_params_set_avail_min(pcm_handle, swparams, n) < 0) {
		fprintf(stderr, "Can't set avail_min to %ju\n", n);
		return FALSE;
	}

	/* round up to closest transfer boundary */
	n = (buffer_size / xfer_align) * xfer_align;
	if (start_delay <= 0) {
		start_threshold = n + (double) snd_rate * start_delay / 1000000;
	} else
		start_threshold = (double) snd_rate * start_delay / 1000000;
	if (start_threshold < 1)
		start_threshold = 1;
	if (start_threshold > n)
		start_threshold = n;
	if (verbose)
		printf("Start threshold would be %lu\n\r", start_threshold);
#if 0
	// NOTE: this makes playback not work.  Dunno why.
	if (snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, start_threshold) < 0) {
		fprintf(stderr, "Can't set start threshold\n");
		return FALSE;
	}
#endif

	if (stop_delay <= 0) 
		stop_threshold = buffer_size + (double) snd_rate * stop_delay / 1000000;
	else
		stop_threshold = (double) snd_rate * stop_delay / 1000000;
	if (snd_pcm_sw_params_set_stop_threshold(pcm_handle, swparams, stop_threshold) < 0) {
		fprintf(stderr, "Can't set stop threshold\n");
		return FALSE;
	}

	if (snd_pcm_sw_params_set_xfer_align(pcm_handle, swparams, xfer_align) < 0) {
		fprintf(stderr, "Can't set xfer align\n");
		return FALSE;
	}

	if (snd_pcm_sw_params(pcm_handle, swparams) < 0) {
		fprintf(stderr, "unable to install sw params:\n");
		snd_pcm_sw_params_dump(swparams, tlog);
		return FALSE;
	}


	/* ready to enter the SND_PCM_STATE_PREPARED status */
	if (snd_pcm_prepare(pcm_handle) < 0) {
		fprintf(stderr, "Can't enter prepared state\n");
		return FALSE;
	}

	if (verbose)
		snd_pcm_dump(pcm_handle, tlog);

	bits_per_sample = snd_pcm_format_physical_width(snd_format);
	bits_per_frame = bits_per_sample * snd_channels;
	chunk_bytes = chunk_size * bits_per_frame / 8;

	if (verbose)
		printf("Audio buffer size should be %ju bytes\n\r", chunk_bytes);

//	audiobuf = realloc(audiobuf, chunk_bytes);
//	if (audiobuf == NULL) {
//		error("not enough memory");
//		exit(EXIT_FAILURE);
//	}

        
        if (iomode != 0) {
		pcm_handle_in=pcm_handle;
	} else {
		pcm_handle_out=pcm_handle;
	}


	return TRUE;
}




/* I/O error handler */
/* grabbed from alsa-utils-0.9.0rc5/aplay/aplay.c */
/* what: string "overrun" or "underrun", just for user information */
static void xrun(char *what, snd_pcm_t *pcm_handle)
{
	(void)what;

	snd_pcm_status_t *status;
	int res;
	
	snd_pcm_status_alloca(&status);
	if ((res = snd_pcm_status(pcm_handle, status))<0) {
		fprintf(stderr, "status error: %s\n", snd_strerror(res));
		return;		
		//exit(EXIT_FAILURE);
	}
	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
		struct timeval now, diff, tstamp;
		gettimeofday(&now, 0);
		snd_pcm_status_get_trigger_tstamp(status, &tstamp);
		timersub(&now, &tstamp, &diff);
 
		//fprintf(stderr, "Buffer %s!!! (at least %.3f ms long)\n",
		//	what,
		//	diff.tv_sec * 1000 + diff.tv_usec / 1000.0);

		if (verbose) {

			fprintf(stderr, "Status:\n");
			snd_pcm_status_dump(status, tlog);
		}
                //if(pcm_handle_out==pcm_handle) snd_pcm_drop(pcm_handle_out);


		if ((res = snd_pcm_prepare(pcm_handle))<0) {
			fprintf(stderr, "xrun: prepare error: %s\n",
				snd_strerror(res));
				return;
			//exit(EXIT_FAILURE);
		}
		return;		/* ok, data should be accepted again */
	}
	fprintf(stderr, "read/write error\n");
	//exit(EXIT_FAILURE);
}


/* Helper function.  If the sound setup fails, release the device, because if
 * we try to open it twice, the application will block */
int soundinit(char* device_in, char* device_out)
{
    int rc=1;
    
    #ifdef WEBRTC_ECHO
    
    AecmConfig config;
    int status;
    
    config.cngMode = AecmTrue;
    config.echoMode = 1;
    aecmInst = WebRtcAecm_Create();
    if(!aecmInst)
    {
		printf("cannot create aec!\r\n");
		return 0;		
	} 
	
	status = WebRtcAecm_Init(aecmInst, 8000);
	if(status !=0)
	{
		printf("aec init fail!\r\n");
		WebRtcAecm_Free(aecmInst);
		return 0;	
	} 
	
	status = WebRtcAecm_set_config(aecmInst, config);
	if(status !=0)
	{
		printf("aec config fail!\r\n");
		WebRtcAecm_Free(aecmInst);
		return 0;	
	}  
    
    printf("aec creare success!\r\n");
 #endif
    
    
    
    
    
    printf("ALSA in: %s, ALSA out: %s\r\n", device_in, device_out);	
	
#ifdef WEBRTC_ECHO
  in_rptr=0;
  out_rptr=0;
#endif
	
    rdcfg(device_in, device_out);

    //Open ALSA
    if(!soundinit_2(1)) //record
    {
     printf("Record not avaloable\r\n");
     rc=0;
    }
    if(!soundinit_2(0)) //playback
    {
     printf("Playback not avaliable\r\n");
     rc=0;
    }
    if (rc != TRUE && (pcm_handle_in || pcm_handle_out)) 
    {	
     soundterm();
    }
    else
    {  //fill playing buffer with silency for run playing by trashold
      short tpcm[2500]={0,};
      soundplay(2500, tpcm);
    }
    return rc;
}

/* Helper function.  If the sound setup fails, release the device, because if
 * we try to open it twice, the application will block */
int soundinit4(char* device_in, char* device_out)
{
    int rc;
      
    //apply users config
    rdcfg(device_in, device_out);

	//list audio devices
    rc=system("aplay --list-devices");
	
    printf("\r\n------------Initialise Headset audio----------\r\n");
    //Open ALSA
    rc=1;
    printf("Audio input: ");
    if(!soundinit_2(1)) //record
    {
     printf("Mike not found\r\n");
     if(pcm_handle_in) snd_pcm_close(pcm_handle_in);
     pcm_handle_in = NULL; 
     rc=0;
    }
    
	printf("Audio output: ");
	if(!soundinit_2(0)) //playback
    {
     printf("Speaker not found\r\n");
     rc=0;
     if(pcm_handle_out) snd_pcm_close(pcm_handle_out);
     pcm_handle_out = NULL;
    }
    

    {  //fill playing buffer with silency for run playing by trashold
      short tpcm[2500]={0,};
      soundplay(2500, tpcm);
    }

    if (rc != TRUE && (pcm_handle_in || pcm_handle_out)) 
    {	
     soundterm();
    }
    else
    {  //fill playing buffer with silency for run playing by trashold
      short tpcm[2500]={0,};
      soundplay(2500, tpcm);
    }
    return rc;
    //return 1; //software can be run without additional Headset audio HW in test mode
}

/* Close the audio device and the mixer */
void soundterm(void)
{
	#ifdef WEBRTC_ECHO
	if(aecmInst)
	{
		WebRtcAecm_Free(aecmInst);
		aecmInst=0;
	}	
	#endif
	
	if(pcm_handle_in) snd_pcm_close(pcm_handle_in);
	pcm_handle_in = NULL;
	if(pcm_handle_out) snd_pcm_close(pcm_handle_out);
	pcm_handle_out = NULL;
	if (mixer) {
		snd_mixer_close(mixer);
		mixer = NULL;
	}
}

/* This is a hack to open the audio device once with O_RDWR, and then give the
 * descriptors to sfmike/sfspeaker.  Use this when opening the same device
 * twice, once read only and once write only, doesn't work.  AFAIK, not an
 * issue with ALSA.
 */
void sound_open_file_descriptors(int *audio_io, int *audio_ctl)
{
	(void)audio_io;
	(void)audio_ctl;
	return;
}

/* Input suspend handler */
/* grabbed from alsa-utils-0.9.0rc5/aplay/aplay.c */
static void suspend(void)
{
	int res;
    snd_pcm_t *pcm_handle=pcm_handle_in;
    
	if (!quiet_mode)
		fprintf(stderr, "Suspended. Trying resume. "); fflush(stderr);
	while ((res = snd_pcm_resume(pcm_handle)) == -EAGAIN)
		sleep(1);	/* wait until suspend flag is released */
	if (res < 0) {
		if (!quiet_mode)
			fprintf(stderr, "Failed. Restarting stream. "); fflush(stderr);
		if ((res = snd_pcm_prepare(pcm_handle)) < 0) {
			fprintf(stderr, "suspend: prepare error: %s\n", snd_strerror(res));
			//exit(EXIT_FAILURE);
		}
	}
	if (!quiet_mode)
		fprintf(stderr, "Done.\n");
}


/*
 * Play a sound (update playbuffer asynchronously).  Buf contains ULAW encoded audio, 8000 Hz, mono, signed bytes
 * grabbed from alsa-utils-0.9.0rc5/aplay/aplay.c
 *
 * buf: where the samples are, if stereo then interleaved
 * len: number of samples (not bytes).  Doesn't matter for mono 8 bit.
 */
int soundplay(int len, unsigned char *buf)
{
	int rc;
#ifdef WEBRTC_ECHO 
int i, n;
#endif	
	
	
  if(!pcm_handle_out) return 0; //no Speaker HW

  
  
#ifdef WEBRTC_ECHO  
  //=======================================================
  //-------add samples to ring buffer for use in echo cancellator----
  //=======================================================
  n=len; //number of samples will be played
  i=MAX_RBUF-in_rptr; //free space from inptr to end of ring buffer
  if(i>n) i=n; //number of samples will be copy after inptr
  memcpy(rbuf+in_rptr, buf, 2*i); //copy samples after ptr
  in_rptr+=i; //move inptr;
  n-=i; //number of samples left uncopied
  if(n>0)
  {
   memcpy(rbuf, buf+i, 2*n); //copy tail to start of ring buffer
   in_rptr=n; //ring input pointer
  }
  if(in_rptr>=MAX_RBUF) in_rptr=0;
  //=======================================================
#endif  
  
  
    snd_pcm_t *pcm_handle=pcm_handle_out;
	/* the function expects the number of frames, which is equal to bytes
	 * in this case */
	
		rc = snd_pcm_writei(pcm_handle, buf, len);
	
		
		if (rc == -EAGAIN || (rc >= 0 && rc < len)) {
			//fprintf(stderr, "Playback uncompleet: %d form %d\n", rc, len);
			//snd_pcm_wait(pcm_handle, 1000);
		} else if (rc == -EPIPE) {
			 //Experimental: when a buffer underrun happens, then
			 //wait some extra time for more data to arrive on the
			 //network.  The one skip will be longer, but less
			 //buffer underruns will happen later.  Or so he
			 //thought... 
			//usleep(100000);
                       // fprintf(stderr, "underrun\n");
            
#ifdef WEBRTC_ECHO
  in_rptr=0;
  out_rptr=0;
#endif

            
			xrun("underrun", pcm_handle);
                       
		} else if (rc == -ESTRPIPE) {
                        fprintf(stderr, "suspend\n");
			//suspend();
		} else if (rc < 0) {
			fprintf(stderr, "Write error: %s\n", snd_strerror(rc));
			return -EIO;
		}

	     return rc;
}

/* Try to open the mixer */
/* returns FALSE on failure */
static int mixer_open_2()
{
	int rc;

	if ((rc=snd_mixer_open(&mixer, 0)) < 0) {
		fprintf(stderr, "Can't open mixer: %s\n", snd_strerror(rc));
		return FALSE;
	}

	if ((rc=snd_mixer_attach(mixer, devAudioControl)) < 0) {
		fprintf(stderr, "Mixer attach error to %s: %s\n",
			devAudioControl, snd_strerror(rc));
		return FALSE;
	}

	if ((rc=snd_mixer_selem_register(mixer, NULL, NULL)) < 0) {
		fprintf(stderr, "Mixer register error: %s\n", snd_strerror(rc));
		return FALSE;
	}

	if ((rc=snd_mixer_load(mixer)) < 0) {
		fprintf(stderr, "Mixer load error: %s\n", snd_strerror(rc));
		return FALSE;
	}

	return TRUE;
}

static snd_mixer_elem_t *get_mixer_elem(char *name, int index)
{
	snd_mixer_selem_id_t *sid;
	snd_mixer_elem_t *elem;

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, index);
	snd_mixer_selem_id_set_name(sid, name);

	elem= snd_mixer_find_selem(mixer, sid);
	if (!elem) {
		fprintf(stderr, "Control '%s',%d not found.\n",
			snd_mixer_selem_id_get_name(sid),
			snd_mixer_selem_id_get_index(sid));
	}
	return elem;
}


/* Try to open the mixer, but only once. */
static int mixer_open()
{
	if (mixer_failed)
		return FALSE;

	if (mixer)
		return TRUE;

	if (mixer_open_2() == TRUE)
		return TRUE;

	if (mixer) {
		snd_mixer_close(mixer);
		mixer = NULL;
	}
	mixer_failed ++;
	return FALSE;
}

/* Set the playback volume from 0 (silence) to 100 (full on). */
void soundplayvol(int value)
{
	long vol;
   

	if (mixer_open() != TRUE)
		return;
	if (mixer_playback_failed)
		return;
	if (!playback_elem) {
		playback_elem = get_mixer_elem(playback_mixer_elem, 0);
		if (!playback_elem) {
			mixer_playback_failed = 1;
			return;
		}
		snd_mixer_selem_get_playback_volume_range(playback_elem,
			&playlevel_min, &playlevel_max);
	}

	vol = playlevel_min + (playlevel_max - playlevel_min) * value / 100;
	snd_mixer_selem_set_playback_volume(playback_elem, 0, vol);
	snd_mixer_selem_set_playback_volume(playback_elem, 1, vol);
}


/* Set recording gain from 0 (minimum) to 100 (maximum). */
void soundrecgain(int value)
{
	long vol;

	if (mixer_open() != TRUE)
		return;
	if (mixer_capture_failed)
		return;
	if (!capture_elem) {
		capture_elem = get_mixer_elem(capture_mixer_elem, 0);
		if (!capture_elem) {
			mixer_capture_failed = 1;
			return;
		}
		snd_mixer_selem_get_capture_volume_range(capture_elem,
			&reclevel_min, &reclevel_max);
	}

	// maybe unmute, or enable "rec" switch and so forth...
	vol = reclevel_min + (reclevel_max - reclevel_min) * value / 100;
	snd_mixer_selem_set_capture_volume(capture_elem, 0, vol);
	snd_mixer_selem_set_capture_volume(capture_elem, 1, vol);
}

/* select the output - speaker, or audio output jack.  Not implemented */
void sounddest(int where)
{
	(void)where;

	return;
}





/* Record some audio non-blocking (as much as accessable and fits into the given buffer) */
int soundgrab1(char *buf, int len)
{
    size_t result = 0;
    if(IsGo) {
        ssize_t r;
        //size_t count = len;
        snd_pcm_t *pcm_handle=pcm_handle_in;
   

/*  I don't care about the chunk size.  We just read as much as we need here.
 *  Seems to work.
        if (sleep_min == 0 && count != chunk_size) {
            fprintf(stderr, "Chunk size should be %lu, not %d\n\r",
                chunk_size, count);
		count = chunk_size;
        }
*/

	// Seems not to be required.
        //int rc = snd_pcm_state(pcm_handle_in);

        //if (rc == SND_PCM_STATE_PREPARED)
        //snd_pcm_start(pcm_handle);
        

       // do {
		      r = snd_pcm_readi(pcm_handle, buf, len);
	//    } while (r == -EAGAIN);
	

        if(r == -EAGAIN) return 0;
	    if(r>0) result=r;
	    
        if (r == -EPIPE) {
			xrun("overrun", pcm_handle);
		} else if (r == -ESTRPIPE) {
                        fprintf(stderr, "suspend");
			suspend();
		} else if (r < 0) {
			if (r == -4) {
				/* This is "interrupted system call, which
				 * seems to happen quite frequently, but does
				 * no harm.  So, just return whatever has been
				 * already read. */
			}
			fprintf(stderr, "read error: %s (%ju); state=%d\n\r",
				snd_strerror(r), r, snd_pcm_state(pcm_handle));
		}
    }
    return result;
}


int soundgrab(char *buf, int len)
{
 int i;

 #ifdef WEBRTC_ECHO
 int j,k;
 #endif
 

  if((!pcm_handle_in)||(!IsGo)) return 0; //No Mike HW

 i=soundgrab1(aud_buf+aud_ptr, len-aud_ptr);
 if((i>0)&&(i<=MAX_AUD_LEN)) aud_ptr+=i;
 i=0;
 if(aud_ptr>=len) 
 {
  #ifdef WEBRTC_ECHO 
  //===========================================================
  
  
  k=in_rptr-out_rptr;//unused samples in echo buffer
  if(k<0) k+=MAX_RBUF; //ring
  if(k>=160) //have a frame of unsused samples
  {
   j=MAX_RBUF-out_rptr; //number of samples after out_ptr
   if(j>160) j=160; //whole frame or part only
   memcpy(echobuf, rbuf+out_rptr, 2*j); //copy to echobuffer
   out_rptr+=160; //move out_ptr
   if(j<160) //if only part of frame was copied
   {
    memcpy(echobuf+j, rbuf, 2*(160-j)); //copy tail to end of echobuffer
    out_rptr-=MAX_RBUF; //ring pointer
   }
   if(out_rptr>=MAX_RBUF) out_rptr=0;
   
   
   //use 160 samples frame in echobuf for updating farend echo
  WebRtcAecm_BufferFarend(aecmInst, echobuf, 160);
   //for test HERE we copy 160 samples from echobuf to aud_buf
   ////memcpy(aud_buf, echobuf, 2*160); //reflection test
  }
  
  //process nearend 160 samples frame in aud_buf to echobuf
  WebRtcAecm_Process(aecmInst, aud_buf, NULL, outbuf, 160, 80);
  memcpy(aud_buf, outbuf, 320);
  //=============================================
  #endif 
   
  memcpy(buf, aud_buf, len*sizeof(short));
  
  
  ////////////aud_ptr=0;
  aud_ptr-=len; //len of tail in grab collector
  if(aud_ptr) memcpy(aud_buf, aud_buf+len, 2*aud_ptr); //copy tail to start of grab collector
  
  
  i=len; 
 }
 return i;
}


/* This is called *before* starting to record.  Flush pending input.*/

void soundflush(void)
{
//	printf("SOUND FLUSH\n\r");
        snd_pcm_drop(pcm_handle_in);	/* this call makes the state go to "SETUP" */
	snd_pcm_prepare(pcm_handle_in);	/* and now go to "PREPARE" state, ready for "RUNNING" */                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
	
}

void soundflush1(void)
{
//	printf("SOUND FLUSH\n\r");
        snd_pcm_drop(pcm_handle_out);	/* this call makes the state go to "SETUP" */
	snd_pcm_prepare(pcm_handle_out);	/* and now go to "PREPARE" state, ready for "RUNNING" */                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
	
}


int soundrec(int on)
{   
 IsGo=on;
 return IsGo;
}


void sounddev(unsigned char dev)
{
	PlayPath=(int)dev;	
}


#endif //#ifndef _WIN32
