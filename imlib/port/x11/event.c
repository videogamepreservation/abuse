#include "system.h"
#include "xinclude.h"
#include "image.hpp"
#include "palette.hpp"
#include "mdlread.hpp"
#include "video.hpp"
#include "gifread.hpp"
#include "macs.hpp"
#include "mouse.hpp"
#include "event.hpp"
#include "sprite.hpp"
#include "xinclude.h"


extern Display *display;
extern int screen_num;
extern Screen *screen_ptr;
extern Window mainwin;
extern int pixel_scale;

int showkeys = 0;

void event_handler::flush_screen()
{
  update_dirty(screen);
}

int event_handler::get_key_flags()
{
  Window w1,w2;
  int j;
  unsigned mask;
  XQueryPointer(display,mainwin,&w1,&w2,&j,&j,&j,&j,&mask);
  return ((mask&ShiftMask)!=0)<<3 | ((mask&ControlMask)!=0)<<2 |
	 ((mask&8)!=0)<<1;
}


int event_handler::event_waiting()
{
  if (ewaiting) return 1;
  if (XPending(display))
    ewaiting=1;
/*  else if (last_keystat!=get_key_flags())
    ewaiting=1;*/
  return ewaiting;
}

event_handler::event_handler(image *screen, palette *pal)
{
	extern int start_argc;
	extern char **start_argv;
  int i;
  CHECK(screen && pal);
  mouse=new JCMouse(screen,pal);
  mhere=mouse->exsist();
  last_keystat=get_key_flags();
  XSelectInput(display,mainwin,
    KeyPressMask | VisibilityChangeMask | ButtonPressMask | ButtonReleaseMask |
    ButtonMotionMask | PointerMotionMask | KeyReleaseMask |
    ExposureMask | StructureNotifyMask);
  mhere=1;
  ewaiting=0;
  for (i=0 ; i<start_argc ; i++)
	if (!strcmp(start_argv[i], "-showkeys"))
	  showkeys = 1;
}

