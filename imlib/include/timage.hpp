#ifndef __TIMAGE_HPP_
#define __TIMAGE_HPP_

#include "image.hpp"
#include "macs.hpp"
#include "palette.hpp"
#include "filter.hpp"
#include "jmalloc.hpp"

/* data is stored in the following format

  skip amount, data size, data  // no scan line wraps allowed

*/


class trans_image       // transpernet image
{
  unsigned char *data;
  short w,h;
  
public :  
  short height() { return h; }
  short width() { return w; }  
  trans_image(image *im, char *name);  // name has no meaning if MEM_CHECK is off
  void put_image(image *screen, int x, int y);   // always transparent   

  // if screen x & y offset already calculated save a mul
  // and no clipping, but fast use this
  void put_image_offseted(image *screen, uchar *s_off);   
  void put_image_filled(image *screen, int x, int y, 
			uchar fill_color);
  void put_fade(image *screen, int x, int y,
			   int frame_on, int total_frames, 
			   color_filter *f, palette *pal);
  void put_fade_tint(image *screen, int x, int y,
		     int frame_on, int total_frames, 
		     uchar *tint,
		     color_filter *f, palette *pal);
  void put_color(image *screen, int x, int y, int color);
  unsigned char *clip_y(image *screen, int x1, int y1, int x2, int y2, 
	                               int x, int &y, int &ysteps);

  void put_blend16(image *screen, image *blend, int x, int y, 
	           int blendx, int blendy, int blend_amount, color_filter *f, palette *pal);
  void put_double_remaped(image *screen, int x, int y, unsigned char *remap, unsigned char *remap2);
  void put_remaped(image *screen, int x, int y, unsigned char *remap);
  void put_predator(image *screen, int x, int y);
  void put_scan_line(image *screen, int x, int y, int line);   // always transparent   
  unsigned char *t_data() { return data; }
  void make_color(int c);
  int size();
  image *make_image();
  ~trans_image() { jfree(data); }
} ;


#endif





