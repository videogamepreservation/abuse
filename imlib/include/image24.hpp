#ifndef __IMAGE_24__
#define __IMAGE_24__

#include "palette.hpp"
#include "macs.hpp"
#include "filter.hpp"

class image24
{
  int w,h;
  unsigned char *data;  
public :
  int width() { return w; }
  int height() { return h; }
  image24(unsigned short width, unsigned short height,
          unsigned char *buffer=NULL);          
  void pixel(short x, short y, unsigned char &r, 
                               unsigned char &g,
			       unsigned char &b)
    { CHECK(x>=0 && y>=0 && x<w && y<h);
      unsigned char *p=data+y*w*3; r=*(p++); g=*(p++); b=*(p++); }
  void putpixel(short x, short y, unsigned char r,
				  unsigned char g, 
			          unsigned char b)
    { CHECK(x>=0 && y>=0 && x<w && y<h);
      unsigned char *p=data+(y*w+x)*3; *(p++)=r; *(p++)=g; *(p++)=b; }
  unsigned char *scan_line(short y) { return data+y*w*3; }
  image *dither(palette *pal);
  void           clear              (unsigned char r=0, unsigned char g=0, 
               			     unsigned char b=0);   
  void add_error(int x, int y, int r_error, int g_error, int b_error, int error_mult);  
  ~image24() { jfree(data); }
} ;


#endif

