#include "emm.hpp"
#include "image.hpp"
#include "macs.hpp"
#include "system.h"
#include "dos.h"
#include "system.h"
#include "exitproc.hpp"
#include "dprint.hpp"

#include <math.h>
#ifdef __DOS
  #include <dir.h>
#else
  #include <unistd.h>
#endif
#include <stdlib.h>

extern unsigned char current_background;
char *imerr_messages[]={"No error",
      		 	"Error occured while reading",
			"Incorrect file type",
			"File is corrupted",
		 	"File not found",
			"Memory allocation trouble",
			"Operation/file type not supported",
			"Error occured while writing, (disk full?)"};
			
                         
short imerror=0;
short swpfile_num=0;

short current_error()
{ return imerror; }

void clear_errors()
{
  if (imerror)
  { dprintf("Program stopped, error : ");
    if (imerror<=imMAX_ERROR)
      dprintf("%s\n",imerr_messages[imerror]);
    else
      dprintf("Unsonsponsered error code, you got trouble\n"); 
#ifdef __DOS_ONLY
    sound(300);
    delay(100);
    nosound();
#else
    dprintf("%c%c\n",7,8);
#endif
    exit(1);
  }
}

void set_error(short x)
{ imerror=x; }

short last_error()
{
  short ec;
  ec=imerror;
  imerror=0;
  return ec;
}

linked_list image_list;


image_descriptor::image_descriptor(short length, short height,
				   int keep_dirties, int static_memory)

{ clipx1=0; clipy1=0; 
  l=length; h=height; 
  clipx2=l-1; clipy2=h-1; 
  keep_dirt=keep_dirties; 
  static_mem=static_memory;  
}

void image::change_size(short new_width, short new_height, unsigned char *page)
{
  delete_page();
  w=new_width;
  h=new_height;
  make_page(new_width,new_height,page);
}

image::~image()
{
  image_list.unlink((linked_node *)this);
  delete_page();
  if (special)
    delete special;

}


void make_block(size_t size)
{
  void *dat=jmalloc(size,"make_block : tmp");
  CONDITION(dat,"Memory error : could not make block\n");
  if (dat) jfree((char *)dat);
}

unsigned char image::pixel(short x, short y)
{
  CONDITION(x>=0 && x<width() && y>=0 && y<height(),
     "image::pixel Bad pixel xy");
  return (*(scan_line(y)+x));
}

void image::putpixel(short x, short y, char color)
{
  CONDITION(x>=0 && x<width() && y>=0 && y<height(),
     "image::putpixel Bad pixel xy");
  if (special)
  { if (x>=special->x1_clip() && x<=special->x2_clip() &&
	y>=special->y1_clip() && y<=special->y2_clip())
      (*(scan_line(y)+x))=color;
  } else (*(scan_line(y)+x))=color;
}


image::image(short width, short height, unsigned char *page_buffer, short create_descriptor)
{
  w=width;
  h=height;  
  if (create_descriptor || page_buffer)
  { 
    if (create_descriptor==2)
      special=new image_descriptor(width,height,1,(page_buffer!=NULL));
    else special=new image_descriptor(width,height,0,(page_buffer!=NULL));
  } else special=NULL;
  make_page(width,height,page_buffer);
  //  image_list.add_end((linked_node *) this);
}

image::image(spec_entry *e, bFILE *fp)
{
  short i; 
  fp->seek(e->offset,0);
  w=fp->read_short();
  h=fp->read_short();
  special=NULL;
  make_page(w,h,NULL);
  for (i=0;i<h;i++)
    fp->read(scan_line(i),w);
  //  image_list.add_end((linked_node *) this); 
}

image::image(bFILE *fp)
{
  short i; 
  w=fp->read_short();
  h=fp->read_short();
  special=NULL;
  make_page(w,h,NULL);
  for (i=0;i<h;i++)
    fp->read(scan_line(i),w);
  //  image_list.add_end((linked_node *) this); 
}

void image_uninit()
{
/*  image *im;
  while (image_list.first())
  {
    im=(image *)image_list.first();
    image_list.unlink((linked_node *)im);
    delete im;
  } */
}


void image_cleanup(int ret, void *arg)
{ image_uninit(); }

void image_init()
{
  unsigned char bt[2];
  unsigned short wrd,*up;
  bt[0]=1;
  bt[1]=0;
  up=(unsigned short *)bt;
  wrd=int_to_intel(*up);
  if (wrd!=0x01)
  { dprintf("Compiled under wrong ENDING-nes, edit system.h and try again\n");
    dprintf("1 (intel) = %d\n",(int)wrd);
    exit(1);
  }
  imerror=0;
  exit_proc(image_cleanup,image_uninit); // important that possible shared memory is deallocated!
}


long image::total_pixels(unsigned char background)
{
  short i,j;
  long co;
  unsigned char *c;
  for (co=0,i=height()-1;i>=0;i--)
  { c=scan_line(i);
    for (j=width()-1;j>=0;j--,c++)
      if (*c!=background) co++;
  }
  return co;
}

void image::clear(short color)
{
  short i;
  if (color==-1) color=current_background;
  if (special)
  { if (special->x1_clip()<=special->x2_clip())
      for (i=special->y2_clip();i>=special->y1_clip();i--)
	memset(scan_line(i)+special->x1_clip(),color,
	       special->x2_clip()-special->x1_clip()+1);
  }
  else
    for (i=height()-1;i>=0;i--)
      memset(scan_line(i),color,width());
  add_dirty(0,0,width()-1,height()-1);
}


