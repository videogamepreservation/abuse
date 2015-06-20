#include "video.hpp"
#include "image.hpp"
#include "event.hpp"
#include "filter.hpp"
#include "event.hpp"
#include "jwindow.hpp"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/X.h>

int jw_left=5,jw_right=5,jw_top=15,jw_bottom=5;

int frame_top() { return 0; }
int frame_bottom() { return 0; }
int frame_left() { return 0; }
int frame_right() { return 0; }

extern void update_dirty_window(Window win, image *im, int xoff, int yoff);
extern Window root,mainwin;
extern Display *display;
extern int screen_num;

struct jxwin
{
  Window win;  
} ;

void set_frame_size(int x)
{  
  return ;
}

void window_manager::close_window(jwindow *j)
{
  if (j==first)
    first=first->next;
  else
  {
    for (jwindow *k=first;k->next!=j;k=k->next);
    k->next=j->next;
  }
  XFlush(display);
  delete j;
}

void window_manager::hide_windows()
{
  for (jwindow *p=first;p;p=p->next)
    p->hidden=1;
}

void window_manager::show_windows()
{
  for (jwindow *p=first;p;p=p->next)
    p->hidden=0;

}

void window_manager::hide_window(jwindow *j)
{
  j->hidden=1;
}

void window_manager::show_window(jwindow *j)
{
  j->hidden=0;
}

void window_manager::get_event(event &ev)
{
  jwindow *j;
  eh->get_event(ev);
  if (ev.type==EV_KEY)
    key_state[ev.key]=1;
  else if (ev.type==EV_KEYRELEASE)
    key_state[ev.key]=0;

  if (state==inputing)
  {
    int f=0;
    for (j=first;j;j=j->next)
      if (!j->hidden && ((Window)ev.window)==((jxwin *)j->local_info)->win)
      { f=1;
        ev.window=j;
      }
    if (!f) ev.window=NULL;
    
    if (ev.window)
    {
      int closew=0,movew=0;
      if (ev.type==EV_KEY && ev.key==JK_ESC)
        closew=1;
    
      if (closew)
        ev.type=EV_CLOSE_WINDOW;
      else if (ev.window)
        ev.window->inm->handle_event(ev,0,0);
    }
  }

  if (ev.type==EV_REDRAW)
  {
    if (ev.window)
      ev.window->screen->add_dirty(ev.redraw.x1,ev.redraw.y1,ev.redraw.x2,ev.redraw.y2);
    else
      screen->add_dirty(ev.redraw.x1,ev.redraw.y1,ev.redraw.x2,ev.redraw.y2);
    flush_screen();
    ev.type=EV_SPURIOUS;   // we took care of this one by ourselves.
  }
}

void jwindow::resize(int L, int H)
{
  XWindowChanges v;
  v.width=L;
  v.height=H;
  XFlush(display);
  XConfigureWindow(display,((jxwin *)local_info)->win,CWWidth|CWHeight,&v);
  l=L; h=H;
}

void window_manager::resize_window(jwindow *j, int l, int h)
{
  jwindow *p;
  j->resize(l,h);
  j->redraw(hi,med,low,font());
}

void window_manager::move_window(jwindow *j, int x, int y)
{
  return ;
}

window_manager::window_manager(image *Screen, palette *Pal, int Hi, 
                               int Med, int Low, JCFont *Font)
{
  screen=Screen; hi=Hi; low=Low; med=Med; first=NULL; pal=Pal;
  bk=pal->find_closest(0,0,0);
  state=inputing; fnt=Font; wframe_fnt=Font;
  memset(key_state,0,sizeof(key_state));
  eh=new event_handler(screen,pal);
}

jwindow *window_manager::new_window(int x, int y, int l, int h, ifield *fields, char *Name)
{
  if (x>screen->width()-4) x=screen->width()-4;
  if (y>screen->height()-4) y=screen->height()-4;
  
  jwindow *j=new jwindow(x,y,l,h,this,fields,Name),*k;
  j->hidden=0;
  if (!first)
    first=j;
  else
  {
    k=first;
    while (k->next) k=k->next;
    k->next=j;
    j->next=NULL;
  }
  j->redraw(hi,med,low,font());
  return j;
}

void window_manager::flush_screen()
{
  jwindow *p,*q;

  int mx,my,but;
  image *mouse_pic,*mouse_save;
  
  update_dirty(screen);

  for (p=first;p;p=p->next)
  {
    if (!p->hidden)
      update_dirty_window(((jxwin *)p->local_info)->win,p->screen,0,0);
  }
}


