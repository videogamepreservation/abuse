#include "video.hpp"
#include "sprite.hpp"
#include "image.hpp"
#include "filter.hpp"
#include "mdlread.hpp"
#include "monoprnt.hpp"
#include "mouse.hpp"
#include "dprint.hpp"
#include "doscall.hpp"
#include "i86.h"

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

static int Mbut,Mx,My;


class mouse_detector
{
  public :
  int detected;
  mouse_detector()
  {
    union  REGS in,out;
    memset(&in,0,sizeof(in));
    in.w.ax=0;
    int386(0x33,&in,&out);
    detected=(((unsigned short)out.w.ax)==0xffff);
  } 
} mouse_detect;

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
  sp=NULL;

  union  REGS in,out;
  memset(&in,0,sizeof(in));

  here=mouse_detect.detected;

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

    memset(&in,0,sizeof(in));
    in.w.ax=4;
    in.w.cx=40;
    in.w.dx=40;
   
    int386(0x33,&in,&out);

  } else dprintf("mouse not detected\n");

  mx=Screen->width()/2;
  my=Screen->height()/2;

}

void JCMouse::update(int newx, int newy, int new_but)
{

  int butn,xx,yy;
  if (newx<0)
  {
    if (here)
    {
      union REGS in,out;

      in.w.ax=0x0b;
      int386(0x33,&in,&out);    // get the mouse movement

      xx=(signed short)out.w.cx;
      yy=(signed short)out.w.dx;

      in.w.ax=5;                 // get the button press info
      in.w.bx=1|2|4;
      int386(0x33,&in,&out);
      butn=out.w.ax;
      

      lx=mx; ly=my; lbut=but;
      but=butn;
      mx+=xx;
      my+=yy;
      if (mx<0) mx=0;
      if (my<0) my=0;
      if (mx>=screen->width()) mx=screen->width()-1;
      if (my>=screen->height()) my=screen->height()-1;
    }
  } else
  { mx=newx; my=newy; but=new_but; }
}



JCMouse::~JCMouse()
{
  if (sp) 
  {
    delete sp->visual;
    delete sp;
  }
}



void JCMouse::set_position(int new_mx, int new_my)
{
  mx=new_mx;
  my=new_my;
}
