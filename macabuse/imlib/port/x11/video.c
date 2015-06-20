#include <X11/Xlib.h> 
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifndef NO_XSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <signal.h>
#endif

#include <string.h>
#ifdef _AIX
#include <strings.h>
#endif

#include "filter.hpp"
#include "globals.hpp"
#include "system.h"
#include "video.hpp"
#include "dos.h"
#include "xinclude.h"
#include "macs.hpp"
#include "bitmap.h"
#include "image.hpp"
#include "jmalloc.hpp"

unsigned char current_background;

extern unsigned long xres, yres;
extern palette *lastl;
int X_xoff,Y_yoff,vmode;
image *screen;
char display_name[120];

Window root;
Display *display;
int screen_num;
Screen *screen_ptr;
unsigned border_width,depth;
Colormap XCMap;
Window mainwin;
GC gc;
XFontStruct *font_info;
XVisualInfo *my_visual;
Visual *X_visual;

uchar last_load_palette[256*4];  // store word alligned for faster access




// ========================================================================
// makes a null cursor
// ========================================================================

static Cursor CreateNullCursor(Display *display, Window root)
{
    Pixmap cursormask; 
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
    xgc.function = GXclear;
    gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
    dummycolour.pixel = 0;
    dummycolour.red = 0;
    dummycolour.flags = 04;
    cursor = XCreatePixmapCursor(display, cursormask, cursormask,
          &dummycolour,&dummycolour, 0,0);
    XFreePixmap(display,cursormask);
    XFreeGC(display,gc);
    return cursor;
}



#ifndef NO_SHM
// ************** SHM Vars *******************
int shm_base,shm_error_base,shm_finish_event,doShm=0,do24=0;


struct event_node
{
  XEvent report;  
  event_node *next;  
} ;


void wait_shm_finish()
{
  event_node *en,*first=NULL;
  while (1)
  { 
    en=(event_node *)jmalloc(sizeof(event_node),"shm_wait : event");
    XNextEvent(display, &en->report);

    // if this is the finishing event, put all the stored events back
    // on the que
    if (en->report.type==shm_base+ShmCompletion)
    {
      jfree(en);
      
      while (first)
      {
	XPutBackEvent(display,&first->report);
	en=first;
	first=first->next;
	jfree(en);	
      }
      return ;      
    } else // put the event on the que to be puback
    {
      en->next=first;     // put the back in reverse order
      first=en;
    }    
  }
}

    
#endif


int get_vmode()
{ return vmode; }

void getGC(Window win, GC *gc, XFontStruct *font_info)
{
  XGCValues values;
  *gc=XCreateGC(display,win,0,&values);
  XSetFont(display, *gc, font_info->fid);
  XSetForeground(display, *gc, BlackPixel(display,screen_num));
  XSetLineAttributes(display, *gc, 1, LineSolid, CapRound, JoinRound);
}


class XImage_Info             // stored in the extended desciptor
{
public :
  
#ifndef NO_SHM
  XShmSegmentInfo X_shminfo;    
#endif
  XImage *XImg;
} ;


