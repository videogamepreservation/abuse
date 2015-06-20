#include "sound.hpp"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

sound_effect::sound_effect(char *filename)
{
  data=(void *)strcpy((char *)malloc(strlen(filename)+1),filename);      // insert code here  
}


sound_effect::~sound_effect()
{
  free(data); 
}


void sound_effect::play(int volume, int pitch, int panpot)
{
//  printf("Play sound effect %s\n",data);
}

int sound_init(int argc, char **argv)
{
	return 0;  
}

void sound_uninit()
{
  ;
  
}

void song::set_volume(int vol)  { ; }

song::song(char *filename)
{
  data=NULL;
  Name=strcpy((char *)malloc(strlen(filename)+1),filename);
  song_id=0;
}

song::~song()
{
  if (playing())
    stop();  
  if (data) free(data);
  free(Name);  
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
