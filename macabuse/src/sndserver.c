extern "C"
{
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <linux/soundcard.h>
#include <sys/time.h>
#include "DoomDef.h"
};

#include "sndserver.h"
#include "wadread.h"

#define SOUNDDIR LIBDIR "/sfx/"

typedef struct wadinfo_struct
{
  char	identification[4];		// should be IWAD
  int	numlumps;
  int	infotableofs;
} wadinfo_t;

typedef struct filelump_struct
{
  int	filepos;
  int	size;
  char	name[8];
} filelump_t;

static int mytime = 0;		// an internal time keeper
int numsounds;			// number of sound effects
int longsound;			// longest sound effect
int lengths[NUMSFX];		// lengths of all sound effects
unsigned char mixbuffer[MIXBUFFERSIZE];	// mixing buffer
int sfxdevice;			// file descriptor of sfx device
int musdevice;			// file descriptor of music device
unsigned char *channels[8];	// the channel data pointers
unsigned char *channelsend[8];	// the channel data end pointers
int channelstart[8];		// time that the channel started playing
int channelhandles[8];		// the channel handles

static void derror(char *msg)
{
  fprintf(stderr, "error: %s\n", msg);
  exit(-1);
}

int mix(void)
{

  register int i, j, d;

//  if (channels[0]) fprintf(stderr, ".");

//  for (d=i=0 ; i<8 ; i++)
//    d |= (int) channels[i];
//  if (!d) return 0;

  // mix into the mixing buffer
  for (i=0 ; i<MIXBUFFERSIZE ; i++)
  {
    d = 0;
    j = 8;
    do {
      if (channels[--j])
      {
        d += *channels[j]++ - 128;
      }
    } while (j);
    if (d > 127) mixbuffer[i] = 255;
    else if (d < -128) mixbuffer[i] = 0;
    else mixbuffer[i] = (unsigned char) (d+128);
//    if (d > 127) mixbuffer[i] = 0;
//    else if (d < -128) mixbuffer[i] = 255;
//    else mixbuffer[i] = (unsigned char) (-d+127);
  }

  // check for freed channels
  for (j=0 ; j<8 ; j++)
  {
    if (channels[j] == channelsend[j]) channels[j] = 0;
  }

  return 1;

}

void grabdata(int c, char **v)
{

  int i;

  numsounds = NUMSFX;
  longsound = 0;

  openwad("../frame/doom.wad");

  for (i=1 ; i<NUMSFX ; i++)
  {
    if (!S_sfx[i].link)
    {
      S_sfx[i].data = getsfx(S_sfx[i].name, &lengths[i]);
      if (longsound < lengths[i]) longsound = lengths[i];
    } else {
      S_sfx[i].data = S_sfx[i].link->data;
      lengths[i] = lengths[(S_sfx[i].link - S_sfx)/sizeof(struct sfxinfo_t)];
    }
    /*
    // test only
    {
      int fd;
      char name[10];
      sprintf(name, "sfx%d", i);
      fd = open(name, O_WRONLY|O_CREAT, 0644);
      write(fd, S_sfx[i].data, lengths[i]);
      close(fd);
    }
    */
  }

}

void opensfxdev(int c, char **v)
{

  int i, rc;

  // open the sound device
  sfxdevice = open("/dev/dsp", O_WRONLY, 0);
  if (sfxdevice < 0) derror("Could not open /dev/dsp");

  // set it up for the proper sound format
  i = 8;
  rc = ioctl(sfxdevice, SNDCTL_DSP_SAMPLESIZE, &i);
  if (rc < 0) fprintf(stderr, "SAMPLESIZE failed\n");
  i = 11111;
  rc = ioctl(sfxdevice, SNDCTL_DSP_SPEED, &i);
  if (rc < 0) fprintf(stderr, "SPEED failed\n");
  i = 0;
  rc = ioctl(sfxdevice, SNDCTL_DSP_STEREO, &i);
  if (rc < 0) fprintf(stderr, "STEREO failed\n");
//  i = 2;
//  rc = ioctl(sfxdevice, SNDCTL_DSP_SUBDIVIDE, &i);
//  if (rc < 0) fprintf(stderr, "SUBDIVIDE failed\n");

}

void closesfxdev(void)
{
  close(sfxdevice);
}

void openmusdev(int c, char **v)
{
}

void closemusdev(void)
{
}

static struct timeval last={0,0}, now;
static struct timezone whocares;

void updatesounds(void)
{
  int deltatime;
  int rc;

  rc = mix();
  rc = ioctl(sfxdevice, SNDCTL_DSP_POST, 0);
//  rc = ioctl(sfxdevice, SNDCTL_DSP_SYNC, 0);
  if (rc < 0) fprintf(stderr, "SYNC failed?!\n");
  write(sfxdevice, mixbuffer, MIXBUFFERSIZE);
/*
  gettimeofday(&now, &whocares);
  deltatime = (now.tv_sec - last.tv_sec)*1000000 + now.tv_usec - last.tv_usec;
  deltatime = deltatime - (1 * MIXBUFFERSIZE * 1000000) / (1 * SPEED);
  last = now;
  if (deltatime < 0)
    usleep (-deltatime);
  if (rc)
  {
    write(sfxdevice, mixbuffer, MIXBUFFERSIZE);
    fprintf(stderr, ".");
  }
*/
}

