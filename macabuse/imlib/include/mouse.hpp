#ifndef __MOUSE_HPP_
#define __MOUSE_HPP_
#include "image.hpp"
#include "sprite.hpp"

class JCMouse
{
  int here,but;
  sprite *sp;
  int lx,ly,lbut,mx,my;
  int cx,cy;                       // center of mouse cursor
public :
  JCMouse(image *Screen, palette *pal);
  void set_shape(image *im, int centerx=0, int centery=0);
  void update(int newx=-1, int newy=-1, int new_but=-1);
  void set_shape(image *im) { if (here) sp->change_visual(im); }
  int x() { if (here) return mx; else return 0; }
  int y() { if (here) return my; else return 0; }
  int drawx() { return mx-cx; }
  int drawy() { return my-cy; }
  int lastx() { if (here) return lx; else return 0; }
  int lasty() { if (here) return ly; else return 0; }
  int last_button() { if (here) return lbut; else return 0; }
  int button() { return but; }
  int exsist() { return here; }
  int center_x() { return cx; }
  int center_y() { return cy; }

  sprite *mouse_sprite() { return sp; }
  void set_position(int new_mx, int new_my);
  ~JCMouse();
  
#ifdef __MAC__
	int set_button(int b) { return (but = b); }
#endif
} ;

#endif

