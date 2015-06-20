#include "light.hpp"
#include <stdlib.h>
#include "image.hpp"
#include "macs.hpp"
#include "video.hpp"
#include "palette.hpp"
#include "timing.hpp"
#include "specs.hpp"
#include "dprint.hpp"
#include "filter.hpp"
#include "status.hpp"
#include "dev.hpp"

light_source *first_light_source=NULL;
unsigned char *white_light,*white_light_initial,*green_light,*trans_table;
short ambient_ramp=0;
short shutdown_lighting_value,shutdown_lighting=0;
extern char disable_autolight;   // defined in dev.hpp

int light_detail=MEDIUM_DETAIL;

long light_to_number(light_source *l)
{
  
  if (!l) return 0;
  int x=1;
  for (light_source *s=first_light_source;s;s=s->next,x++)
    if (s==l) return x;
  return 0;
}


light_source *number_to_light(long x)
{
  if (x==0) return NULL;
  x--;
  light_source *s=first_light_source;
  for (;x && s;x--,s=s->next);
  return s;       
}

light_source *light_source::copy()
{
  next=new light_source(type,x,y,inner_radius,outer_radius,xshift,yshift,next);
  return next;
}

void delete_all_lights()
{
  while (first_light_source)
  {
    if (dev_cont)
      dev_cont->notify_deleted_light(first_light_source);

    light_source *p=first_light_source;
    first_light_source=first_light_source->next;
    delete p;
  }
}

void delete_light(light_source *which)
{
  if (dev_cont)
    dev_cont->notify_deleted_light(which);

  if (which==first_light_source)
  {
    first_light_source=first_light_source->next;
    delete which;
  }
  else
  {
    light_source *f=first_light_source;
    for (;f->next!=which && f;f=f->next);
    if (f)
    {
      f->next=which->next;
      delete which;
    }
  }
}

void light_source::calc_range()
{
  switch (type)
  {
    case 0 :
    { x1=x-(outer_radius>>xshift); y1=y-(outer_radius>>yshift);
      x2=x+(outer_radius>>xshift); y2=y+(outer_radius>>yshift);
    } break;
    case 1 :
    { x1=x-(outer_radius>>xshift); y1=y-(outer_radius>>yshift);
      x2=x+(outer_radius>>xshift); y2=y;
    } break;
    case 2 :
    { x1=x-(outer_radius>>xshift); y1=y;
      x2=x+(outer_radius>>xshift); y2=y+(outer_radius>>yshift);
    } break;
    case 3 :
    { x1=x; y1=y-(outer_radius>>yshift);
      x2=x+(outer_radius>>xshift); y2=y+(outer_radius>>yshift);
    } break;
    case 4 :
    { x1=x-(outer_radius>>xshift); y1=y-(outer_radius>>yshift);
      x2=x; y2=y+(outer_radius>>yshift);
    } break;


    case 5 :
    { x1=x; y1=y-(outer_radius>>yshift);
      x2=x+(outer_radius>>xshift); y2=y;
    } break;
    case 6 :
    { x1=x-(outer_radius>>xshift); y1=y-(outer_radius>>yshift);
      x2=x; y2=y;
    } break;
    case 7 :
    { x1=x-(outer_radius>>xshift); y1=y;
      x2=x; y2=y+(outer_radius>>yshift);
    } break;
    case 8 :
    { x1=x; y1=y;
      x2=x+(outer_radius>>xshift); y2=y+(outer_radius>>yshift);
    } break;
    case 9 :
    {
      x1=x;
      y1=y;
      x2=x+xshift;
      y2=y+yshift;
    } break;

  }
  mul_div=(1<<16)/(outer_radius-inner_radius)*64;
}

light_source::light_source(char Type, long X, long Y, long Inner_radius, 
			   long Outer_radius, long Xshift,  long Yshift, light_source *Next)
{ 
  type=Type; 
  x=X; y=Y; 
  inner_radius=Inner_radius;  
  outer_radius=Outer_radius; 
  next=Next; 
  known=0;
  xshift=Xshift;
  yshift=Yshift;
  calc_range(); 
}


int count_lights()
{
  int t=0;
  for (light_source *s=first_light_source;s;s=s->next)
    t++;
  return t;
}

light_source *add_light_source(char type, long x, long y, 
			       long inner, long outer, long xshift, long yshift)
{
  first_light_source=new light_source(type,x,y,inner,outer,xshift,yshift,first_light_source);
  return first_light_source;
}


#define TTINTS 9
uchar *tints[TTINTS];
uchar bright_tint[256];

