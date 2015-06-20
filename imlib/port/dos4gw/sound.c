extern "C" {
#include "sos.h"
#include "sosm.h"
} ;

#include "profile.h"
#include "readwav.hpp"
#include "jmalloc.hpp"
#include <stdlib.h>
#include "sound.hpp"
#include <stdio.h>
#include "dprint.hpp"
#include "specs.hpp"
#include "doscall.hpp"
#include "specs.hpp"
#include "macs.hpp"
#include "timing.hpp"
#include <dos.h>

static W32      wDIGIDeviceID=-1;                            // digital device ID
static W32      wMIDIDeviceID=-1;                            // midi device ID

static uchar sos_need_uninit_midi_system=0,
             sos_need_uninit_midi_driver=0,
             sos_need_uninit_digi_system=0,
             sos_need_uninit_digi_driver=0,
	     sos_need_uninit_timer=0,
             sos_need_remove_timer_event=0;

static HANDLE             hDIGIDriver;               // handle to digital driver
static HANDLE             hMIDIDriver;               // handle to MIDI driver
static HANDLE             hDIGITimer;                // handle to digital mixer
static _SOS_DIGI_DRIVER   sDIGIDriver;               // digital driver structure
static _SOS_MIDI_DRIVER   sMIDIDriver;               // midi driver structure
static PSTR               pMelodicPatch=NULL;        // melodic FM instruments
static PSTR               pDrumPatch=NULL;           // drum FM instruments
static _SOS_SAMPLE        sos_sam;
static int s_init=0;                   // flags saying what has been initalized

extern void add_timer_call(void ( *fun)(), int and_call_mask);
extern void remove_timer_call(void ( *fun)());

void print_sound_options() { ; }                         // print the options avaible for sound
void song::set_volume(int volume)  // 0..127
{ 
  if ((s_init&MUSIC_INITIALIZED) && data)
    sosMIDISetSongVolume(song_id,volume);
}

extern void (*install_timer_handler)(void (*fun)())=NULL;   // function to install timer
extern void (*uninstall_timer_handler)()=NULL;

static HANDLE jc_timer_handle;
extern uchar timer_installed;
static void install_timer(void (*fun)())
{
  sosTIMERRegisterEvent(100,fun,&jc_timer_handle);
}

static void uninstall_timer()
{
  sosTIMERRemoveEvent( jc_timer_handle);
}
// BOOL  sosEZGetConfig( PSTR szName )
BOOL sosEZGetConfig(char *szName )
{
 _INI_INSTANCE  sInstance;
 BOOL           wError;
 
 // reset digital and MIDI driver structures
 memset( &sDIGIDriver, 0, sizeof( _SOS_DIGI_DRIVER ) );
 memset( &sMIDIDriver, 0, sizeof( _SOS_MIDI_DRIVER ));
 
 // open .ini file 
 if ( !hmiINIOpen( &sInstance, szName ) )
 return( _FALSE );
 
 // locate section for digital settings
 if ( !hmiINILocateSection( &sInstance, "DIGITAL" ) )
 {  
    // close file
    hmiINIClose( &sInstance );
    
    // return error
    return( _FALSE );
  }
 
 // fetch device ID, Port, DMA, IRQ
 wError   =  hmiINIGetItemDecimal( &sInstance, "DeviceID", &wDIGIDeviceID );
 wError   =  hmiINIGetItemDecimal( &sInstance, "DevicePort", &sDIGIDriver.sHardware.wPort );
 wError   =  hmiINIGetItemDecimal( &sInstance, "DeviceDMA", &sDIGIDriver.sHardware.wDMA );
 wError   =  hmiINIGetItemDecimal( &sInstance, "DeviceIRQ", &sDIGIDriver.sHardware.wIRQ );
 
 // error
 if ( !wError )
 {
  // close file
  hmiINIClose( &sInstance );
  
  // return error
  return( _FALSE );
}
 
 // locate section for MIDI settings
 if ( !hmiINILocateSection( &sInstance, "MIDI" ) )
 {  
    // close file
    hmiINIClose( &sInstance );
    
    // return error
    return( _FALSE );
  }
 
 // fetch device ID, Port, DMA, IRQ
 wError   =  hmiINIGetItemDecimal( &sInstance, "DeviceID", &wMIDIDeviceID );
 wError   =  hmiINIGetItemDecimal( &sInstance, "DevicePort", &sMIDIDriver.sHardware.wPort );
 
 // error
 if ( !wError )
 {
  // close file
  hmiINIClose( &sInstance );
  
  // return error
  return( _FALSE );
}
 
 // close file
 hmiINIClose( &sInstance );
 
 // return success
 return( _TRUE );
}