void image::make_page(short width, short height, unsigned char *page_buffer)
{
  XImage_Info *xi;  
  if (special && !special->static_mem)
  {
    xi=new XImage_Info;
    special->extended_descriptor=(void *)xi;    
#ifndef NO_SHM
    if (doShm)
    {
      width=(width+3)&(0xffffffff-3);
      // create the image
      xi->XImg = XShmCreateImage(display,
				 X_visual,
				 my_visual->depth,
				 ZPixmap,
				 0,
				 &xi->X_shminfo,
				 do24 ? width*2 : width,
				 do24 ? height*2 : height );

      w=width=xi->XImg->bytes_per_line/(do24 ? 2 : 1);  // adjust image size to X requirments
      
      
      // create the shared memory segment
      xi->X_shminfo.shmid = shmget (IPC_PRIVATE, do24 ? width*height*4 : width*height, IPC_CREAT | 0777);
      ERROR(xi->X_shminfo.shmid>=0,"shmget() failed, go figure");

      xi->X_shminfo.readOnly=False;
      

      // attach to the shared memory segment to us
      xi->XImg->data = xi->X_shminfo.shmaddr =
		(char *) shmat(xi->X_shminfo.shmid, 0, 0);
      ERROR(xi->XImg->data,"shmat() failed, go figure");


      // get the X server to attach to it to the X server
      ERROR(XShmAttach(display, &xi->X_shminfo),"XShmAttach() failed, go figure");
      XSync(display,False); // make sure segment gets attached
      ERROR(shmctl(xi->X_shminfo.shmid,IPC_RMID,NULL)==0,"shmctl failed, why?"); 

      if (do24)
        data=(uchar *)jmalloc(width*height,"24 bit image data");
      else data=(unsigned char *) (xi->XImg->data);

    } else 
#endif
    {
      width=(width+3)&(0xffffffff-3);        
      if (!page_buffer)
        page_buffer=(unsigned char *)jmalloc(do24 ? width*height*4 : width*height,"image::data");
      
      xi->XImg = XCreateImage(	display,
    				X_visual,
    				my_visual->depth,
    				ZPixmap,
    				0,
    				(char *)page_buffer,
    				do24 ? width*2  : width, 
			        do24 ? height*2 : height,
    				32,
    				0 );
      ERROR(xi->XImg,"XCreateImage failed");

      w=width=(xi->XImg->bytes_per_line/(do24 ? 2 : 1));  // adjust image size to X requirments

      if (do24)
        data=(uchar *)jmalloc(width*height,"24 bit image data");
      else data=(unsigned char *) (xi->XImg->data);
    }    
  }
  else 
  {
    if (!page_buffer)
      data=(unsigned char *)jmalloc(width*height,"image::data");
    else data=page_buffer;
  }

  if (special)
    special->resize(width,height);
}




void image::delete_page()
{ 
  XImage_Info *xi;  
  if (special && !special->static_mem && special->extended_descriptor)
  {    
    xi=(XImage_Info *)special->extended_descriptor;    
#ifndef NO_SHM
    if (doShm)
    {    
      XFlush(display);
      XSync(display,False);  // maker sure anything with this image is drawn!

      // Dettach the memory from the server
      ERROR(XShmDetach(display, &xi->X_shminfo),"XShmDetach() failed, go figure");

      XSync(display,False);  // maker sure server detached

      // detach the memory from us, it will be deleted!
      ERROR(shmdt(xi->X_shminfo.shmaddr)>=0,"shmdt failed, oops");

      xi->XImg->data=NULL;  // tell X not to try to free the memory, cause we already did.

      XDestroyImage(xi->XImg);    

      if (do24)         // if 24 bit, we need to free addition "real" page 
        jfree(data);

    } else
#endif
    {
      if (!special->static_mem)
        jfree(xi->XImg->data);
      xi->XImg->data=NULL;                  // make sure X doesn't try to free static memory
      XDestroyImage(xi->XImg);    

      if (do24)         // if 24 bit, we need to free addition "real" page 
        jfree(data);
    }
    delete xi;
  }
  else if (!special)
    jfree(data);      
}


#ifndef NO_SHM
#ifdef __sgi
void clean_shm(...)
#else
void clean_shm(int why)      // on exit, delete all images, so shm get de-allocated.
#endif
{
  while (image_list.first())
  {
    image *i=(image *)image_list.first();
    delete i;
  }
}
#endif

