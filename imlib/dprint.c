#include <stdlib.h>
#ifdef __WATCOMC__
#include "i86.h"
#endif
#include <stdarg.h>
#include <stdio.h>
#include "macs.hpp"

void  (*dprint_fun)(char *)=NULL;
void  (*dget_fun)(char *,int)=NULL;

void set_dprinter(void (*stat_fun)(char *))
{
  dprint_fun=stat_fun;
}

void set_dgetter(void (*stat_fun)(char *,int))
{
  dget_fun=stat_fun;
}

void dprintf(const char *format, ...)
{
  if (dprint_fun)
  {
    char st[1000],a,*sp;
    int y;
    va_list ap;
    va_start(ap, format);
    vsprintf(st,format,ap);
    va_end(ap);
    dprint_fun(st);
  }
}


void dgets(char *buf, int size)
{
  if (dget_fun)
  {
    dget_fun(buf,size);
  } else
    ERROR(0,"dgets called but no handler set up");  
}