int sosEZInitSystem2(W32 wDDeviceID, W32 wMDeviceID )
{
  // set up the digital driver
  sDIGIDriver.wDriverRate       =  11025;
  sDIGIDriver.wDMABufferSize    =  0x1000;

  int inst=timer_installed;
  if (inst)    // if a timer is already installed, uninstall and reinstall with us    
    timer_uninit();

  install_timer_handler=install_timer;
  uninstall_timer_handler=uninstall_timer;
  

  // initialize the timer system
  sosTIMERInitSystem( _TIMER_DOS_RATE, _SOS_DEBUG_NORMAL );
  sos_need_uninit_timer=1;

  if (inst)
    timer_init();


  // initialize the digital and midi systems
  sosDIGIInitSystem( _NULL, _SOS_DEBUG_NORMAL );
  sos_need_uninit_digi_system=1;

  sosMIDIInitSystem( _NULL, _SOS_DEBUG_NORMAL );
  sos_need_uninit_midi_system=1;

  // check to see if the midi device is to be initialized
  if ( wMDeviceID != -1 )
  {
    // initialize MIDI
    sMIDIDriver.wID = wMDeviceID;
    if ( sosMIDIInitDriver( &sMIDIDriver, &hMIDIDriver ) )
    {
      sound_uninit();
      return 0;
    } else sos_need_uninit_midi_driver=1;
  }

  // initialize the digital driver
  if ( wDDeviceID != -1 )
  {
    // initialize digital
    sDIGIDriver.wID = wDDeviceID;
    if ( sosDIGIInitDriver( &sDIGIDriver, &hDIGIDriver ) )
    {
      sound_uninit();
      return 0;
    } else
    {
      sos_need_uninit_digi_driver=1;
      s_init|=SFX_INITIALIZED;
    }
  }


  // register digital timer event (mixer)
  if ( wDDeviceID != -1  || wMDeviceID!=-1)
  {
    sos_need_remove_timer_event=1;

    sosTIMERRegisterEvent(100, sDIGIDriver.pfnMixFunction,
			&hDIGITimer );
  }


  // check driver type, if it is an OPL2/3 driver, we
  // need to load the patch files for the driver.
  if (wMIDIDeviceID == _MIDI_FM || wMIDIDeviceID == _MIDI_OPL3 && wMIDIDeviceID != -1 )
  {
    bFILE *fp=open_file("music/melodic.bnk","rb");
    if (fp->open_failure())
    {
      dprintf("Missing file music/melodic.bnk, sound disabled\n");
      sound_uninit();
      delete fp;
      return 0;
    }
    
    long l=fp->file_size();
    pMelodicPatch=(PSTR)jmalloc(l,"Melodic bank");
    fp->read(pMelodicPatch,l);
    delete fp;


    fp=open_file("music/drum.bnk","rb");
    if (fp->open_failure())
    {
      dprintf("Missing file music/drum.bnk, sound disabled\n");
      sound_uninit();
      delete fp;
      return 0;
    }
    
    l=fp->file_size();
    pDrumPatch=(PSTR)jmalloc(l,"Drum bank");
    fp->read(pDrumPatch,l);
    delete fp;


    if ((sosMIDISetInsData( hMIDIDriver, (LPSTR)pMelodicPatch, 1)) || 
	(sosMIDISetInsData( hMIDIDriver, (LPSTR)pDrumPatch, 1)))
    {
      sound_uninit();
      return 0;
    }
    s_init|=MUSIC_INITIALIZED;
  } else if (wMIDIDeviceID!=-1) 
     s_init|=MUSIC_INITIALIZED;


  return s_init;
}