void event_handler::get_event(event &ev)
{
  int kf,kf_change=0;
  event *ep;
  while (!ewaiting)
    event_waiting();

  ep=(event *)events.first();
  if (ep)
  { ev=*ep;
    events.unlink((linked_node *)ep);
    delete ep;
    ewaiting=events.first()!=NULL;
  }
  else
  {
    // note : that the mouse status
    // should be know even if
    // other event has occured
    ev.mouse_move.x=mouse->x();
    ev.mouse_move.y=mouse->y();
    ev.mouse_button=mouse->button();

/*    kf=get_key_flags();
    if (kf!=last_keystat)
    { last_keystat=kf;
      kf_change=1;
      ev.type=EV_KEY_SPECIAL;
      ev.key_special.shift=kf&2;
      ev.key_special.ctrl=kf&4;
      ev.key_special.alt=kf&8;
    } */

    XEvent xev;
    image *n;
    int np,ij,sp;
    linked_node *pagep;
    if (XPending(display))
    {
      XNextEvent(display,&xev);
      ev.window=(jwindow *)xev.xany.window;
      switch (xev.type)
      { 
        case ConfigureNotify :
 	{
          if (pixel_scale == 1 && xev.xany.window==mainwin && (screen->width()!=xev.xconfigure.width || 
		screen->height()!=xev.xconfigure.height))
          {
	    XFlush(display);
	    int new_width=xev.xconfigure.width&~3;  // must be word alligned
	    if (new_width!=xev.xconfigure.width)
	      XResizeWindow(display,mainwin,new_width,xev.xconfigure.height);

	    screen->change_size(new_width,xev.xconfigure.height);
	    screen->clear_dirties();
	    screen->clear();

            xres=screen->width()-1;
            yres=screen->height()-1;
            ev.type=EV_RESIZE;
          }
          else 
	    ev.type=EV_SPURIOUS;
        } break;
          
        case Expose :
        { ev.type=EV_REDRAW;
          ev.redraw.x1=xev.xexpose.x;
          ev.redraw.y1=xev.xexpose.y;
          ev.redraw.x2=ev.redraw.x1+xev.xexpose.width;
          ev.redraw.y2=ev.redraw.y1+xev.xexpose.height;
          ev.redraw.start=NULL;
        } break;
        case KeyPress :
        case KeyRelease :
        {
          if (!kf_change)  // if the key flags changed, it's not a real key
          {
            char buf;
            KeySym ks;
            XLookupString(&xev.xkey,&buf,1,&ks,NULL); 
            if (xev.type==KeyPress)
			{
              ev.type=EV_KEY; 
			  if (showkeys)
				fprintf(stderr, "keysym 0x%x\n", ks);
			}
            else ev.type=EV_KEYRELEASE;
            switch (ks)
            { case XK_Down : 		ev.key=JK_DOWN; break;
              case XK_Up :  		ev.key=JK_UP;  break;
              case XK_Left : 		ev.key=JK_LEFT;  break;
              case XK_Right : 		ev.key=JK_RIGHT;  break;
              case XK_Control_L : 	ev.key=JK_CTRL_L;  break;
              case XK_Control_R : 	ev.key=JK_CTRL_R;  break;
              case XK_Execute : 	ev.key=JK_CTRL_R;  break;
              case XK_Alt_L : 		ev.key=JK_ALT_L;  break;
              case XK_Alt_R : 		ev.key=JK_ALT_R;  break;
              case XK_Shift_L : 	ev.key=JK_SHIFT_L;  break;
              case XK_Shift_R : 	ev.key=JK_SHIFT_R;  break;
              case XK_Num_Lock : 	ev.key=JK_NUM_LOCK;  break;
              case XK_Home : 		ev.key=JK_HOME;  break;
              case XK_End : 		ev.key=JK_END;  break;
              case XK_BackSpace :	ev.key=JK_BACKSPACE;  break;
              case XK_Tab :		ev.key=JK_TAB;  break;
              case XK_Return :		ev.key=JK_ENTER;  break;
              case XK_Caps_Lock :	ev.key=JK_CAPS;  break;
              case XK_Escape :		ev.key=JK_ESC;  break;
              case XK_F1 :              ev.key=JK_F1; break;
              case XK_F2 :              ev.key=JK_F2; break;
              case XK_F3 :              ev.key=JK_F3; break;
              case XK_F4 :              ev.key=JK_F4; break;
              case XK_F5 :              ev.key=JK_F5; break;
              case XK_F6 :              ev.key=JK_F6; break;
              case XK_F7 :              ev.key=JK_F7; break;
              case XK_F8 :              ev.key=JK_F8; break;
              case XK_F9 :              ev.key=JK_F9; break;
              case XK_F10 :             ev.key=JK_F10; break;
	      case XK_Insert :          ev.key=JK_INSERT; break;
	      case XK_KP_0 :          ev.key=JK_INSERT; break;
	      case XK_Page_Up :         ev.key=JK_PAGEUP; break;
	      case XK_Page_Down :       ev.key=JK_PAGEDOWN; break;
              default :
               if (buf)
               {
                 ev.key=(int)buf;
               }
               else ev.type=EV_SPURIOUS;
            }
          }
        } break;
        case MotionNotify : 
        {
          mouse->update(xev.xmotion.x/pixel_scale,xev.xmotion.y/pixel_scale,mouse->button());
          ev.type=EV_MOUSE_MOVE;
          ev.mouse_move.x=mouse->x();
          ev.mouse_move.y=mouse->y();
        } break;
        case ButtonRelease :
        { switch (xev.xbutton.button)
          { case 1 : ev.mouse_button&=(0xff-1); break;
            case 2 : ev.mouse_button&=(0xff-4); break;
            case 3 : ev.mouse_button&=(0xff-2); break;
          }
        } // no break here please...
        case ButtonPress :
        { 
          if (xev.type==ButtonPress)
          { switch (xev.xbutton.button)
            { case 1 : ev.mouse_button|=1; break;
              case 2 : ev.mouse_button|=4; break;
              case 3 : ev.mouse_button|=2; break;
            }
          }
          mouse->update(mouse->x(),mouse->y(),ev.mouse_button);
          ev.type=EV_MOUSE_BUTTON;
          ev.mouse_move.x=mouse->x();
          ev.mouse_move.y=mouse->y();
        } break;
      }
    }
    ewaiting=0;
  }
}


void event_handler::add_redraw(int X1, int Y1, int X2, int Y2, void *Start)
{
  event *ev;
  ev=new event;
  ev->type=EV_REDRAW;
  ev->redraw.x1=X1; ev->redraw.x2=X2;
  ev->redraw.y1=Y1; ev->redraw.y2=Y2; ev->redraw.start=Start;
  events.add_end((linked_node *)ev);
}

event_handler::~event_handler() { delete mouse; }






