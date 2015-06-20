// Watcom graphics support


#include "globals.hpp"
#include "system.h"
#include "video.hpp"
#include "dos.h"
#include "macs.hpp"
#include "bitmap.h"
#include "image.hpp"
#include "palette.hpp"
#include "jmalloc.hpp"
#include "doscall.hpp"
#include <conio.h>

extern unsigned long xres, yres;
#define vga_getgraphmem() ((unsigned char *)VESA_video_mem)
#define vga_setpage(x) (VESA_set_bank(x))
#define WinAAttributes() vesa_mode_info[2]
#define WinBAttributes() vesa_mode_info[3]
#define WinGranularity() (*((unsigned short *)(vesa_mode_info+4)))
#define WinSize() (*((unsigned short *)(vesa_mode_info+6)))
#define WinASegment() (*((unsigned short *)(vesa_mode_info+8)))
#define WinBSegment() (*((unsigned short *)(vesa_mode_info+10)))
#define BankSize() vesa_mode_info[0x1c]
#define WINDOW_A 0
#define WINDOW_B 1
#define WINDOW_DEFINED 1
#define WINDOW_WRITABLE 4

unsigned char current_background;

extern palette *lastl;
int vmode,VESA_write_window=0;
image *screen;
unsigned char *VESA_video_mem;
char *dos_pal_memory=NULL;
unsigned char *vesa_mode_info=NULL;
int write_bank;
int gran_shift=0;

#include <i86.h>

#define MKPTR(x)  ((   (((unsigned long)x)&0xffff0000)>>12)+  (((unsigned long)x)&0xffff))

#pragma pack( 1 ) // pack to bytes boundardy


/* VESA video card capabilities block  */
struct  ModeInfoBlock {
	short  ModeAttributes;
	char WinAAttributes;
	char WinBAttributes;
	unsigned short WinGranularity;
	unsigned short  WinSize;
	unsigned short  WinASegment;
	unsigned short  WinBSegment;
	unsigned long *WinFuncPtr;
	short  BytesPerScanLine;
	short  XResolution;
	short  YResolution;
	char XCharSize;
	char YCharSize;
	char NumberOfPlanes;
	char BitsPerPixel;
	char NumberOfBanks;
	char MemoryModel;
	char BankSize;
	char NumberOfImagePages;
	char SizeOfBank;
	char RedMaskSize;
	char RedFieldPosition;
	char GreenMaskSize;
	char GreenFieldPosition;
	char BlueMaskSize;
	char BlueFieldPosition;
	char RsvdMaskSize;
	char RsvdFieldPosition;
	char DirectColorModeInfo;
	char Reserved[216];
	} vesa_info;

struct VgaInfoBlock
{
  unsigned char       VESASignature[4];
  unsigned short      ver;
  unsigned long       OEMStringPtr;
  unsigned char       Capabilities[4];
  unsigned long       VideoModePtr;
  unsigned short      TotalMemory;
  unsigned char       Reserved[236];
} ;

#pragma pack( ) // pack back to normal


void get_VESA_mode_info(ModeInfoBlock *s, unsigned short mode)
{
  int size=sizeof(ModeInfoBlock)<256 ? 256 : sizeof(ModeInfoBlock);
  ModeInfoBlock *k=(ModeInfoBlock *)alloc_low_memory(size);
  if (!k)
  {
    printf("Memory error : could not alloc low memory for VESA mode info structure\n");
    exit(0);
  }  

  rminfo rm;
  memset(&rm,0,sizeof(rm));
  rm.eax=0x4f01;
  rm.ecx=mode;
  rm.es=((long)(k))>>4;
  rm.edi=0;        
  RM_intr(0x10,&rm);

  memcpy(s,k,sizeof(ModeInfoBlock));
  
  free_low_memory(k);
}

int usable_mode(ModeInfoBlock *m)
{

  if ((m->ModeAttributes&1)==1 &&        // mode available?
      (m->ModeAttributes&8)==8 &&        // color mode?
      (m->ModeAttributes&16)==16 &&        // graphics mode?
      (m->NumberOfPlanes==1)   &&        // packed pixel form
      (m->BitsPerPixel==8)     &&        // 256 color mode
      (m->MemoryModel==4)                // VESA packed pixel
      )
    return 1;
  else return 0;
}