image *image::copy()
{
  image *im;
  unsigned char *c,*dat;
  int i;
  dat=(unsigned char *)jmalloc(width(),"image copy");
  im=new image(width(),height());
  for (i=height()-1;i>=0;i--)
  { c=scan_line(i);
    memcpy(dat,c,width());
    c=im->scan_line(i);
    memcpy(c,dat,width());
  }
  jfree((char *)dat);
  return im;
}



void image::line(short x1, short y1,short x2, short y2, unsigned char color)
{
  short i,xc,yc,er,n,m,xi,yi,xcxi,ycyi,xcyi;
  unsigned dcy,dcx;
  // check to make sure that both endpoint are on the screen

  short cx1,cy1,cx2,cy2;

  // check to see if the line is completly clipped off
  get_clip(cx1,cy1,cx2,cy2);  
  if ((x1<cx1 && x2<cx1) || (x1>cx2 && x2>cx2) || 
      (y1<cy1 && y2<cy1) || (y1>cy2 && y2>cy2))
    return ;
 
  if (x1>x2)        // make sure that x1 is to the left
  {    
    i=x1; x1=x2; x2=i;  // if not swap points
    i=y1; y1=y2; y2=i;
  }  

  // clip the left side
  if (x1<cx1)
  {  
    int my=(y2-y1);       
    int mx=(x2-x1),b;
    if (!mx) return ;
    if (my)
    {
      b=y1-(y2-y1)*x1/mx;      
      y1=my*cx1/mx+b;
      x1=cx1;      
    }
    else x1=cx1;
  }

  // clip the right side
  if (x2>cx2)
  {  
    int my=(y2-y1);       
    int mx=(x2-x1),b;
    if (!mx) return ;
    if (my)
    {
      b=y1-(y2-y1)*x1/mx;      
      y2=my*cx2/mx+b;
      x2=cx2;      
    }
    else x2=cx2;
  }

  if (y1>y2)        // make sure that y1 is on top
  {    
    i=x1; x1=x2; x2=i;  // if not swap points
    i=y1; y1=y2; y2=i;
  }  

  // clip the bottom
  if (y2>cy2)
  {  
    int mx=(x2-x1);       
    int my=(y2-y1),b;
    if (!my)
      return ;
    if (mx)
    {
      b=y1-(y2-y1)*x1/mx;      
      x2=(cy2-b)*mx/my;
      y2=cy2;
    }
    else y2=cy2;
  }

  // clip the top
  if (y1<cy1)
  {  
    int mx=(x2-x1);       
    int my=(y2-y1),b;
    if (!my) return ;
    if (mx)
    {
      b=y1-(y2-y1)*x1/mx;      
      x1=(cy1-b)*mx/my;
      y1=cy1;
    }
    else y1=cy1;
  }


  // see if it got cliped into the box, out out
  if (x1<cx1 || x2<cx1 || x1>cx2 || x2>cx2 || y1<cy1 || y2 <cy1 || y1>cy2 || y2>cy2)
    return ;
  
    

  if (x1>x2)
  { xc=x2; xi=x1; }
  else { xi=x2; xc=x1; }


  // assume y1<=y2 from above swap operation
  yi=y2; yc=y1;

  add_dirty(xc,yc,xi,yi);
  dcx=x1; dcy=y1;
  xc=(x2-x1); yc=(y2-y1);
  if (xc<0) xi=-1; else xi=1;
  if (yc<0) yi=-1; else yi=1;
  n=abs(xc); m=abs(yc);
  ycyi=abs(2*yc*xi);
  er=0;

  if (n>m)
  {
    xcxi=abs(2*xc*xi);
    for (i=0;i<=n;i++)
    {
      *(scan_line(dcy)+dcx)=color;
      if (er>0)
      { dcy+=yi;
	er-=xcxi;
      }
      er+=ycyi;
      dcx+=xi;
    }
  }
  else
  {
    xcyi=abs(2*xc*yi);
    for (i=0;i<=m;i++)
    {
      *(scan_line(dcy)+dcx)=color;
      if (er>0)
      { dcx+=xi;
	er-=ycyi;
      }
      er+=xcyi;
      dcy+=yi;
    }
  }
}


void image::put_image(image *screen, short x, short y, char transparent)
{
  short i,j,xl,yl;
  unsigned char *pg1,*pg2,*source,*dest;
  if (screen->special)  // the screen is clipped then we onl want to put
		    // part of the image
    put_part(screen,x,y,0,0,width()-1,height()-1,transparent);
  else
  {
    if (x<screen->width() && y<screen->height())
    {
      xl=width();
      if (x+xl>screen->width())     // clip to the border of the screen
				xl=screen->width()-x;
      yl=height();
      if (y+yl>screen->height())
				yl=screen->height()-y;

      int startx=0,starty=0;
      if (x<0) { startx=-x; x=0; }
      if (y<0) { starty=-y; y=0; }

      if (xl<0 || yl<0) return ;

      screen->add_dirty(x,y,x+xl-1,y+yl-1);
      for (j=starty;j<yl;j++,y++)
      {
				pg1=screen->scan_line(y);
				pg2=scan_line(j);
				if (transparent)
				{
				  for (i=startx,source=pg2+startx,dest=pg1+x;i<xl;i++,source++,dest++)
			            if (*source!=current_background) *dest=*source;
				} else memcpy(&pg1[x],pg2,xl);   // strait copy
      }
    }
  }
}

