#include "pmenu.hpp"

void pmenu::move(int new_x, int new_y)
{
  wm->move_window(bar,new_x,new_y);
}

pmenu::pmenu(int X, int Y, pmenu_item *first, image *screen, window_manager *wmanager)
{ 
  top=first; 
  active=NULL; 
  wm=wmanager;

  short cx1,cy1,cx2,cy2;
  screen->get_clip(cx1,cy1,cx2,cy2);
  if (cx1<X) cx1=X;
  int w=cx2-cx1+1;
  int h=wm->frame_font()->height()+4;


  bar=wm->new_window(X,Y,w-WINDOW_FRAME_LEFT-WINDOW_FRAME_RIGHT,
		     h-WINDOW_FRAME_TOP-WINDOW_FRAME_BOTTOM,NULL);
  bar->set_moveability(0);  // can't drag this window
  bar->screen->wiget_bar(0,0,w-1,h-1,wm->bright_color(),wm->medium_color(),
		    wm->dark_color());



  int total=0,tx,tw;
  pmenu_item *p=top;
  for (;p;p=p->next) total++;

  tw=w/(total+1);
  tx=tw/2;

  for (p=top;p;p=p->next,tx+=tw) 
    p->draw_self(bar,itemx(p,wm),1,itemw(p,wm),1,wm,p==active);      
/*  }
  else
  {
    for (p=top;p;p=p->next,tx+=tw) 
      p->draw(bar,itemx(p,wm),1,itemw(p,wm),1,wm,p==active);
  }*/
  
}

pmenu_item::pmenu_item(int ID, char *Name, char *on_off_flag, int Hotkey, pmenu_item *Next)
{ 
  xp=-1;
  id=ID; 
  hotkey=Hotkey;
  on_off=on_off_flag;
  if (Name)
    n=strcpy((char *)jmalloc(strlen(Name)+1,"pmenu_item::name"),Name); 
  else n=NULL;
  next=Next;
  sub=NULL;
}

pmenu_item::pmenu_item(char *Name, psub_menu *Sub, pmenu_item *Next, int xpos)
{
  xp=xpos;
  id=0; hotkey=-1;
  next=Next;
  on_off=NULL;
  CONDITION(Name,"Sub menu cannot have a NULL name");
  n=strcpy((char *)jmalloc(strlen(Name)+1,"pmenu_item::name"),Name); 
  sub=Sub;
}

pmenu_item *pmenu_item::find_id(int search_id) 
{ 
  if (id==search_id) return this;
  else if (sub) return sub->find_id(search_id);
  else return NULL;
}

pmenu_item *pmenu_item::find_key(int key)
{
  if (key==hotkey && hotkey!=-1) return this;
  else if (sub) return sub->find_key(key);
  else return NULL;
}

pmenu::~pmenu()
{
  while (top)
  {
    pmenu_item *p=top;
    top=top->next;
    delete p;
  }
  if (bar) wm->close_window(bar);
}

psub_menu::~psub_menu() 
{ 
  if (win)   
    wm->close_window(win);
    
  while (first)
  {
    pmenu_item *tmp=first;
    first=first->next;
    delete tmp;
  }
}

pmenu_item *psub_menu::find_id(int search_id)
{
  for (pmenu_item *f=first;f;f=f->next)
  {
    pmenu_item *ret=f->find_id(search_id);
    if (ret) return ret;
  }
  return NULL;
}

pmenu_item *psub_menu::find_key(int key)
{
  for (pmenu_item *f=first;f;f=f->next)
  {
    pmenu_item *ret=f->find_key(key);
    if (ret) return ret;
  }
  return NULL;
}


void psub_menu::hide(jwindow *parent, int x, int y, window_manager* wm)
{
  int w,h;
  calc_size(w,h,wm);
  short cx1,cy1,cx2,cy2;
  screen->get_clip(cx1,cy1,cx2,cy2);
  if (w+x>cx2)
    x=cx2-w;


  if (win)
  {
    if (active!=-1)
    {
      int w,h;
      calc_size(w,h,wm);
      item_num(active)->draw(win,x+3,y+3+active*(wm->frame_font()->height()+1),w-6,0,wm,0);
    }
    wm->close_window(win);
    win=NULL;
  }
}

void psub_menu::calc_size(int &w, int &h, window_manager *wm)
{
  int tw=wm->frame_font()->width(),th=wm->frame_font()->height();
  w=h=0;
  for (pmenu_item *p=first;p;p=p->next)
  {
    if (p->name())
    {
      int l=strlen(p->name())*tw+8;
      if (p->on_off) l+=tw*4;
      if (l>w) w=l;   
    }
    h++;
  }
  h=h*(th+1)+8;
}

