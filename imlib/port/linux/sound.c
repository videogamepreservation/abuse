#include "sound.hpp"
#include "readwav.hpp"
#include "specs.hpp"
#include "sdriver.hpp"
#include "jmalloc.hpp"
#include "specs.hpp"
#include "macs.hpp"
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
#include "dprint.hpp"
#include <fcntl.h>
#include <linux/limits.h>


int sound_child_fd=-1,
    sound_parent_fd=-1,
    sound_status[2],
    sound_child_pid;


#define BUF_BITS 9
#define BUF_SIZE (1<<BUF_BITS)
#define NUM_CHANNELS 8
#define SAMPLE_SPEED 11025

int s_dev,full_pot=0,start_seq;


unsigned char buf[BUF_SIZE];   // mixing buffer


struct sfx
{
  unsigned char *data;
  long size;  
} ;
 
struct channel
{
  unsigned char *data;                  // next data to be qued
  long left;                   // how much is left to play?  
  unsigned char volume_shift;  
  unsigned long add_time;      // time at which the channel started playing  
} channels[NUM_CHANNELS];





sfx *seffects=NULL;
short total_effects=0;

void sound_watch(int f);

// add a new sound effect into the channel list, if no free channel
// oldest gets replaced with new sound
void play(short id, unsigned char volume)
{
  int i,free_channel=-1;
  unsigned long last_channel_time=0;  
  for (i=0;i<NUM_CHANNELS;i++)
  {
    if (channels[i].data)
    {      
      if (channels[i].add_time>last_channel_time)
      {	
        last_channel_time=channels[i].add_time;
	if (free_channel==-1)
  	  free_channel=i;	
      }      
    }    
    else free_channel=i;        
  }

  channels[free_channel].add_time=last_channel_time+1;
  channels[free_channel].data=seffects[id].data+1024;
  channels[free_channel].left=seffects[id].size-1024;
  channels[free_channel].volume_shift=(127-volume)>>4;
}

int output_sounds()  // return 0 if no sounds to ouput
{
  unsigned char *s;
  int i,j,bytes=0;  

//  usleep((BUF_SIZE*90000/SAMPLE_SPEED)); // sleep long enough for old sample to alomst finish
  
  
  signed short sums[BUF_SIZE],run_size,*sp;
  memset(sums,0,BUF_SIZE*sizeof(short));
  for (j=0;j<NUM_CHANNELS;j++)
  {
    if (channels[j].data)
    {
      if (channels[j].left<BUF_SIZE)    // hom many bytes will this channel run for?
        run_size=channels[j].left;
      else run_size=BUF_SIZE;

      if (run_size>bytes)          // how many bytes to output?
        bytes=run_size;
      
      // add the chanels together into a short
      for (i=0,sp=sums;i<run_size;i++,sp++)
        *sp+=(((signed short)*(channels[j].data++))-128)>>channels[j].volume_shift;

      if (channels[j].left<=BUF_SIZE)
        channels[j].data=NULL;
      else channels[j].left-=BUF_SIZE;
      
    }
  }



    
  for (i=0,sp=sums,s=buf;i<BUF_SIZE;i++,s++,sp++)
    if (*sp<-128) *s=0;
    else if (*sp>127) *s=255;
    else *s=(unsigned char)(*sp+128);


  write(s_dev, buf, BUF_SIZE);
  return 1;    // always say we have something, we will do blanks if nothing else
  
}