void calc_tint(uchar *tint, int rs, int gs, int bs, int ra, int ga, int ba, palette *pal)
{
  palette npal;
  memset(npal.addr(),0,256);
  int i=0;
  for (;i<256;i++)
  {
    npal.set(i,(int)rs,(int)gs,(int)bs);
    rs+=ra; if (rs>255) rs=255; if (rs<0) rs=0;
    gs+=ga; if (gs>255) gs=255; if (gs<0) gs=0;
    bs+=ba; if (bs>255) bs=255; if (bs<0) bs=0;
  }
  filter f(pal,&npal);
  filter f2(&npal,pal);

  for (i=0;i<256;i++,tint++)
    *tint=f2.get_mapping(f.get_mapping(i));
}


void calc_light_table(palette *pal)
{
  
  white_light_initial=(unsigned char *)jmalloc(256*64,"light table");
  white_light=white_light_initial;

//  green_light=(unsigned char *)jmalloc(256*64,"green light");
  int i=0;
  for (;i<TTINTS;i++)
      tints[i]=(uchar *)jmalloc(256,"color tint");

  bFILE *fp=open_file("light.tbl","rb");
  int recalc=0;
  if (fp->open_failure()) 
  {
    recalc=1;
    delete fp;
  }
  else
  {
    if (fp->read_short()!=calc_crc((unsigned char *)pal->addr(),768))
      recalc=1;
    else
    {
      fp->read(white_light,256*64);
//      fp->read(green_light,256*64);
      for (i=0;i<TTINTS;i++)
        fp->read(tints[i],256);
      fp->read(bright_tint,256);
//      trans_table=(uchar *)jmalloc(256*256,"transparency table");
//      fp.read(trans_table,256*256);
    }
    delete fp;
  }

  if (recalc)
  {  
    fprintf(stderr,"Palette has changed, recalculating light table...\n");
    stat_man->push("white light",NULL);
    int color=0;
    for (;color<256;color++)
    {
      unsigned char r,g,b;
      pal->get(color,r,g,b);
      stat_man->update(color*100/256);
      for (int intensity=63;intensity>=0;intensity--)
      {
	if (r>0 || g>0 || b>0)
          white_light[intensity*256+color]=pal->find_closest(r,g,b);
	else
          white_light[intensity*256+color]=0;
	if (r) r--;  if (g) g--;  if (b) b--; 	
      } 



    }
    stat_man->pop();

/*    stat_man->push("green light",NULL);
    for (color=0;color<256;color++)
    {
      stat_man->update(color*100/256);
      unsigned char r,g,b;
      pal->get(color,b,r,g);
      r=r*3/5; b=b*3/5; g+=7; if (g>255) g=255;

      for (int intensity=63;intensity>=0;intensity--)
      {
	if (r>0 || g>0 || b>0)
          green_light[intensity*256+color]=pal->find_closest(r,g,b);
	else
          green_light[intensity*256+color]=0;
	if (r) r--;  
	if ((intensity&1)==1)
	  if (g) g--;  
	if (b) b--;
      }
    }
    stat_man->pop(); */

    stat_man->push("tints",NULL);
    uchar t[TTINTS*6]={0,0,0,0,0,0, // normal
                   0,0,0,1,0,0,     // red
		   0,0,0,1,1,0,     // yellow
		   0,0,0,1,0,1,     // purple
		   0,0,0,1,1,1,     // gray
		   0,0,0,0,1,0,     // green
		   0,0,0,0,0,1,     // blue
		   0,0,0,0,1,1,     // cyan 






		   0,0,0,0,0,0   // reverse green  (night vision effect)
		 } ;
    uchar *ti=t+6;
    uchar *c;
    for (i=0,c=tints[0];i<256;i++,c++) *c=i;  // make the normal tint (maps everthing to itself)
    for (i=0,c=tints[TTINTS-1];i<256;i++,c++)  // reverse green
    {
      int r=pal->red(i)/2,g=255-pal->green(i)-30,b=pal->blue(i)*3/5+50;
      if (g<0) g=0;
      if (b>255) b=0;
      *c=pal->find_closest(r,g,b);
    }
    for (i=0;i<256;i++)
    {
      int r=pal->red(i)+(255-pal->red(i))/2,
          g=pal->green(i)+(255-pal->green(i))/2,
          b=pal->blue(i)+(255-pal->blue(i))/2;
      bright_tint[i]=pal->find_closest(r,g,b);
    }

    // make the colored tints
    for (i=1;i<TTINTS-1;i++)
    {
      stat_man->update(i*100/(TTINTS-1));
      calc_tint(tints[i],ti[0],ti[1],ti[2],ti[3],ti[4],ti[5],pal);
      ti+=6;
    }
    stat_man->pop();
/*    fprintf(stderr,"calculating transparency tables (256 total)\n");
    trans_table=(uchar *)jmalloc(256*256,"transparency table");

    uchar *tp=trans_table;
    for (i=0;i<256;i++)
    {      
      uchar r1,g1,b1,r2,g2,b2;
      pal->get(i,r1,g1,b1);
      if ((i%16)==0)
        fprintf(stderr,"%d ",i);
      for (int j=0;j<256;j++,tp++)
      {
	if (r1==0 && r2==0 && b2==0)
	  *tp=j;
	else
	{
	  pal->get(j,r2,g2,b2);       
	  *tp=pal->find_closest((r2-r1)*3/7+r1,(g2-g1)*3/7+g1,(b2-b1)*3/7+b1);
	}
      }
    }*/


    bFILE *f=open_file("light.tbl","wb");
    if (f->open_failure())    
      dprintf("Unable to open file light.tbl for writing\n");
    else
    {
      f->write_short(calc_crc((unsigned char *)pal->addr(),768));
      f->write(white_light,256*64);
//      f->write(green_light,256*64);
      for (int i=0;i<TTINTS;i++)
        f->write(tints[i],256);
      fp->write(bright_tint,256);
//    f.write(trans_table,256*256);
    }
    delete f;
  }
}


