#include "globals.hpp"
#include "system.h"
#include "video.hpp"
#include "dos.h"
#include "xinclude.h"
#include "macs.hpp"
#include "bitmap.h"
#include "image.hpp"

//#define DIRECT_SCREEN


#define CGAY(y) ((y%2)?(y/2)*80:0x2000+((y-1)/2)*80)

unsigned char current_background;
extern unsigned int xres,yres;
int xoff,yoff,vmode;
image *screen;

int get_vmode()
{ return vmode; }


void set_mode(int mode, int argc, char **argv)
{
  unsigned char *page;

  vmode=mode;
  int i,fail;
  for (i=1,fail=0;i<argc && !fail;i++)
    if (!strcmp(argv[i],"-vmode"))
    {
      if (i==argc-1) fail=1;
      else
      { if (!strcmp(argv[i+1],"VGA_320x200x256"))
	  mode=19;
	else if (!strcmp(argv[i+1],"TRI_640x480x256"))
	  mode=TRI_640x480x256;
	else if (!strcmp(argv[i+1],"TRI_800x600x256"))
	  mode=TRI_800x600x256;
	else if (!strcmp(argv[i+1],"TRI_1024x768x256"))
	  mode=TRI_1024x768x256;
	else if (!strcmp(argv[i+1],"CGA_640x200x2"))
	  mode=CGA_640x200x2;
	else fail=1;
      }
    }
  if (fail)
  {
    printf("Expected one of the folloing video modes after -vmode switch\n"
	   "  CGA_640x200x2      CGA 2 color graphics (dithered imaging)\n"
	   "  VGA_320x200x256    Standard vga 256 color mode\n"
	   "  TRI_640x480x256    Trident super vga\n"
	   "  TRI_800x640x256    Trident super vga\n"
	   "  TRI_1024x768x256   Trident super vga (1 meg video memory needed)\n"
	   "example : %s -vmode VGA_320x200x256\n",argv[0]);
    exit(0);
  }
  vmode=mode;
  switch (mode)
  {
    case TRI_1024x768x256 : xres=1023; yres=767; break;
    case TRI_800x600x256  : xres=799;  yres=599; break;
    case TRI_640x480x256  : xres=639;  yres=479; break;
    case VGA_320x200x256  : xres=319;  yres=199; break;
    case CGA_640x200x2    : xres=639;  yres=199; break;
  }
  asm {
    mov ax, mode
    int 0x10
  }
  page=NULL;
#ifdef DIRECT_SCREEN
  if (mode==19)
    page=(unsigned char *)MK_FP(0xa000,0);
#endif
  screen=new image(xres+1,yres+1,page,2);
  screen->clear();
  update_dirty(screen);
}

void close_graphics()
{
  asm {
    mov ax, 3
    int 0x10
  }
}


void put_image(image *im, int x, int y)
{
  int ls,iy,i,w;
  unsigned int sp;
  long off;
  char *sl;
  ls=-1;
  iy=0;
  w=im->width();
  while (iy<im->height())
  {
    sl=im->scan_line(iy++);
    if (vmode==CGA_640x200x2)
    {
      int byte_off,bit_off;
      char compsl[80];
      byte_off=0;

      compsl[0]=peekb(0xb800,CGAY(y)+x/8);
      compsl[(w-2)/8]=peekb(0xb800,CGAY(y)+(x+w-2)/8);
      bit_off=128>>(x%8);
      for (i=0;i<w;i++)
      {
	if (!sl[i])
	  compsl[byte_off]&=(0xff^bit_off);
	else compsl[byte_off]|=bit_off;
	bit_off>>=1;
	if (!bit_off)
	{ byte_off++; bit_off=128; }

      }
      memcpy(MK_FP(0xb800,CGAY(y)+x/8),compsl,(w+7)/8);
    }
    else
    {
      sp=(long)(((long) y*(long)(xres+1)+(long)x)>>16);
      if (sp!=ls)
      {
	outportb(0x3c4,14);
	outportb(0x3c5,(sp)^2);
	ls=sp;
      }
      off=(((long)y*(long)(xres+1)+(long)x)&(long)0xffff);



      for (i=0;i<w;i++)
      {
	pokeb(0xa000,off&0xffff,sl[i]);
	if (off>=0xffff)
	{ off=0;
	  sp++;
	  outportb(0x3c4,14);
	  outportb(0x3c5,(sp)^2);
	}
	else
	  off++;
      }
    }
    y++;
  }
}

