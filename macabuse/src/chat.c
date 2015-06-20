#include "chat.hpp"

extern char *symbol_str(char *name);

chat_console::chat_console(window_manager *WM, JCFont *font, int width, int height) :
  console(WM,font,width,height<4 ? 4 : height,symbol_str("CHAT"))
{
  clear();
  cx=0;
  cy=h-1;
  lastx=xres/2-screen_w()/2;
  lasty=yres-screen_h()-WINDOW_FRAME_TOP-WINDOW_FRAME_BOTTOM;
}

chat_console *chat=NULL;

void chat_console::clear()
{
  memset(screen,' ',w*h);
  memset(screen+w*(h-2),'-',w);
  redraw();
}

void chat_console::put_all(char *st)
{
  memmove(screen,screen+w,w*(h-3));
  memset(screen+w*(h-3),' ',w);  
  memcpy(screen+w*(h-3),st,strlen(st));
  redraw();
}


void chat_console::draw_user(char *st)
{
  memset(screen+w*(h-1),' ',w);
  memcpy(screen+w*(h-1),st,strlen(st));
  cx=strlen(st);
  cy=h-1;
  redraw();
}