void set_mode(video_mode mode, int argc, char **argv)
{
  unsigned char *page;
  vmode=mode;
  XVisualInfo vis_info;
  int items,i;
  XEvent report;
  XWMHints *wm_hints;
  XClassHint *class_hints;
  XTextProperty winName,iconName;
  XSizeHints *size_hints;
  char *win_name=argv[0],        // name the window and the icon the executable name
       *icon_name=argv[0];
  Pixmap icon_pixmap;
  
  // get the default display name from the enviroment variable DISPLAY
  char *ds_name=getenv("DISPLAY");
  if (!ds_name) ds_name="unix:0.0";
  strcpy(display_name,ds_name); 
 
  int suppress_shm=0;

  // check for command line display name
  for (i=1;i<argc;i++)
  {
    if (!strcasecmp(argv[i],"-display") || !strcasecmp(argv[i],"-disp"))
    {
      i++;      
      strcpy(display_name,argv[i]);  
    }    
    else if (!strcasecmp(argv[i],"-noshm"))
      suppress_shm=1;
    else if (!strcasecmp(argv[i],"-24bit"))
      do24=1;
  }
    
  display=XOpenDisplay(display_name);  
  if (!display)
  {
    printf("Cound not connect to X server named '%s'\n",display_name);
    exit(1);
  }

#ifndef NO_SHM
  // check for the MITSHM extension
  int major_op;
  
  if (suppress_shm)
    doShm=0;
  else
    doShm = XQueryExtension(display,"MIT-SHM",&major_op,&shm_base,&shm_error_base);

  // make sure it's a local connection
  if (doShm)
  {    
    char *d = display_name;
    while (*d && (*d != ':')) d++;
    if (*d) *d = 0;
    if (strcasecmp(display_name, "unix") && display_name[0]!=0) 
      doShm = 0;   
  }

  if (doShm)
  {
    printf("Using MITSHM extension!\n");
    int sigs[29]={SIGHUP,SIGINT,SIGQUIT,SIGILL,SIGTRAP,
		SIGABRT,SIGIOT,SIGBUS,SIGFPE,SIGKILL,
		SIGUSR1,SIGSEGV,SIGUSR2,SIGPIPE,SIGALRM,
                SIGTERM,SIGCHLD,SIGCONT,SIGSTOP,
		SIGTSTP,SIGTTIN,SIGTTOU,SIGIO,
		SIGURG,SIGXCPU,SIGXFSZ,SIGVTALRM,SIGPROF,
		SIGWINCH};

    for (int i=0;i<29;i++)
      signal(sigs[i],clean_shm);
  }
#endif
  depth=do24 ? 24 : 8;
  screen_num = DefaultScreen(display);
  screen_ptr = DefaultScreenOfDisplay(display);
  unsigned int lxres=xres,lyres=yres;
  ERROR(XGetGeometry(display,RootWindow(display,screen_num), &root,
	&X_xoff,&Y_yoff,&lxres,&lyres,&border_width,&depth),
	"can't get root window attributes");
  xres=lxres;
  yres=lyres;

  {
    vis_info.c_class=PseudoColor;       
    vis_info.depth=8;
    my_visual=XGetVisualInfo(display,VisualClassMask | VisualDepthMask, &vis_info,&items);
    X_visual=my_visual->visual;

    if (items>0)
    {
      int rc;


      rc=XMatchVisualInfo(display, screen_num, 8, PseudoColor, &vis_info);
      ERROR(rc, "What the hell? Non-8 bit Psuedo color..\n");
      X_visual = my_visual->visual = vis_info.visual;
      //    ERROR(my_visual->depth==8,"What the hell? Non-8 bit Psuedo color..\n");
      printf("Using 8 bit Psuedo color\n");
    }
    else
    { 
      printf("X windows screen type not supported\n");
      exit(0); 
    }
  }

  switch (mode)
  {
    case VMODE_320x200 : 
    { xres=320; yres=200; } break;
    case VMODE_640x480 :
    { xres=640; yres=480; } break;
      
  }

  for (i=1;i<argc;i++)
  { if (!strcmp(argv[i],"-size"))
    { i++; if (!sscanf(argv[i],"%d",&xres)) xres=320;
      i++; if (!sscanf(argv[i],"%d",&yres)) yres=200;
    }
  }


  Colormap tmpcmap;
  
  tmpcmap = XCreateColormap(display, XRootWindow(display,
						my_visual->screen), X_visual, AllocNone);

  int attribmask = CWColormap | CWBorderPixel;
  XSetWindowAttributes attribs;
  attribs.border_pixel = 0;
  attribs.colormap = tmpcmap;
  
  mainwin=XCreateWindow(display,
			XRootWindow(display,my_visual->screen),
			0,0,do24 ? xres*2 : xres,do24 ? yres*2 : yres,
			0,
			my_visual->depth,
			InputOutput,
			X_visual,
			attribmask,
			&attribs);
  xres--; yres--;


  icon_pixmap=XCreateBitmapFromData(display,mainwin,bitmap_bits,
	bitmap_width, bitmap_height);
  ERROR((size_hints=XAllocSizeHints()),"memory allocation error");
  ERROR((wm_hints=XAllocWMHints()),"memory allocation error");
  ERROR((class_hints=XAllocClassHint()),"memory allocation error");

  int lock=0;
  for (i=1;i<argc;i++)
    if (!strcmp(argv[i],"-lock_size"))
     lock=1;

  if (lock)
  {
    size_hints->flags=PPosition | PSize | PMinSize | PMaxSize;
    size_hints->min_width  = xres+1;
    size_hints->min_height = yres+1;
    size_hints->max_width  = xres+1;
    size_hints->max_height = yres+1;
  }
  else
  {
    size_hints->flags=PPosition | PSize | PMinSize;
    size_hints->min_width  = 320;
    size_hints->min_height = 200;
  }


  ERROR(XStringListToTextProperty(&win_name,1,&winName),"alloc failed");
  ERROR(XStringListToTextProperty(&icon_name,1,&iconName),"alloc fialed");
  wm_hints->initial_state=NormalState;  // not iconified at first
  wm_hints->input=1;                  // needs keyboard input
  wm_hints->icon_pixmap=icon_pixmap;
  wm_hints->flags=StateHint | IconPixmapHint | InputHint;
  class_hints->res_name=argv[0];
  class_hints->res_class="(C) 1995 Crack dot Com, Jonathan Clark";
  XSetWMProperties(display,mainwin,&winName,&iconName,argv,argc,size_hints,
	wm_hints,class_hints);

  XSelectInput(display,mainwin,
    KeyPressMask | VisibilityChangeMask | ButtonPressMask | ButtonReleaseMask |
    ButtonMotionMask | PointerMotionMask | KeyReleaseMask |
    ExposureMask | StructureNotifyMask);

  ERROR(font_info=XLoadQueryFont(display,"9x15"),"cannot open 9x15");
  getGC(mainwin,&gc,font_info);

/* try to determine what type of monitor we are using */


  // detect which type of screen the X client will run on...
  // if they requested 24 bit mode, check for that type of display
 


  XSetBackground(display,gc,BlackPixel(display,screen_num));

  XMapWindow(display,mainwin);
  do
  { XNextEvent(display, &report);
  } while (report.type!= Expose);     // wait for our window to pop up


  XDefineCursor(display,mainwin, CreateNullCursor(display,mainwin));

  screen=new image(xres+1,yres+1,NULL,2);  

  XCMap=XCreateColormap(display,mainwin,X_visual,AllocAll);  
  screen->clear();
  update_dirty(screen);

  for (i=1;i<argc;i++)
  { if (!strcmp(argv[i],"-grab_pointer"))
    {
      i++;
      int sec;
      if (i<argc && sscanf(argv[i],"%d",&sec))
      { printf("-grab_pointer : delaying for %d seconds\n",sec);
	sleep(sec);
      }
      if (XGrabPointer(display,mainwin,True,
		       PointerMotionMask|ButtonMotionMask|
		       Button1MotionMask|
		       Button2MotionMask|
		       Button3MotionMask|
		       Button4MotionMask|
		       Button5MotionMask,
		       GrabModeAsync,
		       GrabModeAsync,
		       mainwin,
		       None,
		       CurrentTime)==GrabSuccess)
        printf("Pointer grab in effect!\n");
      else
        printf("Unable to grab pointer  :(\n");
    }
  }
    
}

