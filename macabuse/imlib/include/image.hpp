#ifndef _IMGAE_HPP_
#define _IMGAE_HPP_
#include <stdlib.h>
#include "linked.hpp"
#include "palette.hpp"
#include "system.h"
#include "specs.hpp"
#include "dprint.hpp"
#define MAX_DIRTY 200
#define Inew(pointer,type); { make_block(sizeof(type)); pointer=new type; }

extern char  *imerr_messages[];  // correspond to imERRORS
#define imREAD_ERROR 	       1
#define imINCORRECT_FILETYPE   2
#define imFILE_CORRUPTED       3
#define imFILE_NOT_FOUND       4
#define imMEMORY_ERROR         5
#define imNOT_SUPPORTED        6
#define imWRITE_ERROR          7
#define imMAX_ERROR 	       7

short current_error();
void clear_errors();
void set_error(short x);
short last_error();
void make_block(size_t size);
void image_init();
void image_uninit();
extern linked_list image_list;


class filter;


class dirty_rect : public linked_node
{
public :
  short dx1,dy1,dx2,dy2;
  dirty_rect(short x1, short y1, short x2, short y2)
  { dx1=x1; dy1=y1; dx2=x2; dy2=y2; 
    if (x2<x1 || y2<y1) 
      dprintf("add incorrect dirty\n");
  }
  virtual short compare(void *n1, short field)
  { return ((dirty_rect *)n1)->dy1>dy1; }
} ;

class image_descriptor
{
  short l,h;
  short clipx1, clipy1, clipx2, clipy2;
public :  
  unsigned char keep_dirt,
	        static_mem;      // if this flag is set then don't free memory on exit
  
  linked_list dirties;
  void *extended_descriptor;              // type depends on current system

  image_descriptor(short length, short height,
		  int keep_dirties=1, int static_memory=0);
  short bound_x1(short x1)  { return x1<clipx1 ? clipx1 : x1; }
  short bound_y1(short y1)  { return y1<clipy1 ? clipy1 : y1; }
  short bound_x2(short x2)  { return x2>clipx2 ? clipx2 : x2; }
  short bound_y2(short y2)  { return y2>clipy2 ? clipy2 : y2; }
  short x1_clip() { return clipx1; }
  short y1_clip() { return clipy1; }
  short x2_clip() { return clipx2; }
  short y2_clip() { return clipy2; }
  void dirty_area(short x1, short y1, short x2, short y2) { ;}
  void clean_area(short x1, short y1, short x2, short y2) { ; }
  void clear_dirties();
  short get_dirty_area(short &x1, short &y1, short &x2, short &y2) { return 0; }
  void get_clip(short &x1, short &y1, short &x2, short &y2)
    { x1=clipx1; y1=clipy1; x2=clipx2; y2=clipy2; }
  void set_clip(short x1, short y1, short x2, short y2)
    { if (x2<x1) x2=x1;
      if (y2<y1) y2=y1;
      if (x1<0) clipx1=0; else clipx1=x1;
      if (y1<0) clipy1=0; else clipy1=y1;
      if (x2>=l) clipx2=l-1; else clipx2=x2;
      if (y2>=h) clipy2=h-1; else clipy2=y2;
    }
  void reduce_dirties();
  void add_dirty(int x1, int y1, int x2, int y2);
  void delete_dirty(int x1, int y1, int x2, int y2);
  void resize(short length, short height)
   { l=length; h=height; clipx1=0; clipy1=0; clipx2=l-1; clipy2=h-1; }
} ;

class image : public linked_node
{ 
  void make_page(short width, short height, unsigned char *page_buffer);
  void delete_page();
public :
  short w,h;
  unsigned char *data;
	void HackW(short _w) { w = _w; }
	void HackH(short _h) { h = _h; }



  image_descriptor *special;
  image(spec_entry *e, bFILE *fp);
  image(bFILE *fp);
  image(short width, short height,                 // required
	unsigned char *page_buffer=NULL,
	short create_descriptor=0);        // 0=no, 1=yes, 2=yes & keep dirties
  unsigned char  pixel              (short x, short y);
  void           putpixel           (short x, short y, char color);
  unsigned char *scan_line          (short y) { return data+y*w; }
  unsigned char *next_line          (short lasty, unsigned char *last_scan) 
                                    { return last_scan+w; }          
  long           total_pixels       (unsigned char background=0);
  image         *copy               ();    // makes a copy of an image
  void           clear              (short color=-1);  // -1 is background color
  void           to_24bit           (palette &pal);
  short          width              () { return (short)w; }
  short          height             () { return (short)h; }
  void           scroll             (short x1, short y1, short x2, short y2, short xd, short yd);
  void           fill_image         (image *screen, short x1, short y1, short x2, short y2, 
				     short allign=1);
  void           put_image          (image *screen, short x, short y, char transparent=0);
  void           put_part           (image *screen, short x, short y, short x1, short y1, 
				     short x2, short y2, char transparent=0);
  void           put_part_xrev      (image *screen, short x, short y, short x1, short y1, 
				     short x2, short y2, char transparent=0);
  void           put_part_masked    (image *screen, image *mask, short x, short y, 
				     short maskx, short masky, short x1, short y1, short x2, short y2);
  image         *copy_part_dithered (short x1, short y1, short x2, short y2);
  void           bar                (short x1, short y1, short x2, short y2, unsigned char color);
  void           xor_bar            (short x1, short y1, short x2, short y2, unsigned char color);
  void 	         wiget_bar          (short x1, short y1, short x2, short y2, 
				     unsigned char light, unsigned char med, unsigned char dark);
  void           line               (short x1, short y1, short x2, short y2, unsigned char color);
  void           rectangle          (short x1, short y1, short x2, short y2, unsigned char color);
  void           burn_led           (short x, short y, long num, short color, short scale=1);
  void           set_clip           (short x1, short y1, short x2, short y2);
  void           get_clip           (short &x1,short &y1,short &x2,short &y2);
  void           in_clip            (short x1, short y1, short x2, short y2);

  void           dirt_off           () { if (special && special->keep_dirt) special->keep_dirt=0; }
  void           dirt_on            () { if (special) special->keep_dirt=1; }

  void           add_dirty          (int x1, int y1, int x2, int y2) 
                                    { if (special) special->add_dirty(x1,y1,x2,y2); }
  void           delete_dirty       (int x1, int y1, int x2, int y2) 
                                    { if (special) special->delete_dirty(x1,y1,x2,y2); }
  void           clear_dirties      () { if (special) special->clear_dirties(); }
  void           dither             (palette *pal); // use a b&w palette!
  void           resize             (short new_width, short new_height);
  void           change_size        (short new_width, short new_height, unsigned char *page=NULL);
  void           flood_fill         (short x, short y, unsigned char color);
  image         *create_smooth      (short smoothness=1); // 0 no smoothness
  void           unpack_scanline    (short line, char bitsperpixel=1);
  unsigned char  brightest_color    (palette *pal);
  void           flip_x	  	    ();
  void           flip_y             ();
  void           make_color         (unsigned char color);
  unsigned char  darkest_color      (palette *pal, short noblack=0);

  ~image();
} ;


class image_controller
{
public :
  image_controller() { image_init(); }
  ~image_controller()
  { 
     image_uninit(); 
  }
} ;



#endif








