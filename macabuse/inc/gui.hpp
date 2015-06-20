#ifndef __GUI_HPP_
#define __GUI_HPP_
#include "jwindow.hpp"


class ico_button : public ifield
{
  int up,act,u,d,ua,da;  // up, down, up active, down active
  int activate_id;         // sent when if not -1 when object receives a draw actove
  char key[16];
public : 
  ico_button(int X, int Y, int ID, int Up, int down, int upa, int downa, ifield *Next, int act_id=-1, char *help_key=NULL);

  virtual void area(int &x1, int &y1, int &x2, int &y2, window_manager *wm);
  virtual void draw_first(image *screen, window_manager *wm) { draw(0,screen,wm); }
  virtual void draw(int active, image *screen, window_manager *wm); 
  virtual void handle_event(event &ev, image *screen, window_manager *wm, input_manager *im);

  virtual char *read() { return (char *)&up; }
  void set_xy(int X, int Y) { x=X; y=Y; }
  int X() { return x; }
  int Y() { return y; }
  int status() { return up; }
  void set_act_id(int id);
} ;

class ico_switch_button : public ifield
{
  ifield *blist,*cur_but;
  int act;
  public :
  ico_switch_button(int X, int Y, int ID, int start_on, ifield *butts, ifield *Next);
  virtual void area(int &x1, int &y1, int &x2, int &y2, window_manager *wm);
  virtual void draw_first(image *screen, window_manager *wm) { cur_but->draw_first(screen,wm); }
  virtual void draw(int active, image *screen, window_manager *wm) { cur_but->draw(active,screen,wm); act=active; }
  virtual void handle_event(event &ev, image *screen, window_manager *wm, input_manager *im);
  virtual ifield *unlink(int id);
  virtual char *read() { return cur_but->read(); }
  ~ico_switch_button();
} ; 

#endif