void close_graphics()
{
  if (lastl) delete lastl; lastl=NULL;
  if (screen)
    delete screen;
  XUnloadFont(display,font_info->fid);
  XFreeGC(display,gc);
  XFree((char *)my_visual);
  XCloseDisplay(display);
}

void copy_24part(uchar *Xdata, image *im, int x1, int y1, int x2, int y2)
{
  uchar *src=im->scan_line(y1)+x1;
  ushort *dst=(ushort *)Xdata+(y1*2)*im->width()+x1;

  int src_add=im->width()-(x2-x1+1);

  int x,y,w=(x2-x1+1)*2;
  ushort v;
  for (y=y1;y<=y2;y++)
  {
    for (x=x1;x<=x2;x++)
    {
      v=*(src++);
      *(dst++)=(v|(v<<8));
    }    
    dst=(ushort *) ((uchar *)dst+src_add*2);
    memcpy(dst,(uchar *)dst-im->width()*2,w);
    dst=(ushort *)((uchar *)dst+im->width()*2);

    src+=src_add;
  }
}



void put_part_image(Window win, image *im, int x, int y, int x1, int y1, int x2, int y2)
{
  CHECK(im->special);
  XImage_Info *xi=(XImage_Info *)im->special->extended_descriptor;

  if (do24)
    copy_24part((uchar *)xi->XImg->data,im,x1,y1,x2,y2);

#ifndef NO_SHM
  if (doShm)
  {    
    XEvent ev;
    XSync(display,False);
    if (XCheckTypedEvent(display, ConfigureNotify,&ev)==False)
    {
      if (do24)
        XShmPutImage(display,win,gc,xi->XImg,x1*2,y1*2,x*2,y*2,(x2-x1+1)*2,(y2-y1+1)*2,True);
      else
        XShmPutImage(display,win,gc,xi->XImg,x1,y1,x,y,x2-x1+1,y2-y1+1,True);
      // wait for the Put to finish
      wait_shm_finish();
    } else     // screen size changed,  better wait till this event is handled cause put might be invalid
      XPutBackEvent(display,&ev);

  }  
  else  
#endif
  if (do24)
    XPutImage(display,win,gc,xi->XImg,x1*2,y1*2,x*2,y*2,(x2-x1+1)*2,(y2-y1+1)*2);
  else
    XPutImage(display,win,gc,xi->XImg,x1,y1,x,y,x2-x1+1,y2-y1+1);

}

