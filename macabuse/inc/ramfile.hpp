#ifndef MEMORY_FILE
#define MEMORY_FILE

#include "jmalloc.hpp"
#include "system.h"

class memory_file
{
  unsigned char *data;

  public :
  long wp,rp,size;

  memory_file(int size) : size(size)
  //{{
  {
    data=(unsigned char *)jmalloc(size,"memory file");
    wp=rp=0;
  }
  //}}

  ~memory_file()
  //{{
  {
    jfree(data);
  }
  //}}

  void write_short(unsigned short x)
  //{{{
  { 
    x=int_to_local(x);
    write(&x,2);
  }
  //}}

  void write_long(unsigned long x) 
  //{{{
  { 
    x=long_to_local(x);
    write(&x,4);
  }
  //}}}

  unsigned short read_short()
  //{{{
  {
    unsigned short x;
    read(&x,2); 
    return int_to_local(x);
  }
  //}}}

  unsigned long read_long()
  //{{{
  {
    unsigned long x;
    read(&x,4); 
    return long_to_local(x);
  }
  //}}

  long read(void *buf, int rsize)
  //{{{
  {
    if (rp+rsize>wp)
      rsize=wp-rp;
    memcpy(buf,data+rp,rsize);
    rp+=rsize;
    return rsize;
  }
  //}}

  long write(void *buf, int wsize)
  //{{
  {
    if (wp+wsize>size)
      wsize=size-wp;
    memcpy(data+wp,buf,wsize);
    wp+=wsize;
    return wsize;
  }
  //}}

  void write_byte(unsigned char c)
  //{{{
  {
    if (wp+1<=size)
      data[wp++]=c;
  }
  //}}

  unsigned char read_byte()
  //{{
  {
    if (rp+1<=wp)
      return data[rp++];
     return 0;
  }
  //}}

} ;


#endif
//{{{ Emacs Locals
// Local Variables:
// folded-file: t
// End:
//}}}
