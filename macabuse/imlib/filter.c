#include "image.hpp"
#include "macs.hpp"
#include "filter.hpp"

extern unsigned char current_background;

filter::filter(palette *from, palette *to)   // creates a conversion filter from one palette to another
{
  nc=from->pal_size() > to->pal_size() ? from->pal_size() : to->pal_size();
  unsigned char *p=fdat=(unsigned char *)jmalloc(nc,"conversion filter");
  unsigned char *r,*g,*b;
  r=g=b=(unsigned char *)from->addr();
  g++;
  b+=2;

  int dk=to->darkest(1);
  for (int i=0;i<nc;i++,p++,r+=3,g+=3,b+=3)
  {
    *p=to->find_closest(*r,*g,*b);

    // make sure non-blacks don't get remapped to the transparency
    if ((*r!=0 || *g!=0 || *b!=0) && (to->red(*p)==0 && to->green(*p)==0 && to->blue(*p)==0))
      *p=dk;
  }

}

void filter::clear()
{
  int i;
  for (i=0;i<nc;i++)
    fdat[i]=i;
}

void filter::max_threshold(int minv, char blank)
{
  int i;
  CONDITION(minv>=0 && minv<nc,"Bad minv");
  for (i=0;i<minv;i++)
    fdat[i]=blank;
}

void filter::min_threshold(int maxv, char blank)
{
  int i;
  CONDITION(maxv>=0 && maxv<nc,"bad maxv value in filter::max_thresh");
  for (i=nc-1;i>=maxv;i--)
    fdat[i]=(unsigned) blank;
}


void filter::set(int color_num, char change_to)
{
  CONDITION(color_num>=0 && color_num<nc,"Bad colors_num");
  fdat[color_num]=(unsigned) change_to;
}


filter::filter(int colors)
{
  CONDITION(colors>=0 && colors<=256,"bad colors value");
  nc=colors;
  make_block(nc);
  fdat=(unsigned char *)jmalloc(nc,"filter");
  clear();
}

void filter::apply(image *im)
{
  int x,y;
  unsigned char *c;
  CONDITION(im,"null image passed in filter::apply\n");
  for (y=im->height()-1;y>=0;y--)
  {
    c=im->scan_line(y);
    for (x=im->width()-1;x>=0;x--)
    {
      CONDITION((unsigned) c[x]<nc,"not enough filter colors");
      c[x]=fdat[(unsigned) c[x]];
    }
  }
}


palette *compare_pal;

int color_compare(void *c1, void *c2)
{
  long v1,v2;  
  unsigned char r1,g1,b1,r2,g2,b2;
  compare_pal->get(  *((unsigned char *)c1),r1,g1,b1);
  compare_pal->get(  *((unsigned char *)c2),r2,g2,b2);
  v1=(int)r1*(int)r1+(int)g1*(int)g1+(int)b1*(int)b1;
  v2=(int)r2*(int)r2+(int)g2*(int)g2+(int)b2*(int)b2;
  if (v1<v2) return -1;
  else if (v1>v2) return 1;
  else return 0;     
}


/*color_filter::color_filter(palette *pal, int color_bits)
{
  unsigned char map[256],*last_start,*start;
  int i,last_color=0,color;
  compare_pal=pal;  
  for (i=0;i,256;i++)
    map[i]=i;

  qsort(map,1,1,color_compare); 
  colors=1<<color_bits;
  last_start=color_table=(unsigned char *)malloc(colors*colors*colors);

  
 
  last_color=map[0];
  last_dist=0;
  
  
  for (i=1;i<colors;i++)
  {   
    color=map[i<<(8-color_bits)];
    dist=       

    memset(c,
  }
  
  
}*/


color_filter::color_filter(palette *pal, int color_bits, void (*stat_fun)(int))
{ 
  color_bits=5;      // hard code 5 for now
  int r,g,b,rv,gv,bv,
      c,i,max=pal->pal_size(),
      lshift=8-color_bits;
  unsigned char *pp;
  
  long dist_sqr,best;
  int colors=1<<color_bits;
  color_table=(unsigned char *)jmalloc(colors*colors*colors,"color_filter");
  for (r=0;r<colors;r++)
  {
    if (stat_fun) stat_fun(r);
    rv=r<<lshift;    
    for (g=0;g<colors;g++)
    {
      gv=g<<lshift;      
      for (b=0;b<colors;b++)
      {
	bv=b<<lshift;      
        best=0x7fffffff;
        for (i=0,pp=(unsigned char *)pal->addr();i<max;i++)
        { 
          register long rd=*(pp++)-rv,
                        gd=*(pp++)-gv,
                        bd=*(pp++)-bv;
	  
          dist_sqr=(long)rd*rd+(long)bd*bd+(long)gd*gd;
          if (dist_sqr<best)
          { best=dist_sqr;
            c=i;
          }
        }
        color_table[r*colors*colors+g*colors+b]=c;
      }
    }
  }  
}

color_filter::color_filter(spec_entry *e, bFILE *fp)
{
  fp->seek(e->offset,0);
  fp->read_short();
  int colors=32;
  color_table=(unsigned char *)jmalloc(colors*colors*colors,"color_filter : loaded");
  fp->read(color_table,colors*colors*colors);
}

int color_filter::size()
{
  int colors=32;
  return 2+colors*colors*colors;
}

int color_filter::write(bFILE *fp)
{
  int colors=32;
  fp->write_short(colors);
  return fp->write(color_table,colors*colors*colors)==colors*colors*colors;
}


void filter::put_image(image *screen, image *im, short x, short y, char transparent)
{
  short cx1,cy1,cx2,cy2,x1=0,y1=0,x2=im->width()-1,y2=im->height()-1;
  screen->get_clip(cx1,cy1,cx2,cy2);
  
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

  


  int xl=x2-x1+1;
  int yl=y2-y1+1;

  screen->add_dirty(x,y,x+xl-1,y+yl-1);

  uchar *pg1=screen->scan_line(y),*source,*dest;
  uchar *pg2=im->scan_line(y1);
  int i;
  for (int j=0;j<yl;j++)        
  {
    for (i=0,source=&pg2[x1],dest=&pg1[x];i<xl;i++,source++,dest++)	
      if (!transparent || *source!=current_background) 
        *dest=fdat[*source];
    pg1=screen->next_line(y+j,pg1);  
    pg2=im->next_line(y1+j,pg2);
  }

}





