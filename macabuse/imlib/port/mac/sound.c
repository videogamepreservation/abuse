#include "sound.hpp"
#include "jmalloc.hpp"
#include "readwav.hpp"
#include "dprint.hpp"
#include <sound.h>
#include <stdio.h>
#include <string.h>

// Maximum number of channels open at one time
#define	MAXCHANNELS	8
#define USE_ASYNCH

class AsynchChannel {
public:
	SndChannelPtr	channel;
	int playing;
	SndListHandle sound;
};

pascal void DonePlaying(SndChannelPtr channel, SndCommand *cmd);

SndCallBackUPP		DonePlayingCB;

class CSoundManager
{
public:
	AsynchChannel	Channel[MAXCHANNELS];
	int Channels;
	int Next;

	// Construction & destruction
	int Init();
	void Uninit();
	
	void PlaySnd(void *, unsigned long size, unsigned int vol);
	
	void Silence();
} SoundMan;

int CSoundManager::Init()
{
	OSErr err = noErr;

#ifdef USE_ASYNCH
	DonePlayingCB = NewSndCallBackProc(DonePlaying);
#else
	DonePlayingCB = nil;
#endif

	Next = 0;
	Channels = MAXCHANNELS;

	for(short i = 0; i < Channels; i++) 
	{
		char Data[64];
		SndListHandle snd;
		SndListPtr sp;

#if 1
		// Create resource headers for all channels
		sp = (SndListPtr)&Data[0];
		sp->format = 1;
		sp->numModifiers = 1;
		sp->modifierPart[0].modNumber = 5;
		sp->modifierPart[0].modInit = 0x80;
		sp->numCommands = 1;
		sp->commandPart[0].cmd = 0x8051;
		sp->commandPart[0].param1 = 0;
		sp->commandPart[0].param2 = 0x14;
		
		SoundHeaderPtr p = (SoundHeaderPtr)&sp->dataPart[0];
		p->loopStart = 0;
		p->loopEnd = 1;
		p->sampleRate = rate11khz;
		p->encode = 0;
		p->baseFrequency = 0x3c;

		PtrToHand((Ptr)sp,(Handle*)&snd,64);
		HLock((Handle)snd);
#else
		snd = (SndListHandle)GetResource('snd ',1001);
		HLock((Handle)snd);
#endif

		Channel[i].channel = nil;
		Channel[i].playing = FALSE;
		Channel[i].sound = snd;

		// Create all the channels upon creation
		err = SndNewChannel(&Channel[i].channel, 
							sampledSynth, 
							initMono + initNoInterp,
							(SndCallBackUPP)DonePlayingCB);
							
		if(err != noErr) 
		{
			dprintf("Aiiiieee! Sound manager couldn't initialize!");
			break;
		}
	}
	
	return (err == noErr);
}

void CSoundManager::Uninit()
{	
	Silence();
	
	for(int i = 0; i < Channels; i++) 
	{
		OSErr err;
		
		err = SndDisposeChannel (Channel[i].channel, true);
		
		Channel[i].channel = nil;
		Channel[i].playing = FALSE;
		DisposeHandle((Handle)Channel[i].sound);
		Channel[i].sound = nil;
	}
}


void CSoundManager::PlaySnd(void *data, unsigned long size, unsigned int vol)
{
	AsynchChannel *	channel = nil;
	OSErr		err = noErr;
	SndCommand cmd;
	short chan;
	
	// Find a channel to play from
#ifdef USE_ASYNCH
	channel = nil;
	for(short i = 0; i < Channels; i++)
	{
		if(Channel[i].playing == FALSE)
		{
			channel = &Channel[i];
			chan = i;
			break;
		}
	}
	// If all the channels were busy ignore sound
	if (channel == nil)
		return;	
#else
	channel = &Channel[Next];
	Next = (Next+1)%Channels;
#endif

	// grab sound channel		
	channel->playing = TRUE;

	cmd.cmd = quietCmd;
	cmd.param1 = 0;
	cmd.param2 = 0;
	err = SndDoImmediate(channel->channel, &cmd);
	cmd.cmd = flushCmd;
	err = SndDoImmediate(channel->channel, &cmd);
	
	SoundHeaderPtr p = (SoundHeaderPtr)&(**channel->sound).dataPart[0];
	p->samplePtr = (Ptr)data;
	p->length = size;
	
	cmd.cmd = volumeCmd;
	cmd.param2 = (vol<<17) | (vol<<1);
	err = SndDoImmediate(channel->channel, &cmd);
	
	// play sound on channel
	err = SndPlay (channel->channel, channel->sound, 1);
	
	if (err != noErr) 
	{
		channel->playing = TRUE;
		dprintf("Couldn't play sound!\n");
	}
#ifdef USE_ASYNCH
	else
	{
		// setup callback
		cmd.cmd = callBackCmd;
		cmd.param1 = chan;
		cmd.param2 = (long)channel;
		err = SndDoCommand (channel->channel, &cmd, 0);
	}
#endif
}

void CSoundManager::Silence()
{
	for(short i = 0; i < Channels; i++)
	{
		if(Channel[i].playing)
		{
			SndCommand	cmd;
			OSErr		err;
			
			cmd.cmd = quietCmd;
			cmd.param1 = 0;
			cmd.param2 = 0;
			err = SndDoImmediate(Channel[i].channel, &cmd);
			cmd.cmd = flushCmd;
			err = SndDoImmediate(Channel[i].channel, &cmd);
			
			// Mark the channel as empty
			Channel[i].playing = FALSE;
		}
	}
}

#pragma segment Main
pascal void DonePlaying(SndChannelPtr channel, SndCommand *cmd)
{
#if 0
	dprintf("%x done.\n",(int)cmd->param2);
#endif
	((AsynchChannel *)(cmd->param2))->playing = FALSE;
}

sound_effect::sound_effect(char *filename)
{
	long rate;
	
	data = read_wav(filename,rate,size);
}


sound_effect::~sound_effect()
{
  if (data)
  	jfree(data); 
}


void sound_effect::play(int volume, int pitch, int panpot)
{
	SoundMan.PlaySnd(data,size,volume);
}

int sound_init(int argc, char **argv)
{
	return SoundMan.Init();
}

void sound_uninit()
{
  SoundMan.Uninit();
}


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
  dprintf("play song %s, volume %d\n",name(),volume);  
  song_id=1;  
}


void song::stop(long fadeout_time)                                       // time in ms
{
  dprintf("stop song %s, fade timeout %d\n",name(),fadeout_time);
  song_id=0;
  
}

int song::playing()
{
  return song_id;  
}

void song::set_volume(int volume)                  // 0...127
{
}