void put_image(image *im, int x, int y)
{
  put_part_image(mainwin,im,x,y,0,0,im->width()-1,im->height()-1);  
}



void update_dirty_window(Window win, image *im, int xoff, int yoff)
{

  int count;
  dirty_rect *dr,*q;
  image *Xim;
  CHECK(im->special);  // make sure the image has the ablity to contain dirty areas
  if (im->special->keep_dirt==0)
    put_image(im,xoff,yoff);
  else
  {
    count=im->special->dirties.number_nodes();
    if (!count) return;  // if nothing to update, return
    dr= (dirty_rect *) (im->special->dirties.first());
    while (count>0)
    {
      put_part_image(win,im,xoff+dr->dx1,yoff+dr->dy1,dr->dx1,dr->dy1,dr->dx2,dr->dy2);     
//      XDrawRectangle(display,mainwin,gc,xoff+dr->dx1,yoff+dr->dy1,
//		     xoff+dr->dx2-dr->dx1+1,yoff+dr->dy2-dr->dy1+1);
      q=dr;
      dr=(dirty_rect *) (dr->next());
      im->special->dirties.unlink((linked_node *)q);
      delete q;
      count--;
    }
  }
//  XFlush(display);
}

void update_dirty(image *im, int xoff, int yoff)
{ update_dirty_window(mainwin,im,xoff,yoff); }

