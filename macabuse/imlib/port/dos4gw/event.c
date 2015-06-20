#include "system.h"
#include "image.hpp"
#include "palette.hpp"
#include "mdlread.hpp"
#include "video.hpp"
#include "dos.h"
#include "macs.hpp"
#include "mouse.hpp"
#include "conio.h"
#include "event.hpp"
#include "sprite.hpp"
#include "monoprnt.hpp"
#include <ctype.h>
#include <conio.h>
#include <dos.h>

#define __EMU_MOUSE__

void event_handler::flush_screen()
{
#ifdef __EMU_MOUSE__
  mouse->mouse_sprite()->x=mouse->drawx();
  mouse->mouse_sprite()->y=mouse->drawy();
  mouse->mouse_sprite()->get_background();
  mouse->mouse_sprite()->draw();
#endif
  update_dirty(screen);
#ifdef __EMU_MOUSE__
  mouse->mouse_sprite()->restore_background();
#endif
}

unsigned char shift_pressed;

int key_translate(unsigned char ch)
{
  switch (ch)
  {
    case 30 : return shift_pressed ? 'A' : 'a'; break;
    case 48 : return shift_pressed ? 'B' : 'b'; break;
    case 46 : return shift_pressed ? 'C' : 'c'; break;
    case 32 : return shift_pressed ? 'D' : 'd'; break;
    case 18 : return shift_pressed ? 'E' : 'e'; break;
    case 33 : return shift_pressed ? 'F' : 'f'; break;
    case 34 : return shift_pressed ? 'G' : 'g'; break;
    case 35 : return shift_pressed ? 'H' : 'h'; break;
    case 23 : return shift_pressed ? 'I' : 'i'; break;
    case 36 : return shift_pressed ? 'J' : 'j'; break;
    case 37 : return shift_pressed ? 'K' : 'k'; break;
    case 38 : return shift_pressed ? 'L' : 'l'; break;
    case 50 : return shift_pressed ? 'M' : 'm'; break;
    case 49 : return shift_pressed ? 'N' : 'n'; break;
    case 24 : return shift_pressed ? 'O' : 'o'; break;
    case 25 : return shift_pressed ? 'P' : 'p'; break;
    case 16 : return shift_pressed ? 'Q' : 'q'; break;
    case 19 : return shift_pressed ? 'R' : 'r'; break;
    case 31 : return shift_pressed ? 'S' : 's'; break;
    case 20 : return shift_pressed ? 'T' : 't'; break;
    case 22 : return shift_pressed ? 'U' : 'u'; break;
    case 47 : return shift_pressed ? 'V' : 'v'; break;
    case 17 : return shift_pressed ? 'W' : 'w'; break;
    case 45 : return shift_pressed ? 'X' : 'x'; break;
    case 21 : return shift_pressed ? 'Y' : 'y'; break;
    case 44 : return shift_pressed ? 'Z' : 'z'; break;
    case 51 : return shift_pressed ? '<' : ','; break;
    case 52 : return shift_pressed ? '>' : '.'; break;
    case 53 : return shift_pressed ? '?' : '/'; break;      
    case 39 : return shift_pressed ? ':' : ';'; break;
    case 40 : return shift_pressed ? '"' : '\''; break;
    case 26 : return shift_pressed ? '{' : '['; break;
    case 27 : return shift_pressed ? '}' : ']'; break;
    case 43 : return shift_pressed ? '|' : '\\'; break;
    case 13 : return shift_pressed ? '+' : '='; break;
    case 12 : return shift_pressed ? '_' : '-'; break;      
    case 11 : return shift_pressed ? ')' : '0'; break;
    case 10 : return shift_pressed ? '(' : '9'; break;
    case  9 : return shift_pressed ? '*' : '8'; break;
    case  8 : return shift_pressed ? '&' : '7'; break;
    case  7 : return shift_pressed ? '^' : '6'; break;
    case  6 : return shift_pressed ? '%' : '5'; break;
    case  5 : return shift_pressed ? '$' : '4'; break;
    case  4 : return shift_pressed ? '#' : '3'; break;
    case  3 : return shift_pressed ? '@' : '2'; break;
    case  2 : return shift_pressed ? '!' : '1'; break;
    case 41 : return shift_pressed ? '~' : '`'; break;
     
    case 14 : return JK_BACKSPACE; break;
    case 15 : return JK_TAB; break;
    case 28 : return JK_ENTER; break;
    case 1 : return JK_ESC; break;
    case 57 : return JK_SPACE; break;
      
    case 72 : return JK_UP; break;
    case 80 : return JK_DOWN; break;
    case 75 : return JK_LEFT; break;
    case 77 : return JK_RIGHT; break;       
    case 29 : return JK_CTRL_L; break;     
    case 56 : return JK_ALT_L; break;
    case 42 : return JK_SHIFT_L; break;     
    case 54 : return JK_SHIFT_R; break;     
    case 58 : return JK_CAPS; break;     
    case 69 : return JK_NUM_LOCK; break;         
    case 71 : return JK_HOME; break;     
    case 79 : return JK_END; break;     
    case 83 : return JK_DEL; break;     
    case 82 : return JK_INSERT; break;

    case 59 : return JK_F1; break;     
    case 60 : return JK_F2; break;     
    case 61 : return JK_F3; break;     
    case 62 : return JK_F4; break;     
    case 63 : return JK_F5; break;     
    case 64 : return JK_F6; break;     
    case 65 : return JK_F7; break;     
    case 66 : return JK_F8; break;     
    case 67 : return JK_F9; break;     
    case 68 : return JK_F10; break;        
    case 73 : return JK_PAGEUP; break;
    case 81 : return JK_PAGEUP; break;
    default :
      return 0;      
  }
}  


