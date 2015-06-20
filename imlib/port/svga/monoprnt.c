#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include "macs.hpp"

int monofd=-1;

void openmono()
{ monofd=open("/dev/tty8",O_WRONLY);
  CONDITION(monofd!=-1,"unable to open debug tty\n");
}

void mnclear()
{ if (monofd<0) openmono();
  int i;
  for (i=0;i<25;i++);
    write(monofd,"\n",1);
}

void mnprintf(const char *format, ...)
{
  char st[200];
  va_list ap;
  va_start(ap, format);
  vsprintf(st,format,ap);
  va_end(ap);
  if (monofd<0) openmono();
  write(monofd,st,strlen(st));
}