void psub_menu::draw(jwindow *parent, int x, int y, window_manager *wmanager)
{
  if (win) wm->close_window(win);
  wm=wmanager;


  int w,h,i=0;
  calc_size(w,h,wm);
  short cx1,cy1,cx2,cy2;
  screen->get_clip(cx1,cy1,cx2,cy2);
  if (parent->x+w+x>cx2)
    x=cx2-w-parent->x;
  if (h+y+parent->y>cy2)
    if (parent->y+parent->h+wm->frame_font()->height()>cy2)
      y=-h;
     else y=y-h+wm->frame_font()->height()+5;
     

  win=wm->new_window(parent->x+x,parent->y+y,
		     w-WINDOW_FRAME_LEFT-WINDOW_FRAME_RIGHT,
		     h-WINDOW_FRAME_TOP-WINDOW_FRAME_BOTTOM,NULL);
  win->set_moveability(0);
  win->screen->wiget_bar(0,0,w-1,h-1,wm->bright_color(),wm->medium_color(),wm->dark_color());

  int has_flags=0;
  pmenu_item *p=first;
  for (;p;p=p->next) if (p->on_off) has_flags=1;
  x=has_flags ? 3+wm->frame_font()->width() : 3;
  y=3;
 
  for (p=first;p;p=p->next,i++,y+=wm->frame_font()->height()+1)
    p->draw(win,x,y,w-6,0,wm,i==active);

}

void pmenu_item::draw_self(jwindow *parent, int x, int y, int w, int top, window_manager *wm, int active)
{
  int bx=x;
  if (on_off) bx=x-wm->frame_font()->width();

  if (!n)
  {
    int h=wm->frame_font()->height();
    parent->screen->wiget_bar(x,y+h/2-1,x+w-1,y+h/2,wm->dark_color(),wm->medium_color(),wm->bright_color());
  } else
  {
    if (active)
    {
      if (xp!=-1)
        parent->screen->xor_bar(bx,y,x+w-1,y+wm->frame_font()->height()+1,wm->dark_color());
      else
      {
	parent->screen->bar(bx,y,x+w-1,y+wm->frame_font()->height()+1,wm->dark_color());
	wm->frame_font()->put_string(parent->screen,x+1,y+1,n,wm->medium_color());	
	if (on_off && *on_off) wm->frame_font()->put_string(parent->screen,bx+1,y+1,"*",wm->medium_color());
      }
    } else
    {
      if (xp!=-1)
        parent->screen->xor_bar(bx,y,x+w-1,y+wm->frame_font()->height()+1,wm->dark_color());
      else
      {
	parent->screen->bar(bx,y,x+w-1,y+wm->frame_font()->height()+1,wm->medium_color());
	wm->frame_font()->put_string(parent->screen,x+1,y+1,n,wm->bright_color());
	if (on_off && *on_off) wm->frame_font()->put_string(parent->screen,bx+1,y+1,"*",wm->bright_color());
      }
    }
  }
}

void pmenu_item::draw(jwindow *parent, int x, int y, int w, int top,
		      window_manager *wm, int active)
{  
  if (n)
  {
    if (active)
    {      
      draw_self(parent,x,y,w,top,wm,active);
      if (sub)
      {
	if (top)
          sub->draw(parent,x,y+wm->frame_font()->height()+2,wm);
	else
	  sub->draw(parent,x+w,y,wm);
      }
    }
    else
    {
      if (sub)
      {
	if (top)
          sub->hide(parent,x,y+wm->frame_font()->height()+2,wm);
	else
	  sub->hide(parent,x+w,y,wm);
      }
      draw_self(parent,x,y,w,top,wm,active);

    }

  } else draw_self(parent,x,y,w,top,wm,active);
}

int pmenu::itemx(pmenu_item *p, window_manager *wm)
{
  if (p->xp!=-1) return p->xp;
  int w=bar->screen->width();
  

  int total=0,tx,tw,i=0,x;
  for (pmenu_item *pp=top;pp;pp=pp->next,i++) 
  { if (pp==p) x=i;
    total++;
  }

  
  tw=w/(total+1); 
  return tw/2+x*tw;
}


void pmenu::draw(image *screen, window_manager *wm, int top_only)
{

}


