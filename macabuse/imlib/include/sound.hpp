#ifndef __SOUND_HPP_
#define __SOUND_HPP_

/* options are passed via command line */
     
#define SFX_INITIALIZED    1
#define MUSIC_INITIALIZED  2

int sound_init(int argc, char **argv);
void sound_uninit();
void print_sound_options();                         // print the options avaible for sound

class sound_effect
{
  long size;
  void *data;
public :
  sound_effect(char *filename);
  void play(int volume=127, int pitch=128, int panpot=128);
  ~sound_effect();
} ;


class song
{
  char *Name;
  unsigned char *data;
  unsigned long song_id;
public :
  char *name() { return Name; }
  song(char *filename);
  void play(unsigned char volume=127);
  void stop(long fadeout_time=0);                                        // time in ms
  int playing();
  void set_volume(int volume);
  ~song();
} ;
#endif











