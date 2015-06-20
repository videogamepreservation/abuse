#include "packet.hpp"
#include "jmalloc.hpp"
#include <stdlib.h>
#include <string.h>


int packet::advance(long offset)
{
  ro+=offset;
  return ro<=rend;
}

void packet::write_long(ulong x)
{
  x=lltl(x);
  write((uchar *)&x,4);
}

void packet::write_short(ushort x)
{
  x=lstl(x);
  write((uchar *)&x,2);
}

void packet::write_byte(uchar x)
{
  write(&x,1);
}

packet::~packet()
{ jfree(buf); }

packet::packet(int prefix_size) 
{ 
  pre_size=prefix_size;

#ifdef MANAGE_MEM
  int sp=alloc_space;
  alloc_space=ALLOC_SPACE_STATIC;
#endif

  buf_size=1000;
  buf=(uchar *)jmalloc(buf_size,"packet buffer");
  reset(); 

#ifdef MANAGE_MEM
  alloc_space=sp;
#endif
}

void packet::get_string(char *st, int len)
{
  char *b=(char *)buf+ro;
  while (len>1 && !eop() && *b)
  {
    *st=*b; 
    st++;
    ro++;
    b++;
    len--;
  }
  if (*b==0) ro++;
  *st=0;
}

void packet::reset() 
{ ro=wo=rend=pre_size; }

void packet::make_bigger(int max)
{
  if (buf_size<max)
  {
    buf_size=max;
    buf=(uchar *)jrealloc(buf,max,"packet buffer"); 
  }
}

int packet::read(uchar *buffer, int size)
{
  if (size>rend-ro)
    size=rend-ro;

  if (size>0)
  {
    memcpy(buffer,buf+ro,size);
    ro+=size;
    return size;
  } else return 0;
}


int packet::write(uchar *buffer, int size)
{
  if (size>buf_size-wo)
    make_bigger(wo+size);
  if (size>0)
  {
    memcpy(buf+wo,buffer,size);
    wo+=size;
    rend=wo;
    return size;
  }
  return 0;
}



void packet::insert_into(packet &pk)
{
  pk.write(buf+pre_size,rend-pre_size);
}