light_patch *light_patch::copy(light_patch *Next)
{ 
  light_patch *p=new light_patch(x1,y1,x2,y2,Next); 
  p->total=total;
  if (total)    
  {
    p->lights=(light_source **)jmalloc(total*sizeof(light_source *),"light patches");
    memcpy(p->lights,lights,total*(sizeof(light_source *)));
  }
  else 
    p->lights=NULL; 
  return p;
}

#define MAX_LP 6

// insert light into list make sure the are sorted by y1
void insert_light(light_patch *&first, light_patch *l)
{
  if (!first)
    first=l;
  else if (l->y1<first->y1)
  {
    l->next=first;
    first=l;
  } else
  {
    light_patch *p=first;
    for (;p->next && p->next->y1<l->y1;p=p->next);
    l->next=p->next;
    p->next=l;
  }
}

void add_light(light_patch *&first, long x1, long y1, long x2, long y2, 
			    light_source *who)
{  
  light_patch *last=NULL,*next;
  light_patch *p=first;
  for (;p;p=next)
  {
    next=p->next;
    // first see if light patch we are adding is enclosed entirely by another patch
    if (x1>=p->x1 && y1>=p->y1 && x2<=p->x2 && y2<=p->y2)
    {
      if (p->total==MAX_LP) return ;

      if (x1>p->x1)
      {
	light_patch *l=p->copy(NULL);
	l->x2=x1-1;
	insert_light(first,l);
      }
      if (x2<p->x2)
      {
	light_patch *l=p->copy(NULL);
	l->x1=x2+1;
	insert_light(first,l);
      }
      if (y1>p->y1)
      {
	light_patch *l=p->copy(NULL);
	l->x1=x1;
	l->x2=x2;
	l->y2=y1-1;
	insert_light(first,l);
      }
      if (y2<p->y2)
      {
	light_patch *l=p->copy(NULL);
	l->x1=x1;
	l->x2=x2;
	l->y1=y2+1;
	insert_light(first,l);
      }
      p->x1=x1; p->y1=y1; p->x2=x2; p->y2=y2;
      // p has possibly changed it's y1, so we need to move it to it's correct sorted 
      // spot in the list
      if (first==p)
        first=first->next;
      else
      {
	light_patch *q=first;
	for (;q->next!=p;q=q->next);
	q->next=p->next;
      }
      insert_light(first,p);
      

      p->total++;     
      p->lights=(light_source **)jrealloc(p->lights,sizeof(light_source *)*p->total,"patch_list");
      p->lights[p->total-1]=who;
      return ;
    }

    // see if the patch completly covers another patch.
    if (x1<=p->x1 && y1<=p->y1 && x2>=p->x2 && y2>=p->y2)
    {
      if (x1<p->x1)
        add_light(first,x1,y1,p->x1-1,y2,who);
      if (x2>p->x2)
        add_light(first,p->x2+1,y1,x2,y2,who);
      if (y1<p->y1)
        add_light(first,p->x1,y1,p->x2,p->y1-1,who);
      if (y2>p->y2)
        add_light(first,p->x1,p->y2+1,p->x2,y2,who);
      if (p->total==MAX_LP)  return ;
      p->total++;     
      p->lights=(light_source **)jrealloc(p->lights,sizeof(light_source *)*p->total,"patch_list");
      p->lights[p->total-1]=who;
      return ;
    }

    // see if we intersect another rect
    if (!(x2<p->x1 || y2<p->y1 || x1>p->x2 || y1>p->y2))  
    {
      int ax1,ay1,ax2,ay2;
      if (x1<p->x1)
      {
        add_light(first,x1,max(y1,p->y1),p->x1-1,min(y2,p->y2),who);
	ax1=p->x1;
      } else
	ax1=x1;

      if (x2>p->x2)
      {
        add_light(first,p->x2+1,max(y1,p->y1),x2,min(y2,p->y2),who);
	ax2=p->x2;
      } 
      else
	ax2=x2;

      if (y1<p->y1)
      {       
        add_light(first,x1,y1,x2,p->y1-1,who);
	ay1=p->y1;
      } else 
	ay1=y1;

      if (y2>p->y2)
      {
        add_light(first,x1,p->y2+1,x2,y2,who);
	ay2=p->y2;
      } else 
	ay2=y2;

       
      add_light(first,ax1,ay1,ax2,ay2,who);      

      return ;    
    }
  }
}
 