int addsfx(int sfxid, int volume)
{

  int i;
  int rc = -1;
  static unsigned short handlenums = 0;
  int oldest = mytime;
  int oldestnum = 0;

  for (i=0 ; i<8 ; i++)
  {
    if (!channels[i])
    {
      channelsend[i] =
        (channels[i] = (unsigned char *) S_sfx[sfxid].data) + lengths[sfxid];
      if (!handlenums) handlenums = 100;
      channelhandles[i] = rc = handlenums++;
      channelstart[i] = mytime;
      break;
    }
    else
    {
      if (channelstart[i] < oldest)
      {
        oldestnum = i;
	oldest = channelstart[i];
      }
    }
  }

  // if no channels were available, kill oldest sound and replace it
  if (i == 8)
  {
    channelsend[oldestnum] =
      (channels[oldestnum] = (unsigned char *) S_sfx[sfxid].data)
       + lengths[sfxid];
    if (!handlenums) handlenums = 100;
    channelhandles[oldestnum] = rc = handlenums++;
    channelstart[i] = mytime;
  }

  return rc;

}

void outputushort(int num)
{

  static unsigned char buff[5] = { 0, 0, 0, 0, '\n' };
  static char *badbuff = "xxxx\n";

  // outputs a 16-bit # in hex or "xxxx" if -1.
  if (num < 0)
  {
    write(1, badbuff, 5);
  } else {
    buff[0] = num>>12;
    buff[0] += buff[0] > 9 ? 'a'-10 : '0';
    buff[1] = (num>>8) & 0xf;
    buff[1] += buff[1] > 9 ? 'a'-10 : '0';
    buff[2] = (num>>4) & 0xf;
    buff[2] += buff[2] > 9 ? 'a'-10 : '0';
    buff[3] = num & 0xf;
    buff[3] += buff[3] > 9 ? 'a'-10 : '0';
    write(1, buff, 5);
  }

}

void initdata(void)
{
  int i;
  for (i=0 ; i<sizeof(channels)/sizeof(unsigned char *) ; i++) channels[i] = 0;
  gettimeofday(&last, &whocares);
  usleep(100000);
}

int main(int c, char **v)
{

  int done = 0;
  int rc, nrc, sndnum, handle = 0;
  unsigned char commandbuf[10];
  fd_set fdset, scratchset;
  struct timeval zerowait = { 0, 0 };

  grabdata(c, v);	// get sound data
  initdata();		// init any data

  opensfxdev(c, v);	// open sfx device
  openmusdev(c, v);	// open music device
  fprintf(stderr, "ready\n");

  // parse commands and play sounds until done
  FD_ZERO(&fdset);
  FD_SET(0, &fdset);
  while (!done)
  {
    mytime++;
    do {
      scratchset = fdset;
      rc = select(FD_SETSIZE, &scratchset, 0, 0, &zerowait);
      if (rc > 0)
      {
  //      fprintf(stderr, "select is true\n");
	// got a command
	nrc = read(0, commandbuf, 1);
	if (!nrc) { done = 1; rc = 0; }
	else {
	  switch (commandbuf[0])
	  {
	    case 'p': 		// play a new sound effect
	      read(0, commandbuf, 3);
	      commandbuf[0] -= commandbuf[0]>='a' ? 'a'-10 : '0';
	      commandbuf[1] -= commandbuf[1]>='a' ? 'a'-10 : '0';
	      sndnum = (commandbuf[0]<<4) + commandbuf[1];
  //	    fprintf(stderr, "cmd: play sound %d\n", sndnum);
	      handle = addsfx(sndnum, 127); // returns the handle
//	      outputushort(handle);
	      break;
	    case 'q':
	      read(0, commandbuf, 1);
	      done = 1; rc = 0;
	      break;
	    case 's':
	      {
	        int fd;
		read(0, commandbuf, 3);
		commandbuf[2] = 0;
		fd = open((char*)commandbuf, O_CREAT|O_WRONLY, 0644);
		commandbuf[0] -= commandbuf[0]>='a' ? 'a'-10 : '0';
		commandbuf[1] -= commandbuf[1]>='a' ? 'a'-10 : '0';
		sndnum = (commandbuf[0]<<4) + commandbuf[1];
		write(fd, S_sfx[sndnum].data, lengths[sndnum]);
		close(fd);
	      }
	      break;
	    default:
	      fprintf(stderr, "Did not recognize command\n");
	      break;
	  }
	}
      }
    } while (rc > 0);
    updatesounds();
  }

  closesfxdev();
  closemusdev();

}