int sound_init(int argc, char **argv) 
{  
  int i;  
  char buf[PIPE_BUF];
  for (i=1;i<argc;i++)
  {
    if (!strcmp(argv[i],"-nosound"))
      return 0;
  }

  if (sound_child_fd!=-1)
  {
   fprintf(stderr,"Sound already initialized\n");
    return 1;
  }

  int fds[2];
  if (pipe(fds)==-1) 
  {
    fprintf(stderr,"error creating pipe for sound driver\n");
    exit(1);
  } 

  if (pipe(sound_status)==-1) 
  {
    fprintf(stderr,"error creating pipe for sound driver\n");
    exit(1);
  } 


  sound_child_pid=fork();
  if (sound_child_pid)                // if we are the parent, then check that status from the child
  {

    char status;
    read(fds[0],buf,PIPE_BUF);
    if (buf[0])
    {
      sound_child_fd=fds[0];
      sound_parent_fd=fds[1];
      atexit(sound_uninit);          // make sure the child dies when the program ends
      return SFX_INITIALIZED;
    } else 
    {
      dprintf("sound : child returned init failure\n");
      return 0;      
    }
  } else
  {
    int fd=open("/dev/mixer",O_WRONLY);
    if (fd!=-1)
    {
      int vol=127;
      ioctl(fd,MIXER_WRITE(SOUND_MIXER_VOLUME),&vol);
      close(fd);
    } else fprintf(stderr,"sound driver : Unable to open /dev/mixer, can't set volume\n");

    
    buf[0]=0;                             // get ready to send failure
    s_dev=open("/dev/dsp",O_WRONLY,0);
    if (s_dev<0)
    {
      fprintf(stderr,"sound driver : Unable to open /dev/dsp, sound effects disabled\n");    
      write(fds[1],buf,PIPE_BUF);
      exit(1);
    }
    dprintf("sound driver : opened /dev/dsp\n");
    i = 0x00020000|BUF_BITS;	/* 2 fragments of 2^BUF_BITS bytes */
    ioctl(s_dev, SNDCTL_DSP_SETFRAGMENT, &i);

    i = 8;     // samples are 8 bit
    if (ioctl(s_dev, SNDCTL_DSP_SAMPLESIZE, &i)<0)
    {
      fprintf(stderr,"SNDDRV : Sample size 8 failed, sound effects disabled\n");    
      close(s_dev);
      write(fds[1],buf,PIPE_BUF);
      exit(1);
    }

    i = SAMPLE_SPEED; 
    if (ioctl(s_dev, SNDCTL_DSP_SPEED, &i)<0)
    {
      fprintf(stderr,"SNDDRV : dsp_speed failed, sound effects disabled\n");    
      close(s_dev);
      write(fds[1],buf,PIPE_BUF);
      exit(1);
    }

    i = 0;     // no stero
    if (ioctl(s_dev, SNDCTL_DSP_STEREO, &i)<0)
    {
      fprintf(stderr,"SNDDRV : Sample size 8 failed, sound effects disabled\n");    
      close(s_dev);
      write(fds[1],buf,PIPE_BUF);
      exit(1);
    }

    
    for (i=0;i<NUM_CHANNELS;i++)    // clear all the sound channels
      channels[i].data=NULL;  


    buf[0]=1;
    write(fds[1],buf,PIPE_BUF);        // send a success signal to parent
    sound_child_fd=fds[0];
    sound_parent_fd=fds[1];
    sound_watch(sound_child_fd);
  }
  CHECK(0);                         // should never get here!
  return 0;                         
}


int sound_fd_ready_to_read(int fd)
{
  struct timeval tv={0,0};
  fd_set kbd_set,ex_set;
  FD_ZERO(&kbd_set);
  FD_SET(fd,&kbd_set);
  memcpy((void *)&ex_set,(void *)&kbd_set,sizeof(ex_set));
  select(FD_SETSIZE,&kbd_set,NULL,&ex_set,&tv);                // check for exception
  return (FD_ISSET(fd,&kbd_set) || FD_ISSET(fd,&ex_set));
}


void sound_quit(int client_fd)
{
  close(sound_parent_fd);
  close(sound_child_fd);
  close(sound_status[0]);
  close(sound_status[1]);
  close(s_dev);
  exit(0);  
}

short new_id()
{
  int i;
  for (i=0;i<total_effects;i++)
    if (seffects[i].size==0)
      return i;
  i=total_effects;  
  total_effects++;  
  seffects=(sfx *)realloc(seffects,sizeof(sfx)*(total_effects)); 
  seffects[i].size=0;  
  return i;  
}