light_patch *find_patch(int screenx, int screeny, light_patch *list)
{
  for (;list;list=list->next)
  {
    if (screenx>=list->x1 && screenx<=list->x2 && screeny>=list->y1 && screeny<=list->y2)
      return list;
  }
  return NULL;
}

/* shit
int calc_light_value(light_patch *which, long x, long y)
{
  int lv=0;
  int t=which->total;
  for (register int i=t-1;i>=0;i--)
  {    
    light_source *fn=which->lights[i];
    if (fn->type==9)
    {
      lv=fn->inner_radius;
      i=0;
    }
    else
    {
      long dx=abs(fn->x-x)<<fn->xshift;
      long dy=abs(fn->y-y)<<fn->yshift;
      long  r2;
      if (dx<dy)
        r2=dx+dy-(dx>>1);
      else r2=dx+dy-(dy>>1);
    
      if (r2>=fn->inner_radius)
      {
	if (r2<fn->outer_radius)
	{
	  lv+=((fn->outer_radius-r2)*fn->mul_div)>>16;
	}
      } else lv=63;	  
    }
  }
  if (lv>63) return 63; 
  else
    return lv;
} */


void reduce_patches(light_patch *f)   // find constant valued patches
{
  
}

light_patch *make_patch_list(int width, int height, long screenx, long screeny)
{
  light_patch *first=new light_patch(0,0,width-1,height-1,NULL);

  for (light_source *f=first_light_source;f;f=f->next)   // determine which lights will have effect
  {
    long x1=f->x1-screenx,y1=f->y1-screeny,
        x2=f->x2-screenx,y2=f->y2-screeny;
    if (x1<0) x1=0;
    if (y1<0) y1=0;
    if (x2>=width)  x2=width-1;
    if (y2>=height) y2=height-1;

    if (x1<=x2 && y1<=y2)
      add_light(first,x1,y1,x2,y2,f);
  }
  reduce_patches(first);

  return first;
}


void delete_patch_list(light_patch *first)
{
  while (first)
  {
    light_patch *p=first;
    first=first->next;
    delete p;
  }
}

/*
#ifdef __WATCOMC__
extern "C" {
extern long MAP_PUT(long pad, long screen_addr, long remap, long w);
} ;
#else*/

inline void MAP_PUT(long screen_addr, long remap, long w)
{ 
  register int cx=w;
  register int di=screen_addr;
  register int si=remap;
  while (cx--) 
    *((uchar *)(di++))=*((uchar *)si+*((uchar *)di)); 
}

inline void MAP_2PUT(long in_addr, long out_addr, long remap, long w)
{ 
  while (w--) 
  {
    uchar x=*(((uchar *)remap)+(*(uchar *)(in_addr++)));
    *((uchar *)(out_addr++))=x;
    *((uchar *)(out_addr++))=x;
  }
}

/*
#endif

inline void PUT8(long *addr, uchar *remap)
{
  register ulong in_pixels;
  register ulong pixel;
  register ulong out_pixels;
  in_pixels=*addr;
  pixel=in_pixels;
  out_pixels=remap[(uchar)pixel];
  
  pixel=in_pixels;
  pixel>>=8;
  pixel=remap[(uchar)pixel];
  pixel<<=8;
  out_pixels|=pixel;

  pixel=in_pixels;
  pixel>>=16;
  pixel=remap[(uchar)pixel];
  pixel<<=16;
  out_pixels|=pixel;

  pixel=in_pixels;
  pixel>>=24;
  pixel=remap[(uchar)pixel];
  pixel<<=24;
  out_pixels|=pixel;

  *addr=out_pixels;        // send out bus

  // do next 4
  in_pixels=addr[1];

  pixel=in_pixels;
  pixel&=0xff;
  out_pixels=remap[pixel];
  
  pixel=in_pixels;
  pixel>>=8;
  pixel=remap[(uchar)pixel];
  pixel<<=8;
  out_pixels|=pixel;

  pixel=in_pixels;
  pixel>>=16;
  pixel=remap[(uchar)pixel];
  pixel<<=16;
  out_pixels|=pixel;

  pixel=in_pixels;
  pixel>>=24;
  pixel=remap[(uchar)pixel];
  pixel<<=24;
  out_pixels|=pixel;  
  addr[1]=out_pixels;        // send out bus
  
}

inline long MAP_PUT2(long dest_addr, long screen_addr, long remap, long w)
{ while (w--) 
  { 
    *((uchar *)(dest_addr))=*((uchar *)remap+*((uchar *)screen_addr)); 
    screen_addr++; 
    dest_addr++;
  } 
  return dest_addr;
}

*/