void update_dirty(image *im, int xoff, int yoff)
{

  int count,x1,y1,x2,y2;
  dirty_rect *dr,*q;
  image *Xim;
#ifdef DIRECT_SCREEN
  if (im!=screen || vmode!=19)
  {
#endif
  CHECK(im->special);  // make sure the image has the ablity to contain dirty areas
  if (im->special->keep_dirt==0)
    put_image(im,xoff,yoff);
  else
  {
    count=im->special->dirties.number_nodes();
    if (!count) return;  // if nothing to update, return
    (linked_node *) dr=im->special->dirties.first();
    while (count>0)
    {
      x1=dr->dx1;               // update this area for this dirty rectangle
      y1=dr->dy1;
      x2=dr->dx2;
      y2=dr->dy2;

  int dt_matrix[]={0,  136,24, 170,
		   68, 204,102,238,
		   51, 187, 17,153,
		   119,255, 85,221};


      int ls,iy,i;
      unsigned int sp;
      long off;
      char *sl;
      ls=-1;
      iy=y1;
      while (iy<=y2)
      {
	sl=im->scan_line(iy);
	if (vmode!=19 && vmode!=CGA_640x200x2)
	{
	  sp=(long)(((long) (iy+yoff)*(long)(xres+1)+(long)x1+xoff)>>16);
	  if (sp!=ls)
	  {
	    outportb(0x3c4,14);
	    outportb(0x3c5,(sp)^2);
	    ls=sp;
	  }
	  off=(((long)(iy+yoff)*(long)(xres+1)+(long)x1+xoff)&(long)0xffff);
	  for (i=x1;i<=x2;i++)
	  {
	    pokeb(0xa000,off&0xffff,sl[i]);
	    if (off>=0xffff)
	    { off=0;
	      sp++;
	      outportb(0x3c4,14);
	      outportb(0x3c5,(sp)^2);
	    }
	    else
	      off++;
	  }
	}
	else if (vmode!=CGA_640x200x2)
	  memcpy(MK_FP(0xa000,(iy+yoff)*320+x1),&sl[x1+xoff],(x2-x1+1));
	else
	{
	  int byte_off,bit_off,w;
	  char compsl[80];
	  byte_off=0;
	  memcpy(compsl,MK_FP(0xb800,CGAY((iy+yoff))+(x1+xoff)/8),(x2+xoff)/8-(x1+xoff)/8+1);
	  bit_off=128>>((x1+xoff)%8);
	  for (i=(x1+xoff);i<=(x2+xoff);i++)
	  {
	    if (last_loaded()->red(sl[i])>dt_matrix[i%4+((iy+yoff)%4)*4])
	      compsl[byte_off]&=(0xff^bit_off);
	    else
	      compsl[byte_off]|=bit_off;
	    bit_off>>=1;
	    if (!bit_off)
	    { byte_off++;
	      bit_off=128;
	    }

	  }
	  memcpy(MK_FP(0xb800,CGAY((iy+yoff))+(x1+xoff)/8),compsl,(xoff+x2)/8-(x1+xoff)/8+1);
	}
	iy++;

      }
      q=dr;
      (linked_node *)dr=dr->next();
      im->special->dirties.unlink((linked_node *)q);
      delete q;
      count--;
    }
  }
#ifdef DIRECT_SCREEN
  }
#endif
}

extern palette *lastl;

void palette::load()
{
  if (lastl)
    delete lastl;
  lastl=copy();
  unsigned s,o;
  if (get_vmode()!=CGA_640x200x2)
  {
    s=FP_SEG(pal); o=FP_OFF(pal);
    asm {
    mov ax, o
    mov si, ax
    push ds
    mov ax, s
    mov ds, ax
    mov cx, 128
    mov dx, 986
    }
retrace_start :
    asm {
    in al,dx
    test al, 8
    jnz retrace_start
    }
vert_retrace :
    asm {
    in al, dx
    test al,8
    jz vert_retrace
    xor al,al
    mov dx, 968
    out dx, al
    mov dx, 969
    cld
    }
load_again :
    asm {
    lodsb
    shr al,1
    shr al,1
    out dx,al             //     ; {three bytes per color}
    jmp jadr1
    }
jadr1 :
    asm {
    lodsb
    shr al,1
    shr al,1
    out dx,al
    jmp jadr2 :
    }
jadr2 :
    asm {
    lodsb
    shr al,1
    shr al,1
    out dx,al
    jmp jadr3
    }
jadr3 :
    asm {
    loop load_again
    mov dx, 986
    }
retrace_start2 :
    asm {
    in al,dx
    test al, 8
    jnz retrace_start2
    }
vert_retrace2 :
    asm {
    in al, dx
    test al,8
    jz vert_retrace2
    xor al,al
    mov dx, 968
    out dx, al
    mov dx, 969
    mov cx,128
    mov al, 128
    mov dx, 968
    out dx, al
    mov dx, 969
  }
load_again2 :
  asm {
    lodsb
    shr al,1
    shr al,1
     out dx,al
    lodsb
    shr al,1
    shr al,1
    out dx,al
    lodsb
    shr al,1
    shr al,1
    out dx,al
    loop load_again2
    pop ds
    }
  }
  else lastl->black_white();
  current_background=bg;
}

void palette::load_nice()
{ load(); }