void sound_uninit()
{
  if (sos_need_remove_timer_event)
  {
    sos_need_remove_timer_event=0;
    sosTIMERRemoveEvent( hDIGITimer );
  }

  if (sos_need_uninit_timer)
  {
    int inst=timer_installed;
    if (inst)    // if a timer is already installed, uninstall and reinstall with us 
    {
      timer_uninit();
      install_timer_handler=NULL;
      uninstall_timer_handler=NULL;
    }


    sos_need_uninit_timer=0;
    sosTIMERUnInitSystem(0);


    if (inst)
    {
      timer_init();        // reinitial timer to use old method
    }
  }

  if (sos_need_uninit_digi_driver)
  {
    sos_need_uninit_digi_driver=0;
    sosDIGIUnInitDriver( hDIGIDriver, _TRUE, _TRUE );
    s_init&=~SFX_INITIALIZED;
  }

  if (sos_need_uninit_digi_system)
  {
    sos_need_uninit_digi_system=0;
    sosDIGIUnInitSystem();
  }


  if (sos_need_uninit_midi_driver)
  {
    sos_need_uninit_midi_driver=0;
    sosMIDIUnInitDriver( hMIDIDriver, _TRUE );
    s_init=s_init&(~MUSIC_INITIALIZED);
  }

  if (sos_need_uninit_midi_system)
  {
    sos_need_uninit_midi_system=0;
    sosMIDIUnInitSystem();
  }



  if (pDrumPatch)
  {
    jfree(pDrumPatch);
    pDrumPatch=NULL;
  }

  if (pMelodicPatch)
  {
    jfree(pMelodicPatch);
    pMelodicPatch=NULL;
  }

}

int sound_init(int argc, char **argv)
{
  if (s_init!=0) sound_uninit();

  atexit(sound_uninit);
  for (int i=1;i<argc;i++)
  {
    if (!strcmp(argv[i],"-nosound"))
    {
      dprintf("sound : sound disabled (-nosound)\n");
      wDIGIDeviceID=-1;
      wMIDIDeviceID=-1;    
      return 0;
    }
  }

  if (sosEZGetConfig("sndcard.cfg")==_FALSE)
  {
    wDIGIDeviceID=-1;
    wMIDIDeviceID=-1;    
    dprintf("Sound configuration was not saved by setup program, sound disabled\n");
    return 0;    
  }

  if (wDIGIDeviceID==-1 && wMIDIDeviceID==-1)
    return 0;

  memset(&sos_sam,0,sizeof(sos_sam));
  sos_sam.wBitsPerSample=8;
  sos_sam.wChannels=1;
  sos_sam.wFormat=_PCM_UNSIGNED;
  sos_sam.wRate=11025;
  sos_sam.wPanPosition=_PAN_CENTER;


  return sosEZInitSystem2(wDIGIDeviceID, wMIDIDeviceID);



/*  sosTIMERInitSystem( _TIMER_DOS_RATE, _SOS_DEBUG_NORMAL );    
  sos_need_uninit_timer=1;
  sosDIGIInitSystem( _NULL, _SOS_DEBUG_NORMAL );
  sos_need_uninit_digi=1;
  sosMIDIInitSystem(_NULL,_SOS_DEBUG_NORMAL);
  sos_need_uninit_midi=1;


  if (wDIGIDeviceID!=-1)
  {
    sDIGIDriver.wDriverRate       =  11025;
    sDIGIDriver.wDMABufferSize    =  0x1000;

    sDIGIDriver.wID = wDIGIDeviceID;

    if ( sosDIGIInitDriver( &sDIGIDriver, &hDIGIDriver ) )
    {
      sound_uninit();
      return 0;
    }
    int inst=timer_installed;
    if (inst)    // if a timer is already installed, uninstall and reinstall with us    
      timer_uninit();

    install_timer_handler=install_timer;
    uninstall_timer_handler=uninstall_timer;
  
    if (inst)
      timer_init();

    sosTIMERRegisterEvent(100,sDIGIDriver.pfnMixFunction,
			  &hDIGITimer );
    sos_need_remove_timer_event=1;

    s_init|=SFX_INITIALIZED;
  }

  if (wMIDIDeviceID!=-1)
  {
    sMIDIDriver.wID = wMIDIDeviceID;
    if (sosMIDIInitDriver( &sMIDIDriver, &hMIDIDriver ))
    {
      sound_uninit();
      return 0;
    }
  } 







  // check driver type, if it is an OPL2/3 driver, we
  // need to load the patch files for the driver.
  if (wMIDIDeviceID == _MIDI_FM || wMIDIDeviceID == _MIDI_OPL3 && wMIDIDeviceID != -1 )
  {
    bFILE *fp=open_file("music/melodic.bnk","rb");
    if (fp->open_failure())
    {
      dprintf("Missing file music/melodic.bnk, sound disabled\n");
      sound_uninit();
      delete fp;
      return 0;
    }
    
    long l=fp->file_size();
    pMelodicPatch=(PSTR)jmalloc(l,"Melodic bank");
    fp->read(pMelodicPatch,l);
    delete fp;


    fp=open_file("music/drum.bnk","rb");
    if (fp->open_failure())
    {
      dprintf("Missing file music/drum.bnk, sound disabled\n");
      sound_uninit();
      delete fp;
      return 0;
    }
    
    l=fp->file_size();
    pDrumPatch=(PSTR)jmalloc(l,"Drum bank");
    fp->read(pDrumPatch,l);
    delete fp;


    if ((sosMIDISetInsData( hMIDIDriver, (LPSTR)pMelodicPatch, 1)) || 
	(sosMIDISetInsData( hMIDIDriver, (LPSTR)pDrumPatch, 1)))
    {
      sound_uninit();
      return 0;
    }
    s_init|=MUSIC_INITIALIZED;
  } else if (wMIDIDeviceID!=-1) s_init|=MUSIC_INITIALIZED;

  memset(&sos_sam,0,sizeof(sos_sam));
  sos_sam.wBitsPerSample=8;
  sos_sam.wChannels=1;
  sos_sam.wFormat=_PCM_UNSIGNED;
  sos_sam.wRate=11025;
  sos_sam.wPanPosition=_PAN_CENTER;

  return s_init; */
}