jwindow::jwindow(int X, int Y, int L, int H, window_manager *wm, ifield *fields, char *Name)
{
  ifield *i;
  int x1,y1,x2,y2;
  l=0; h=0; 
  if (fields)
    for (i=fields;i;i=i->next)
    {
      i->area(x1,y1,x2,y2,wm);
      if ((int)y2+1>(int)h) 
        h=y2+1;
      if ((int)x2+1>(int)l) 
        l=x2+1;
    }
  else { l=2; h=2; }

  if (L<=0) { l=l-L; } else l=L;
  if (H<=0) { h=h-H; } else h=H;

 if (Y<0) y=yres-h+Y-WINDOW_FRAME_TOP-WINDOW_FRAME_BOTTOM-1; else y=Y;
 if (X<0) x=xres-l+X-WINDOW_FRAME_LEFT-WINDOW_FRAME_RIGHT-1; else x=X;

  backg=wm->medium_color();
  l+=WINDOW_FRAME_RIGHT; h+=WINDOW_FRAME_BOTTOM;
  if (!fields) { l+=WINDOW_FRAME_LEFT; h+=WINDOW_FRAME_TOP; }

  if (l<18) l=18;
  if (h<12) h=12;
  screen=new image(l,h,NULL,2);
  l=screen->width();
  h=screen->height();
  screen->clear(backg);

  next=NULL;
  inm=new input_manager(screen,wm,fields);
  if (Name==NULL)
    name=strcpy((char *)jmalloc(strlen(" ")+1,"jwindow::window name")," ");  
  else
    name=strcpy((char *)jmalloc(strlen(Name)+1,"jwindow::window name"),Name);
  local_info=(void *)jmalloc(sizeof(jxwin),"Xwindow struct");

  XWindowAttributes wa;
  XGetWindowAttributes(display,mainwin,&wa);
  Window w=XCreateSimpleWindow(display,
                              root,
                              wa.x+x,wa.y+y,
                              l,h,
                              5,
	                      BlackPixel(display,screen_num),
                              WhitePixel(display,screen_num)); 
  ((jxwin *)local_info)->win=w;

  XSelectInput(display,w,
    KeyPressMask | VisibilityChangeMask | ButtonPressMask | ButtonReleaseMask |
    ButtonMotionMask | PointerMotionMask | KeyReleaseMask |
    ExposureMask | StructureNotifyMask);

  XSetTransientForHint(display,w,mainwin);
  XSetWindowColormap(display,w,wa.colormap);


  XTextProperty xtext;
  ERROR(XStringListToTextProperty(&Name,1,&xtext),"X alloc failed"); 


  XSizeHints *xsize;
  ERROR((xsize=XAllocSizeHints()),"X alloc failed");
  xsize->flags=PPosition | PSize | PMinSize | PMaxSize;
  xsize->min_width=l;
  xsize->min_height=h;
  xsize->max_width=l;
  xsize->max_height=h;


  XWMHints *wm_hints;
  ERROR((wm_hints=XAllocWMHints()),"X alloc failed");
  wm_hints->initial_state=NormalState;  // not iconified at first
  wm_hints->input=1;                  // needs keyboard input
  wm_hints->flags=StateHint | InputHint;

  
  XSetWMProperties(display,w,&xtext,&xtext,&Name,0,xsize,wm_hints,NULL);
  XFree(xtext.value);
  XFree(xsize);
  XFree(wm_hints);



  XEvent report;
  XMapWindow(display,((jxwin *)local_info)->win);
  do
  { XNextEvent(display, &report);
  } while (report.type!= Expose);     // wait for our window to pop up
  x=y=0;
}

void jwindow::local_close()
{
  XDestroyWindow(display,((jxwin *)local_info)->win);
  jfree(local_info);
}

