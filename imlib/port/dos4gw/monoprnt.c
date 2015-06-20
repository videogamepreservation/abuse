#include <stdio.h>
#include <mem.h>
#include <dos.h>
#include <stdarg.h>

#define v_mem() ((unsigned char *)0xb0000)

int mcx=0,mcy=0;
void mnclear()
{ memset(v_mem(),0,160*25); }


void mnprintf(const char *format, ...)
{
  char st[200],a,*sp;
  int y;
  va_list ap;
  va_start(ap, format);
  vsprintf(st,format,ap);
  va_end(ap);
  sp=st;
  while (*sp)
  { a=0;
    if (*sp=='\n')
      a=1;
    else { if (mcx==79)
	     a=1;
	   *(v_mem()+mcy*160+mcx*2)=*sp;
	   *(v_mem()+mcy*160+mcx*2+1)=27;
	   mcx++; }
    if (a)
    {
      mcx=0;
      mcy++;
      if (mcy==25)
      {
	for (y=0;y<24;y++)
	  memcpy(v_mem()+(y)*160,v_mem()+(y+1)*160,160);
	memset(v_mem()+24*160,0,160);
	mcy=24;
      }
    }
    sp++;
  }
}