void (__interrupt __far *old_key_intr)();
unsigned char key_que[256],key_que_head=0,key_que_tail=0,
                alt_flag=0,ctrl_flag=0,shift_flag=0,
                nice_key_mode=0;   // chain to old interrupt ?
              
void __interrupt keyboard_intr_handler()
{  
  unsigned char k=inp(0x60);                // read the key code
  if (k!=224)                               // 224 is extended keyboard info
                                            // we can just throw it away
    key_que[key_que_head++]=k;                // stick it in the que  
  outp(0x20,0x20);                          // notify serviced int
  
  // allow the user to uninstall the key board handler
  // by pressing CTRL-ALT-SHIFT-C  :)  (don't want people to do it!)
  
  switch (k&127)
  {
    case 56 : alt_flag=!(k&128);     break;
    case 29 : ctrl_flag=!(k&128);    break;
    case 54 :
    case 42 : shift_flag=!(k&128);  break;
    case 46 : if (alt_flag & ctrl_flag & shift_flag) 
                _dos_setvect(9,old_key_intr); break;
  }  
  if (nice_key_mode)
    _chain_intr(old_key_intr);
}


int event_handler::get_key_flags()
{
  return 0;
}



void keyboard_init()
{
  old_key_intr=_dos_getvect(9);
  _dos_setvect(0x8009,keyboard_intr_handler);
}

  


void keyboard_uninit()          // install the old int 9 handler
{  
  _dos_setvect(9,old_key_intr);
}


char *key_names[]= {"Up Arrow","Down Arrow","Left Arrow","Right Arrow",
                    "Left Ctrl","Right Ctrl","Left Alt","Right Alt",
                    "Left Shift","Right Shift","Caps Lock","Num Lock",
                    "Home","End","Del","F1","F2","F3","F4","F5","F6",
                    "F7","F8","F9","F10"};


char *jkey_name(int key)
{
  static char sing[2];
 if (key>255)
    return key_names[key-256];
  else if (key==JK_BACKSPACE)
    return "Backspace";
  else if (key==JK_TAB)
    return "Tab";
  else if (key==JK_ENTER)
    return "Enter";
  else if (key==JK_ESC)
    return"Esc";
  else if (key==JK_SPACE)
    return "Space";
  else if (isprint(key))
  {
    sing[0]=key;
    sing[1]=0;
    return sing;
  } 
  return NULL;
}

int event_handler::event_waiting()
{
  int ch;
  event *evt;
  
  while (key_que_head!=key_que_tail)
  { 
    int up=key_que[key_que_tail]&128,
        key=key_que[key_que_tail]&127;
      
    evt=new event;
   
    evt->key=key_translate(key);
    if (up)
    {
      evt->type=EV_KEYRELEASE;
    }
    else  
    {
      evt->type=EV_KEY;
    }
    
    if (evt->key==JK_SHIFT_L || evt->key==JK_SHIFT_R)
      shift_pressed=!up;    
    events.add_end((linked_node *)evt);     
    ewaiting=1;     
    key_que_tail++;
  }    
  if (mhere)
  {
    mouse->update();
    if (mouse->lastx()!=mouse->x() || mouse->lasty()!=mouse->y()
       || mouse->last_button()!=mouse->button())
      ewaiting=1;
  }
  return ewaiting;
}

event_handler::event_handler(image *screen, palette *pal)
{
  CHECK(screen && pal);
  mouse=new JCMouse(screen,pal);
  mhere=mouse->exsist();
  last_keystat=get_key_flags();
  ewaiting=0;
  keyboard_init();  
}


void event_handler::get_event(event &ev)
{
  int kf,kf_change=0,ch;
  event *evt;
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
    kf=get_key_flags();
    if (kf!=last_keystat)
    { last_keystat=kf;
      kf_change=1;
      ev.type=EV_KEY_SPECIAL;
      ev.key_special.shift=kf&2;
      ev.key_special.ctrl=kf&4;
      ev.key_special.alt=kf&8;
    }
    else if (mhere && (mouse->last_button()!=mouse->button()))
      ev.type=EV_MOUSE_BUTTON;
    else if (mhere && (mouse->lastx()!=mouse->x() ||
	     mouse->lasty()!=mouse->y()))
      ev.type=EV_MOUSE_MOVE;
    else ev.type=EV_SPURIOUS;

    if (ev.type==EV_MOUSE_MOVE)
    {
      mouse->mouse_sprite()->x=mouse->x();
      mouse->mouse_sprite()->y=mouse->y();
    }
    ewaiting=0;
  }
  ev.mouse_move.x=mouse->x();
  ev.mouse_move.y=mouse->y();
  ev.mouse_button=mouse->button();

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
  delete mouse;
  keyboard_uninit(); 
}