void jwindow::redraw(int hi, int med, int low, JCFont *fnt)
{
  if (jw_right>=3)
    screen->rectangle(0,0,l-3,h-3,low);
  if (jw_right>=2)
    screen->rectangle(1,1,l-2,h-2,med);
  if (jw_right>=1)
    screen->rectangle(2,2,l-1,h-1,hi);


  
  screen->wiget_bar(0,0,l-1,8,hi,med,low);
  screen->line(1,1,l-2,1,low);
  screen->line(1,3,l-2,3,low);
  screen->line(1,5,l-2,5,low);
  screen->line(1,7,l-2,7,low);

  screen->wiget_bar(4,3,10,5,hi,med,low);
  screen->rectangle(3,2,11,6,0);  

  screen->line(0,8,l-1,8,0);
  if (jw_right>=1)
    screen->wiget_bar(0,9,l-1,h-1,hi,med,low);  
    screen->wiget_bar(0,9,l-1,h-1,hi,med,low);
  if (jw_right>=2)
    screen->wiget_bar(4,13,l-jw_right,h-jw_right,low,med,hi);


  if (name && name[0])
  {
    short cx1,cy1,cx2,cy2;
    screen->get_clip(cx1,cy1,cx2,cy2);
    screen->set_clip(14,1,l-2,WINDOW_FRAME_TOP-8);
    screen->bar(14,1,14+fnt->width()*strlen(name),WINDOW_FRAME_TOP-8,med);
    fnt->put_string(screen,14,1,name,low);  
    screen->set_clip(cx1,cy1,cx2,cy2);
  }
  
  screen->bar(x1(),y1(),x2(),y2(),backg);
  inm->redraw();
}


ifield *input_manager::unlink(int id)     // unlinks ID from fields list and return the pointer to it
{ 
  for (ifield *i=first,*last;i;i=i->next)
  {
    if (i->id==id) 
    {
      if (i==first)
	first=first->next;
      else
        last->next=i->next;
      if (active==i)
        active=first;
      return i;
    }
    last=i;
  }
  return NULL;   // no such id
}

input_manager::~input_manager() 
{ ifield *i; 
  while (first) 
  { i=first; 
    first=first->next; 
    delete i; 
  } 
} 

void input_manager::handle_event(event &ev, int xoff, int yoff)
{
  ifield *i,*in_area=NULL;
  int x1,y1,x2,y2;
  ev.mouse_move.x-=xoff;
  ev.mouse_move.y-=yoff;

  if (ev.type==EV_MOUSE_BUTTON && ev.mouse_button==1)
  {
    for (i=first;i;i=i->next)
    {
      i->area(x1,y1,x2,y2,wm);
      if (ev.mouse_move.x>=x1 && ev.mouse_move.y>=y1 &&
          ev.mouse_move.x<=x2 && ev.mouse_move.y<=y2)
        in_area=i;
    }
    if (in_area!=active && in_area  && in_area->selectable())
    {
      if (active)
        active->draw(0,screen,wm);
      active=in_area; 
      active->draw(1,screen,wm);
    }
  } 
  if (ev.type==EV_KEY && ev.key==JK_TAB && active)
  { 
    active->draw(0,screen,wm);
    do
    {
      active=active->next;
      if (!active) active=first;
    } while (active && !active->selectable());
    active->draw(1,screen,wm);
  }
  if (active)
  {
    if (ev.type!=EV_MOUSE_MOVE && ev.type!=EV_MOUSE_BUTTON)
      active->handle_event(ev,screen,wm);
    else
    {
      active->area(x1,y1,x2,y2,wm);
      if (ev.mouse_move.x>=x1 && ev.mouse_move.y>=y1 &&
          ev.mouse_move.x<=x2 && ev.mouse_move.y<=y2)
      active->handle_event(ev,screen,wm);
    }
  }
  
  ev.mouse_move.x+=xoff;
  ev.mouse_move.y+=yoff;
}

void input_manager::redraw()
{
  ifield *i;
  for (i=first;i;i=i->next)
    i->draw_first(screen,wm);
  if (active)
    active->draw(1,screen,wm);
}

input_manager::input_manager(image *Screen, window_manager *WM, ifield *First)
{
  screen=Screen;
  wm=WM;
  active=first=First;
  while (active && !active->selectable()) active=active->next;
  redraw();
}

void input_manager::remap(filter *f)
{
  for (ifield *i=first;i;i=i->next)
   i->remap(f);
  redraw();
}

void input_manager::add(ifield *i) 
{ ifield *f=first;
  if (i->selectable())
  {
    if (!f)
      first=i;
    else
    {
      while (f->next) f=f->next;
      f->next=i; 
    }
  }
}

ifield *input_manager::get(int id)
{
  ifield *f;
  for (f=first;f;f=f->next)
  {
    ifield *ret=f->find(id);
    if (ret) return ret;
  }
  return NULL;
}









