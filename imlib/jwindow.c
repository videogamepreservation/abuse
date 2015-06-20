#include "video.hpp"
#include "image.hpp"
#include "event.hpp"
#include "filter.hpp"
#include "event.hpp"
#include "jwindow.hpp"

int jw_left=5,jw_right=5,jw_top=15,jw_bottom=5;

int frame_top() { return jw_top; }
int frame_bottom() { return jw_bottom; }
int frame_left() { return jw_left; }
int frame_right() { return jw_right; }

ifield::~ifield() { ; }

void set_frame_size(int x)
{  
  if (x<1) x=1;
  jw_left=x;
  jw_right=x;
  jw_top=10+x;
  jw_bottom=x;
}

 // true if a window lies in this area
int window_manager::window_in_area(int x1, int y1, int x2, int y2)
{
  for (jwindow *f=first;f;f=f->next) 
    if (f->x<=x2 && f->y<=y2 && f->x+f->l-1>=x1 && f->y+f->h-1>=y1)
      return 1;
  return 0;
}

void window_manager::grab_focus(jwindow *j)
{ grab=j; }

void window_manager::release_focus()
{ grab=NULL; }


void window_manager::close_window(jwindow *j)
{
  jwindow *k;
  if (grab==j) grab=NULL;
  if (state==dragging && j==drag_window)  // close the window we were dragging
    state=inputing;

  if (j==first)
    first=first->next;
  else
  {
    for (k=first;k->next!=j;k=k->next)
      k->screen->add_dirty(j->x-k->x,j->y-k->y, 
                   j->x+j->l-1-k->x,j->y+j->h-1-k->y);
    k->screen->add_dirty(j->x-k->x,j->y-k->y, 
                   j->x+j->l-1-k->x,j->y+j->h-1-k->y);
    k->next=j->next;
  }
  screen->add_dirty(j->x,j->y,j->x+j->l-1,j->y+j->h-1);
  delete j;
}

void window_manager::hide_windows()
{
  jwindow *p;
  for (p=first;p;p=p->next)
  {
    if (!p->property.hidden)
    {
      p->property.hidden=1;
      screen->add_dirty(p->x,p->y,p->x+p->l-1,p->y+p->h-1);
    }
  }
}

void window_manager::show_windows()
{
  jwindow *p;
  for (p=first;p;p=p->next)
    if (p->property.hidden)
      show_window(p);      
}

void window_manager::hide_window(jwindow *j)
{
  jwindow *k;
  if (j==first)
    first=first->next;
  else
  {
    for (k=first;k->next!=j;k=k->next)
      k->screen->add_dirty(j->x-k->x,j->y-k->y, 
                   j->x+j->l-1-k->x,j->y+j->h-1-k->y);
    k->screen->add_dirty(j->x-k->x,j->y-k->y, 
                   j->x+j->l-1-k->x,j->y+j->h-1-k->y);
    k->next=j->next;
  }
  screen->add_dirty(j->x,j->y,j->x+j->l-1,j->y+j->h-1);
  j->property.hidden=1;
}

void window_manager::show_window(jwindow *j)
{
  if (j->property.hidden)
  {
    j->property.hidden=0;
    j->screen->add_dirty(0,0,j->l-1,j->h-1);
  }
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
    for (ev.window=NULL,j=first;j;j=j->next)
      if (!j->property.hidden && ev.mouse_move.x>=j->x && ev.mouse_move.y>=j->y &&
          ev.mouse_move.x<j->x+j->l && ev.mouse_move.y<j->y+j->h)
        ev.window=j;

    if (!ev.window && grab) ev.window=grab;

    if (ev.window)
    {
      int closew=0,movew=0;

      if ((ev.type==EV_MOUSE_BUTTON && ev.mouse_button==1 && ev.window &&
	   ev.mouse_move.x>=ev.window->x && ev.mouse_move.y>=ev.window->y &&
	   ev.mouse_move.x<ev.window->x+ev.window->l && ev.mouse_move.y<ev.window->y+ev.window->y1()))
      {
	if (ev.mouse_move.x-ev.window->x<11) closew=1;
	else if (ev.window->property.moveable) movew=1;
      } else if (grab)
        ev.window=grab;

      if (ev.type==EV_KEY && ev.key==JK_ESC)
        closew=1;

      
    
      if (closew)
        ev.type=EV_CLOSE_WINDOW;
      else if (movew)
      {	
	int red=0;
	if (ev.window==first)       // see if we need to raise the window
	{
	  first=first->next;
	  if (first)
	    red=1;
	}
	else
	{
	  jwindow *last=first;
	  for (;last->next!=ev.window;last=last->next);
	  if (ev.window->next)
	    red=1;
	  last->next=ev.window->next;
	}
	if (!first)
	  first=ev.window;
	else
	{
	  jwindow *last=first;
	  for (;last->next;last=last->next);
	  last->next=ev.window;
	}
	ev.window->next=NULL;
	if (red)
	{
	  jwindow *j=ev.window,*p;
/*	  screen->add_dirty(j->x,j->y,j->x+j->l-1,j->y+j->h-1);
	  for (p=first;p!=j;p=p->next)
	    p->screen->add_dirty(j->x-p->x,j->y-p->y,j->x+j->l-1-p->x,j->y+j->h-1-p->y);*/
	  j->screen->add_dirty(0,0,j->l-1,j->h-1);
	  flush_screen();
	}

        state=dragging;
        drag_window=ev.window;
        drag_mousex=ev.window->x-ev.mouse_move.x;
        drag_mousey=ev.window->y-ev.mouse_move.y;
        ev.type=EV_SPURIOUS;
      } else if (ev.window)
        ev.window->inm->handle_event(ev,ev.window,this);
    }
  } else if (state==dragging)
  {
    ev.window=drag_window;
    if (ev.type==EV_MOUSE_BUTTON && ev.mouse_button==0)  // user released the mouse
    {
      state=inputing;
      ev.type=EV_SPURIOUS;
    } else if (ev.type==EV_MOUSE_MOVE)
    {
       move_window(drag_window,ev.mouse_move.x+drag_mousex,ev.mouse_move.y+drag_mousey);
       flush_screen();
       ev.type=EV_DRAG_WINDOW;
       ev.window_position.x=ev.mouse_move.x+drag_mousex;
       ev.window_position.y=ev.mouse_move.y+drag_mousey;
    }
  } 
  if (ev.type==EV_REDRAW)
  {
    for (j=first;j;j=j->next) 
       j->screen->add_dirty(ev.redraw.x1-j->x,ev.redraw.y1-j->y,
		     ev.redraw.x2-j->x,ev.redraw.y2-j->y);
    screen->add_dirty(ev.redraw.x1,ev.redraw.y1,ev.redraw.x2,ev.redraw.y2);
    flush_screen();
    ev.type=EV_SPURIOUS;   // we took care of this one by ourselves.
  }
}

