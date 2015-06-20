#include "crc.hpp"

unsigned short calc_crc(unsigned char *buf, long len)
{
  unsigned char c1=0,c2=0;
  while (len)
  {
    len--;
    c1+=*buf; 
    c2+=c1;
    buf++;
  }
  return (c2<<8)|c1;
}


 
ulong crc_file(bFILE *fp)
{
  uchar crc1=0,crc2=0,crc3=0,crc4=0;

  int size=0x1000;
  uchar *buffer=(uchar *)jmalloc(size,"crc_buffer"),*c;
  long l=fp->file_size();
  long cur_pos=fp->tell();
  fp->seek(0,0);
  while (l)
  {
    int nr=fp->read(buffer,size);
    if (nr==0) l=0;
    else
    {
      l-=nr;
      for (c=buffer;nr;nr--,c++)
      {      
	crc1+=*c;
	crc2+=crc1;
	crc3+=crc2;
	crc4+=crc3;
      }
    }
  }
  fp->seek(cur_pos,0);
  jfree(buffer);
  return (crc1|(crc2<<8)|(crc3<<16)|(crc4<<24));
}
