#ifndef __ITEMS_HPP__
#define __ITEMS_HPP__
#include "image.hpp"
#include "specs.hpp"
#include "points.hpp"
#include "timage.hpp"
#include <stdio.h>
#include <stdlib.h>

#define AUTOTILE_WIDTH 6
#define AUTOTILE_HEIGHT 3

class boundary : public point_list      //  a list of points with 
{
public :  
  boundary(bFILE *fp,char *er_name);
  unsigned char *inside;     // tells which side of the line is on the inside
  boundary(boundary *p);      // flips the *inside list
  ~boundary() { if (tot) jfree(inside); }
} ;

class backtile
{
public :
  unsigned short next;
  image *im;
  backtile(spec_entry *e, bFILE *fp);
  backtile(bFILE *fp);
  long size() { return 2+4+im->width()*im->height(); }
  ~backtile() { delete im; }
} ;

class foretile
{
public :
  trans_image *im;
  unsigned short next;
  unsigned char damage;
  uchar ylevel;            // for fast intersections, this is the y level offset for the ground
                           // if ground is not level this is 255
  boundary *points;

  image *micro_image;

  foretile(bFILE *fp);
  long size() { return im->width()*im->height()+4+2+1+points->size(); }
  ~foretile() { delete im; delete points; delete micro_image; }
} ;

class figure
{
public :
  trans_image *forward,*backward;
  unsigned char hit_damage,xcfg;
  signed char advance;
  point_list *hit;
  boundary *f_damage,*b_damage;
  int size();

  figure(bFILE *fp, int type);
  int width() { return forward->width(); }
  int height() { return forward->height(); }

/*  long size(int type)         // taken from spaint items
  {
    if 
    return forward->width()*backward->height()+4+
                       1+1+       // hit & xcfg
		       touch->size()+
		       hit->size()+
		       damage->size(); 
  }*/

  ~figure() { delete forward; delete backward; 
	      delete hit;
	      delete f_damage; delete b_damage; }
} ;

class char_tint
{
  public :
  uchar data[256];
  ~char_tint() { ; }
  char_tint(bFILE *fp);               // should be a palette entry
} ;

#endif
















