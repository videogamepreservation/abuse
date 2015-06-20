#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>

#include "sound.hpp"
extern "C" {
#include "dmx.h"
} ;

#define NO_CARD 0

int dma=0,irq=0,addr=0;
unsigned long card=NO_CARD,music_card=NO_CARD;

char *erm[] =
{ "",
  "could not find card address (port)\n",
  "could not allocate DMA buffer\n"
  "interrupt not detected\n"
  "DMA channel could not be identified\n"};

#define NUMBER_CHANNELS 4              // default number of sound channels
#define SAMPLE_RATE 22050              // default sampling rate

void sound_uninit()
{
  DMX_DeInit();  
}        

void print_sound_options()
{
  printf("  Sound cards supported :\n"        
         "    -sblaster        Sound Blaster\n"
         "    -gravis          Gravis Ultrasound\n"
         "    -proaudio        Pro Audio Spectrum\n"
         "    -speaker         PC speaker\n"
         "  Music cards supports :\n"
         "    -adlib\n"
         "    -mpu401\n"
         "    -gravis\n"
         "    -speaker         PC speaker\n"
         "  Sound options :\n"
         "    -samplerate 5000..50000     default :%d\n"
         "    -channels 1..4\n            default : %d\n",SAMPLE_RATE,
            NUMBER_CHANNELS);
}                      


int sound_init(int argc, char **argv)
{
  int i,return_code,
      sr=SAMPLE_RATE,nc=NUMBER_CHANNELS;
  unsigned short ver;
  
  for (i=1;i<argc;i++)
  {    
    if (!strcmp(argv[i],"-samplerate"))
    {
      i++;
      if (atoi(argv[i])<5000 || atoi(argv[i])>50000)
        printf("Bad sample rate : valid range is 5000..50000, default is %d",SAMPLE_RATE);
      else 
      {
	printf("Sample rate set to %d\n",sr);
        sr=SAMPLE_RATE;
      }           
    } else if (!strcmp(argv[i],"-channels"))
    {      
      i++;
      if (atoi(argv[i])<1 || atoi(argv[i])>4)
        printf("Bad # of channels : valid range is 1..4, default is %d",NUMBER_CHANNELS);
      else 
      {
	printf("# of channels set to %d\n",nc);
        nc=NUMBER_CHANNELS;
      }           
    } else if (!strcmp(argv[i],"-sblaster"))
    { card=AHW_SOUND_BLASTER; 
      if (music_card==NO_CARD)
        music_card=AHW_ADLIB;
    }
    else if (!strcmp(argv[i],"-proaudio"))
    {
      card=AHW_MEDIA_VISION;
      if (music_card==NO_CARD)
        music_card=AHW_ADLIB;
    }
    else if (!strcmp(argv[i],"-gravis") || !strcmp(argv[i],"-gus"))    
      music_card=card=AHW_ULTRA_SOUND;         
    else if (!strcmp(argv[i],"-speaker"))
      music_card=AHW_PC_SPEAKER;
    else if (!strcmp(argv[i],"-adlib"))
      music_card=AHW_ADLIB;
    else if (!strcmp(argv[i],"mpu401"))
      music_card=AHW_MPU_401;
    else if (!strcmp(argv[i],"-detect"))
      music_card=card=AHW_ANY;    
    else if (!strcmp(argv[i],"-irq"))
    { i++;
      irq=atoi(argv[i]);
    } else if (!strcmp(argv[i],"-dma"))
    { i++;
      dma=atoi(argv[i]);
    }  else if (!strcmp(argv[i],"-addr"))
    { i++;
      addr=atoi(argv[i]);
    }              
  }

  if (card==NO_CARD && music_card==NO_CARD)
     return 0;  

  TSM_Install(140);
  atexit(TSM_Remove);  

  if (card==AHW_ANY || card==AHW_MEDIA_VISION)
  {    
    if ( MV_Detect() == 0 )
    {      
      printf( "Detected MEDIA VISION card.  (Pro Audio)\n" );      
      card=AHW_MEDIA_VISION;
      music_card=AHW_ADLIB;      
    }    
    else if (card==AHW_MEDIA_VISION)
    {	
      printf("Warning : Detect failed for MEDIA VISION (Pro Audio) card.\n");
      card=NO_CARD;      
    }
  } 
  
  if (card==AHW_ANY || card==AHW_ULTRA_SOUND)
  {      
    if ( GF1_Detect() == 0 )
    {      
      printf( "Detected GRAVIS Ultrasound card.\n" );
      music_card=card=AHW_ULTRA_SOUND;
    }    
    else if (card==AHW_ULTRA_SOUND)
    {	
      printf("Warning : Detect failed for GRAVIS Ultrasound card.\n");
      card=NO_CARD;     
    }      
  }    
    
  if (card==AHW_ANY)
  {    
    if (!addr || !dma || !irq)
    {      
      addr = dma = -1; irq = 0;
      return_code=SB_Detect( &addr, &irq, &dma, &ver);  
      if (!return_code)      
        printf( "SBlaster defaults are Port:%xh Irq:%d  DMA:%d\n", addr, irq, dma );
	
      printf( "Looking for a SOUND BLASTER card...\n" );
      addr = irq = dma = -1;
      return_code = SB_Detect( &addr, &irq, &dma, &ver );      
      if (!return_code)
      {             
	printf( "Found SoundBlaster card\n" );
	printf( "ADDR:%03xh IRQ:%d DMA:%d  Ver:%2x.%02x\n",addr, irq, dma, ver>>8,ver&0xff);
	SB_SetCard( addr, irq, dma );
      } 
    } else SB_SetCard( addr, irq, dma );
  } 
    
  if (music_card==AHW_ANY || music_card==AHW_ADLIB)
  {
    int waitstates,board_type;
    if (AL_Detect(&waitstates,&board_type)==0)
    {
      printf("Detected Adlib music card, waitstates=%d, board type=%d\n",
        waitstates,board_type);
      music_card=AHW_ADLIB;
    } else if (music_card==AHW_ADLIB)
    {
      printf("Warning : Adlib not detected\n");
      music_card=NO_CARD;
    }
  }
  
  if (music_card==AHW_MPU_401 || music_card==AHW_MPU_401)
  {
    int type;
    if (MPU_Detect(&addr,&type)==0)
    {
      printf("Detected MPU-401, port address = %x(hex), type=",addr);
      switch (type)
      {
        case 0 : printf("GENERAL MIDI\n"); break;
        case 1 : printf("SCC-1\n"); break;
        default : printf("unknown\n");
      }
    } else if (music_card==AHW_MPU_401)    
    { 
      printf("Failed to detect MPU 401\n");
      music_card=NO_CARD;
    }  
  }
  
  if (card==NO_CARD && music_card==NO_CARD)
    return 0;
  
  printf( "Initializing Sound effect subsystem...\n" );
  return_code = DMX_Init( 140, 20, music_card, card);
  printf("Initialized\n");
  
  if (!return_code)
  {
    printf("Sound card init failed\n");
    card=NO_CARD;
    return 0;
  }

  WAV_PlayMode( nc, sr);
  return 1;  
}


