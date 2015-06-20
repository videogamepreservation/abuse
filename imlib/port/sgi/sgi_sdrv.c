#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include <dmedia/audio.h>
#include <dmedia/audiofile.h>
}
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <bstring.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

void output_sample(unsigned char *buf, int size);
void sound_uninit();
int sound_init();

#define BUF_SIZE 512

ALconfig audioconfig;
ALport audioport;




void output_samples(unsigned char *buf, int size)
{
  ALwritesamps(audioport,buf,size);
}

#include "gen_drv.c"



void sound_uninit()
{
  ALfreeconfig(audioconfig);
  ALcloseport(audioport);
}

int sound_init()
{
  long pvbuf[4];
  pvbuf[0] = AL_OUTPUT_COUNT;
  pvbuf[2] = AL_MONITOR_CTL;
  if (ALgetparams(AL_DEFAULT_DEVICE, pvbuf, 4) < 0) 
  {    
    fprintf(stderr,"sound driver : No audio hardware\n");
    return 0;
  }
  pvbuf[0] = AL_OUTPUT_RATE;
  pvbuf[1] = 11025;
  if (ALsetparams(AL_DEFAULT_DEVICE,pvbuf,4) < 0)
  {    
    fprintf(stderr,"sound driver : Could not set sample rate\n");
    return 0;
  }

  ALseterrorhandler(0);
  audioconfig = ALnewconfig();
  if (!audioconfig)
  {
    fprintf(stderr,"failed to create audio config\n");
    return 0;
  }
  else if (ALsetchannels(audioconfig,AL_MONO))
  { fprintf(stderr,"sound driver : could not set audio channels\n");
    return 0;
  }
  else if (ALsetqueuesize(audioconfig,BUF_SIZE))
  { 
    fprintf(stderr,"sound driver : could not set audio que size\n");
    ALfreeconfig(audioconfig);
    return 0;
  } else if (ALsetwidth (audioconfig, AL_SAMPLE_8))
  {
    fprintf(stderr,"sound driver :could not set 8 bit samples\n");
    ALfreeconfig(audioconfig);
    return 0;
  }

  audioport=ALopenport("Abuse sound driver","w",audioconfig);
  if (!audioport)
  {
    fprintf(stderr,"sound driver : could not open audio port\n");
    ALfreeconfig(audioconfig);
    return 0;
  }

  int i=0,j;
  uchar *vd=volume_table;
  for (;i<32;i++) 
  {
    for (j=0;j<256;j++,vd++)
      *vd=(j-128)*i/31+128;
  }
  return 1;
}