ushort min_light_level;
// calculate the light value for this block.  sum up all contritors
inline int calc_light_value(light_patch *lp,   // light patch to look at
			    long sx,           // screen x & y
			    long sy)
{
  int lv=min_light_level,r2,light_count;
  register int dx,dy;           // x and y distances

  light_source **lon_p=lp->lights;

  for (light_count=lp->total;light_count>0;light_count--)
  {
    light_source *fn=*lon_p;
    register long *dt=&(*lon_p)->type; 
                                     // note we are accessing structure members by bypassing the compiler
                                     // for speed, this may not work on all compilers, but don't
                                     // see why it shouldn't..  all members are long
    
    if (*dt==9)                      // (dt==type),  if light is a Solid rectangle, return it value
      return fn->inner_radius;
    else
    {
      dt++;
      dx=abs(*dt-sx); dt++;               // xdist between light and this block  (dt==x)
      dx<<=*dt;  dt++;                    // shift makes distance further, 
                                          // making light skinner. (dt==xshift)

      dy=abs(*dt-sy); dt++;                   // ydist (dt==y)
      dy<<=*dt;  dt++;                        // (dt==yshift)

      if (dx<dy)                     // calculate approximate distance
        r2=dx+dy-(dx>>1);
      else r2=dx+dy-(dy>>1);
      
      if (r2<*dt)                    // if this withing the light's outer radius?  (dt==outer_radius)
      {		
	int v=*dt-r2; dt++;		
	lv+=v*(*dt)>>16;
      }
    }
    lon_p++;
  }

  if (lv>63) 
    return 63;          // lighting table only has 64 (256 bytes) entries 
  else return lv;
}


/*#ifdef __WATCOMC__

extern "C" void remap_line_asm(uchar *screen_line,uchar *light_lookup,uchar *remap_line,int count);

#else */

void remap_line_asm2(uchar *addr,uchar *light_lookup,uchar *remap_line,int count)
//inline void remap_line_asm2(uchar *addr,uchar *light_lookup,uchar *remap_line,int count)
{
  while (count--)
  {
    uchar *off=light_lookup+(((long)*remap_line)<<8); 
    remap_line++;

    *addr=off[*addr]; 
    addr[1]=off[addr[1]];
    addr[2]=off[addr[2]];
    addr[3]=off[addr[3]];
    addr[4]=off[addr[4]];
    addr[5]=off[addr[5]];
    addr[6]=off[addr[6]];
    addr[7]=off[addr[7]];
    addr+=8;

  }
}

//#endif


inline void put_8line(uchar *in_line, uchar *out_line, uchar *remap, uchar *light_lookup, int count)
{
  uchar v;
  int x;
  for (x=0;x<count;x++)                        
  {                                            
    uchar *off=light_lookup+(((long)*remap)<<8);

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    remap++;
  }
}