void image::fill_image(image *screen, short x1, short y1, short x2, short y2, short allign)
{
  short i,j,w,xx,start,xl,starty;
  unsigned char *pg1,*pg2;
  CHECK(x1<=x2 && y1<=y2);  // we should have gotten this

  if (screen->special)
  { x1=screen->special->bound_x1(x1);
    y1=screen->special->bound_y1(y1);
    x2=screen->special->bound_x2(x2);
    y2=screen->special->bound_y2(y2);
  }
  else
  { if (x1<0) x1=0;
    if (y2<0) y1=0;
    if (x2>=screen->width())  x2=screen->width()-1;
    if (y2>=screen->height()) y2=screen->height()-1;
  }
  if (x2<0 || y2<0 || x1>=screen->width() || y1>=screen->height())
    return ;
  screen->add_dirty(x1,y1,x2,y2);
  w=width();
  if (allign)
  {
    start=x1%w;
    starty=y1%height();
  }
  else
  { start=0;
    starty=0;
  }
  for (j=y1;j<=y2;j++)
  {
    pg1=screen->scan_line(j);
    pg2=scan_line(starty++);
    if (starty>=height()) starty=0;
    i=x1;
    xx=start;
    while (i<=x2)
    {
      xl=min(w-xx,x2-i+1);

      memcpy(&pg1[i],&pg2[xx],xl);
      xx=0;
      i+=xl;
    }
  }
}


void image::put_part(image *screen, short x, short y,
		short x1, short y1, short x2, short y2, char transparent)
{
  short xl,yl,j,i;
  short cx1,cy1,cx2,cy2;
  unsigned char *pg1,*pg2,*source,*dest;
  CHECK(x1<=x2 && y1<=y2);

  screen->get_clip(cx1,cy1,cx2,cy2);


  // see if the are to be put is outside of actual image, if so adjust 
  // to fit in the image
  if (x1<0) { x+=-x1; x1=0; }  
  if (y1<0) { y+=-y1; y1=0; }  
  if (x2>=width()) x2=width()-1;
  if (y2>=height()) y2=height()-1;
  if (x1>x2 || y1>y2) return ;      // return if it was adjusted so that nothing will be put
    

  // see if the image gets clipped of the screen
  if (x>cx2 || y>cy2 || x+(x2-x1)<cx1 || y+(y2-y1)<cy1) return ;

  
  if (x<cx1)
  { x1+=(cx1-x); x=cx1; }
  if (y<cy1)
  { y1+=(cy1-y); y=cy1; }

  if (x+x2-x1+1>cx2)
  { x2=cx2-x+x1; }

  if (y+y2-y1+1>cy2)
  { y2=cy2-y+y1; }
  if (x1>x2 || y1>y2) return ;    

  


  xl=x2-x1+1;
  yl=y2-y1+1;

  screen->add_dirty(x,y,x+xl-1,y+yl-1);

  pg1=screen->scan_line(y);
  pg2=scan_line(y1);

  if (transparent)
  {
    for (j=0;j<yl;j++)        
    {
      for (i=0,source=&pg2[x1],dest=&pg1[x];i<xl;i++,source++,dest++)	
        if (*source!=current_background) *dest=*source;
      pg1=screen->next_line(y+j,pg1);  
      pg2=next_line(y1+j,pg2);
    }      
  }
  else
  for (j=0;j<yl;j++)
  {	
    memcpy(&pg1[x],&pg2[x1],xl);   // strait copy
    pg1=screen->next_line(y+j,pg1);  
    pg2=next_line(y1+j,pg2);
  }    
}

void image::put_part_xrev(image *screen, short x, short y,
		short x1, short y1, short x2, short y2, char transparent)
{
  short xl,yl,j,i;
  short cx1,cy1,cx2,cy2;
  unsigned char *pg1,*pg2,*source,*dest;
  CHECK(x1<=x2 && y1<=y2);

  i=x1; x1=width()-x2-1;  // reverse the x locations
  x2=width()-i-1;

  if (x1<0)
  { x-=x1; x1=0; }
  if (y1<0)
  { y-=y1; y1=0; }

  if (screen->special)
  {
    screen->special->get_clip(cx1,cy1,cx2,cy2);
    if (x>cx2 || y>cy2 || x+(x2-x1)<0 || y+(y2-y1)<0) return ;
    if (x<cx1)
    { x1+=(cx1-x); x=cx1; }
    if (y<cy1)
    { y1+=(cy1-y); y=cy1; }
    if (x+x2-x1+1>cx2)
    { x2=cx2-x+x1; }
    if (y+y2-y1+1>cy2)
    { y2=cy2-y+y1; }
  }
  else  if (x>screen->width() || y>screen->height() || x+x2<0 || y+y2<0)
    return ;

  if (x<screen->width() && y<screen->height() && x1<width() && y1<height() &&
      x1<=x2 && y1<=y2)
  {
    if (x2>=width())
      x2=width()-1;
    if (y2>=height())
      y2=height()-1;
    xl=x2-x1+1;
    if (x+xl>screen->width())
      xl=screen->width()-x;
    yl=y2-y1+1;
    if (y+yl>screen->height())
      yl=screen->height()-y;
    screen->add_dirty(x,y,x+xl-1,y+yl-1);
    for (j=0;j<yl;j++)
    {
      pg1=screen->scan_line(y+j);
      pg2=scan_line(y1+j);
      if (transparent)
      {
				for (i=0,source=&pg2[x1],dest=&pg1[x+xl-1];i<xl;i++,source++,dest--)
          if (*source!=current_background) *dest=*source;
      }
      else 
				for (i=0,source=&pg2[x1],dest=&pg1[x+xl-1];i<xl;i++,source++,dest++)
          *dest=*source;
    }
  }
}