void modify_path(char *filename, char *new_filename)
{
  char *n=new_filename;  
#ifdef __WATCOMC__  
  while (*filename)
  {
    if ((*filename)=='/')
      *n='\\';
     else *n=*filename;
    filename++;
    n++;    
  }
  *n=0; 
#else
  strcpy(new_filename,filename);  
#endif 
}


sound_effect::sound_effect(char *filename)
{
  char nf[100];
  modify_path(filename,nf);  
  data=WAV_LoadPatch(nf);
  if (!data)
  {    
    printf("error loading %s\n",nf);
    exit(1);
  }  
}

void sound_effect::play(int volume, int pitch, int panpot)
{
  if (card!=NO_CARD && data)
    SFX_PlayPatch(data,pitch,panpot,volume,0,1);  
}



song::song(char *filename)
{
  Name=strcpy((char *)malloc(strlen(filename)+1),filename);
  
  int fh = open(filename, O_RDONLY|O_BINARY); 
  if ( fh==-1)  
  {    
    printf("Unable to open song %s\n",filename);   
    data=NULL;   
  }    
  else
  {    
    unsigned long fl=filelength(fh);
    if (fl<sizeof(DMX_HEADER) || !memcmp( data, "MUS", 3 ))
    {
      data=NULL;
      printf("Bad music file %s\n",filename);      
    }
    else
    {
      data=(unsigned char *)malloc(fl);
      read(fh,data,fl);
      song_id=MUS_RegisterSong(data);
      if (song_id==-1)
      {
	printf("Error registering song %s\n",name());
	free(data);
	data=NULL;	
      }
      
    }    
    close(fh);
  }
}  
  

song::~song()
{
  if (playing())
    stop();  
  MUS_UnregisterSong(song_id);
  if (data) free(data);
  free(Name);  
}


void song::play(unsigned char volume)
{
  if (song_id!=-1)
    MUS_PlaySong(song_id,volume);  
}

int song::playing()
{
  if (song_id==-1) return 0;
  else return MUS_QrySongPlaying(song_id);
}

void song::stop(long fadeout_time)                 // time in ms
{
  if (song_id!=-1)
    MUS_FadeOutSong(song_id,fadeout_time);  
}



void set_music_volume(int volume)                  // 0...127
{
  if (volume!=0)
    MUS_SetMasterVolume(volume);
}  