int psub_menu::handle_event(jwindow *parent, int x, int y, window_manager *wm, event &ev)
{
  int w,h;
  calc_size(w,h,wm);
  short cx1,cy1,cx2,cy2;
  screen->get_clip(cx1,cy1,cx2,cy2);

  x=win->x;
  y=win->y;

  int has_flags=0,dx=3;
  for (pmenu_item *p=first;p;p=p->next) if (p->on_off) has_flags=1;
  if (has_flags) dx+=wm->frame_font()->width();

  int th=wm->frame_font()->height();
  if (ev.mouse_move.x>=x && ev.mouse_move.y>=y && ev.mouse_move.x<x+w && ev.mouse_move.y<y+h)
  {
    int new_active=(ev.mouse_move.y-y-3)/(th+1);
    if (item_num(new_active)==NULL) new_active=-1;

    if (new_active!=active)
    {
      if (active!=-1)
        item_num(active)->draw(win,dx,3+active*(th+1),w-6,0,wm,0);
      active=new_active;
      if (active!=-1)
        item_num(active)->draw(win,dx,3+active*(th+1),w-6,0,wm,1);
    }
    if (ev.type==EV_MOUSE_BUTTON)
    {
      if (active!=-1)
        return item_num(active)->handle_event(win,dx,3+active*(th+1),w-6,0,wm,ev);
      else return 0;
    } else return 1;
  } else if (active!=-1)
    return item_num(active)->handle_event(win,win->x+dx,win->y+3+active*(th+1),w-6,0,wm,ev);
  else return 0;


}

int pmenu_item::handle_event(jwindow *parent, int x, int y, int w, int top, 
			     window_manager *wm, event &ev)
{
  x+=parent->x;
  y+=parent->y;
  if (ev.mouse_move.x>=x && ev.mouse_move.y>=y && ev.mouse_move.x<x+w &&
      ev.mouse_move.y<y+wm->frame_font()->height()+2)
  {
    if (sub) return 1;
    else 
    {
      if (ev.type==EV_MOUSE_BUTTON &&n)
        wm->push_event(new event(id,(char *)this));
      return 1;
    }    
  } else if (sub)
  { 
    if (top)
      return sub->handle_event(parent,x,y+wm->frame_font()->height()+2,wm,ev);
    else return sub->handle_event(parent,x+w,y,wm,ev);
  } else return 0;
}

pmenu_item *pmenu::inarea(int mx, int my, image *screen, window_manager *wm) 
{ 
  short cx1,cy1,cx2,cy2;
  screen->get_clip(cx1,cy1,cx2,cy2);  
  mx-=bar->x;
  my-=bar->y;
  if (mx<0 || my<0 || mx>=bar->screen->width() || my>=bar->screen->height()) return NULL;
  else 
  {
    for (pmenu_item *p=top;p;p=p->next)
    {
      if (!p->next) return p;
      else if (itemx(p->next,wm)>mx) return p;
    }
    return NULL;
  }
}

int psub_menu::own_event(event &ev) 
{ 
  if (win && ev.window==win) return 1; else
    for (pmenu_item *p=first;p;p=p->next)
      if (p->own_event(ev)) 
        return 1; 
  return 0; 
}

int pmenu_item::own_event(event &ev) 
{ 
  if (sub) 
    return sub->own_event(ev); 
  else return 0; 
}

pmenu_item::~pmenu_item() 
{ if (n) jfree(n); if (sub) delete sub; 
}

int pmenu::handle_event(event &ev, image *screen, window_manager *wm)
{
  if (!active && ev.window!=bar) return 0;
/* 
    int yes=0;
    if (ev.window==bar) yes=1;    // event in top bar?
    else
    {
      for (pmenu_item *p=top;p && !yes;p=p->next)  // event in submenu?
      if (p->own_event(ev)) yes=1;
    }
    if (!yes) return 0;        // event is not for us...
  }*/

  switch (ev.type)
  {
    case EV_KEY :
    {
      for (pmenu_item *p=top;p;p=p->next)
      { 
	pmenu_item *r=p->find_key(ev.key);
	if (r)
	{ 
	  wm->push_event(new event(r->id,(char *)r));
	  return 1;
	}
      }
      return 0;
    } break;
    case EV_MOUSE_MOVE :
    {
      pmenu_item *new_selection=inarea(ev.mouse_move.x,ev.mouse_move.y,screen,wm);
      if (!new_selection && active && 
	  active->handle_event(bar,itemx(active,wm),1,itemw(active,wm),1,wm,ev))
	return 1;
      else if (active!=new_selection)
      {
	if (active)
	  active->draw(bar,itemx(active,wm),1,itemw(active,wm),1,wm,0);
	active=new_selection;
	if (active)
	  active->draw(bar,itemx(active,wm),1,itemw(active,wm),1,wm,1);
      }
      if (active) return 1;
      else return 0;
    } break;
    case EV_MOUSE_BUTTON :
    {
      if (active) 
      {
        if (active->handle_event(bar,itemx(active,wm),1,itemw(active,wm),1,wm,ev))
	{
	  active->draw(bar,itemx(active,wm),1,itemw(active,wm),1,wm,0);
	  active=NULL;
	  return 1;
	} else return 0;
      }
      else return 0;
    } break;
  }
  return 0;
}