void image::put_part_masked(image *screen, image *mask, short x, short y,
		short maskx, short masky,
		short x1, short y1, short x2, short y2)
{
  short xl,yl,j,i,ml,mh;
  short cx1,cy1,cx2,cy2;
  unsigned char *pg1,*pg2,*pg3;
  CHECK(x1<=x2 && y1<=y2);

  if (screen->special)
  {
    screen->special->get_clip(cx1,cy1,cx2,cy2);
    if (x>cx2 || y>cy2 || x+(x2-x1)<0 || y+(y2-y1)<0) return ;
    if (x<cx1)
    { x1+=(cx1-x); x=cx1; }
    if (y<cy1)
    { y1+=(cy1-y); y=cy1; }
    if (x+x2-x1>cx2)
    { x2=cx2+x1-x; }
    if (y+y2-y1>cy2)
    { y2=cy2+y1-y; }
  }
  else  if (x>screen->width() || y>screen->height() || x+x1<0 || y+y1<0)
    return ;

  ml=mask->width();
  mh=mask->height();
  if (x<screen->width() && y<screen->height() && x1<width() && y1<height() &&
      maskx<ml && masky<mh && x1<=x2 && y1<=y2)
  {

    if (x2>=width())
      x2=width()-1;
    if (y2>=height())
      y2=height()-1;
    xl=x2-x1+1;
    if (x+xl>screen->width())
      xl=screen->width()-x-1;
    yl=y2-y1+1;
    if (y+yl>screen->height())
      yl=screen->height()-y-1;
    screen->add_dirty(x,y,x+xl-1,y+yl-1);
    for (j=0;j<yl;j++)
    {
      pg1=screen->scan_line(y+j);
      pg2=scan_line(y1+j);
      pg3=mask->scan_line(masky++);
      if (masky>=mh)           // wrap the mask around if out of bounds
				masky=0;
      for (i=0;i<xl;i++)
      {
				if (pg3[maskx+i])          // check to make sure not 0 before putting
				  pg1[x+i]=pg2[x1+i];
				if (maskx>=ml)            // wrap x around if it goes to far
				  maskx=0;
      }
    }
  }
}



unsigned char image::brightest_color(palette *pal)
{ unsigned char *p,r,g,b,bri;
  short i,j;
  long brv;
  brv=0; bri=0;
  for (j=0;j<h;j++)
  {
    p=scan_line(j);
    for (i=0;i<w;i++)
    { pal->get(p[i],r,g,b);
      if ((long)r*(long)g*(long)b>brv)
      { brv=(long)r*(long)g*(long)b;
	bri=p[i];
      }
    }
  }
  return bri;
}

unsigned char image::darkest_color(palette *pal, short noblack)
{ unsigned char *p,r,g,b,bri;
  short i,j;
  long brv,x;
  brv=(long)258*(long)258*(long)258; bri=0;
  for (j=0;j<h;j++)
  {
    p=scan_line(j);
    for (i=0;i<w;i++)
    { pal->get(p[i],r,g,b);
      x=(long)r*(long)g*(long)b;
      if (x<brv && (x || !noblack))
      { brv=x;
	bri=p[i];
      }
    }
  }
  return bri;
}

void image::rectangle(short x1, short y1,short x2, short y2, unsigned char color)
{
  line(x1,y1,x2,y1,color);
  line(x2,y1,x2,y2,color);
  line(x1,y2,x2,y2,color);
  line(x1,y1,x1,y2,color);
}

void image::set_clip(short x1, short y1, short x2, short y2)
{
  // If the image does not already have an Image descriptor, allocate one.

  if (!special)
  { 
    // create a new image descriptor withj no dirty rectangle keeping
    special=new image_descriptor(width(),height(),0);
  }
  special->set_clip(x1,y1,x2,y2);  // set the image descriptor what the clip
		       // should be it will adjust to fit wiothin the image.
}

void image::get_clip (short &x1, short &y1, short &x2, short &y2)
{
  if (special)
    special->get_clip(x1,y1,x2,y2);
  else
  { x1=0; y1=0; x2=width()-1; y2=height()-1; }
}

