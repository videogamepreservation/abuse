#include "video.hpp"
#include "sprite.hpp"
#include "image.hpp"
#include "filter.hpp"
#include "mdlread.hpp"
#include "monoprnt.hpp"
#include "mouse.hpp"
#include "dprint.hpp"
#include <vgamouse.h>
#include <vga.h>

unsigned char def_mouse[]=
    { 0,2,0,0,0,0,0,0,
      2,1,2,0,0,0,0,0,
      2,1,1,2,0,0,0,0,
      2,1,1,1,2,0,0,0,
      2,1,1,1,1,2,0,0,
      2,1,1,1,1,1,2,0,
      0,2,1,1,2,2,0,0,
      0,0,2,1,1,2,0,0,
      0,0,2,1,1,2,0,0,
      0,0,0,2,2,0,0,0 };    // 8x10

int Mbut,Mx,My;

void JCMouse::set_shape(image *im, int centerx, int centery)
{
  sp->change_visual(im,1);
  cx=-centerx;
  cy=-centery;
}


JCMouse::JCMouse(image *Screen, palette *pal)
{
  image *im;
  int br,dr;
  filter f;
  but=0;
  cx=cy=0;
  vga_setmousesupport(1);
  long mtype=MOUSE_MICROSOFT;
  char *mtype_str=getenv("MOUSE_TYPE");
  if (mtype_str)
  {
    if (!strcmp(mtype_str,"MOUSESYSTEMS"))
      mtype=MOUSE_MOUSESYSTEMS;
    else if (!strcmp(mtype_str,"MMSERIES"))
      mtype=MOUSE_MMSERIES;
    else if (!strcmp(mtype_str,"LOGITECH"))
      mtype=MOUSE_LOGITECH;
    else if (!strcmp(mtype_str,"BUSMOUSE"))
      mtype=MOUSE_BUSMOUSE;
    else if (!strcmp(mtype_str,"PS2"))
      mtype=MOUSE_PS2;
    else dprintf("Unknown mouse type %s\n",mtype_str);
  } else
  {
    dprintf("Warning : env variable MOUSE_TYPE not defined, set to :\n"
	    "  MMSERIES, LOGITECH, BUSMOUSE, or PS2\n"
	    "  *if* you do not have a MS compatiable mouse\n");
    
  }


  here=!(mouse_init("/dev/mouse",mtype,MOUSE_DEFAULTSAMPLERATE));
  here=1;

  if (here)                     // is it here?
  {
    screen=Screen;
    br=pal->brightest(1);
    dr=pal->darkest(1);
    f.set(1,br);
    f.set(2,dr);
    im=new image(8,10,def_mouse);
    f.apply(im);
    sp=new sprite(Screen,im,100,100);
    mouse_setxrange(0,Screen->width()-1);
    mouse_setyrange(0,Screen->height()-1);
    mouse_setwrap(MOUSE_NOWRAP);
    mouse_update();
    update();
    update();
  }
  mx=Screen->width()/2;
  my=Screen->height()/2;

}

void JCMouse::update(int newx, int newy, int new_but)
{
  int l,r,m;
  if (newx<0)
  {
    lbut=but; lx=mx; ly=my;
    mouse_update();
    mx=mouse_getx();
    my=mouse_gety();
    but=mouse_getbutton();
    l=but&4; r=but&1; m=but&2;
    but=but&(0xff-7);
    if (l) but+=1;
    if (r) but+=2;
    if (m) but+=4;
  } else 
  { mx=newx; my=newy; but=new_but; }
}

void JCMouse::set_position(int new_mx, int new_my)
{
  mx=new_mx;
  my=new_my;
  mouse_setposition(new_mx,new_my);
}


JCMouse::~JCMouse()
{
  mouse_close();
}

