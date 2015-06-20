#include "system.h"
#include "image.hpp"
#include "palette.hpp"
#include "mdlread.hpp"
#include "video.hpp"
#include "gifread.hpp"
#include "macs.hpp"
#include "mouse.hpp"
#include "event.hpp"
#include "sprite.hpp"
#include "mackeys.hpp"

#include <OSEvents.h>

extern CWindowPtr mainwin;
const short MACEVENTS = mUpMask | mDownMask | keyUpMask | keyDownMask | autoKeyMask;
short OldEventMask;
EventRecord MacEv;
int mmoved,mac_ready,keyed;
char mac_modifier[8],mac_oldmod[8];

int mac_map[8] = 
{
	JK_COMMAND,
	JK_SHIFT_L,
	JK_CAPS,
	JK_ALT_L,
	JK_CTRL_L,
	JK_SHIFT_R,
	JK_ALT_R,
	JK_CTRL_R,
};

void event_handler::flush_screen()
{
  update_dirty(screen);
}

int event_handler::get_key_flags()
{
	return 0;
}

int event_handler::event_waiting()
{
	unsigned char km[16];
	int i;
	
	if (ewaiting || mmoved || mac_ready || keyed)
		return 1;

	// Get key states & map to array
	GetKeys((unsigned long*)&km);
	for (i=0; i<MAC_LAST-MAC_COMMAND; i++) {
		mac_modifier[i] = (km[ (i+MAC_COMMAND) >> 3 ] >> ((i+MAC_COMMAND) & 7)) & 1;
		if (mac_modifier[i] != mac_oldmod[i])
			keyed = 1;
	}

	mouse->update();
	if (mouse->lastx() != mouse->x() || mouse->lasty() != mouse->y())
	{
		mmoved = 1;
	}

	mac_ready = (mac_ready)? 1 : GetOSEvent(MACEVENTS,&MacEv);

	return ewaiting || mmoved ||mac_ready || keyed;
}

event_handler::event_handler(image *screen, palette *pal)
{
  CHECK(screen && pal);
  mouse=new JCMouse(screen,pal);
  mhere=1;
  OldEventMask = *((short*)0x0144);
	SetEventMask(MACEVENTS);
  ewaiting=0;
  mmoved = 0;
  mac_ready = 0;
}

void event_handler::get_event(event &ev)
{
  int kf,kf_change=0;
  event *ep;
  
  while (!event_waiting()) ;

  ep=(event *)events.first();
  if (ep)
  {
  	ev=*ep;
    events.unlink((linked_node *)ep);
    delete ep;
    ewaiting = (events.first()!=NULL);
  }
  else if (mmoved) {
  	ev.type = EV_MOUSE_MOVE;
    ev.mouse_move.x=mouse->x();
    ev.mouse_move.y=mouse->y();
    ev.mouse_button=mouse->button();
    mouse->mouse_sprite()->x = mouse->x();
    mouse->mouse_sprite()->y = mouse->y();
  	mmoved = 0;
  }
  else if (keyed) {
  	int found;
  	
    ev.mouse_move.x=mouse->x();
    ev.mouse_move.y=mouse->y();
    ev.mouse_button=mouse->button();
		found = -1;
		keyed = 0;
  	for (int i=0; i<MAC_LAST-MAC_COMMAND; i++) 
  	{
			if (mac_modifier[i] != mac_oldmod[i])
				if (found<0) 
					found = i;
				else
					keyed = 1;
		}
		ev.key = mac_map[found];
		mac_oldmod[found] = mac_modifier[found];
		if (mac_modifier[found])
			ev.type = EV_KEY;
		else
			ev.type = EV_KEYRELEASE;
	}
  else if (mac_ready)
  {
    // note : that the mouse status
    // should be know even if
    // other event has occured
    ev.mouse_move.x=mouse->x();
    ev.mouse_move.y=mouse->y();
    ev.mouse_button=mouse->button();

    switch (MacEv.what)
    {
     	/*
        case ConfigureNotify :
      		break;
        case updateEvt :
        {
        	ev.type=EV_REDRAW;
          ev.redraw.x1=0;
          ev.redraw.y1=0;
          ev.redraw.x2=ev.redraw.x1+10;  // must replace this to redraw whole window!
          ev.redraw.y2=ev.redraw.y1+10;
          ev.redraw.start=NULL;
        } break;
      */
      case keyDown :
      case keyUp :
      case autoKey :
      {
        if (MacEv.what==keyUp)
          ev.type=EV_KEYRELEASE; 
        else
        	ev.type=EV_KEY;
        switch ((MacEv.message & keyCodeMask)>>8)
        {
        	case 0x7d : 	ev.key=JK_DOWN; break;
          case 0x7e :  	ev.key=JK_UP;  break;
          case 0x7b : 	ev.key=JK_LEFT;  break;
          case 0x7c : 	ev.key=JK_RIGHT;  break;
          /*
          case XK_Control_L : 	ev.key=JK_CTRL_L;  break;
          case XK_Control_R : 	ev.key=JK_CTRL_R;  break;
          case XK_Alt_L : 		ev.key=JK_ALT_L;  break;
          case XK_Alt_R : 		ev.key=JK_ALT_R;  break;
          case XK_Shift_L : 	ev.key=JK_SHIFT_L;  break;
          case XK_Shift_R : 	ev.key=JK_SHIFT_R;  break;
          */
          case 0x47 : 	ev.key=JK_NUM_LOCK;  break;
          case 0x73 : 	ev.key=JK_HOME;  break;
          case 0x77 : 	ev.key=JK_END;  break;
          case 0x33 :		ev.key=JK_BACKSPACE;  break;
          case 0x30 :		ev.key=JK_TAB;  break;
          case 0x4c :		ev.key=JK_ENTER;  break;
          case 0x39 :		ev.key=JK_CAPS;  break;
        	case 0x35 :		ev.key=JK_ESC;  break;
         	case 0x71 :   ev.key=JK_F1; break;
          case 0x78 :   ev.key=JK_F2;break;
          case 0x63 :   ev.key=JK_F3; break;
          case 0x76 :   ev.key=JK_F4; break;
          case 0x60 :   ev.key=JK_F5; break;
          case 0x61 :   ev.key=JK_F6; break;
          case 0x62 :   ev.key=JK_F7; break;
          case 0x64 :   ev.key=JK_F8; break;
          case 0x65 :   ev.key=JK_F9; break;
          case 0x6d :   ev.key=JK_F10; break;
      		case 0x67 :   ev.key=JK_INSERT; break;
      		case 0x74 :   ev.key=JK_PAGEUP; break;
      		case 0x79 :   ev.key=JK_PAGEDOWN; break;
      		
          default :
            // modifiers in event.modifiers
						ev.key = MacEv.message &charCodeMask;
        }
      } break;
      /*
      case MotionNotify : 
      {
        mouse->update(MacEv.xmotion.x,MacEv.xmotion.y,mouse->button());
        ev.type=EV_MOUSE_MOVE;
        ev.mouse_move.x=mouse->x();
        ev.mouse_move.y=mouse->y();
      } break;
      */
      case mouseUp :
      case mouseDown :
      { 
        ev.type=EV_MOUSE_BUTTON;
      } break;
      default:
      	ev.type = EV_SPURIOUS;
      	break;
    }
    mac_ready=0;
  }
  else
  	ev.type = EV_SPURIOUS;
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

event_handler::~event_handler() 
{
	SetEventMask(OldEventMask);
	delete mouse; 
}






