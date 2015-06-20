#ifndef __TOOLS_HPP_
#define __TOOLS_HPP_

#include "jwindow.hpp"
#include "input.hpp"
#include "specs.hpp"
#include "scroller.hpp"
#include "visobj.hpp"

class tool_picker : public spicker
{
  filter *map;
  visual_object **icons; 
  int *ids;
  int total_icons;
  int iw,ih;
  palette *old_pal;

  public :

  // you are expected keep image and id list in memory, tool_picker does not copy them
  tool_picker(int X, int Y, int ID, 
	      int show_h, visual_object **Icons, int *Ids, int total_ic, 
	      palette *icon_palette, palette *pal, window_manager *wm, ifield *Next);

  virtual void draw_item(window_manager *wm, image *screen, int x, int y, int num, int active);
  virtual int total() { return total_icons; }
  virtual int item_width(window_manager *wm) { return iw; } 
  virtual int item_height(window_manager *wm) { return ih; }
  virtual void note_new_current(window_manager *wm, image *screen, input_manager *inm, int x) 
  { wm->push_event(new event(ids[x],NULL)); }

  void remap(palette *pal, window_manager *wm, image *screen);
  ~tool_picker();
} ;


#endif







