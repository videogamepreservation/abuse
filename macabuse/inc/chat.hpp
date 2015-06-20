#ifndef CHAT_HPP_
#define CHAT_HPP_

#include "console.hpp"

class chat_console : public console
{

  public :
  int chat_event(event &ev) { if (!con_win) return 0; else return con_win==ev.window; }
  void draw_user(char *st);
  void put_all(char *st);
  void clear();
  chat_console(window_manager *WM, JCFont *font, int width, int height);
  
} ;

extern chat_console *chat;

#endif


