#include "video.hpp"
#include "sprite.hpp"
#include "image.hpp"
#include "filter.hpp"
#include "mdlread.hpp"
#include "monoprnt.hpp"
#include "mouse.hpp"

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



void JCmouse::set_shape(image *im, int centerx=0, int centery=0)
{
  sp->change_visual(im,1);
  cx=-centerx;
  cy=-centery;
}



JCMouse::JCMouse(image *Screen, palette *pal)
{
  image *im;
  int br,dr,h;
  filter f;
  but=0;
  cx=cy=0;
#ifdef __DOS
  asm {
    mov ax, 0             // detect the mouse
    int 0x33
    mov h, ax
  }
  here=h;
#else 
  here=1;
#endif
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
#ifdef __DOS
    asm {                // mouse the mouse pointer to 40,40
      mov ax, 4
      mov cx, 40
      mov dx, 40
      int 0x33
    }
#endif
  }
}

void JCMouse::update(int newx, int newy, int new_but)
{
  int butn,xx,yy;
  if (newx<0)
  {
#ifdef __DOS
    if (here)
    {
      asm {
        mov ax, 0x0b
        int 0x33            // get the mouse movement
        mov xx,cx
        mov yy,dx
        mov ax, 3
        int 0x33           // get the mouse button status
        mov butn,bx
      }
      lx=mx; ly=my; lbut=but;
      but=butn;
      mx+=xx;
      my+=yy;
      if (mx<0) mx=0;
      if (my<0) my=0;
      if (mx>=screen->width()) mx=screen->width()-1;
      if (my>=screen->height()) my=screen->height()-1;
    }
#else
    Window w1,w2;
    int j;
    unsigned int mask;
    lx=mx; ly=my; lbut=but;
    XQueryPointer(display,mainwin,&w1,&w2,&j,&j,&mx,&my,&mask);
    but=((mask&Button1Mask)!=0)|
         ((mask&Button2Mask)!=0)<<2|
         ((mask&Button3Mask)!=0)<<1;
#endif
  } else 
  { mx=newx; my=newy; but=new_but; }
}
