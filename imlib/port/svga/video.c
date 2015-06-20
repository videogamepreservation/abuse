#include "globals.hpp"
#include "system.h"
#include "video.hpp"
#include "dos.h"
#include "macs.hpp"
#include "image.hpp"
#include "vga.h"
#include "palette.hpp"
#include "jmalloc.hpp"


unsigned char current_background;
extern unsigned int xres,yres;
extern palette *lastl;
int vmode;
image *screen;
uchar *v_addr;

int get_vmode()
{ return vmode; }

extern void JCMouse_init ();

void set_mode(int mode, int argc, char **argv)
{
  int fail=0,i;
  if (mode==19) 
    mode=G320x200x256;
  for (i=1;i<argc;i++)
  {
    if (!strcmp(argv[i],"-vmode"))
    { i++;
      if (!strcmp(argv[i],"G320x200x256")) mode=G320x200x256;
      else if (!strcmp(argv[i],"G640x480x256")) mode=G640x480x256;
      else if (!strcmp(argv[i],"G800x600x256")) mode=G800x600x256;
      else if (!strcmp(argv[i],"G1024x768x256")) mode=G1024x768x256;
      else fail=1;
    }
  }  
  if (fail)
  { printf("Graphics modes supported :\n"
            "  G320x200x256, G640x480x256, G800x600x256, G1024x768x256\n"
            "  usage : %s [program options] [-vmode G....]\n",argv[0]);
    exit(1);
  }
  JCMouse_init();
  vga_init();
  vga_setmode(mode);
  v_addr=vga_getgraphmem();
  xres=vga_getxdim()-1;
  yres=vga_getydim()-1;

  vmode=mode;
  screen=new image(xres+1,yres+1,NULL,2);
  screen->clear();
  update_dirty(screen);
}

void close_graphics()
{
  if (lastl)
    delete lastl;
  lastl=NULL;
  vga_setmode(0);
}

inline void jmemcpy(long *dest, long *src, long size)
{
  register long *s=src;
  register long *d=dest;

  size>>=2;  // doing word at a time
  while (size--)
    *(d++)=*(s++);  
}

void put_part(image *im, int x, int y, int x1, int y1, int x2, int y2)
{
  unsigned long screen_off;
  int ys,ye,         // ystart, yend
        xs,xe,      
        page,last_page=-1,yy;
  long breaker;
  unsigned char *line_addr;

  if (y>(int)yres || x>(int)xres) return ;
  CHECK(y1>=0 && y2>=y1 && x1>=0 && x2>=x1);


  if (y<0)
  { y1+=-y; y=0; }
  ys=y1;
  if (y+(y2-y1)>=(int)yres)
    ye=(int)yres-y+y1-1;
  else ye=y2;

  if (x<0)
  { x1+=-x; x=0; }
  xs=x1;
  if (x+(x2-x1)>=(int)xres)
    xe=(int)xres-x+x1-1;
  else xe=x2;
  if (xs>xe || ys>ye) return ;

  // find the memory offset for the scan line of interest
  screen_off=((long)y*(long)(xres+1));


  for (yy=ys;yy<=ye;yy++,screen_off+=(xres+1))
  {
    page=screen_off>>16;     // which page of 64k are we on?
    if (page!=last_page)     
    { last_page=page;
      vga_setpage(page);     // switch to new bank
    }

    line_addr=im->scan_line(yy)+xs;  // get address of image scan line

    // breaker is the number of bytes before the page split
    breaker=(long)0xffff-(long)(screen_off&0xffff)+1;


    // see if the slam gets split by the page break
    if (breaker>x+xe-xs)
    {
      void *dest=v_addr+(screen_off&0xffff)+x;
      int size=xe-xs+1;
      memcpy(dest,line_addr,size);
    }
    else if (breaker<=x)
    { last_page++;
      vga_setpage(last_page);
      memcpy(v_addr+x-breaker,line_addr,xe-xs+1);
    }
    else
    {
      memcpy(v_addr+(screen_off&0xffff)+x,line_addr,breaker-x);
      last_page++;
      vga_setpage(last_page);
      memcpy(v_addr,line_addr+breaker-x,xe-xs-(breaker-x)+1);
    } 
    y++;   
  }
}

void put_image(image *im, int x, int y)
{ put_part(im,x,y,0,0,im->width()-1,im->height()-1); }


void update_dirty(image *im, int xoff, int yoff)
{

  int count;
  dirty_rect *dr,*q;
  CHECK(im->special);  // make sure the image has the ablity to contain dirty areas
  if (im->special->keep_dirt==0)
    put_image(im,0,0);
  else
  {
    count=im->special->dirties.number_nodes();
    if (!count) return;  // if nothing to update, return
    (linked_node *) dr=im->special->dirties.first();
    while (count>0)
    {
      put_part(im,dr->dx1+xoff,dr->dy1+yoff,dr->dx1,dr->dy1,dr->dx2,dr->dy2);
      q=dr;
      (linked_node *)dr=dr->next();
      im->special->dirties.unlink((linked_node *)q);
      delete q;
      count--;
    }
  }
}


void palette::load()
{
  if (lastl)
    delete lastl;
  lastl=copy();
  for (int i=0;i<ncolors;i++)
    vga_setpalette(i,red(i)>>2,green(i)>>2,blue(i)>>2);
}

void palette::load_nice()
{ load(); }


void image::make_page(short width, short height, unsigned char *page_buffer)
{
  if (page_buffer)
    data=page_buffer;
  else data=(unsigned char *)jmalloc(width*height,"image::data");
}
void image::delete_page()
{
  if (!special || !special->static_mem)
    jfree(data);      
}