void jwindow::resize(int L, int H)
{
  screen->change_size(L,H);
  l=L; h=H;
}

void window_manager::resize_window(jwindow *j, int l, int h)
{
  jwindow *p;
  screen->add_dirty(j->x,j->y,j->x+j->l-1,j->y+j->h-1);
  for (p=first;p!=j;p=p->next)
    p->screen->add_dirty(j->x-p->x,j->y-p->y,j->x+j->l-1-p->x,j->y+j->h-1-p->y);
  j->resize(l,h);
  if (!frame_suppress)
  j->redraw(hi,med,low,frame_font());
}

void window_manager::move_window(jwindow *j, int x, int y)
{
  jwindow *p;
  screen->add_dirty(j->x,j->y,j->x+j->l-1,j->y+j->h-1);
  for (p=first;p!=j;p=p->next)
    p->screen->add_dirty(j->x-p->x,j->y-p->y,j->x+j->l-1-p->x,j->y+j->h-1-p->y);
  j->x=x;
  j->y=y;
  j->screen->add_dirty(0,0,j->l-1,j->h-1);
}

window_manager::window_manager(image *Screen, palette *Pal, int Hi, 
                               int Med, int Low, JCFont *Font)
{
  screen=Screen; hi=Hi; low=Low; med=Med; first=NULL; pal=Pal; grab=NULL;
  bk=pal->find_closest(0,0,0);
  state=inputing; fnt=Font;  wframe_fnt=Font;
  memset(key_state,0,sizeof(key_state));
  eh=new event_handler(screen,pal);
  frame_suppress=0;
}

jwindow *window_manager::new_window(int x, int y, int l, int h, ifield *fields, char *Name)
{
  if (x>screen->width()-4) x=screen->width()-25;
  if (y>screen->height()-4) y=screen->height()-10;
  
  jwindow *j=new jwindow(x,y,l,h,this,fields,Name),*k;
  j->property.hidden=0;
  if (!first)
    first=j;
  else
  {
    k=first;
    while (k->next) k=k->next;
    k->next=j;
    j->next=NULL;
  }
  if (!frame_suppress)
    j->redraw(hi,med,low,frame_font());
  return j;
}

void window_manager::flush_screen()
{
  jwindow *p,*q;

  int mx,my,but;
  image *mouse_pic,*mouse_save;
  
  if (has_mouse())
  {    
    mouse_pic=eh->mouse_sprite()->visual;
    mouse_save=eh->mouse_sprite()->save;
    mx=eh->mouse->drawx();
    my=eh->mouse->drawy();

    screen->put_part(mouse_save,0,0,mx,my,mx+mouse_pic->width()-1,my+mouse_pic->height()-1);
    mouse_pic->put_image(screen,mx,my,1);
  }
  
  for (p=first;p;p=p->next)
    if (!p->property.hidden)
       screen->delete_dirty(p->x,p->y,p->x+p->l-1,p->y+p->h-1);
  update_dirty(screen);

  if (has_mouse())
    mouse_save->put_image(screen,mx,my);


  for (p=first;p;p=p->next)
  {
    if (!p->property.hidden)
    {
      if (has_mouse())
      {      
	p->screen->put_part(mouse_save,0,0,mx-p->x,my-p->y,
			    mx-p->x+mouse_pic->width()-1,
			    my-p->y+mouse_pic->height()-1);
	if (has_mouse())
        mouse_pic->put_image(p->screen,mx-p->x,my-p->y,1);
      }
      

//      screen->delete_dirty(p->x,p->y,p->x+p->l-1,p->y+p->h-1);
      for (q=p->next;q;q=q->next)
        if (!q->property.hidden)
          p->screen->delete_dirty(q->x-p->x,
                              q->y-p->y,
                              q->x+q->l-1-p->x,
                              q->y+q->h-1-p->y);
      update_dirty(p->screen,p->x,p->y);
      if (has_mouse())
         mouse_save->put_image(p->screen,mx-p->x,my-p->y,0);
    }
  }
}