void palette::load()
{
  if (lastl)
    delete lastl;
  lastl=copy();

  {
    int i;

      XColor color;
      color.flags=DoRed|DoBlue|DoGreen;
      for (i=0;i<ncolors;i++)
      {
	color.pixel=i;
	color.red=red(i)<<8|0xff; color.green=green(i)<<8|0xff; color.blue=blue(i)<<8|0xff;
	XStoreColor(display,XCMap,&color);
      }
      XSetWindowColormap(display,mainwin,XCMap);

    current_background=bg;
  }
}

struct hist_entry
{
  long total;
  unsigned char org;
} ;

static int histcompare(const void *i, const void *j)
{ 
  if (((hist_entry *)i)->org==0) return -1;  // make sure the background gets allocated
  else return(((hist_entry *)j)->total - ((hist_entry *)i)->total); 
}


void palette::load_nice()
{
  int i,fail,y,x,wd,j;
  long closest_val,closest_pixel,k;
  char *gotten;
  image *im;
  unsigned char *sl;
  palette *Xpal;
  hist_entry *histogram;
  Colormap stdcm;
  XColor color;

  if (do24) { load(); return ; }

  filter f(ncolors);

  histogram=(hist_entry *)jmalloc(sizeof(hist_entry)*ncolors,"palette::histogram"); 
  gotten=(char *)jmalloc(ncolors,"palette::gotten_colors");

  memset(gotten,0,ncolors);
  for (i=0;i<ncolors;i++)
  { histogram[i].total=0;
  histogram[i].org=i;
  }

  for (i=image_list.number_nodes(),im=(image *)image_list.first();i;i--,im=(image *)im->next()) 
  { 
    if (im!=screen)
    { 
      wd=im->width();
      for (y=im->height();y;y--)
      {
        sl=im->scan_line(y-1);
        for (x=0;x<wd;x++)
          histogram[sl[x]].total++;
      }
    }
  }
  qsort(histogram,ncolors,sizeof(hist_entry),histcompare);
  stdcm=XDefaultColormap(display,screen_num);
  Xpal=NULL;
  for (i=0,fail=0;i<ncolors;i++)
  {
    color.red=red(histogram[i].org)<<8; 
    color.green=green(histogram[i].org)<<8; 
    color.blue=blue(histogram[i].org)<<8; 

    if (XAllocColor(display,stdcm,&color))
      f.set(histogram[i].org,color.pixel);
    else  // if we couldn't allocate that color from X, find the closest.
    {
      if (!Xpal)
      { Xpal=new palette(ncolors);
      for (j=0;j<ncolors;j++)
      {
        color.pixel=j;
        XQueryColor(display,stdcm,&color);
        Xpal->set(j,color.red>>8,color.green>>8,color.blue>>8);
      }
      }
      closest_val=0xfffffff;
      for (j=0;j<ncolors;j++)
      {
        k=Xpal->red(j)*Xpal->red(j)+Xpal->green(j)*Xpal->green(j)+
          Xpal->blue(j)*Xpal->blue(j);
        if (k<closest_val)
        { closest_val=k;
        closest_pixel=j;
        }
      }
      f.set(histogram[i].org,closest_pixel);
    }
  }
  if (!Xpal)
    Xpal=new palette(ncolors);
  // store the color mapping we created back to the palette.
  for (j=0;j<ncolors;j++)
    Xpal->set(j,red(f.get_mapping(j)),green(f.get_mapping(j)),blue(f.get_mapping(j)));
  for (j=0;j<ncolors;j++)
    set(j,Xpal->red(j),Xpal->green(j),Xpal->blue(j));  
  bg=f.get_mapping(bg);
      
  // now remap all the images to fit this colormap!
  for (i=image_list.number_nodes(),im=(image *)image_list.first();i;i--,im=(image *)im->next()) 
    f.apply(im);

  delete Xpal;
  jfree(histogram);
  jfree(gotten);

  current_background=bg;
}


void switch_mode(video_mode new_mode)
{
  close_graphics();
  char *argv[]= {"Game","-noshm"};
  
  set_mode(new_mode,2,argv);
}

