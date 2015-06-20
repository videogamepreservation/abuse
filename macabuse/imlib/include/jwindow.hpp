#ifndef __JWIN__
#define __JWIN__
#include "video.hpp"
#include "image.hpp"
#include "event.hpp"
#include "filter.hpp"
#include "event.hpp"
#include "fonts.hpp"

class ifield;
class window_manager;
class jwindow;

int frame_top();
int frame_bottom();
int frame_left();
int frame_right();

void set_frame_size(int x);

#define WINDOW_FRAME_TOP  frame_top()
#define WINDOW_FRAME_BOTTOM frame_bottom()
#define WINDOW_FRAME_LEFT  frame_left()
#define WINDOW_FRAME_RIGHT frame_right()


class input_manager
{
  ifield *first,*active,*grab;
  window_manager *wm;
  jwindow *cur;
  int no_selections_allowed;
  image *screen;
public :
  input_manager(image *Screen, window_manager *WM, ifield *First);
  void handle_event(event &ev, jwindow *j, window_manager *wm);
  ifield *get(int id);
  void redraw();
  void add(ifield *i);
  void remap(filter *f);
  ifield *unlink(int id);     // unlinks ID from fields list and return the pointer to it
  ifield *current() { return active; }
  void clear_current();
  void grab_focus(ifield *i);
  void release_focus();
  void allow_no_selections();
  void next_active(image *screen, window_manager *wm);
  ~input_manager(); 
} ;

class ifield
{
public :
  int x,y;
  int id;
  ifield *next;
  virtual void area(int &x1, int &y1, int &x2, int &y2, window_manager *wm) = 0;
  virtual void draw_first(image *screen, window_manager *wm)              = 0;
  virtual void draw(int active, image *screen, window_manager *wm)        = 0;
  virtual void handle_event(event &ev, image *screen, window_manager *wm, input_manager *im) = 0;
  virtual int selectable() { return 1; }
  virtual void remap(filter *f) { ; }
  virtual char *read() = 0;
  virtual ifield *find(int search_id) { if (id==search_id) return this; else return NULL; }
  virtual ifield *unlink(int id) { return NULL; } 
  virtual ~ifield();
} ;

struct jwindow_properties
{
  uchar moveable,
        hidden;
  
} ;


class jwindow
{
  
public:
  image *screen;
  jwindow_properties property;
  jwindow *next;
  char *name;
  int x,y,l,h,backg;
  input_manager *inm;
  void *local_info;     // pointer to info block for local system (may support windows)
  jwindow(int X, int Y, int L, int H, window_manager *wm, ifield *fields, char *Name=NULL);
  void redraw(int hi, int med, int low, JCFont *fnt);
  void resize(int L, int H);
  void clear(int color=0) { screen->bar(x1(),y1(),x2(),y2(),color); }
  int x1() { return WINDOW_FRAME_LEFT; }
  int y1() { return WINDOW_FRAME_TOP; } 
  int x2() { return l-WINDOW_FRAME_RIGHT-1; }
  int y2() { return h-WINDOW_FRAME_BOTTOM-1; } 
  void clip_in() { screen->set_clip(x1(),y1(),x2(),y2()); }
  void clip_out() { screen->set_clip(0,0,l-1,h-1); }
  char *read(int id) { return inm->get(id)->read(); }  
  void local_close();
  void set_moveability(int x);
  ~jwindow() { local_close(); delete screen; delete inm; jfree(name); }
} ;

class window_manager
{
public :
  event_handler *eh;
  jwindow *first,*grab;
  image *mouse_pic,*mouse_save;
  palette *pal;
  int hi,med,low,bk;                            // bright, medium, dark and black colors
  int key_state[512];
  enum { inputing,dragging } state;
  int drag_mousex, drag_mousey,frame_suppress;
  jwindow *drag_window;
  JCFont *fnt,*wframe_fnt;

  window_manager(image *Screen, palette *Pal, int Hi, int Med, int Low, JCFont *Font); 

  jwindow *new_window(int x, int y, int l, int h, ifield *fields=NULL, char *Name=NULL);
  void set_frame_font(JCFont *fnt) { wframe_fnt=fnt; }
  JCFont *frame_font() { return wframe_fnt; }
  void close_window(jwindow *j);
  void resize_window(jwindow *j, int l, int h);
  void move_window(jwindow *j, int x, int y);
  void get_event(event &ev);
  void push_event(event *ev) { eh->push_event(ev); }
  int event_waiting() { return eh->event_waiting(); }
  void flush_screen();
  int bright_color() { return hi; }
  int medium_color() { return med; }
  int dark_color() { return low; }
  int black() { return bk; }
  void set_colors(int Hi, int Med, int Low) { hi=Hi; med=Med; low=Low; }
  JCFont *font() { return fnt; }
  int has_mouse() { return eh->has_mouse(); }
  void mouse_status(int &x, int &y, int &button) {eh->mouse_status(x,y,button); }	       
  void set_mouse_shape(image *im, int centerx, int centery) 
  { eh->set_mouse_shape(im,centerx,centery); }

  void set_mouse_position(int mx, int my)
  { eh->set_mouse_position(mx,my); }

  int key_pressed(int x) { return key_state[x]; }
  ~window_manager() { delete eh; while (first) close_window(first); }
  void hide_windows();
  void show_windows();
  void hide_window(jwindow *j);
  void show_window(jwindow *j);
  void set_frame_suppress(int x) { frame_suppress=x; }
  void grab_focus(jwindow *j);
  void release_focus();
  int window_in_area(int x1, int y1, int x2, int y2); // true if a window lies in this area
} ;

#endif