int get_VESA_mode(char **s)
{
  int get_xres,get_yres,show_menu=1,found=0;

  if (sscanf(s[0],"%d",&get_xres)!=1 || sscanf(s[1],"%d",&get_yres)!=1)
  { 
    show_menu=1;
    found=1;
  }

  VgaInfoBlock *b=(VgaInfoBlock *)alloc_low_memory(sizeof(VgaInfoBlock));

  if (!b)
  {
    printf("Memory error : could not alloc low memory for VESA mode info structure\n");
    exit(0);
  }  
  
  rminfo rm;
  memset(&rm,0,sizeof(rm));
  rm.eax=0x4f00;
  rm.es=((long)(b))>>4;
  rm.edi=0;        
  RM_intr(0x10,&rm);

  if (memcmp(b->VESASignature,"VESA",4))
  {
    printf("No VESA driver detected.  You need to install a VESA TSR.\n");
    free_low_memory(b);
    exit(0);
  }
  if ((b->ver>>8)==0 || (b->ver&&0xff)>2)
  {
    printf("Your VESA driver is out dated, please upgrade.\n");
    free_low_memory(b);
    exit(0);
  }

  VESA_video_mem=(unsigned char *)MKPTR(b->VideoModePtr);
  
  printf("Video card info : %s\n",(char *)MKPTR(b->OEMStringPtr));
  short *modes=(short *)MKPTR(b->VideoModePtr);   // type cast low mem pointer

  while (*modes!=-1 && !found)
  {
    struct ModeInfoBlock m;
    get_VESA_mode_info(&m,*modes);
    if (usable_mode(&m))
    {                     // this is a usable mode
      if (m.XResolution==get_xres && m.YResolution==get_yres)
      {
	free_low_memory(b);
	return *modes;
      }
    }	
    modes++;
  }

  if (show_menu)
  {
    int i=1;
    printf("Bad resoultion size or size not supported by VESA driver\n"
	   "Please choice one of the below :\n");
    modes=(short *)MKPTR(b->VideoModePtr);
    while (*modes!=-1)            // list all chosable modes
    {
      struct ModeInfoBlock m;
      get_VESA_mode_info(&m,*modes);

      if (usable_mode(&m))
      {
	printf("%d) %d X %d\n",i,m.XResolution,m.YResolution);
	i++;
      }
      modes++;
    }

    int d=0;
    do
    {
      fprintf(stderr,"Enter mode # (1-%d)>",i-1);
      char ln[100];
      fgets(ln,100,stdin);
      if (!sscanf(ln,"%d",&d))
        d=0;
    } while (!d);
    d--;

    modes=(short *)MKPTR(b->VideoModePtr);
    while (*modes!=-1)
    {
      struct ModeInfoBlock m;
      get_VESA_mode_info(&m,*modes);
      if (usable_mode(&m))
      {
	if (d) d--;
	else return *modes;
      }
      modes++;
    }

    printf("Your VESA driver is reporting incorrect information, try getting\n"
	   "univbe or univesa from any simtel mirror ftp site such as\n"
	   "wuarchive.wustl.edu.\n");
    free_low_memory(b);
    exit(0);
  }

  free_low_memory(b);
  return found;
}

void VESA_set_bank(int x)
{
  if (vmode!=0x13)
  {
    rminfo rm;
    memset(&rm,0,sizeof(rm));
    rm.eax=0x4f05;
    rm.ebx=VESA_write_window;
    rm.edx=x<<gran_shift;
    RM_intr(0x10,&rm);
  }
}

int VESA_get_mode()
{
  rminfo rm;
  memset(&rm,0,sizeof(rm));
  rm.eax=0x4f03;
  RM_intr(0x10,&rm);
  return rm.ebx&0xffff;
}

int VESA_set_mode(int mode)
{
  rminfo rm;
  memset(&rm,0,sizeof(rm));
  if (mode!=19 && mode!=3)
  {
    rm.eax=0x4f02;
    rm.ebx=mode;
    RM_intr(0x10,&rm);
    vmode=mode;

    if (VESA_get_mode()==mode)
    {
      get_VESA_mode_info(&vesa_info,mode);

      if ((vesa_info.WinAAttributes & WINDOW_DEFINED) &&
	  (vesa_info.WinAAttributes & WINDOW_WRITABLE))
      {
        VESA_write_window=WINDOW_A;
	VESA_video_mem=(unsigned char *)((long)vesa_info.WinASegment<<4);
      }
      else if ((vesa_info.WinBAttributes & WINDOW_DEFINED) &&	       
	       (vesa_info.WinBAttributes & WINDOW_WRITABLE))
      {
        VESA_write_window=WINDOW_B;
	VESA_video_mem=(unsigned char *)((long)vesa_info.WinBSegment<<4);
      }
      else VESA_video_mem=(unsigned char *)0xa0000;

      xres=vesa_info.XResolution-1;
      yres=vesa_info.YResolution-1;

      if (vesa_info.WinGranularity==1)  // 1K pages
        gran_shift=6;
      else if (vesa_info.WinGranularity==2)  // 2K pages
        gran_shift=5;
      else if (vesa_info.WinGranularity==4)  // 4K pages
        gran_shift=4;
      else if (vesa_info.WinGranularity==8)  // 8K pages
        gran_shift=3;
      else if (vesa_info.WinGranularity==16)  // 16K pages
        gran_shift=2;
      else if (vesa_info.WinGranularity==32)  // 16K pages
        gran_shift=1;
      else if (vesa_info.WinGranularity==64)  // 16K pages
        gran_shift=0;
      else
      {
	printf("VESA : window granularity not supported (%d)\n",vesa_info.WinGranularity);
	exit(0);
      }

      

//      exit(0);
      return 1;
    } else return 0;
  }
  else
  {
    vmode=mode;
    rm.eax=mode;
    RM_intr(0x10,&rm);
    VESA_video_mem=(unsigned char *)0xa0000;
    VESA_write_window=WINDOW_A;
    xres=320-1;
    yres=200-1;
    return 1;
  }
}

