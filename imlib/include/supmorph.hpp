#ifndef __SUPER_MORPH_HPP__
#define __SUPER_MORPH_HPP__
#include "jmalloc.hpp"
#include "timage.hpp"

class super_morph
{
public :
  int t;
  unsigned char *movers;
  int w,h;
  super_morph(trans_image *h1, trans_image *h2, int aneal_steps, void (*stat_fun)(int));  
  ~super_morph() { if (t) jfree(movers); }
} ;


struct stepper
{
  long x,y,r,g,b,dx,dy,dr,dg,db;
} ;

class smorph_player
{
  stepper *steps;
  unsigned char *hole;
public :
  int w,h,f_left,t;
  smorph_player(super_morph *m, palette *pal, image *i1, image *i2, int frames, int dir);
  int show(image *screen, int x, int y, color_filter *fil, palette *pal, int blur_threshold);
  ~smorph_player() { jfree(hole); jfree(steps);  }
} ;


#endif