void light_screen(image *sc, long screenx, long screeny, uchar *light_lookup, ushort ambient)
{
  int lx_run,ly_run;                     // light block x & y run size in pixels ==  (1<<lx_run)

  if (shutdown_lighting && !disable_autolight)
    ambient=shutdown_lighting_value;

  switch (light_detail) 
  {
    case HIGH_DETAIL : 
    { lx_run=2; ly_run=1; } break;       // 4 x 2 patches
    case MEDIUM_DETAIL :
    { lx_run=3; ly_run=2; } break;       // 8 x 4 patches  (default)
    case LOW_DETAIL :
    { lx_run=4; ly_run=3; } break;       // 16 x 8 patches
    case POOR_DETAIL :                   // poor detail is no lighting
    return ;
  }
  if ((int)ambient+ambient_ramp<0)
    min_light_level=0;
  else if ((int)ambient+ambient_ramp>63)
    min_light_level=63;
  else min_light_level=(int)ambient+ambient_ramp;

  if (ambient==63) return ;
  short cx1,cy1,cx2,cy2;
  sc->get_clip(cx1,cy1,cx2,cy2);

  unsigned char *mint=light_lookup+min_light_level*256;

  light_patch *first=make_patch_list(cx2-cx1+1,cy2-cy1+1,screenx,screeny);


  int ytry=(1<<ly_run),xtry=(1<<lx_run);
  int calcx_mask=(0xefffffff-(xtry-1));
  int calcy_mask=(0xefffffff-(ytry-1));
  int scr_w=screen->width();


  int prefix_x=(screenx&7);
  int prefix=screenx&7;
  if (prefix)
    prefix=8-prefix;
  int suffix_x=cx2-cx1-(screenx&7);

  int inside_xoff=(screenx+7)&(~7);

  int suffix=(cx2-cx1-prefix+1)&7;


  long remap_size=((cx2-cx1+1-prefix-suffix)>>lx_run);

  uchar *remap_line=(uchar *)jmalloc(remap_size,"light remap line");

  light_patch *f=first;
  uchar *screen_line=screen->scan_line(cy1)+cx1;


  for (int y=cy1;y<=cy2;)
  {
    int x,count;
//    while (f->next && f->y2<y) 
//      f=f->next;
    uchar *rem=remap_line;

    int todoy=4-((screeny+y)&3);
    if (y+todoy>cy2)
      todoy=cy2-y+1;

    int calcy=((y+screeny)&(~3))-cy1;
    

    if (suffix)
    {
      light_patch *lp=f;
      for (;(lp->y1>y-cy1 || lp->y2<y-cy1 || 
			      lp->x1>suffix_x || lp->x2<suffix_x);lp=lp->next);
      long caddr=(long)screen_line+cx2-cx1+1-suffix;
      uchar *r=light_lookup+(((long)calc_light_value(lp,suffix_x+screenx,calcy)<<8));
      switch (todoy)
      {
	case 4 :
	{ 
	  MAP_PUT(caddr,(long)r,suffix); caddr+=scr_w;
	}
	case 3 :
	{ MAP_PUT(caddr,(long)r,suffix); caddr+=scr_w;}
	case 2 :
	{ MAP_PUT(caddr,(long)r,suffix); caddr+=scr_w;}
	case 1 :
	{ 
	  MAP_PUT(caddr,(long)r,suffix);
	}
      }
    }

    if (prefix)
    {
      light_patch *lp=f;
      for (;(lp->y1>y-cy1 || lp->y2<y-cy1 || 
			      lp->x1>prefix_x || lp->x2<prefix_x);lp=lp->next);

      uchar *r=light_lookup+(((long)calc_light_value(lp,prefix_x+screenx,calcy)<<8));
      long caddr=(long)screen_line;
      switch (todoy)
      {
	case 4 :
	{ 
	  MAP_PUT(caddr,(long)r,prefix); 
	  caddr+=scr_w; 
	}
	case 3 :
	{ MAP_PUT(caddr,(long)r,prefix); caddr+=scr_w; }
	case 2 :
	{ MAP_PUT(caddr,(long)r,prefix); caddr+=scr_w; }
	case 1 :
	{ MAP_PUT(caddr,(long)r,prefix); }
      }
      screen_line+=prefix;
    }


 

    for (x=prefix,count=0;count<remap_size;count++,x+=8,rem++)    
    {
      light_patch *lp=f;
      for (;(lp->y1>y-cy1 || lp->y2<y-cy1 || lp->x1>x || lp->x2<x);lp=lp->next);
      *rem=calc_light_value(lp,x+screenx,calcy);
    }
    uchar *addr,*oaddr;


    switch (todoy)
    {
      case 4 :    
      remap_line_asm2(screen_line,light_lookup,remap_line,count);  y++; todoy--;  screen_line+=scr_w;
      case 3 :
      remap_line_asm2(screen_line,light_lookup,remap_line,count);  y++; todoy--;  screen_line+=scr_w;
      case 2 :
      remap_line_asm2(screen_line,light_lookup,remap_line,count);  y++; todoy--;  screen_line+=scr_w;
      case 1 :
      remap_line_asm2(screen_line,light_lookup,remap_line,count);  y++; todoy--;  screen_line+=scr_w;
    } 


    screen_line-=prefix;



  }


  while (first)
  {
    light_patch *p=first;
    first=first->next;
    delete p;
  }
  jfree(remap_line);
}