void set_mode(video_mode lmode, int argc, char **argv)
{
  int i,j;
  int mode;

  if (lmode==VMODE_320x200)
  {
    mode=19;
    xres=320; yres=200;
  }
  else 
  {
    char *prm []={"640","480"};
    mode=get_VESA_mode(prm);
  }


  if (!VESA_set_mode(mode))
  {
    printf("Unable to set video mode.  The correct VESA driver is not loaded\n"
           "or it does not support this video mode.\n");
    exit(1);
  }

  screen=new image(xres+1,yres+1,NULL,2);
  screen->clear();
  update_dirty(screen);

}

int get_vmode()
{ return vmode; }

void close_graphics()
{
  if (lastl)
    delete lastl;
  lastl=NULL;
  VESA_set_mode(3);  // switch to text mode
  delete screen;
}

void put_part(image *im, int x, int y, int x1, int y1, int x2, int y2)
{
  unsigned short screen_off;
  int i,ys,ye,xs,xe,page,last_page=-1,yy;
  long breaker;
  unsigned char *screen_addr,*line_addr;

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

  int virtual_screen_width=im->width();
  line_addr=im->scan_line(ys)+xs;

  for (yy=ys;yy<=ye;yy++)
  {
    page=(long)y*(long)(xres+1)>>16;
    if (page!=last_page)
    { last_page=page;
      vga_setpage(page);
    }

    // find the memory offset for the scan line of interest
    screen_off=((long)y*(long)(xres+1))&0xffff;

    // breaker is the number of bytes beofer the page split
    breaker=(long)0xffff-(long)screen_off+1;

    if (breaker>x+xe-xs)
      memcpy(vga_getgraphmem()+screen_off+x,line_addr,xe-xs+1);
    else if (breaker<=x)
    { last_page++;
      vga_setpage(last_page);
      memcpy(vga_getgraphmem()+x-breaker,line_addr,xe-xs+1);
    }
    else
    {
      memcpy(vga_getgraphmem()+screen_off+x,line_addr,breaker-x);
      last_page++;
      vga_setpage(last_page);
      memcpy(vga_getgraphmem(),line_addr+breaker-x,xe-xs-(breaker-x)+1);
    }
    y++;
    line_addr+=virtual_screen_width;
  }
}

void put_image(image *im, int x, int y)
{ put_part(im,x,y,0,0,im->width()-1,im->height()-1); }


void update_dirty(image *im, int xoff, int yoff)
{

  int count,x1,y1,x2,y2;
  dirty_rect *dr,*q;
  image *Xim;
  CHECK(im->special);  // make sure the image has the ablity to contain dirty areas
  if (im->special->keep_dirt==0)
    put_image(im,0,0);
  else
  {
    count=im->special->dirties.number_nodes();
    if (!count) return;  // if nothing to update, return
    dr=(dirty_rect *)im->special->dirties.first();
    while (count>0)
    {
/*      if (dr->dx1+xoff<0) dr->dx1=0-xoff;
      if (dr->dy1+yoff<0) dr->dy1=0-yoff;
      if (dr->dx2+xoff>xres) dr->dx2=xres-xoff;
      if (dr->dy2+yres>yres) dr->dy2=yres-yoff;
      if (dr->dx1<=dr->dx2 && dr->dy1<=dr->dy2) */
      put_part(im,dr->dx1+xoff,dr->dy1+yoff,dr->dx1,dr->dy1,dr->dx2,dr->dy2);
      q=dr;
      dr=(dirty_rect *)dr->next();
      im->special->dirties.unlink((linked_node *)q);
      delete q;
      count--;
    }
  }
}


void palette::load()
{
  int i;
  unsigned char *a=(unsigned char *)addr();
  if (lastl)
    delete lastl;
  lastl=copy();

  outp(0x3c8,0);
  for (i=0;i<768;i++,a++)
    outp(0x3c9,(*a)>>2);

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


void switch_mode(video_mode new_mode)
{
  close_graphics();
  char *empty[]= {"game"};
  set_mode(new_mode,1,empty);
}