void sound_watch(int f)
{
  int wait=0;
  char fn[200];
  while (1)
  {    
    if (output_sounds())
      wait=0;
    else wait=1;    

    if (wait || sound_fd_ready_to_read(f))    // is there a command waiting?
    {
      unsigned char cmd;

      int status=read(f,&cmd,1);
      
      if (status!=1)
      { sound_quit(f); }
      
      switch (cmd)
      {
        case SDRIVER_QUIT : sound_quit(f); break;
        case SDRIVER_LOAD :
	{	  
	  short id=new_id();
	  unsigned short sl;
	  read(f,&sl,2);
	  read(f,fn,sl);
	  long sample_speed;
	  seffects[id].data=(unsigned char *)read_wav(fn,sample_speed,seffects[id].size); 
	  write(sound_status[1],&id,2);
	} break;
        case SDRIVER_UNLOAD :
	{
	  short id;	
	  read(f,&id,2);
	  if (id>=total_effects || !seffects[id].size)
	  {
	    fprintf(stderr,"SNDDRV : bad unload sound id %d\n",id);
	    sound_quit(f);
	  }
	  free(seffects[id].data);
	  seffects[id].size=0;	
	} break;	
        case SDRIVER_PLAY :
	{
	  unsigned char vol;
	  read(f,&vol,1);	  

	  short id;	
	  read(f,&id,2);
	  if (id>=total_effects || !seffects[id].size)
	  {
	    fprintf(stderr,"SNDDRV : bad play sound id %d\n",id);
	    sound_quit(f);
	  }
	  play(id,vol);
	} break;
	default :
	  fprintf(stderr,"SNDDRV : Bad command %d\n",cmd);
	  sound_quit(f);
	  break;
	  
      }          
    }     
  }
}


sound_effect::sound_effect(char *filename)
{
  long rate;    
  short id;  
  if (sound_child_fd>=0)
  {    
    unsigned char cmd=SDRIVER_LOAD;    
    write(sound_parent_fd,&cmd,1);
    unsigned short string_length=strlen(filename)+1;
    write(sound_parent_fd,&string_length,2);
    write(sound_parent_fd,filename,string_length);
    read(sound_status[0],&id,2);
    size=(long)id;    
  }  
}


sound_effect::~sound_effect()
{
  short id;  
  if (sound_child_fd>=0)
  {
    unsigned char cmd=SDRIVER_UNLOAD;   // tell the driver to unload the sound
    write(sound_parent_fd,&cmd,1);    
    id=(short)size;
    write(sound_parent_fd,&id,2);    
  }
}


void sound_effect::play(int volume, int pitch, int panpot)
{  
  if (sound_child_fd>=0)
  {    
    unsigned char cmd=SDRIVER_PLAY;
    write(sound_parent_fd,&cmd,1);    

    unsigned char vol=(unsigned char)volume;
    write(sound_parent_fd,&vol,1);    

    short id=(short)size;
    write(sound_parent_fd,&id,2);
  }
}

void sound_uninit()         // called by parent
{
  if (sound_child_fd>=0)    // mkae sure the child has been forked
  {
    char cmd=SDRIVER_QUIT;
    write(sound_parent_fd,&cmd,1);
    close(sound_parent_fd);
    close(sound_child_fd);
    close(sound_status[0]);
    close(sound_status[1]);
    sound_child_fd=-1;
  }
}


song::song(char *filename)
{
  data=NULL;
  Name=strcpy((char *)jmalloc(strlen(filename)+1,"song name"),filename);
  song_id=0;
}

song::~song()
{
  if (playing())
    stop();  
  if (data) jfree(data);
  jfree(Name);  
}  

void song::play(unsigned char volume)
{
  printf("play song %s, volume %d\n",name(),volume);  
  song_id=1;  
}


void song::stop(long fadeout_time)                                       // time in ms
{
  printf("stop song %s, fade timeout %d\n",name(),fadeout_time);
  song_id=0;
  
}

int song::playing()
{
  return song_id;  
}



void set_music_volume(int volume)                  // 0...127
{
;  
}