void double_light_screen(image *sc, long screenx, long screeny, uchar *light_lookup, ushort ambient,
			 image *out, long out_x, long out_y)
{
  if (sc->width()*2+out_x>out->width() ||
      sc->height()*2+out_y>out->height()) 
    return ;   // screen was resized and small_render has not changed size yet


  int lx_run,ly_run;                     // light block x & y run size in pixels ==  (1<<lx_run)
  switch (light_detail) 
  {
    case HIGH_DETAIL : 
    { lx_run=2; ly_run=1; } break;       // 4 x 2 patches
    case MEDIUM_DETAIL :
    { lx_run=3; ly_run=2; } break;       // 8 x 4 patches  (default)
    case LOW_DETAIL :
    { lx_run=4; ly_run=3; } break;       // 16 x 8 patches
    case POOR_DETAIL :                   // poor detail is no lighting
    return ;
  }
  if ((int)ambient+ambient_ramp<0)
    min_light_level=0;
  else if ((int)ambient+ambient_ramp>63)
    min_light_level=63;
  else min_light_level=(int)ambient+ambient_ramp;

  short cx1,cy1,cx2,cy2;
  sc->get_clip(cx1,cy1,cx2,cy2);


  if (ambient==63)      // lights off, just double the pixels
  {
    uchar *src=sc->scan_line(0);
    uchar *dst=out->scan_line(out_y+cy1*2)+cx1*2+out_x;
    int d_skip=out->width()-sc->width()*2;
    int x,y;
    ushort v;
    for (y=sc->height();y;y--)
    {
      for (x=sc->width();x;x--)
      {
	v=*(src++);
	*(dst++)=v;
	*(dst++)=v;
      }
      dst=dst+d_skip;
      memcpy(dst,dst-out->width(),sc->width()*2);
      dst+=out->width();
    }

    return ;
  }


  unsigned char *mint=light_lookup+min_light_level*256;

  light_patch *first=make_patch_list(cx2-cx1+1,cy2-cy1+1,screenx,screeny);


  int ytry=(1<<ly_run),xtry=(1<<lx_run);
  int calcx_mask=(0xefffffff-(xtry-1));
  int calcy_mask=(0xefffffff-(ytry-1));
  int scr_w=sc->width();
  int dscr_w=out->width();

  int prefix_x=(screenx&7);
  int prefix=screenx&7;
  if (prefix)
    prefix=8-prefix;
  int suffix_x=cx2-cx1-(screenx&7);

  int inside_xoff=(screenx+7)&(~7);

  int suffix=(cx2-cx1-prefix+1)&7;


  long remap_size=((cx2-cx1+1-prefix-suffix)>>lx_run);

  uchar *remap_line=(uchar *)jmalloc(remap_size,"light remap line");












  light_patch *f=first;
  uchar *in_line=sc->scan_line(cy1)+cx1;
  uchar *out_line=out->scan_line(cy1*2+out_y)+cx1*2+out_x;


  for (int y=cy1;y<=cy2;)
  {
    int x,count;
//    while (f->next && f->y2<y) 
//      f=f->next;
    uchar *rem=remap_line;

    int todoy=4-((screeny+y)&3);
    if (y+todoy>cy2)
      todoy=cy2-y+1;

    int calcy=((y+screeny)&(~3))-cy1;
    

    if (suffix)
    {
      light_patch *lp=f;
      for (;(lp->y1>y-cy1 || lp->y2<y-cy1 || 
			      lp->x1>suffix_x || lp->x2<suffix_x);lp=lp->next);
      long caddr=(long)in_line+cx2-cx1+1-suffix;
      long daddr=(long)out_line+(cx2-cx1+1-suffix)*2;

      uchar *r=light_lookup+(((long)calc_light_value(lp,suffix_x+screenx,calcy)<<8));
      switch (todoy)
      {
	case 4 : 
	{ 
	  MAP_2PUT(caddr,daddr,(long)r,suffix); daddr+=dscr_w;
	  MAP_2PUT(caddr,daddr,(long)r,suffix); daddr+=dscr_w; caddr+=scr_w;
	}
	case 3 :
	{ 
	  MAP_2PUT(caddr,daddr,(long)r,suffix); daddr+=dscr_w;
	  MAP_2PUT(caddr,daddr,(long)r,suffix); daddr+=dscr_w; caddr+=scr_w;
	}
	case 2 :
	{ 
	  MAP_2PUT(caddr,daddr,(long)r,suffix); daddr+=dscr_w;
	  MAP_2PUT(caddr,daddr,(long)r,suffix); daddr+=dscr_w; caddr+=scr_w;
	}
	case 1 :
	{ 
	  MAP_2PUT(caddr,daddr,(long)r,suffix); daddr+=dscr_w;
	  MAP_2PUT(caddr,daddr,(long)r,suffix); daddr+=dscr_w; caddr+=scr_w;
	} break;
      }
    }

    if (prefix)
    {
      light_patch *lp=f;
      for (;(lp->y1>y-cy1 || lp->y2<y-cy1 || 
			      lp->x1>prefix_x || lp->x2<prefix_x);lp=lp->next);

      uchar *r=light_lookup+(((long)calc_light_value(lp,prefix_x+screenx,calcy)<<8));
      long caddr=(long)in_line;
      long daddr=(long)out_line;
      switch (todoy)
      {
	case 4 :
	{ 
	  MAP_2PUT(caddr,daddr,(long)r,prefix); daddr+=dscr_w;
	  MAP_2PUT(caddr,daddr,(long)r,prefix); daddr+=dscr_w; caddr+=scr_w;
	}
	case 3 :
	{ 
	  MAP_2PUT(caddr,daddr,(long)r,prefix); daddr+=dscr_w;
	  MAP_2PUT(caddr,daddr,(long)r,prefix); daddr+=dscr_w; caddr+=scr_w;
	}
	case 2 :
	{ 
	  MAP_2PUT(caddr,daddr,(long)r,prefix); daddr+=dscr_w;
	  MAP_2PUT(caddr,daddr,(long)r,prefix); daddr+=dscr_w; caddr+=scr_w;
	}
	case 1 :
	{ 
	  MAP_2PUT(caddr,daddr,(long)r,prefix); daddr+=dscr_w;
	  MAP_2PUT(caddr,daddr,(long)r,prefix); daddr+=dscr_w; caddr+=scr_w;
	} break;
      }
      in_line+=prefix;
      out_line+=prefix*2;
    }


 

    for (x=prefix,count=0;count<remap_size;count++,x+=8,rem++)    
    {
      light_patch *lp=f;
      for (;(lp->y1>y-cy1 || lp->y2<y-cy1 || lp->x1>x || lp->x2<x);lp=lp->next);
      *rem=calc_light_value(lp,x+screenx,calcy);
    }

    rem=remap_line;

    put_8line(in_line,out_line,rem,light_lookup,count); 
    memcpy(out_line+dscr_w,out_line,count*16);
    out_line+=dscr_w;
    in_line+=scr_w; out_line+=dscr_w; y++; todoy--;
    if (todoy)
    {
      put_8line(in_line,out_line,rem,light_lookup,count); 
      memcpy(out_line+dscr_w,out_line,count*16);
      out_line+=dscr_w;
      in_line+=scr_w; out_line+=dscr_w; y++; todoy--;
      if (todoy)
      {
	put_8line(in_line,out_line,rem,light_lookup,count); 
	memcpy(out_line+dscr_w,out_line,count*16);
	out_line+=dscr_w;
	in_line+=scr_w; out_line+=dscr_w; y++; todoy--;
	if (todoy)
	{
	  put_8line(in_line,out_line,rem,light_lookup,count); 
	  memcpy(out_line+dscr_w,out_line,count*16);
	  out_line+=dscr_w;
	  in_line+=scr_w; out_line+=dscr_w; y++;
	}
      }
    }      
    in_line-=prefix;
    out_line-=prefix*2;
  } 


  while (first)
  {
    light_patch *p=first;
    first=first->next;
    delete p;
  }
  jfree(remap_line);
}




