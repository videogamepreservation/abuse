#ifndef _FILTER_HPP
#define _FILTER_HPP
#include "image.hpp"
#include "palette.hpp"
#include "specs.hpp"
#include "jmalloc.hpp"

class filter
{
  unsigned char *fdat;
  int nc;
public :
  filter(int colors=256);
  filter(palette *from, palette *to);     // creates a conversion filter from one palette to another
  void set(int color_num, char change_to);
  unsigned char get_mapping(int color_num) { return fdat[color_num]; }
  void apply(image *im);
  void max_threshold(int minv, char blank=0);
  void min_threshold(int maxv, char blank=0);
  void put_image(image *screen, image *im, short x, short y, char transparent=0);
  void clear();
  ~filter() { jfree(fdat); }
} ;

class color_filter
{
  unsigned char *color_table;
public:
  int size();
  int write(bFILE *fp);
  color_filter(spec_entry *e, bFILE *fp);
  color_filter(palette *pal, int color_bits=6, void (*stat_fun)(int)=NULL);
  unsigned char lookup_color(int r, int g, int b) 
   { return color_table[r*32*32+g*32+b]; }
  unsigned char *table() { return color_table; }
  int total_colors() { return 32; }
  unsigned char *get_table() { return color_table; }
  ~color_filter() { jfree(color_table); }
} ;

#endif




