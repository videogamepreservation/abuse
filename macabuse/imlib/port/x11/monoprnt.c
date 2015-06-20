#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "macs.hpp"

int monofd=-1;


void mnclear()
{ 
  int i;
  for (i=0;i<25;i++);
   printf("\n");
}

void mnprintf(const char *format, ...)
{
  char st[200],a,*sp;
  int y;
  va_list ap;
  va_start(ap, format);
  vsprintf(st,format,ap);
  va_end(ap);
  printf("%s",st);
}