void image::in_clip  (short x1, short y1, short x2, short y2)
{
  if (special)
  {
    if (x1<special->x1_clip())
      x1=special->x1_clip();
    if (y1<special->y1_clip())
      y1=special->y1_clip();
    if (x2>special->x2_clip())
      x2=special->x2_clip();
    if (y2>special->y2_clip())
      y2=special->y2_clip();
  }
  set_clip(x1,y1,x2,y2);
}
// this function reduces the number of dirty rectanges
// to 1 by find the minmum area that can contain all the rectangles and
// making this the new dirty area
void image_descriptor::reduce_dirties()
{
  dirty_rect *p,*q;
  short x1,y1,x2,y2,nn;
  x1=6000; y1=6000;
  x2=0; y2=0;
  p=(dirty_rect *)dirties.first();
  nn=dirties.number_nodes();
  while (nn>0)
  {
    if (p->dx1<x1) x1=p->dx1;
    if (p->dy1<y1) y1=p->dy1;
    if (p->dx2>x2) x2=p->dx2;
    if (p->dy2>y2) y2=p->dy2;
    q=p;
    p=(dirty_rect *)p->next();
    dirties.unlink((linked_node *)q);
    delete q;
    nn--;
  }
  dirties.add_front((linked_node *) new dirty_rect(x1,y1,x2,y2));
}

void image_descriptor::delete_dirty(int x1, int y1, int x2, int y2)
{
  short i,ax1,ay1,ax2,ay2;
  dirty_rect *p,*next;
  if (keep_dirt)
  {
    if (x1<0) x1=0;
    if (y1<0) y1=0;
    if (x2>=(int)l) x2=l-1;
    if (y2>=(int)h) y2=h-1;
    if (x1>x2) return;
    if (y1>y2) return ;

    i=dirties.number_nodes();
    if (!i)
      return ;
    else
    {
      for (p=(dirty_rect *)dirties.first();i;i--,p=(dirty_rect *)next)
      {
        next=(dirty_rect *)p->next();
        // are the two touching?
	if (!(x2<p->dx1 || y2<p->dy1 || x1>p->dx2 || y1>p->dy2))
        {
          // does it take a x slice off? (across)
          if (x2>=p->dx2 && x1<=p->dx1)
          {
            if (y2>=p->dy2 && y1<=p->dy1)
            {
              dirties.unlink((linked_node *)p);
              delete p;
            } 
            else if (y2>=p->dy2)
              p->dy2=y1-1;
            else if (y1<=p->dy1)
              p->dy1=y2+1;
            else
            {
              dirties.add_front((linked_node *) new dirty_rect(p->dx1,p->dy1,p->dx2,y1-1));
              p->dy1=y2+1;
            }
          } 
          // does it take a y slice off (down)
          else if (y2>=p->dy2 && y1<=p->dy1)
          {
            if (x2>=p->dx2)
              p->dx2=x1-1;
            else if (x1<=p->dx1)
              p->dx1=x2+1;
            else 
            {
              dirties.add_front((linked_node *) new dirty_rect(p->dx1,p->dy1,x1-1,p->dy2));
              p->dx1=x2+1;
            }
          }
          // otherwise it just takes a little chunk off
          else 
          {
            if (x2>=p->dx2)      { ax1=p->dx1; ax2=x1-1; }
            else if (x1<=p->dx1) { ax1=x2+1; ax2=p->dx2; }
            else                { ax1=p->dx1; ax2=x1-1; } 
            if (y2>=p->dy2)      { ay1=y1; ay2=p->dy2; }
            else if (y1<=p->dy1) { ay1=p->dy1; ay2=y2; }
            else                { ay1=y1; ay2=y2; }
            dirties.add_front((linked_node *) new dirty_rect(ax1,ay1,ax2,ay2));
          
            if (x2>=p->dx2 || x1<=p->dx1)  { ax1=p->dx1; ax2=p->dx2; }
            else                         { ax1=x2+1; ax2=p->dx2; } 

            if (y2>=p->dy2)
            { if (ax1==p->dx1) { ay1=p->dy1; ay2=y1-1; }
                          else { ay1=y1; ay2=p->dy2;   } }
            else if (y1<=p->dy1) { if (ax1==p->dx1) { ay1=y2+1; ay2=p->dy2; }
                                             else  { ay1=p->dy1; ay2=y2; } }
            else           { if (ax1==p->dx1) { ay1=p->dy1; ay2=y1-1; }
                             else { ay1=y1; ay2=y2; } }
            dirties.add_front((linked_node *) new dirty_rect(ax1,ay1,ax2,ay2));

            if (x1>p->dx1 && x2<p->dx2)
            {
              if (y1>p->dy1 && y2<p->dy2)
              {
                dirties.add_front((linked_node *) new dirty_rect(p->dx1,p->dy1,p->dx2,y1-1));
                dirties.add_front((linked_node *) new dirty_rect(p->dx1,y2+1,p->dx2,p->dy2));
              } else if (y1<=p->dy1)
                dirties.add_front((linked_node *) new dirty_rect(p->dx1,y2+1,p->dx2,p->dy2));
              else 
                dirties.add_front((linked_node *) new dirty_rect(p->dx1,p->dy1,p->dx2,y1-1));
            } else if (y1>p->dy1 && y2<p->dy2)
              dirties.add_front((linked_node *) new dirty_rect(p->dx1,y2+1,p->dx2,p->dy2));
            dirties.unlink((linked_node *) p);
            delete p;
          }
        }
      }
    }
  }
}

