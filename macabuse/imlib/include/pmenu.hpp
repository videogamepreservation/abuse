#ifndef __PMENU_HPP_
#define __PMENU_HPP_

#include "jmalloc.hpp"
#include "input.hpp"

class psub_menu;

class pmenu_item
{
  int hotkey;
public :
  char *n,*on_off;
  psub_menu *sub;

  int id,xp;
  pmenu_item *next;
  pmenu_item(int ID, char *name, char *on_off_flag, int Hotkey, pmenu_item *Next);
  pmenu_item(char *Name, psub_menu *Sub, pmenu_item *Next, int xpos=-1);
  char *name() { return n; }
  pmenu_item *find_id(int search_id);
  pmenu_item *find_key(int key);
  void draw       (jwindow *parent, int x, int y, int w, int top, window_manager *wm, int active);
  void draw_self  (jwindow *parent, int x, int y, int w, int top, window_manager *wm, int active);
  int handle_event(jwindow *parent, int x, int y, int w, int top, window_manager *wm, event &ev);
  int own_event(event &ev);
  ~pmenu_item();
} ;


class psub_menu
{
  pmenu_item *first;
  psub_menu *next;
  int active;
  window_manager *wm;
  jwindow *win;
  pmenu_item *item_num(int x) { pmenu_item *p=first; while (x-- && p) p=p->next; return p; }
public :
  void calc_size(int &w, int &h, window_manager *wm);
  pmenu_item *find_id(int search_id);
  pmenu_item *find_key(int key);
  psub_menu(pmenu_item *First, psub_menu *Next) 
  { first=First; Next=Next; win=0; active=0; }
  int handle_event(jwindow *parent, int x, int y, window_manager *wm, event &ev);
  void draw(jwindow *parent, int x, int y, window_manager *wm);
  void hide(jwindow *parent, int x, int y, window_manager *wm);
  int own_event(event &ev);
  ~psub_menu();
} ;

class pmenu
{
  window_manager *wm;
  jwindow *bar;
  pmenu_item *top,*active;
  int itemw(pmenu_item *p, window_manager *wm) 
  { return strlen(p->name())*wm->frame_font()->width()+2; }
  int itemx(pmenu_item *p, window_manager *wm);
  pmenu_item *inarea(int mx, int my, image *screen, window_manager *wm);
public :
  ~pmenu();
  pmenu(int X, int Y, pmenu_item *first, image *screen, window_manager *wm);
  void move(int new_x, int new_y);
  void draw(image *screen, window_manager *wm, int top_only=0);
  int handle_event(event &ev, image *screen, window_manager *wm);  

} ;

#endif


