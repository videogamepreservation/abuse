#include "dprint.hpp"
#include <stdio.h>
#include "macs.hpp"
#include <string.h>

uchar major_version=2;
uchar minor_version=00;

extern int get_option(char *name);

#ifdef __WATCOMC__
#include "i86.h"

static void setup()
{
  union REGS in;
  in.w.ax=0x03;
  int386(0x10,&in,&in);    // clear screen, set to text mode

  char msg1[100],msg2[100];

  sprintf(msg1," Abuse (Version %d.%02d)\n",major_version,minor_version);
  msg2[0]=0;
  
  int i;
  for (i=0;i<80/2-strlen(msg1)/2;i++) strcat(msg2," ");
  strcat(msg2,msg1);
  dprintf(msg2);

  for (i=0;i<80;i++)
    *((unsigned char *)(0xb8000+i*2+1))=0x17;
}
#else
static void setup()
{
  dprintf(" Abuse (Version %d.%02d)\n",major_version,minor_version);
}
#endif


void show_verinfo(int argc, char **argv)
{
  setup();
  
  if (major_version<1)
  {
    fprintf(stderr,"*******************************************************\n"
            "This is the final beta before we ship.\n"
            "Please report any bugs to abuse-bugs@crack.com.  Include\n"
            "game version number and your system specifications.\n"
            "*** Finger abuse-bugs@crack.com or check\n"
            "http://www.crack.com for the latest version number\n"
            "before submitting any bug reports.\n"
            "*******************************************************\n\n");
  }

}