void add_light_spec(spec_directory *sd, char *level_name)
{
  long size=4+4;  // number of lights and minimum light levels
  for (light_source *f=first_light_source;f;f=f->next)
    size+=6*4+1;
  sd->add_by_hand(new spec_entry(SPEC_LIGHT_LIST,"lights",NULL,size,0));  
}

void write_lights(bFILE *fp)
{
  int t=0;
  light_source *f=first_light_source;
  for (;f;f=f->next) t++;
  fp->write_long(t);
  fp->write_long(min_light_level);
  for (f=first_light_source;f;f=f->next)
  {
    fp->write_long(f->x);
    fp->write_long(f->y);
    fp->write_long(f->xshift);
    fp->write_long(f->yshift);
    fp->write_long(f->inner_radius);
    fp->write_long(f->outer_radius);
    fp->write_byte(f->type);
  }
}


void read_lights(spec_directory *sd, bFILE *fp, char *level_name)
{
  delete_all_lights();
  spec_entry *se=sd->find("lights");
  if (se)
  {
    fp->seek(se->offset,SEEK_SET);
    long t=fp->read_long();
    min_light_level=fp->read_long();
    light_source *last;
    while (t)
    {
      t--;
      long x=fp->read_long();
      long y=fp->read_long();
      long xshift=fp->read_long();
      long yshift=fp->read_long();
      long ir=fp->read_long();
      long ora=fp->read_long();
      long ty=fp->read_byte();

      light_source *p=new light_source(ty,x,y,ir,ora,xshift,yshift,NULL);
      
      if (first_light_source)
        last->next=p;
      else first_light_source=p;
      last=p;
    }
  }
}






