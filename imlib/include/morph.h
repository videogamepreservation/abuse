#ifndef __MORPH_H_
#define __MORPH_H_

#include "image.hpp"
struct point
{
  short sx,sy,ex,ey;    /* starting x -ending x */
  unsigned char sc,ec;             /* start/ending color */
};

struct bmap
{
  unsigned l,h;
  char *data;
} ;


/* bm is an array [1..2] of the struct bmap - they will not be changed */
/* bm[0] is the staring morph bitmap which sx,sy  and sc refer to
   bm[1] is the ending ----                ex,sy and ec ---
  numpoints returns the number of points.
  The function returns an array (which it allocates) of point (note above
  struct)
  bcolor is the background */

extern struct point *morph(image **bm, char bcolor, int *num_points);
#endif
/*  Kiss off ok,   ok, ok, ok.. */














