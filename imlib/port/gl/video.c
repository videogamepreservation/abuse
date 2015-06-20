
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include "globals.hpp"
#include "system.h"
#include "video.hpp"
#include "dos.h"
#include "macs.hpp"
#include "image.hpp"
#include "palette.hpp"
#include "jmalloc.hpp"

image *screen=NULL;
static ulong *frame_buffer=NULL;

uchar current_background=0;

typedef struct
{
	int input;
	int output;
} keymap_t;

static float xs;
static float ys;

Display			*display;
static Colormap			x_cmap;
Window			mainwin;
static GC				x_gc;
static Visual			*my_visual;
static XVisualInfo		*x_visinfo=0;
//static XImage			*x_image;
static int				x_screen;

static int			oktodraw = 0;

static unsigned char	*framebuffer=0;

static int verbose=0;

static short *jcspace=0;

static int			doGLX;
static int glX_errorBase, glX_eventBase;
static GLXContext glX_context;

static int			glX_scaled=0;
static GLfloat			glX_scale_width;
static GLfloat			glX_scale_height;

static int config_notify=0;
static int config_notify_width;
static int config_notify_height;
int win_xres,win_yres;


void PickVisual()
{
  int attribList1[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8,
			GLX_BLUE_SIZE, 8, GLX_GREEN_SIZE, 8, None };
  int attribList2[] = { GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
			GLX_BLUE_SIZE, 8, None };

  doGLX = glXQueryExtension(display, &glX_errorBase, &glX_eventBase);
  if (!doGLX) { printf("OpenGL not supported\n"); exit(0); }

  x_visinfo = glXChooseVisual(display, x_screen, attribList1);
  if (!x_visinfo)
    x_visinfo = glXChooseVisual(display, x_screen, attribList2);
  else
    printf("double-buffered\n");

  if (!x_visinfo)
  { printf("Could not chose an RGB visual.  Use X version.\n");  exit(0); }   
  else
    printf("single-buffered\n");

  my_visual = x_visinfo->visual;
}


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

void CreateWindow(void)
{

  // setup attributes for main window
  int attribmask = CWEventMask  | CWColormap | CWBorderPixel;
  XSetWindowAttributes attribs;
  Colormap tmpcmap;

  tmpcmap = XCreateColormap(display, XRootWindow(display,
						x_visinfo->screen), my_visual, AllocNone);

  attribs.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask;
  attribs.border_pixel = 0;
  attribs.colormap = tmpcmap;

  // create the main window
  mainwin = XCreateWindow(display,
			XRootWindow(display, x_visinfo->screen),
			0, 0,	// x, y
			win_xres+1, win_yres+1,
			0, // borderwidth
			x_visinfo->depth,
			InputOutput,
			my_visual,
			attribmask,
			&attribs );

  XFreeColormap(display, tmpcmap);

}

int *lookup24=NULL;

void palette::load()
{

  int intensity;
  int color;
  int r, g, b;

  lookup24 = (int *)jmalloc(256*4,"24 bit palette lookup");
  for (color = 0 ; color < 256 ; color++)
  {
    r = (int)red(color)<<24;
    g = (int)green(color)<<16;
    b = (int)blue(color)<<8;
    lookup24[color] = r + g + b;
  }
}

void palette::load_nice()
{ load(); }

void image::make_page(short wid, short hi, unsigned char *page_buffer)
{
  if (special && !special->static_mem)
  {
    data=(uchar *)jmalloc(wid*hi,"image data\n");
    if (page_buffer)
      memcpy(data,page_buffer,wid*hi);
  } else if (page_buffer)
    data=page_buffer;
  else data=(uchar *)jmalloc(wid*hi,"image data");
}

void image::delete_page()
{
  if (!special || !special->static_mem)
    jfree(data);      
}

void reset_frame()
{
  glViewport(0, 0, win_xres+1, win_yres+1);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, xres+1, 0, yres+1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  xs=(float)(win_xres+1)/(xres+1);
  ys=(float)(win_yres+1)/(yres+1);

  if (xs!=1.0 || ys!=1.0)
    printf("Pixel scaling -> %lfx%lf\n",xs,ys);

  glPixelZoom(xs,ys);                    // top to bottom pixel lines
  glRasterPos2i(0,0);
}