sound_effect::sound_effect(char *filename)
{  
  if (s_init&SFX_INITIALIZED)
  {
    long sample_speed;
    data=(void *)read_wav(filename,sample_speed,size);  
  } else data=NULL;
};




void sound_effect::play(int volume, int pitch, int panpot)
{
  if ((s_init&SFX_INITIALIZED) && data)
  {
    sos_sam.pSample=(PSTR)data;
    sos_sam.wLength=size;
    sos_sam.wVolume=MK_VOLUME( ((volume<<8)|0xff) , ((volume<<8)|0xff));
    sosDIGIStartSample( hDIGIDriver, &sos_sam);
  }
}


sound_effect::~sound_effect() 
{ 
  if (data)
    jfree(data);       // hope it's not still playing!!! :)
}

song::song(char *filename) 
{
  data=NULL;
  if (s_init&MUSIC_INITIALIZED)
  {
    bFILE *fp=open_file(filename,"rb");
    if (!fp->open_failure())
    {
      long l=fp->file_size();
      data=(uchar *)jmalloc(l+sizeof(_SOS_MIDI_SONG),"song");

      _SOS_MIDI_SONG *sSong;
      sSong= (_SOS_MIDI_SONG *)data;
      memset( sSong, 0, sizeof( _SOS_MIDI_SONG ) );
      sSong->pSong =  ( PSTR )( data + sizeof( _SOS_MIDI_SONG ));

      fp->read(sSong->pSong,l);
      W32 hSong;
      int err;


      if (err=sosMIDIInitSong( sSong, &hSong ))
      {
	dprintf("Error while registering song %s\n",filename);
	if (err==_ERR_NO_HANDLES)
	dprintf("out of handles\n");
	else if (err==_ERR_INVALID_DATA)
	dprintf("invaild data in file\n");

	jfree(data);
	data=NULL;      
      } else song_id=hSong;      
    }
    delete fp;
  }
}

void song::play(unsigned char volume) 
{
  if (s_init&MUSIC_INITIALIZED)
  {
    if (data)
    {
      sosMIDIStartSong((W32)song_id);
      set_volume(volume);
    }
  }
}


void song::stop(long fadeout_time) 
{ 
  if ((s_init&MUSIC_INITIALIZED) && data)
    sosMIDIStopSong((W32)song_id);
}                                        // time in ms


int song::playing() 
{
  if ((s_init&MUSIC_INITIALIZED) && data)
  {
    if (sosMIDISongDone((W32)song_id)==_FALSE)
      return 1;
    else return 0;
  } else return 0;
}


song::~song() 
{
  if (data)
  {
    if (s_init&MUSIC_INITIALIZED)
      sosMIDIUnInitSong((W32)song_id);
    jfree(data);
  }
}







