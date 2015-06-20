#ifndef __GU_STAT_HPP
#define __GU_STAT_HPP
#include "status.hpp"
#include "jwindow.hpp"
#include <string.h>

class gui_status_node;
class gui_status_manager : public status_manager
{
  window_manager *wm;
  char title[40];
  int last_perc;
  public :   
  gui_status_node *first;
  gui_status_manager(window_manager *WM);
  virtual void push(char *name, visual_object *show);
  virtual void update(int percentage);
  virtual void pop();
  void draw_bar(gui_status_node *whom, int perc);
  void set_window_title(char *name) { strncpy(title,name,39); }
  virtual void force_display();
} ;

#endif