void Scaled_f(void)
{
	config_notify = 1;
	glX_scaled = 1;
}

void UnScaled_f(void)
{
	config_notify = 1;
	glX_scaled = 0;
}


void set_mode(int mode, int argc, char **argv)
{
//  Cmd_AddCommand("glx_scaled", Scaled_f);
//  Cmd_AddCommand("glx_unscaled", UnScaled_f);



  /****************************** Open DISPLAY *************************/
  char display_name[100];
  // get the default display name from the enviroment variable DISPLAY
  char *ds_name=getenv("DISPLAY");
  if (!ds_name) ds_name="unix:0.0";
  strcpy(display_name,ds_name);   
  display=XOpenDisplay(display_name);  
  if (!display)
  {
    printf("Cound not connect to X server named '%s'\n",display_name);
    exit(1);
  }

  x_screen = DefaultScreen(display);


  win_xres=640; win_yres=400;
  int i;
  for (i=1;i<argc;i++)
  { if (!strcmp(argv[i],"-size"))
    { i++; sscanf(argv[i],"%d",&win_xres);
      i++; sscanf(argv[i],"%d",&win_yres);
    }
  }
  xres=320;  yres=200;
  PickVisual();


  frame_buffer=(ulong *)jmalloc(xres*yres*4,"24 bit frame buffer\n");
  screen=new image(xres,yres,NULL,2);
  xres--;     yres--;
  win_xres--; win_yres--;

  

  CreateWindow();

  glX_context = glXCreateContext(display, x_visinfo, 0, True);
  glXMakeCurrent(display, mainwin, glX_context);


  // ****************************  Create the Visual  ****************************
  XGCValues xgcvalues;
  int valuemask = GCGraphicsExposures;
  xgcvalues.graphics_exposures = False;
  x_gc = XCreateGC(display, mainwin, valuemask, &xgcvalues );


  XDefineCursor(display, mainwin, CreateNullCursor(display, mainwin));

  XMapWindow(display, mainwin);

  XEvent event;
  do
  {
    XNextEvent(display, &event);
    if (event.type == Expose && !event.xexpose.count)
      oktodraw = 1;
  } while (!oktodraw);

  reset_frame();

  XSynchronize(display, False);
}


void close_graphics()
{
  jfree(frame_buffer);
  delete screen;
  XFreeGC(display,x_gc);
  XFree((char *)my_visual);
  XCloseDisplay(display);
}


static void refresh_frame_buffer(int x1, int y1, int x2, int y2)
{
/*  x1=x1&~3;
  x2=(x2+3)&~3;*/

//  glRasterPos2i(0,yres);


/*  glPixelStorei(GL_UNPACK_ROW_LENGTH,(xres+1));
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,x1);
  glPixelStorei(GL_UNPACK_SKIP_ROWS,y1);
  glRasterPos2i(0,0);*/

//  glDrawPixels(x2-x1+1,y2-y1+1, GL_RGBA, GL_UNSIGNED_BYTE,frame_buffer);
  glRasterPos2i(x1,y1);
  glDrawPixels(x2-x1+1,y2-y1+1, GL_RGBA, GL_UNSIGNED_BYTE,frame_buffer);
//  glRecti(x1,y1,x2,y2);
  glFlush();
  glXSwapBuffers(display,mainwin);
}

void put_part(image *im, int x, int y, int x1, int y1, int x2, int y2)
{
  int width=x2-x1+1,height=y2-y1+1,a,b;

  // first convert to 24 bit  
  int size1=((long)im->width()*(long)im->height());

  
//  ulong *page=frame_buffer+y*(xres+1)+x;
  ulong *page=frame_buffer;
  uchar *sl=im->scan_line(y1)+x1;
  int skip=im->width()-(x2-x1+1);
  int frame_skip=(xres+1)-(x2-x1+1);
  for (y=y1;y<=y2;y++)
  {
    for (x=x1;x<=x2;x++)
      *(page++)=lookup24[*(sl++)];
    sl+=skip;
//    page+=frame_skip;
  } 
  refresh_frame_buffer(x,y,x+(x2-x1),y+(y2-y1));
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








