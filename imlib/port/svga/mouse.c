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

void
JCMouse_init ()
{
  if (mouse_init ("/dev/mouse", vga_getmousetype (), MOUSE_DEFAULTSAMPLERATE))
    {
      printf (0, "failed to initialize mouse; did you set the right type in libvga.config?\n");
      exit (1);
    }
}  

JCMouse::JCMouse(image *Screen, palette *pal)
{
  image *im;
  int br,dr;
  filter f;
  but=0;
  cx=cy=0;
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

