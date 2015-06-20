#ifndef __IDLE_HPP_
#define __IDLE_HPP_

#include "lisp.hpp"
#include "specs.hpp"
#include "image.hpp"
#include "event.hpp"
#include "timing.hpp"

class idle_manager
{
  enum { total_wait_frames=8 } ;

  image *wait_cursors[total_wait_frames],*old;
  time_marker last_idle,last_animate;
  int cursor_frame_on,old_cx,old_cy;

  class event_holder
  {
    public :
    event *ev;
    event_holder *next;
  } *first,*last;

  void que_event(event &ev)
  {
    event_holder *n=new event_holder;
    n->ev=new event;
    *n->ev=ev;

    n->next=0;
    if (last)
      last->next=n;
    else first=n;

    last=n;    
  }

  public :

  idle_manager();
  void idle_reset();
  void idle();
  ~idle_manager();
} ;

extern idle_manager *idle_man;

#endif
