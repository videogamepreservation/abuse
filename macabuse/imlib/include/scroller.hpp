#ifndef _SCROLLER_HPP_
#define _SCROLLER_HPP_

#include "input.hpp"

class scroller : public ifield
{
protected :
  int l,h,drag,vert,last_click;

  int bh();
  int bw();
  void drag_area(int &x1, int &y1, int &x2, int &y2);
  void dragger_area(int &x1, int &y1, int &x2, int &y2);
  int b1x() { if (vert) return x+l; else return x; }
  int b1y() { if (vert) return y; else return y+h; }
  int b2x() { if (vert) return x+l; else return x+l-bw(); }
  int b2y() { if (vert) return y+h-bh(); else return y+h; }
  unsigned char *b1();
  unsigned char *b2();
  void wig_area(int &x1, int &y1, int &x2, int &y2);


  int wig_x();
  int darea() { return (l-bw()-2)-bw()-bw(); }
  void draw_wiget(image *screen, window_manager *wm, int erase);
  int mouse_to_drag(int mx,int my);
public :
  int t,sx;
  scroller(int X, int Y, int ID, int L, int H, int Vert, int Total_items, ifield *Next);
  virtual void area(int &x1, int &y1, int &x2, int &y2, window_manager *wm);
  virtual void draw_first(image *screen, window_manager *wm);
  virtual void draw(int active, image *screen, window_manager *wm);
  virtual void handle_event(event &ev, image *screen, window_manager *wm, input_manager *im);
  virtual char *read() { return (char *)&sx; }

  virtual int activate_on_mouse_move() { return 1; }
  virtual void handle_inside_event(event &ev, image *screen, window_manager *wm, input_manager *inm) {;}
  virtual void scroll_event(int newx, image *screen, window_manager *wm);
  virtual void handle_up(image *screen, window_manager *wm, input_manager *inm);
  virtual void handle_down(image *screen, window_manager *wm, input_manager *inm);
  virtual void handle_left(image *screen, window_manager *wm, input_manager *inm);
  virtual void handle_right(image *screen, window_manager *wm, input_manager *inm);
  virtual void area_config(window_manager *wm) { ; }
  void set_size(int width, int height) { l=width; h=height; }
  virtual void set_x(int x, image *screen, window_manager *wm);
} ;

class spicker : public scroller
{
  protected :
  int r,c,m,last_sel,cur_sel;
  uchar *select;
  public :
  spicker(int X, int Y, int ID, int Rows, int Cols, int Vert, int MultiSelect, ifield *Next);
  int vis() { if (vert) return r; else return c; }
  virtual void area_config(window_manager *wm);
  void set_select(int x, int on);
  int get_select(int x);
  int first_selected();
  virtual void scroll_event(int newx, image *screen, window_manager *wm);
  virtual void handle_inside_event(event &ev, image *screen, window_manager *wm, input_manager *inm);

  // you should define \/
  virtual void draw_background(window_manager *wm, image *screen);
  virtual void draw_item(window_manager *wm, image *screen, int x, int y, int num, int active) = 0;
  virtual int total() = 0;
  virtual int item_width(window_manager *wm) = 0;
  virtual int item_height(window_manager *wm) = 0;
  virtual void note_selection(window_manager *wm, image *screen, input_manager *inm, int x) { ; }
  virtual void note_new_current(window_manager *wm, image *screen, input_manager *inm, int x) { ; }
  virtual int ok_to_select(int num) { return 1; }
  virtual void handle_up(image *screen, window_manager *wm, input_manager *inm);
  virtual void handle_down(image *screen, window_manager *wm, input_manager *inm);
  virtual void handle_left(image *screen, window_manager *wm, input_manager *inm);
  virtual void handle_right(image *screen, window_manager *wm, input_manager *inm);
  virtual void set_x(int x, image *screen, window_manager *wm);
  void reconfigure();   // should be called by constructor after class is ready to take virtual calls
  ~spicker() { if (select) jfree(select); }
} ; 

struct pick_list_item
{
  char *name;
  int number;
} ;

class pick_list : public scroller
{
  int last_sel,cur_sel,th,wid;
  pick_list_item *lis;
  char key_hist[20],key_hist_total;
  image *tex;
  public :
  pick_list(int X, int Y, int ID, int height,
	    char **List, int num_entries, int start_yoffset, ifield *Next, image *texture=NULL);
  virtual void handle_inside_event(event &ev, image *screen, window_manager *wm, input_manager *inm);
  virtual void scroll_event(int newx, image *screen, window_manager *wm);
  virtual char *read() { return (char *)this; }
  virtual void area_config(window_manager *wm);
  virtual void handle_up(image *screen, window_manager *wm, input_manager *inm);
  virtual void handle_down(image *screen, window_manager *wm, input_manager *inm);
  int get_selection() { return lis[cur_sel].number; }
  ~pick_list() { jfree(lis); }
} ;

#endif