// specifies that an area is a dirty
void image_descriptor::add_dirty(int x1, int y1, int x2, int y2)
{
  short i;
  dirty_rect *p;
  if (keep_dirt)
  {
    if (x1<0) x1=0;
    if (y1<0) y1=0;
    if (x2>=(int)l) x2=l-1;
    if (y2>=(int)h) y2=h-1;
    if (x1>x2) return;
    if (y1>y2) return ;
    
    i=dirties.number_nodes();
    if (!i)
      dirties.add_front((linked_node *) new dirty_rect(x1,y1,x2,y2));
    else if (i>=MAX_DIRTY)
    {
      dirties.add_front((linked_node *) new dirty_rect(x1,y1,x2,y2));
      reduce_dirties();  // reduce to one dirty rectangle, we have to many
    }
    else
    {  
      for (p=(dirty_rect *)dirties.first();i>0;i--)
      {

        // check to see if this new rectangle completly encloses the check rectangle
				if (x1<=p->dx1 && y1<=p->dy1 && x2>=p->dx2 && y2>=p->dy2)
				{
				  dirty_rect *tmp=(dirty_rect*) p->next();
				  dirties.unlink((linked_node *)p);
				  delete p;
				  if (!dirties.first())
			  	    i=0;
				  else p=tmp;	  
				}	
				else if (!(x2<p->dx1 || y2<p->dy1 || x1>p->dx2 || y1>p->dy2))
				{	  
			
			
				 
			/*          if (x1<=p->dx1) { a+=p->dx1-x1; ax1=x1; } else ax1=p->dx1;
			          if (y1<=p->dy1) { a+=p->dy1-y1; ay1=y1; } else ay1=p->dy1; 
			          if (x2>=p->dx2) { a+=x2-p->dx2; ax2=x2; } else ax2=p->dx2;
			          if (y2>=p->dy2) { a+=y2-p->dy2; ay2=y2; } else ay2=p->dy2;
				  
				  if (a<50) 
				  { p->dx1=ax1;	  			       // then expand the dirty
				    p->dy1=ay1;
				    p->dx2=ax2;
				    p->dy2=ay2;
				    return ;
				  } 
				  else */
				    {
				      if (x1<p->dx1)
				        add_dirty(x1,max(y1,p->dy1),p->dx1-1,min(y2,p->dy2));
				      if (x2>p->dx2)
				        add_dirty(p->dx2+1,max(y1,p->dy1),x2,min(y2,p->dy2));
				      if (y1<p->dy1)
				        add_dirty(x1,y1,x2,p->dy1-1);
				      if (y2>p->dy2)
				        add_dirty(x1,p->dy2+1,x2,y2);
				      return ;
				    }
				    p=(dirty_rect *)p->next();
				  } else p=(dirty_rect *)p->next();      
	
      } 
      CHECK(x1<=x2 && y1<=y2);
      dirties.add_end((linked_node *)new dirty_rect(x1,y1,x2,y2)); 
    }
  }
}

void image::bar      (short x1, short y1, short x2, short y2, unsigned char color)
{
  short y;
  if (x1>x2 || y1>y2) return ;
  if (special)
  { x1=special->bound_x1(x1);
    y1=special->bound_y1(y1);
    x2=special->bound_x2(x2);
    y2=special->bound_y2(y2);
  }
  else
  { if (x1<0) x1=0;
    if (y1<0) y1=0;
    if (x2>=width())  x2=width()-1;
    if (y2>=height()) y2=height()-1;
  }
  if (x2<0 || y2<0 || x1>=width() || y1>=height() || x2<x1 || y2<y1)
    return ;
  for (y=y1;y<=y2;y++)
    memset(scan_line(y)+x1,color,(x2-x1+1));
  add_dirty(x1,y1,x2,y2);
}

void image::xor_bar  (short x1, short y1, short x2, short y2, unsigned char color)
{
  short y,x;
  if (x1>x2 || y1>y2) return ;
  if (special)
  { x1=special->bound_x1(x1);
    y1=special->bound_y1(y1);
    x2=special->bound_x2(x2);
    y2=special->bound_y2(y2);
  }
  else
  { if (x1<0) x1=0;
    if (y1<0) y1=0;
    if (x2>=width())  x2=width()-1;
    if (y2>=height()) y2=height()-1;
  }
  if (x2<0 || y2<0 || x1>=width() || y1>=height() || x2<x1 || y2<y1)
    return ;

  unsigned char *sl=scan_line(y1)+x1; 
  for (y=y1;y<=y2;y++)
  {
    unsigned char *s=sl;
    for (x=x1;x<=x2;x++,s++)
      *s=(*s)^color;
    sl+=w;
  }

  add_dirty(x1,y1,x2,y2);
}


void image::unpack_scanline(short line, char bitsperpixel)
{
  short x;
  unsigned char *sl,*ex,mask,bt,sh;
  ex=(unsigned char *)jmalloc(width(),"image::unpacked scanline");
  sl=scan_line(line);
  memcpy(ex,sl,width());

  if (bitsperpixel==1)      { mask=128;           bt=8; }
  else if (bitsperpixel==2) { mask=128+64;        bt=4; }
  else 			    {  mask=128+64+32+16; bt=2; }

  for (x=0;x<width();x++)
  { sh=((x%bt)<<(bitsperpixel-1));
    sl[x]=(ex[x/bt]&(mask>>sh))>>(bt-sh-1);
  }

  jfree((char *)ex);
}

