#include "system.h"
#include "image.hpp"
#include "palette.hpp"
#include "mdlread.hpp"
#include "video.hpp"
#include "dos.h"
#include "gifread.hpp"
#include "macs.hpp"
#include "mouse.hpp"
#include "event.hpp"
#include "sprite.hpp"
#include "monoprnt.hpp"
#include "dprint.hpp"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include "keys.hpp"
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <termios.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/keyboard.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#undef EGA
#include <vga.h>


int console_fd;

void event_handler::flush_screen()
{
  mouse->mouse_sprite()->x=mouse->drawx();
  mouse->mouse_sprite()->y=mouse->drawy();
  mouse->mouse_sprite()->get_background();
  mouse->mouse_sprite()->draw();
  update_dirty(screen);
  mouse->mouse_sprite()->restore_background();
}

int event_handler::get_key_flags()
{
  return 0;
}

int shift;

int fd_ready_to_read(int fd)
{
  struct timeval tv={0,0};
  fd_set kbd_set;
  FD_ZERO(&kbd_set);
  FD_SET(fd,&kbd_set);
  select(FD_SETSIZE,&kbd_set,NULL,NULL,&tv);
  return (FD_ISSET(fd,&kbd_set));
}
 

int key_waiting()
{
  return fd_ready_to_read(console_fd);
}

int con_translate(int ch)
{
  int up=(!(ch&0x80)),key=ch&0x7f;
  if (key==42 || key==54)
    shift=up;
   
  switch (key)
  {
    case 73 : ch=JK_PAGEUP;                       break;
    case 81 : ch=JK_PAGEDOWN;                     break;
    case 42 : ch=JK_SHIFT_L;                      break;
    case 54 : ch=JK_SHIFT_R;                      break;
    case 72 : ch=JK_UP;                           break;
    case 80 : ch=JK_DOWN;                         break;
    case 82 : ch=JK_INSERT;                       break;
    case 75 : ch=JK_LEFT;                         break;
    case 77 : ch=JK_RIGHT;                        break;
    case 29 : ch=JK_CTRL_L;                       break;
    case 56 : ch=JK_ALT_L;                        break;
    case 58 : ch=JK_CAPS;                         break;
    case 69 : ch=JK_NUM_LOCK;                     break;
    case 71 : ch=JK_HOME;                         break;
    case 79 : ch=JK_END;                          break;
    case 83 : ch=JK_DEL;                          break;
    case 57 : ch=JK_SPACE;                        break;
    case 1  : ch=JK_ESC;                          break;
    case 28  : ch=JK_ENTER;                       break;
    case 15  : ch=JK_TAB;                         break;
    case 14  : ch=JK_BACKSPACE;                   break;
    case 2  :  ch=shift ? '!' : '1';                break;
    case 3  :  ch=shift ? '@' : '2';                break;
    case 4  :  ch=shift ? '#' : '3';                break;
    case 5  :  ch=shift ? '$' : '4';                break;
    case 6  :  ch=shift ? '%' : '5';                break;
    case 7  :  ch=shift ? '^' : '6';                break;
    case 8  :  ch=shift ? '&' : '7';                break;
    case 9  :  ch=shift ? '*' : '8';                break;
    case 10  :  ch=shift ? '(' : '9';                break;
    case 11  :  ch=shift ? ')' : '0';                break;
    case 12  :  ch=shift ? '_' : '-';                break;
    case 13  :  ch=shift ? '+' : '=';                break;
    case 41  :  ch=shift ? '~' : '`';                break;
    case 26  :  ch=shift ? '{' : '[';                break;
    case 27  :  ch=shift ? '}' : ']';                break;
    case 39  :  ch=shift ? ':' : ';';                break;
    case 40  :  ch=shift ? '"' : '\'';               break;
    case 51  :  ch=shift ? '<' : ',';                break;
    case 52  :  ch=shift ? '>' : '.';                break;
    case 53  :  ch=shift ? '?' : '/';                break;
    case 43  :  ch=shift ? '|' : '\\';               break;
    case 59 :  ch=JK_F1;                             break; 
    case 60 :  ch=JK_F2;                             break; 
    case 61 :  ch=JK_F3;                             break; 
    case 62 :  ch=JK_F4;                             break; 
    case 63 :  ch=JK_F5;                             break; 
    case 64 :  ch=JK_F6;                             break; 
    case 65 :  ch=JK_F7;                             break; 
    case 66 :  ch=JK_F8;                             break; 
    case 67 :  ch=JK_F9;                             break; 
    case 68 :  ch=JK_F10;                            break; 

    default :
      switch (key)
      {
        case 30 : ch='a';       break;
        case 48 : ch='b';       break;
        case 46 : ch='c';       break;
        case 32 : ch='d';       break;
        case 18 : ch='e';       break;
        case 33 : ch='f';       break;
        case 34 : ch='g';       break;
        case 35 : ch='h';       break;
        case 23 : ch='i';       break;
        case 36 : ch='j';       break;
        case 37 : ch='k';       break;
        case 38 : ch='l';       break;
        case 50 : ch='m';       break;
        case 49 : ch='n';       break;
        case 24 : ch='o';       break;
        case 25 : ch='p';       break;
        case 16 : ch='q';       break;
        case 19 : ch='r';       break;

        case 31 : ch='s';       break;
        case 20 : ch='t';       break;
        case 22 : ch='u';       break;
        case 47 : ch='v';       break;
        case 17 : ch='w';       break;
        case 45 : ch='x';       break;
        case 21 : ch='y';       break;
        case 44 : ch='z';       break;
        default : ch=0;       break;
      }
      if (shift) ch-=('a'-'A');
  }
  return ch;
}



