#ifndef __PARTICLE_HPP_
#define __PARTICLE_HPP_

#include "specs.hpp"
#include "image.hpp"

class view;

int defun_pseq(void *args);
void add_panim(int id, long x, long y, int dir);
void delete_panims();      // called by ~level
void draw_panims(view *v);
void tick_panims();
void free_pframes();

struct part
{
  short x,y;
  uchar color;
} ;

class part_frame
{
  public :
  int t,x1,y1,x2,y2;
  part *data;
  part_frame(bFILE *fp);
  void draw(image *screen, int x, int y, int dir);
  ~part_frame();
} ;

class part_sequence
{
  public :
  int tframes;
  int *frames;  // array of id's
  part_sequence(void *args);
  ~part_sequence() { if (tframes) jfree(frames); }
} ;

class part_animation
{
  public :
  part_animation *next;
  part_sequence *seq;
  int frame,dir;
  long x,y;
  part_animation(part_sequence *s, long X, long Y, int Dir, part_animation *Next)
  { x=X; y=Y; seq=s; next=Next; frame=0; dir=Dir; }
} ;


#endif


