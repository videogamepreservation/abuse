#include "input.hpp"
#include "key_cfg.hpp"
#include "game.hpp"
#include "dev.hpp"
#include "gui.hpp"

#ifdef __MAC__
extern int PixMode;
#endif

extern int start_edit,start_running;

enum monitor_draw_type { 
  MONITOR_DOUBLE_PIXEL=1,
  MONITOR_SCANLINED=2,
  MONITOR_SINGLE_PIXEL=3,
  MONITOR_SMOOTH_PIXEL=4,
  MONITOR_ROUND_PIXEL=5

} monitor_draw_mode=MONITOR_DOUBLE_PIXEL;

enum { EDIT_SWITCH=10,
       OK,
       CANCEL
};
        

static ifield *center_ifield(ifield *i, ifield *above, window_manager *eh)
{
  int x1,y1,x2,y2;

  if (above)
  {
    above->area(x1,y1,x2,y2,eh);
    i->y=y2+5;
  }

  i->area(x1,y1,x2,y2,eh);
  i->x=xres/2-(x2-x1)/2;
  return i;
}


void do_monitor_config(window_manager *eh)
{
  int xp,yp;
  load_image_into_screen("art/mon_cfg.spe","monitor_config",xp,yp);
  ifield *first,**p;
  p=&first;

  ifield *first_button,**b,*prev;
  b=&first_button;

  int mode1=cash.reg("art/mon_cfg.spe","mode1",SPEC_IMAGE,1),
    mode1a=cash.reg("art/mon_cfg.spe","mode1+",SPEC_IMAGE,1),
    mode2=cash.reg("art/mon_cfg.spe","mode2",SPEC_IMAGE,1),
    mode2a=cash.reg("art/mon_cfg.spe","mode2+",SPEC_IMAGE,1),
    mode3=cash.reg("art/mon_cfg.spe","mode3",SPEC_IMAGE,1),
    mode3a=cash.reg("art/mon_cfg.spe","mode3+",SPEC_IMAGE,1),
    mode4=cash.reg("art/mon_cfg.spe","mode4",SPEC_IMAGE,1),
    mode4a=cash.reg("art/mon_cfg.spe","mode4+",SPEC_IMAGE,1),
    mode5=cash.reg("art/mon_cfg.spe","mode5",SPEC_IMAGE,1),
    mode5a=cash.reg("art/mon_cfg.spe","mode5+",SPEC_IMAGE,1)
    ;


  int x=xres/2-90/2,y=yp+160;
  ico_button *b1=new ico_button(x,y,MONITOR_DOUBLE_PIXEL,mode1,mode1,mode1a,mode1a,0,-1,"double_pix");
  ico_button *b2=new ico_button(x,y,MONITOR_SCANLINED,mode2,mode2,mode2a,mode2a,0,-1,"scanlined_pix");
  ico_button *b3=new ico_button(x,y,MONITOR_SINGLE_PIXEL,mode3,mode3,mode3a,mode3a,0,-1,"single_pix");
  ico_button *b4=new ico_button(x,y,MONITOR_ROUND_PIXEL,mode4,mode4,mode4a,mode4a,0,-1,"round_pix");
  ico_button *b5=new ico_button(x,y,MONITOR_SMOOTH_PIXEL,mode5,mode5,mode5a,mode5a,0,-1,"smooth_pix");
  b1->next=b2;
  b2->next=b3;
  b3->next=b4;
  b4->next=b5;

  prev=*p=new ico_switch_button(x,y,ID_NULL,
      monitor_draw_mode-1,b1,0);

  p=&((*p)->next);

  prev=*p=(button *)center_ifield(new button(xp+0,yp+0,EDIT_SWITCH,             "   Edit Mode  ",0),prev,eh);
  if (dev&EDIT_MODE)
    ((button *)(*p))->push();

  (*p)->y+=34;
  p=&((*p)->next);

  image *ok_image=cash.img(cash.reg("art/frame.spe","dev_ok",SPEC_IMAGE,1))->copy(),
    *cancel_image=cash.img(cash.reg("art/frame.spe","cancel",SPEC_IMAGE,1))->copy();

  *p=new button(xp+315-20-8,yres/2+480/2-80,OK,ok_image,0); p=&((*p)->next);
  *p=new button(xp+315+20-8,yres/2+480/2-80,CANCEL,cancel_image,0); p=&((*p)->next);
  
  input_manager inm(screen,eh,first);
  inm.allow_no_selections();
  inm.clear_current();
  event ev;

  enum { looping, aborted, finished } state=looping;

  int new_dev=dev;
  monitor_draw_type new_monitor_draw_mode=monitor_draw_mode;
  do
  {
    eh->flush_screen();
    do { eh->get_event(ev); } while (ev.type==EV_MOUSE_MOVE && eh->event_waiting()); 
    inm.handle_event(ev,NULL,eh);
    if (ev.type==EV_KEY && ev.key==JK_ESC)
      state=aborted;
    
    if (ev.type==EV_MESSAGE)
    {
      switch (ev.message.id)
      {
        case OK :
        { state=finished; } break;
        case CANCEL :
        { state=aborted; } break;
        case EDIT_SWITCH :
        { 
          new_dev ^= EDIT_MODE;
        } break;

        case MONITOR_SINGLE_PIXEL :
        { new_monitor_draw_mode=MONITOR_SINGLE_PIXEL; } break;

        case MONITOR_SCANLINED :
        { new_monitor_draw_mode=MONITOR_SCANLINED; } break;

        case MONITOR_DOUBLE_PIXEL :
        { new_monitor_draw_mode=MONITOR_DOUBLE_PIXEL; } break;        

        case MONITOR_ROUND_PIXEL :
        { new_monitor_draw_mode=MONITOR_ROUND_PIXEL; } break;        

        case MONITOR_SMOOTH_PIXEL :
        { new_monitor_draw_mode=MONITOR_SMOOTH_PIXEL; } break;        
      }
    }

  } while (state==looping);

  if (state==finished)
  {
    dev = new_dev;
    monitor_draw_mode=new_monitor_draw_mode;

#ifdef __MAC__
    switch (new_monitor_draw_mode)
    {
      case MONITOR_SINGLE_PIXEL: PixMode=1; break;
      case MONITOR_SCANLINED:    PixMode=2; break;
      case MONITOR_DOUBLE_PIXEL: PixMode=3; break;
      case MONITOR_ROUND_PIXEL:  PixMode=4; break;
      case MONITOR_SMOOTH_PIXEL: PixMode=5; break;
    }
#endif


    if (dev & EDIT_MODE)
    {
      start_edit = 1;
      start_running = 1;
      disable_autolight = 1;
    }
    else
    {
      start_edit = 0;
      start_running = 0;
      disable_autolight = 0;
    }
    monitor_draw_mode=new_monitor_draw_mode;
  }
  delete ok_image;
  delete cancel_image;

}