void jwindow::set_moveability(int x)
{
  property.moveable=x;
}

jwindow::jwindow(int X, int Y, int L, int H, window_manager *wm, ifield *fields, char *Name)
{
  ifield *i;
  int x1,y1,x2,y2;
  l=0; h=0; 
  property.moveable=1;
  if (fields)
    for (i=fields;i;i=i->next)
    {
      i->area(x1,y1,x2,y2,wm);
      if ((int)y2>(int)h) 
        h=y2+1;
      if ((int)x2>(int)l) 
        l=x2+1;
    }
  else { l=2; h=2; }

  if (L<=0) { l=l-L; } else l=L+jw_left;
  if (H<=0) { h=h-H; } else h=H+jw_top;

 if (Y<0) y=yres-h+Y-WINDOW_FRAME_TOP-WINDOW_FRAME_BOTTOM-1; else y=Y;
 if (X<0) x=xres-l+X-WINDOW_FRAME_LEFT-WINDOW_FRAME_RIGHT-1; else x=X;

  backg=wm->medium_color();
  l+=WINDOW_FRAME_RIGHT; h+=WINDOW_FRAME_BOTTOM;
//  if (!fields) { l+=WINDOW_FRAME_LEFT; h+=WINDOW_FRAME_TOP; }

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
}

void jwindow::local_close() { ; }

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


  if (name && name[0] && (name[0]!=' ' || name[1]))
  {
    short cx1,cy1,cx2,cy2;
    screen->get_clip(cx1,cy1,cx2,cy2);
    screen->set_clip(14,1,l-2,WINDOW_FRAME_TOP-4);
    screen->bar(14,1,14+fnt->width()*strlen(name),15-8,med);
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
    ifield *x=i->unlink(id);
    if (x) return x;
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

void input_manager::clear_current()
{
  if (active)
    active->draw(0,screen,wm);

  active=NULL;
}

void input_manager::handle_event(event &ev, jwindow *j, window_manager *wm)
{
  ifield *i,*in_area=NULL;
  int x1,y1,x2,y2;
  if (j)
  {
    ev.mouse_move.x-=j->x;
    ev.mouse_move.y-=j->y;
    cur=j;
  }

  if (!grab)
  {
    if ((ev.type==EV_MOUSE_BUTTON && ev.mouse_button==1) || ev.type==EV_MOUSE_MOVE)
    {
      for (i=first;i;i=i->next)
      {
	i->area(x1,y1,x2,y2,wm);
	if (ev.mouse_move.x>=x1 && ev.mouse_move.y>=y1 &&
	    ev.mouse_move.x<=x2 && ev.mouse_move.y<=y2)
        in_area=i;
      }
      if (in_area!=active && (no_selections_allowed || (in_area && in_area->selectable())))
      {
	if (active)
          active->draw(0,screen,wm);

	active=in_area; 

	if (active)
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
  } else active=grab;

  if (active)
  {
    if (ev.type!=EV_MOUSE_MOVE && ev.type!=EV_MOUSE_BUTTON)
      active->handle_event(ev,screen,wm,this);
    else
    {
      active->area(x1,y1,x2,y2,wm);
      if (grab || (ev.mouse_move.x>=x1 && ev.mouse_move.y>=y1 &&
          ev.mouse_move.x<=x2 && ev.mouse_move.y<=y2))
      {
	if (j)
	  active->handle_event(ev,screen,wm,j->inm);
	else active->handle_event(ev,screen,wm,this);
      }
    }
  }

  if (j)
  {
    ev.mouse_move.x+=j->x;
    ev.mouse_move.y+=j->y;
  }
}

void input_manager::allow_no_selections()
{
  no_selections_allowed=1;
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
  no_selections_allowed=0;
  cur=NULL;
  grab=NULL;
  screen=Screen;
  wm=WM;
  active=first=First;
  while (active && !active->selectable()) active=active->next;
  redraw();
}

void input_manager::grab_focus(ifield *i)
{ grab=i; 
  if (cur)
    wm->grab_focus(cur);
}

void input_manager::release_focus()
{ grab=NULL; 
  if (cur)
    wm->release_focus();
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