void image::dither(palette *pal)
{
  short x,y,i,j;
  unsigned char dt_matrix[]={0,  136,24, 170,
		   68, 204,102,238,
		   51, 187, 17,153,
		   119,255, 85,221};

  unsigned char *sl;
  for (y=height()-1;y>=0;y--)
  {
    sl=scan_line(y);
    for (i=0,j=y%4,x=width()-1;x>=0;x--)
    {
      if (pal->red(sl[x])>dt_matrix[j*4+i])
	sl[x]=255;
      else sl[x]=0;
      if (i==3) i=0; else i++;
    }
  }
}

void image_descriptor::clear_dirties()
{
  dirty_rect *dr;
  dr=(dirty_rect *)dirties.first();  
  while (dr)
  { dirties.unlink(dr);
    delete dr;
    dr=(dirty_rect *)dirties.first();
  }
}

void image::resize(short new_width, short new_height)
{
  int old_width=width(),old_height=height();
  unsigned char *im=(unsigned char *)jmalloc(width()*height(),"image::resized");
  memcpy(im,scan_line(0),width()*height());

  delete_page();
  make_page(new_width,new_height,NULL);
  w=new_width;      // set the new hieght and width
  h=new_height;

  unsigned char *sl1,*sl2;
  short y,y2,x2;
  double yc,xc,yd,xd;



  yc=(double)old_height/(double)new_height;
  xc=(double)old_width/(double)new_width;
  for (y2=0,yd=0;y2<new_height;yd+=yc,y2++)
  {
    y=(int)yd;
    sl1=im+y*old_width;
    sl2=scan_line(y2);
    for (xd=0,x2=0;x2<new_width;xd+=xc,x2++)
    { sl2[x2]=sl1[(int)xd]; }
  }
  jfree(im);
  if (special) special->resize(new_width,new_height);
}

void image::scroll(short x1, short y1, short x2, short y2, short xd, short yd)
{
  short cx1,cy1,cx2,cy2;
  CHECK(x1>=0 && y1>=0 && x1<x2 && y1<y2 && x2<width() && y2<height());
  if (special)
  {
    special->get_clip(cx1,cy1,cx2,cy2);
    x1=max(x1,cx1); y1=max(cy1,y1); x2=min(x2,cx2); y2=min(y2,cy2);
  }
  short xsrc,ysrc,xdst,ydst,xtot=x2-x1-abs(xd)+1,ytot,xt;
  unsigned char *src,*dst;
  if (xd<0) { xsrc=x1-xd; xdst=x1; } else { xsrc=x2-xd; xdst=x2; }
  if (yd<0) { ysrc=y1-yd; ydst=y1; } else { ysrc=y2-yd; ydst=y2; }
  for (ytot=y2-y1-abs(yd)+1;ytot;ytot--)
  { src=scan_line(ysrc)+xsrc;
    dst=scan_line(ydst)+xdst;
    if (xd<0)
      for (xt=xtot;xt;xt--)
        *(dst++)=*(src++);
      else for (xt=xtot;xt;xt--)
        *(dst--)=*(src--);
    if (yd<0) { ysrc++; ydst++; } else { ysrc--; ydst--; }
  }
  add_dirty(x1,y1,x2,y2);
}


image *image::create_smooth(short smoothness)
{
  short i,j,k,l,t,d;
  image *im;
  CHECK(smoothness>=0);
  if (!smoothness) return NULL;
  d=smoothness*2+1;
  d=d*d;
  im=new image(width(),height());
  for (i=0;i<width();i++)
    for (j=0;j<height();j++)
    {
      for (t=0,k=-smoothness;k<=smoothness;k++)
	for (l=-smoothness;l<=smoothness;l++)
	  if (i+k>smoothness && i+k<width()-smoothness && j+l<height()-smoothness && j+l>smoothness)
	    t+=pixel(i+k,j+l);
	  else t+=pixel(i,j);
      im->putpixel(i,j,t/d);
    }
  return im;
}

void image::wiget_bar(short x1, short y1, short x2, short y2, 
   	unsigned char light, unsigned char med, unsigned char dark)
{
  line(x1,y1,x2,y1,light);
  line(x1,y1,x1,y2,light);
  line(x2,y1+1,x2,y2,dark);
  line(x1+1,y2,x2-1,y2,dark);
  bar(x1+1,y1+1,x2-1,y2-1,med);
}

class fill_rec
{
public :
  short x,y;
  fill_rec *last;
  fill_rec(short X, short Y, fill_rec *Last)
  { x=X; y=Y; last=Last; }
} ;

