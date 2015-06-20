#include "cache.hpp"
#include "gui.hpp"
#include "loader2.hpp"

extern char *symbol_str(char *name);

void ico_button::set_act_id(int id)
{
  activate_id=id;
}

ico_switch_button::ico_switch_button(int X, int Y, int ID, int start_on, ifield *butts, ifield *Next)
{
  x=X; y=Y; id=ID;
  next=Next;  
  blist=cur_but=butts;
  act=0;
  for (ifield *b=blist;b;b=b->next) { b->x=x; b->y=y; }
  while (cur_but && start_on--) cur_but=cur_but->next;
  if (!cur_but) cur_but=blist;
}

void ico_switch_button::area(int &x1, int &y1, int &x2, int &y2, window_manager *wm)
{
  x1=10000;
  y1=10000;
  x2=-10000;
  y2=-10000;
  int X1,Y1,X2,Y2;
  for (ifield *b=blist;b;b=b->next)
  {    
    b->area(X1,Y1,X2,Y2,wm);
    if (X1<x1) x1=X1;
    if (Y1<y1) y1=Y1;
    if (X2>x2) x2=X2;
    if (Y2>y2) y2=Y2;
  }
  if (!blist) { x1=x2=x; y1=y2=y; }
}

ifield *ico_switch_button::unlink(int id)
{
  ifield *last=NULL;
  for (ifield *b=blist;b;b=b->next)
  {
    if (b->id==id)
    {
      if (last) last->next=b->next;
      else blist=b->next;
      if (cur_but==b) cur_but=blist;
      return b;
    } 
    ifield *x=b->unlink(id);
    if (x) return x;
    last=b;
  }
  return NULL;
}

void ico_switch_button::handle_event(event &ev, image *screen, window_manager *wm, input_manager *im)
{
  if ((ev.type==EV_KEY && ev.key==13) || (ev.type==EV_MOUSE_BUTTON &&
                                         ev.mouse_button))
  {
    cur_but=cur_but->next;
    if (!cur_but) cur_but=blist;
    cur_but->draw(act,screen,wm);
    cur_but->handle_event(ev,screen,wm,im);
  }

}

void ico_button::draw(int active, image *screen, window_manager *wm)
{
  int x1,y1,x2,y2;
  area(x1,y1,x2,y2,wm); 
 
  if (active!=act  && activate_id!=-1 && active)
    wm->push_event(new event(activate_id,NULL));
 
  if (up && !active)
    cash.img(u)->put_image(screen,x1,y1);
  else if (up && active)
    cash.img(ua)->put_image(screen,x1,y1);
  else if (!up && !active)
    cash.img(d)->put_image(screen,x1,y1);
  else cash.img(da)->put_image(screen,x1,y1);

  if (act!=active && active && activate_id!=-1)
    wm->push_event(new event(activate_id,NULL));
  act=active;
  
  if (active && key[0])
  {
    int g=80;
    screen->bar(0,0,144,wm->font()->height(),0);
    wm->font()->put_string(screen,0,0,symbol_str(key),color_table->lookup_color(g>>3,g>>3,g>>3));
  } else if (!active && key[0])
  {
    screen->bar(0,0,144,20,0);
  }

}

extern long S_BUTTON_PRESS_SND;
extern int sfx_volume;

void ico_button::handle_event(event &ev, image *screen, window_manager *wm, input_manager *im)
{
  if ((ev.type==EV_KEY && ev.key==13) || (ev.type==EV_MOUSE_BUTTON &&
                                         ev.mouse_button))
  {
    int  x1,y1,x2,y2;
    area(x1,y1,x2,y2,wm);
    up=!up;
    draw(act,screen,wm);
    wm->push_event(new event(id,(char *)this));
    if (S_BUTTON_PRESS_SND)
      cash.sfx(S_BUTTON_PRESS_SND)->play(sfx_volume);
  }
}

void ico_button::area(int &x1, int &y1, int &x2, int &y2, window_manager *wm)
{
  x1=x; y1=y;
  x2=x+cash.img(u)->width()-1;
  y2=y+cash.img(u)->height()-1;
}

ico_button::ico_button(int X, int Y, int ID, int Up, int down, int upa, int downa, ifield *Next, int act_id, char *help_key)
{
  if (help_key)
  {
    strncpy(key,help_key,15);
    key[15]=0;
  }
  else key[0]=0;

  up=1;
  x=X; y=Y; id=ID;
  u=Up; d=down;
  ua=upa; da=downa;
  next=Next;
  activate_id=act_id;
}

ico_switch_button::~ico_switch_button()
{
  while (blist)
  {
    ifield *i=blist;
    blist=blist->next;
    delete i;
  }
}
