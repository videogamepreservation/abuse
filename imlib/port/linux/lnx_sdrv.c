/*     Sound driver, (C) 1994 Jonathan Clark    */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/soundcard.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>

#define uchar unsigned char

#define BUF_BITS 9
#define BUF_SIZE (1<<BUF_BITS)
#define NUM_CHANNELS 8
#define SAMPLE_SPEED 11025


int s_dev,full_pot=0,start_seq;

int sound_init();
void output_samples(uchar *buffer, int size);
void sound_uninit();

#include "../sgi/gen_drv.c"         // include generic sound driver code, hackish I know, but I'm outa time :)


int sound_init() 
{  
  int i;  
  s_dev=open("/dev/dsp",O_WRONLY,0);
  if (s_dev<0)
  {
    fprintf(stderr,"SNDDRV : Unable to open /dev/dsp, sound effects disabled\n"
	           "   **Make sure no other process is using this (perhaps old lnx_sdrv?) **\n");    
    return 0;  
  }
  

  i = 0x00020000|BUF_BITS;	/* 2 fragments of 2^BUF_BITS bytes */
  ioctl(s_dev, SNDCTL_DSP_SETFRAGMENT, &i);

  i = 8;     // samples are 8 bit
  if (ioctl(s_dev, SNDCTL_DSP_SAMPLESIZE, &i)<0)
  {
    fprintf(stderr,"SNDDRV : Sample size 8 failed, sound effects disabled\n");    
    close(s_dev);
    s_dev=-1;    
    return 0;    
  }

  i = SAMPLE_SPEED; 
  if (ioctl(s_dev, SNDCTL_DSP_SPEED, &i)<0)
  {
    fprintf(stderr,"SNDDRV : dsp_speed failed, sound effects disabled\n");    
    close(s_dev);
    s_dev=-1;    
    return 0;    
  }

  i = 0;     // no stero
  if (ioctl(s_dev, SNDCTL_DSP_STEREO, &i)<0)
  {
    fprintf(stderr,"SNDDRV : Sample size 8 failed, sound effects disabled\n");    
    close(s_dev);
    s_dev=-1;    
    return 0;    
  }



  int j;
  uchar *vd=volume_table;
  for (;i<32;i++) 
  {
    for (j=0;j<256;j++,vd++)
      *vd=(j-128)*i/31+128;
  }
  return 1;

}


void output_samples(uchar *buffer, int size)
{
  write(s_dev, buf, size);
}


void sound_uninit()
{
  close(s_dev);
}