void image::flood_fill(short x, short y, unsigned char color)
{
  unsigned char *sl,*above,*below;
  fill_rec *recs=NULL,*r;
  unsigned char fcolor;
  sl=scan_line(y);
  fcolor=sl[x];
  if (fcolor==color) return ;
  do
  {
    if (recs)
    { r=recs;
      recs=recs->last;
      x=r->x; y=r->y;
      delete r;
    }
    sl=scan_line(y);
    if (sl[x]==fcolor)
    {
      while (sl[x]==fcolor && x>0) x--;
      if (sl[x]!=fcolor) x++;
      if (y>0)
      {
        above=scan_line(y-1);
        if (above[x]==fcolor)
        { r=new fill_rec(x,y-1,recs);
          recs=r;  
        }
      }
      if (y<height()-1)
      {
        above=scan_line(y+1);
        if (above[x]==fcolor)
        { r=new fill_rec(x,y+1,recs);
          recs=r;
        }
      }



      do
      {
        sl[x]=color; 
        if (y>0)
        { above=scan_line(y-1);
          if (x>0 && above[x-1]!=fcolor && above[x]==fcolor)
          { r=new fill_rec(x,y-1,recs);
            recs=r;  
          }
        }
        if (y<height()-1)
        { below=scan_line(y+1);
          if (x>0 && below[x-1]!=fcolor && below[x]==fcolor)
          { r=new fill_rec(x,y+1,recs);
            recs=r;  
          }
        }
        x++;
      } while (sl[x]==fcolor && x<width());
      x--;
      if (y>0)
      {
        above=scan_line(y-1);
        if (above[x]==fcolor)
        { r=new fill_rec(x,y-1,recs);
          recs=r;
        }
      }
      if (y<height()-1)
      {
        above=scan_line(y+1);
        if (above[x]==fcolor)
        { r=new fill_rec(x,y+1,recs);
          recs=r;
        }
      }
    }
  } while (recs);
}


#define LED_L 5
#define LED_H 5
void image::burn_led(short x, short y, long num, short color, short scale)
{
  char st[100];
  short ledx[]={1,2,1,2,3,3,3,3,1,2,0,0,0,0};
  short ledy[]={3,3,0,0,1,2,4,6,7,7,4,6,1,2};

  short dig[]={2+4+8+16+32+64,4+8,2+4+1+32+16,2+4+1+8+16,64+1+4+8,
             2+64+1+8+16,64+32+1+8+16,2+4+8,1+2+4+8+16+32+64,64+2+4+1+8,1};
  short xx,yy,zz;
  sprintf(st,"%8ld",num);
  for (xx=0;xx<8;xx++)
  {
    if (st[xx]!=' ')
    {
      if (st[xx]=='-')
	zz=10;
      else
	zz=st[xx]-'0';
      for (yy=0;yy<7;yy++)
	if ((1<<yy)&dig[zz])
	  line(x+ledx[yy*2]*scale,y+ledy[yy*2]*scale,x+ledx[yy*2+1]*scale,
	    y+ledy[yy*2+1]*scale,color);
    }
    x+=6*scale;
  }
}

unsigned char dither_matrix[]={0,  136,24, 170,
		     68, 204,102,238,
		     51, 187, 17,153,
		     119,255, 85,221};

image *image::copy_part_dithered (short x1, short y1, short x2, short y2)
{
  short x,y,cx1,cy1,cx2,cy2,ry,rx,bo,dity,ditx;
  image *ret;
  unsigned char *sl1,*sl2;
  get_clip(cx1,cy1,cx2,cy2);
  if (y1<cy1) y1=cy1;
  if (x1<cx1) x1=cx1;
  if (y2>cy2) y2=cy2;
  if (x2>cx2) x2=cx2;
  CHECK(x2>=x1 && y2>=y1);
  if (x2<x1 || y2<y1) return NULL;
  ret=new image((x2-x1+8)/8,(y2-y1+1));
  if (!last_loaded())
    ret->clear();
  else 
    for (y=y1,ry=0,dity=(y1%4)*4;y<=y2;y++,ry++)
    {
      sl1=ret->scan_line(ry);     // sl1 is the scan linefo the return image
      sl2=scan_line(y);          // sl2 is the orginal image scan line
      memset(sl1,0,(x2-x1+8)/8);
      for (bo=7,rx=0,x=x1,ditx=x1%4;x<=x2;x++)
      {
        if (last_loaded()->red(sl2[x])>dither_matrix[ditx+dity]) 
    	  sl1[rx]|=1<<bo;
        if (bo!=0)
  	bo--;
        else
        {
  	  rx++;
	  bo=7;
        }
        ditx+=1; if (ditx>3) ditx=0;
      }
      dity+=4; if (dity>12) dity=0;
    }
  return ret;
}

void image::flip_x()
{
  unsigned char *rev=(unsigned char *)jmalloc(width(),"image tmp::flipped_x"),*sl;
  CONDITION(rev,"memory allocation"); 
  int y,x,i;
  for (y=0;y<height();y++)
  { sl=scan_line(y);
    for (i=0,x=width()-1;x>=0;x--,i++)
      rev[i]=sl[x]; 
    memcpy(sl,rev,width());
  }
  jfree(rev);
}

void image::flip_y()
{
  unsigned char *rev=(unsigned char *)jmalloc(width(),"image::flipped_y"),*sl;
  CONDITION(rev,"memory allocation"); 
  int y;
  for (y=0;y<height()/2;y++)
  { sl=scan_line(y);
    memcpy(rev,sl,width());
    memcpy(sl,scan_line(height()-y-1),width());
    memcpy(scan_line(height()-y-1),rev,width());
  }
}

void image::make_color(unsigned char color)
{
  unsigned char *sl;
  int y,x;
  for (y=0;y<height();y++)
  {
    sl=scan_line(y);
    for (x=width();x;x--,sl++)
      if (*sl) 
        *sl=color;
  }
}
