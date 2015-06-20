#ifndef __SEQUENCE_HPP_
#define __SEQUENCE_HPP_

#include "image.hpp"
#include "items.hpp"
#include "timage.hpp"
#include "cache.hpp"
#include <stdarg.h>

class sequence
{
  int total;
  int *seq;         // array of ids to figures
public :
  // takes a varible number of arguments (ints) specifing indexes into image array
  sequence(int *figures, int total_frames) { total=total_frames; seq=figures; }

  sequence(char *filename, void *pict_list, void *advance_list);

  int next_frame(short &current) { current++; if (current>=total) { current=0; return 0; } return 1; }
  int last_frame(short &current) { current--; if (current<0) { current=total-1; return 0; } return 1; }
  trans_image  *get_frame(short current, int direction) 
   { if (direction>0) return cash.fig(seq[current])->forward; 
                 else return cash.fig(seq[current])->backward; }
  figure *get_figure(short current) { return cash.fig(seq[current]); }
  int cache_in();
  int x_center(short current) { return (short) (cash.fig(seq[current])->xcfg); }
  int length() { return total; }
  int get_advance(int current) { return cash.fig(seq[current])->advance; }
  int size();
  ~sequence();
};

#endif


