#ifndef __INPUT_HPP_
#define __INPUT_HPP_
#include "jwindow.hpp"
#include "filter.hpp"


class button : public ifield
{
  int up,act;
  char *text;
  image *visual,*pressed,*act_pict;
  int act_id;
public : 
  button(int X, int Y, int ID, char *Text, ifield *Next);
  button(int X, int Y, int ID, image *vis, ifield *Next);
  button(int X, int Y, int ID, image *Depressed, image *Pressed, image *active, ifield *Next);

  virtual void area(int &x1, int &y1, int &x2, int &y2, window_manager *wm);
  virtual void draw_first(image *screen, window_manager *wm);
  virtual void draw(int active, image *screen, window_manager *wm); 
  virtual void handle_event(event &ev, image *screen, window_manager *wm, input_manager *im);
  void change_visual(image *new_visual);
  virtual void remap(filter *f);
  virtual ~button() { if (text) jfree(text); }
  void push();
  virtual char *read() { return (char *)&up; }
  int status() { return up; }
  void set_act_id(int id) { act_id=id; }
} ;

class button_box : public ifield
{
  button *buttons;
  int maxdown;
  public :
  button_box(int X, int Y, int ID, int MaxDown, button *Buttons, ifield *Next);
  void add_button(button *b);
  void press_button(int id);      // if button box doesn't contain id, nothing happens
  virtual void remap(filter *f);
  virtual void area(int &x1, int &y1, int &x2, int &y2, window_manager *wm);
  virtual void draw_first(image *screen, window_manager *wm);
  virtual void draw(int active, image *screen, window_manager *wm); 
  virtual void handle_event(event &ev, image *screen, window_manager *wm, input_manager *im);  
  virtual ~button_box();
  virtual char *read();   // return pointer to first button which is depressed
  virtual ifield *find(int search_id);  // should return pointer to item you control with this id
  void arrange_left_right(window_manager *wm);
  void arrange_up_down(window_manager *wm);
} ;

class text_field : public ifield
{
  int cur;
  char *prompt,*data,*format;
  int xstart(window_manager *wm) { return x+wm->font()->width()*(strlen(prompt)+1)+3; }
  int xend(window_manager *wm) { return x+wm->font()->width()*(strlen(prompt)+1+strlen(format))+7; }
  int yend(window_manager *wm) { return y+wm->font()->height()+5; }
  void draw_cur(int color, image *screen, window_manager *wm);
  int last_spot() { int x=strlen(data); while (x && data[x-1]==' ') x--; return x; }
  void draw_text(image *screen, window_manager *wm)
  {
    screen->bar(xstart(wm)+1,y+1,xend(wm)-1,yend(wm)-1,wm->dark_color());
    wm->font()->put_string(screen,xstart(wm)+1,y+3,data);
  }
public : 
  text_field(int X, int Y, int ID, char *Prompt, char *Format, 
                               char *Data, ifield *Next);
  text_field(int X, int Y, int ID, char *Prompt, char *Format, 
                               double Data, ifield *Next);

  virtual void area(int &x1, int &y1, int &x2, int &y2, window_manager *wm);
  virtual void draw_first(image *screen, window_manager *wm);
  virtual void draw(int active, image *screen, window_manager *wm); 
  virtual void handle_event(event &ev, image *screen, window_manager *wm, input_manager *im);
  
  virtual ~text_field() { jfree(prompt); jfree(format); jfree(data); }
  virtual char *read();
  void change_data(char *new_data, int new_cursor,       // cursor==-1, does not change it.
		   int active, image *screen, window_manager *wm);
} ;


class info_field : public ifield
{
  char *text;
  int w,h;
  void put_para(image *screen, char *st, int dx, int dy, int xspace, 
		int yspace, JCFont *font, int color);
public : 
  info_field(int X, int Y, int ID, char *info, ifield *Next);
  virtual void area(int &x1, int &y1, int &x2, int &y2, window_manager *wm);
  virtual void draw_first(image *screen, window_manager *wm);
  virtual void draw(int active, image *screen, window_manager *wm) { ; }
  virtual void handle_event(event &ev, image *screen, window_manager *wm, input_manager *im) { ; }
  virtual char *read() { return text; }
  virtual int selectable() { return 0; } 
  virtual ~info_field() { jfree(text); }
} ;

#endif










