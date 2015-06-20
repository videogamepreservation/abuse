#include "status.hpp"
#include "timing.hpp"
#include "guistat.hpp"

class gui_status_node
{
  public :  
  char *name;
  gui_status_node *next;
  visual_object *show;
  jwindow *stat_win;
  window_manager *wm;
  int last_update;
  time_marker last_time;
  gui_status_node(char *Name, visual_object *Show, window_manager *WM, gui_status_node *Next) 
  { name=strcpy((char *)jmalloc(strlen(Name)+1,"status name"),Name); 
    show=Show;
    next=Next; 
    last_update=0;
    stat_win=NULL;
    wm=WM;
  }
  ~gui_status_node();
} ; 


gui_status_node::~gui_status_node()
{ 
  jfree(name); 
  if (show) 
    delete show; 
  if (stat_win) 
  {
    wm->close_window(stat_win);
    wm->flush_screen();
  }
}

#ifdef __POWERPC__
#pragma global_optimizer on
#endif

void gui_status_manager::draw_bar(gui_status_node *whom, int perc)
{
  long l=whom->stat_win->x2()-whom->stat_win->x1()-6;
  long h=wm->font()->height();

  whom->stat_win->screen->bar(whom->stat_win->x1()+1,whom->stat_win->y2()-h-1,whom->stat_win->x2()-1,
		      whom->stat_win->y2()-1,wm->black());
  whom->stat_win->screen->bar(whom->stat_win->x1()+2,whom->stat_win->y2()-h,whom->stat_win->x2()-2,
		      whom->stat_win->y2()-2,wm->dark_color());
  if (perc)
    whom->stat_win->screen->bar(whom->stat_win->x1()+3,whom->stat_win->y2()-h+1,
				whom->stat_win->x1()+l*perc/100,
				whom->stat_win->y2()-3,wm->bright_color());
}

#ifdef __POWERPC__
#pragma global_optimizer reset
#endif

void gui_status_manager::push(char *name, visual_object *show)
{
  first=new gui_status_node(name,show,wm,first);  
}

gui_status_manager::gui_status_manager(window_manager *WM) 
{ 
  wm=WM; 
  first=NULL; 
  strcpy(title,"STATUS"); 
  last_perc=0;
}

void gui_status_manager::update(int percentage)
{
  last_perc=percentage;
  if (first)
  {
    if (!first->stat_win)
    {
      time_marker now;
      if (now.diff_time(&first->last_time)>1)
      {
	long wx=xres/2,wy=10,len1=strlen(first->name)*wm->font()->width()+10,len2=0,len3,
	  h1=wm->font()->height()+5,h2=first->show ? first->show->height(wm) : 0;

	if (first->show) len2=first->show->width(wm)/2;
	if (len2>len1) len3=len2; else len3=len1;
	wx-=len3/2; 
	
	
	gui_status_node *p=first->next;
	while (p && !p->stat_win) p=p->next;
	if (p) wy=p->stat_win->y+p->stat_win->y2()+5;

	int mx=WINDOW_FRAME_LEFT,my=WINDOW_FRAME_TOP;
	first->stat_win=wm->new_window(wx,wy,len3, h1*2+h2,NULL,"status");
	wm->font()->put_string(first->stat_win->screen,mx+1,my+1,first->name,wm->black());
	wm->font()->put_string(first->stat_win->screen,mx,my,first->name,wm->bright_color());
	if (first->show)
	  first->show->draw(first->stat_win->screen,(first->stat_win->x2()-first->stat_win->x1())/2-
			    first->show->width(wm)/2,my+h1,wm,NULL);

	draw_bar(first,percentage);
	wm->flush_screen();
      }
    } else
    {
      if (percentage>first->last_update)
      {
	first->last_update=percentage;
	draw_bar(first,percentage);
	wm->flush_screen();
      }
    }
  } 
}

void gui_status_manager::force_display()
{
  update(last_perc);
}

void gui_status_manager::pop()
{
  CONDITION(first,"No status's to pop!");
  gui_status_node *p=first;
  first=first->next;
  delete p;
}









