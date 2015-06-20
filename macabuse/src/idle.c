#include "idle.hpp"
#include "jwindow.hpp"

extern window_manager *eh;

idle_manager *idle_man=0;

idle_manager::~idle_manager() 
{ 
  idle_reset(); 
  int i;
  for (i=0;i<total_wait_frames;i++)
    delete wait_cursors[i];
}

idle_manager::idle_manager()
{
  cursor_frame_on=-1;
  first=last=0;
  
  bFILE *fp=open_file("art/wait.spe","rb");
  char name[20];
  spec_directory sd(fp);

  for (int i=0;i<total_wait_frames;i++)
  {
    sprintf(name,"watch_%d",i+1);
    spec_entry *se=sd.find(name);
    if (!se)
    {
      lbreak("could not find %s in %s\n",name,"art/wait.spe");
      exit(0);
    }

    wait_cursors[i]=new image(se,fp);
  }
  delete fp;
}

void idle_manager::idle_reset()
{
  last_idle.get_time();
  if (cursor_frame_on!=-1)
  {
    eh->set_mouse_shape(old,old_cx,old_cy);
    cursor_frame_on=-1;

    while (first)
    {
      eh->push_event(first->ev);
      event_holder *l=first;
      first=first->next;
      delete l;
    }
    last=0;
  }
}

void idle_manager::idle()
{
  time_marker now;

  if (cursor_frame_on==-1)
  {
    if (now.diff_time(&last_idle)>0.5)
    {
      old=eh->eh->mouse->mouse_sprite()->visual->copy();
      old_cx=eh->eh->mouse->center_x();
      old_cy=eh->eh->mouse->center_y();
      cursor_frame_on=0;
      eh->set_mouse_shape(wait_cursors[cursor_frame_on]->copy(),0,0);
      eh->flush_screen();
    }
  } else if (now.diff_time(&last_animate)>0.1)
  {
    cursor_frame_on++;
    if (cursor_frame_on>=total_wait_frames)
      cursor_frame_on=0;

    eh->set_mouse_shape(wait_cursors[cursor_frame_on]->copy(),0,0);
    last_animate.get_time();
      eh->flush_screen();
  } else
  {
    event ev;
    while (eh->event_waiting())
    {
      eh->get_event(ev);
      if (ev.type!=EV_MOUSE_MOVE)
        que_event(ev);
      eh->flush_screen();
    }
  }
}


