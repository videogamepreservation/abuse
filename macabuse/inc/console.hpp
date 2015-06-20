#ifndef __CONSOLE_HPP_
#define __CONSOLE_HPP_
#include "jwindow.hpp"

class console
{
  protected :
  int lastx,lasty,w,h,cx,cy;
  JCFont *fnt;
  char *screen;
  jwindow *con_win;
  window_manager *wm;
  char *name;
  public :

  console(window_manager *WM, JCFont *font, int width, int height, char *Name);
  int showing() { return con_win!=NULL; }
  void show();
  void hide();
  void redraw();
  void put_char(char ch);
  void do_cr();
  int screen_w() { return w*fnt->width(); }
  int screen_h() { return h*fnt->height(); }
  int wx() { return con_win->x1(); }
  int wy() { return con_win->y1(); }
  void draw_cursor();
  void put_string(char *st);
  void draw_char(int x, int y, char ch);
  void toggle() { if (con_win) hide(); else show(); }
  void printf(const char *format, ...);
  ~console();
} ;

class shell_term : public console
{
  char shcmd[300];
  public :
  shell_term(window_manager *WM, JCFont *font, int width, int height, char *Name);
  int handle_event(event &ev, window_manager *wm);
  virtual void prompt();
  virtual void execute(char *st);
} ;

#endif