int event_handler::event_waiting()
{
  int ch;
  unsigned char c;
  event *ev;
  if (ewaiting) return 1;
  
  if (key_waiting()) 
  {
    read(console_fd,&c,1);
    ch=con_translate(c);
    ewaiting=1;

    ev=new event;
    if (c&0x80)
      ev->type=EV_KEYRELEASE;
    else
      ev->type=EV_KEY;
    ev->key=ch;
    events.add_end((linked_node *)ev);
    last_key=ch;

  }
  else if (mhere)
  {
    mouse->update();
    if (mouse->lastx()!=mouse->x() || mouse->lasty()!=mouse->y()
       || mouse->last_button()!=mouse->button())
      ewaiting=1;
  }
  return ewaiting;
}


void event_handler::get_event(event &ev)
{
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

    if (mhere && (mouse->last_button()!=mouse->button()))
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
  // note : that the mouse status
  // should be know even if
  // other event has occured
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

int fd,my_console=-1,child_pid;

void die()
{
  close(console_fd);  
  kill(child_pid,SIGUSR1);
}



event_handler::~event_handler()
{ 
  die();
}

int key_fifo();

event_handler::event_handler(image *screen, palette *pal)
{
  CHECK(screen && pal);
  mouse=new JCMouse(screen,pal);
  mhere=mouse->exsist();
  last_keystat=get_key_flags();
  ewaiting=0;
  last_key=-1;
  console_fd=key_fifo();
  atexit(die);  
}


int key_fifo()  // returns the fd linked to the keydriver
{
  int fd; 
  FILE *fp;
  fp=popen("keydrv","r");
  if (!fp)
  {
    close_graphics();
    printf("Could not find keydrv, please put it in your path\n");
    exit(1);
  }
  fscanf(fp,"%d",&child_pid);   // the keydrv prints out it's fork pid
  pclose(fp);                   // read it, so we can kill it later
  do
  { usleep(10000);
  } while (access("/tmp/jckey-driver",R_OK)); 
  fd=open("/tmp/jckey-driver",O_RDONLY);
  return fd;
}












