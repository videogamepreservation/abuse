#include "profile.hpp"
#include "jwindow.hpp"
#include "property.hpp"
#include "objects.hpp"

extern window_manager *eh;


jwindow *prof_win=NULL;
int prof_height=10;

struct prof_info
{
  ushort otype;
  float total_time;
};


prof_info *prof_list=NULL;


int profiling() { return prof_list!=NULL; }

void profile_toggle()
{
  if (prof_win) { profile_uninit(); }
  else profile_init();
}

int profile_handle_event(event &ev)
{
  if (ev.type==EV_CLOSE_WINDOW && ev.window==prof_win)
  {
    profile_toggle();
    return 1;
  } else return 0;
}

void profile_init()
{
  if (prof_list) { profile_uninit(); }
  prof_list=(prof_info *)jmalloc(sizeof(prof_info)*total_objects,"prof info");
  profile_reset();

  
  prof_win=eh->new_window(prop->getd("profile x",-1),
			  prop->getd("profile y",-1),
			  20*console_font->width(),
			  (prof_height+1)*console_font->height(),
			  NULL,
			  "PROFILE");
}


void profile_reset()
{
  int i;
  for (i=0;i<total_objects;i++)
  {
    prof_list[i].otype=i;
    prof_list[i].total_time=0;
  }  
  
}


void profile_uninit()
{
  if (prof_list) jfree(prof_list);
  prof_list=NULL;
  if (prof_win) { eh->close_window(prof_win); prof_win=NULL; }
}


void profile_add_time(int type, float amount)
{
  if (prof_list)
  { prof_list[type].total_time+=amount; }
}

static int p_sorter(const void *a, const void *b)
{
  if (((prof_info *)a)->total_time<((prof_info *)b)->total_time)
    return 1;
  else if (((prof_info *)a)->total_time>((prof_info *)b)->total_time)
    return -1;
  else return 0;
}

static void profile_sort()
{
  qsort(prof_list,total_objects,sizeof(prof_info),p_sorter);
}


void profile_update()
{
  profile_sort();
  if (prof_list[0].total_time<=0.0) return ;     // nothing took any time!

  int i=0;
  int spliter=(prof_win->x2()+prof_win->x1())/2;
  int max_bar_length=spliter-prof_win->x1();


  float time_scaler=(float)max_bar_length/prof_list[0].total_time;
  
  prof_win->screen->bar(0,prof_win->y1(),prof_win->screen->width()-1,prof_win->screen->height(),0);
  int dy=WINDOW_FRAME_TOP; 
	char s[280];
  for (;i<prof_height;i++)
  {
#if 1
    console_font->put_string(prof_win->screen,spliter+1,dy,object_names[prof_list[i].otype]);
    prof_win->screen->bar(spliter-1-(int)(prof_list[i].total_time*time_scaler),dy+1,
			  spliter-1,
			  dy+console_font->height()-1,eh->bright_color());
#else
		sprintf(s, "%8f:%s", prof_list[i].total_time*1000.0, object_names[prof_list[i].otype]);
    console_font->put_string(prof_win->screen,0,dy,s);
#endif
    dy+=console_font->height()+1;
  }

}






