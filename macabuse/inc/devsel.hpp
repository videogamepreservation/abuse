#ifndef __DEVSCROLL_HPP_
#define __DEVSCROLL_HPP_
#include "scroller.hpp"

class tile_picker : public scroller
{
  int th,last_sel,type,scale,wid,rev;
  public :
  tile_picker(int X, int Y, int ID, int spec_type, window_manager *wm, 
		 int scale, int scroll_h, int Wid, ifield *Next);
  virtual void handle_inside_event(event &ev, image *screen, window_manager *wm, input_manager *inm);
  virtual void scroll_event(int newx, image *screen, window_manager *wm);
  virtual char *read() { return (char *)this; }
  int picw();
  int pich();
  int total();
  int get_current();
  void set_current(int x);
  void recenter(image *screen, window_manager *wm);
  void reverse() { rev=!rev; }
} ;

extern int cur_bg,cur_fg,cur_char;

#endif

