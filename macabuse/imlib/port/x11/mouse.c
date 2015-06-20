#include "video.hpp"
#include "sprite.hpp"
#include "image.hpp"
#include "filter.hpp"
#include "mdlread.hpp"
#include "monoprnt.hpp"
#include "mouse.hpp"
#include "xinclude.h"

extern Window root,mainwin;
extern Display *display;
extern int screen_num;
extern Colormap XCMap;
extern Screen *screen_ptr;
extern unsigned border_width,depth;
extern GC gc;
extern XFontStruct *font_info;
extern XImage *XImg;





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
      0,0,0,2,2,0,0,0 };


void JCMouse::set_shape(image *im, int centerx, int centery)
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
  here=1;
  sp=NULL;
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
  }
  mx=Screen->width()/2;
  my=Screen->height()/2;

}

void JCMouse::update(int newx, int newy, int new_but)
{
  int butn,xx,yy;
  if (newx<0)
  {
    Window w1,w2;
    int j;
    unsigned int mask;
    lx=mx; ly=my; lbut=but;
    XQueryPointer(display,mainwin,&w1,&w2,&j,&j,&mx,&my,&mask);
    but=((mask&Button1Mask)!=0)|
         ((mask&Button2Mask)!=0)<<2|
         ((mask&Button3Mask)!=0)<<1;
  } else 
  { mx=newx; my=newy; but=new_but; }
}

void JCMouse::set_position(int new_mx, int new_my)
{
  mx=new_mx;
  my=new_my;
  XWarpPointer(display,mainwin,mainwin,0,0,0,0,new_mx,new_my);
}

JCMouse::~JCMouse()
{
  if (sp) 
  {
    delete sp->visual;
    delete sp;
  }
}




