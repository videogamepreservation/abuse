#include "menu.hpp"
#include "lisp.hpp"
#include "game.hpp"
#include "timing.hpp"
#include "game.hpp"
#include "id.hpp"
#include "pmenu.hpp"
#include "gui.hpp"
#include "property.hpp"
#include "dev.hpp"
#include "clisp.hpp"
#include "dprint.hpp"
#include "jrand.hpp"
#include "director.hpp"
#include <math.h>
#include "lisp_gc.hpp"

extern palette *old_pal;

struct mask_line
{
  int x,size;
  ushort *remap;
  uchar *light;
} ;


int text_draw(int y, int x1, int y1, int x2, int y2, char *buf, JCFont *font, uchar *cmap, char color);

mask_line *make_mask_lines(image *mask, int map_width)
{
  mask_line *p=(mask_line *)jmalloc(mask->height()*sizeof(mask_line),"mask_line");
  for (int y=0;y<mask->height();y++)
  {
    // find the start of the run..
    uchar *sl=mask->scan_line(y);    
    int x=0;
    while (*sl==0) { sl++; x++; }
    p[y].x=x;

   
    // find the length of the run
    int size=0; 
    uchar *sl_start=sl;
    while (*sl!=0 && x<mask->width()) { sl++; x++; size++; }
    p[y].size=size;

    // now calculate remap for line
    p[y].remap=(ushort *)jmalloc(size*2,"mask remap");
    p[y].light=(uchar *)jmalloc(size,"mask light");
    ushort *rem=p[y].remap;
    uchar *lrem=p[y].light;
    for (x=0;x<size;x++,rem++)
    {
      *(lrem++)=*(sl_start++);
/*      if (x==size/2 || x==size/2-1 || x==size/2+1)
        *rem=(int)(sqrt(0.5)*map_width/2.0);
      else*/ 
      if (x<=size/2)
        *rem=(int)(sqrt(x/(double)(size*2.0))*map_width/2.0);
      else
        *rem=map_width/2-(int)(sqrt((size-x)/(double)(size*2.0))*map_width/2.0);

//      (int)(mask->width()-(sqrt((size-x)/(double)size)*map_width/2.0)+mask->width()/2);
    }
  }
  return p;
}


void scan_map(image *screen, int sx, int sy, image *im1, image *im2, int fade256, long *paddr, mask_line *p, int mask_height, 
	      int xoff, int coff)
{  
  int x1=10000,x2=0;
  int iw=im1->width();  
  ushort r,co,off,cc;
  int y=0;
  uchar *l;

  for (;y<mask_height;y++)
  {
    mask_line *n=p+y;
    uchar *sl=screen->scan_line(y+sy)+sx+n->x;
    uchar *sl2=im1->scan_line(y);
    uchar *sl3=im2->scan_line(y);
    l=n->light;
    ushort *rem=n->remap;
    if (sx+n->x<x1) x1=sx+n->x;    
    int x=0;
    for (;x<n->size;x++,sl++,rem++,l++)   
    {
      r=*rem;

      off=(r+xoff);
      if (off>=iw) off-=iw;

      long p1=*(paddr+sl2[off]);
      long p2=*(paddr+sl3[off]);

      int r1=p1>>16,g1=(p1>>8)&0xff,b1=p1&0xff;
      int r2=p2>>16,g2=(p2>>8)&0xff,b2=p2&0xff;
      int r3=r1+(r2-r1)*fade256/256,
          g3=g1+(g2-g1)*fade256/256,
          b3=b1+(b2-b1)*fade256/256;

      uchar c=color_table->lookup_color(r3>>3,g3>>3,b3>>3);
				
      *sl=*(white_light+((*l)/2+28+jrand()%4)*256+c); 
      
    }
    if (sx+n->x+x>x2) x2=sx+n->x+x;
    
  }
  screen->add_dirty(x1,sy,x2,sy+mask_height-1);

}



void fade_in(image *im, int steps);
void fade_out(int steps);

class ex_char { 
  public :
  uchar frame,char_num;
  int x,y;
  ex_char *next; 
  ex_char (int X, int Y, int Frame, int Char_num, ex_char *Next) { x=X; y=Y; frame=Frame; char_num=Char_num; next=Next; }
} ;

void scale_put      (image *im, image *screen, int x, int y, short new_width, short new_height);
void scale_put_trans(image *im, image *screen, int x, int y, short new_width, short new_height);

void show_end2()
{
  int i;
  int planet=cash.reg("art/endgame.spe","planet",SPEC_IMAGE,1);
  int planet2=cash.reg("art/endgame.spe","dead_planet",SPEC_IMAGE,1);
  int mask=cash.reg("art/endgame.spe","mask",SPEC_IMAGE,1);
  int ship=cash.reg("art/endgame.spe","ship",SPEC_IMAGE,1);


  int explo_snd=lnumber_value(symbol_value(make_find_symbol("P_EXPLODE_SND")));
  int space_snd=lnumber_value(symbol_value(make_find_symbol("SPACE_SND")));
  int zip_snd=lnumber_value(symbol_value(make_find_symbol("SHIP_ZIP_SND")));
  

  mask_line *p=make_mask_lines(cash.img(mask),cash.img(planet)->width());  

  int explo_frames1[8],explo_frames2[7];

  for (i=0;i<8;i++)
  { char nm[100]; sprintf(nm,"small_wite%04d.pcx",i+1);
    explo_frames1[i]=cash.reg("art/exp1.spe",nm,SPEC_CHARACTER,1);
  }

  for (i=0;i<7;i++)
  { char nm[100]; sprintf(nm,"small_fire%04d.pcx",i+1);
    explo_frames2[i]=cash.reg("art/exp1.spe",nm,SPEC_CHARACTER,1);
  }

  int eoff=0,coff=0;

  int ex=xres/2-cash.img(mask)->width()/2;
  int ey=yres/2-cash.img(mask)->height()/2;
  fade_out(16);

  image blank(2,2); blank.clear();
  eh->set_mouse_shape(blank.copy(),0,0);      // don't show mouse


  screen->clear();
  int c[4]={pal->find_closest(222,222,22),
	    pal->find_closest(200,200,200),
	    pal->find_closest(100,100,100),
	    pal->find_closest(64,64,64)};
  ushort sinfo[800*3],*si;

  for (si=sinfo,i=0;i<800;i++)
  {
    *(si++)=jrand()%320; 
    *(si++)=jrand()%200;     
    *(si++)=c[jrand()%4];     
    screen->putpixel(si[-3],si[-2],si[-1]);
  }
  long paddr[256];
  if (old_pal)
  {
    for (i=0;i<256;i++) 
      paddr[i]=(old_pal->red(i)<<16)|(old_pal->green(i)<<8)|(old_pal->blue(i)); 
  }
  else
  {
    for (i=0;i<256;i++) 
      paddr[i]=(pal->red(i)<<16)|(pal->green(i)<<8)|(pal->blue(i));
  }

  int dx=(xres+1)/2-320/2,dy=(yres+1)/2-200/2;


  scan_map(screen,ex,ey,cash.img(planet),cash.img(planet2),0,paddr,p,cash.img(mask)->height(),eoff,coff);      
  image *tcopy=cash.img(planet)->copy();
  fade_in(NULL,32);

  time_marker old_time;


  
  for (i=0;i<80;)
  {
    time_marker new_time;    
    if (new_time.diff_time(&old_time)>0.1)
    {
      if ((i%10)==0 && (sound_avail&SFX_INITIALIZED))
        cash.sfx(space_snd)->play(64);

      old_time.get_time();
      screen->clear();
      int j;
      for (si=sinfo,j=0;j<800;j++,si+=3)
        screen->putpixel(dx+si[0],dy+si[1],si[2]);

      if (i>=30 && i<=37)
      {
	cash.img(planet)->put_image(tcopy,0,0);
	cash.fig(explo_frames1[i-30])->forward->put_image(tcopy,100,50);
        scan_map(screen,ex,ey,tcopy,
	       cash.img(planet2),
	       0,paddr,
	       p,cash.img(mask)->height(),eoff,coff);      
      } 
      else
        scan_map(screen,ex,ey,cash.img(planet),
	       cash.img(planet2),
	       0,paddr,
	       p,cash.img(mask)->height(),eoff,coff);      
      if (i>38)
      {
	int t=i-38;
	image *s=cash.img(ship);
	int nw=s->width()*(t+2)/16,
	    nh=s->height()*(t+2)/16;


        scale_put_trans(s,screen,ex-(i-38)*5,ey+cash.img(mask)->height()/2+t*4,nw,nh);
	if (i==77)
	  if (sound_avail&SFX_INITIALIZED)
            cash.sfx(zip_snd)->play(127);
      }
        
      eoff+=2; if (eoff>=320) eoff-=320;
      coff+=1; if (coff>=320) coff-=320;      
      eh->flush_screen();
      i++;
    }
  }
  delete tcopy;


  ex_char *clist=NULL;
  for (i=0;i<200;)
  {
    time_marker new_time;
    if (new_time.diff_time(&old_time)>0.1)
    {
      if ((i%10)==0 && (sound_avail&SFX_INITIALIZED))
        cash.sfx(space_snd)->play(64);

      old_time.get_time();
      screen->clear();
      int j;
      for (si=sinfo,j=0;j<800;j++,si+=3)
        screen->putpixel(dx+si[0],dy+si[1],si[2]);

      
      scan_map(screen,ex,ey,cash.img(planet),
	       cash.img(planet2),i*256/200,paddr,p,cash.img(mask)->height(),eoff,coff);      

      eoff+=2; if (eoff>=320) eoff-=320;
      coff+=1; if (coff>=320) coff-=320;      

      i++;
      if (i<150 || (i<170 && ((i-149)%2)==0) || (i<180 && ((i-149)%4)==0) || (i<190 && ((i-149)%8)==0))
      {
        clist=new ex_char(ex+jrand()%(cash.img(mask)->width()-cash.img(mask)->width()/3),
			ey+jrand()%(cash.img(mask)->height()-cash.img(mask)->height()/3),0,1,clist);
	if (sound_avail&SFX_INITIALIZED)
          cash.sfx(explo_snd)->play(127);
      }

//      clist=new ex_char(ex+jrand()%(cash.img(mask)->width(),
//			ey+jrand()%(cash.img(mask)->height(),0,1,clist);

      ex_char *c=clist,*last=NULL;
      for (;c;)
      {
	c->frame++;
	if (c->frame>6)
	{
	  ex_char *d=c;
	  if (last) last->next=c->next;
	  else clist=c->next;
	  c=c->next;
	  delete d;
	} else
	{ 
	  last=c; 
	  if (c->char_num)	
	    cash.fig(explo_frames2[c->frame])->forward->put_image(screen,c->x,c->y);	  

	  c->x-=3;
	  c=c->next; 
	}	  
      }

      eh->flush_screen();

    }


  }
  while (clist) { ex_char *p=clist; clist=clist->next; delete p; }

  screen->clear();
  int j;
  for (si=sinfo,j=0;j<800;j++,si+=3)
    screen->putpixel(si[0],si[1],si[2]);

  event ev;
  i=0;
  do
  {
    time_marker new_time;
    if (new_time.diff_time(&old_time)>0.1)
    {
      if ((i%10)==0 && (sound_avail&SFX_INITIALIZED))
        cash.sfx(space_snd)->play(64);

      old_time.get_time();
      scan_map(screen,ex,ey,cash.img(planet),
	       cash.img(planet2),
	       256,paddr,
	       p,cash.img(mask)->height(),eoff,coff);      
      eoff+=2; if (eoff>=320) eoff-=320;
      coff+=1; if (coff>=320) coff-=320;      
      eh->flush_screen();
      i++;
    }
    
    if (eh->event_waiting())
      eh->get_event(ev);

  } while (ev.type!=EV_KEY && ev.type!=EV_MOUSE_BUTTON);


  uchar cmap[32];
  for (i=0;i<32;i++)
    cmap[i]=pal->find_closest(i*256/32,i*256/32,i*256/32);

  void *end_plot=symbol_value(make_find_symbol("plot_end"));


  time_marker start;

  ev.type=EV_SPURIOUS;
  for (i=0;i<320 && ev.type!=EV_KEY;i++)
  {
    screen->clear();
    int j;
    for (si=sinfo,j=0;j<800;j++,si+=3)
      screen->putpixel(dx+si[0],dy+si[1],si[2]);

    scan_map(screen,ex,ey,cash.img(planet),
	     cash.img(planet2),
	     256,paddr,
	     p,cash.img(mask)->height(),eoff,coff);      
    text_draw(205-i,dx+10,dy,dx+319-10,dy+199,lstring_value(end_plot),eh->font(),cmap,eh->bright_color());
    eh->flush_screen();
    time_marker now; while (now.diff_time(&start)<0.18) now.get_time(); start.get_time();

    while (eh->event_waiting() && ev.type!=EV_KEY) eh->get_event(ev);
  }



  for (i=0;i<cash.img(mask)->height();i++)
  {
    jfree(p[i].remap);
    jfree(p[i].light);
  }

  jfree(p);


  delete current_level;
  current_level=NULL;

  fade_out(16);
  screen->clear();


  eh->set_mouse_shape(cash.img(c_normal)->copy(),1,1);
  the_game->set_state(MENU_STATE);
}

void show_sell(int abortable);

void share_end()
{
  fade_out(16);
  image blank(2,2); blank.clear();
  eh->set_mouse_shape(blank.copy(),0,0);      // don't show mouse
  screen->clear();

  image *im=cash.img(cash.reg("art/endgame.spe","tbc",SPEC_IMAGE,1));

  void *to_be=symbol_value(make_find_symbol("to_be_continued"));
  p_ref r1(to_be);

  void *mid_plot=symbol_value(make_find_symbol("plot_middle"));
  p_ref r2(mid_plot);


  int dx=(xres+1)/2-im->width()/2,dy=(yres+1)/2-im->height()/2;
  im->put_image(screen,dx,dy);
  console_font->put_string(screen,xres/2+35,yres/2+100-console_font->height()-2,
			   lstring_value(to_be));
  fade_in(NULL,32);

  uchar cmap[32];
  int i;
  for (i=0;i<32;i++)
    cmap[i]=pal->find_closest(i*256/32,i*256/32,i*256/32);

  event ev; ev.type=EV_SPURIOUS;
  time_marker start;
  for (i=0;i<320 && ev.type!=EV_KEY;i++)
  {
    im->put_image(screen,dx,dy);
    console_font->put_string(screen,xres/2+35,yres/2+100-console_font->height()-2,
			   lstring_value(to_be));

    text_draw(205-i,dx+10,dy,dx+319-10,dy+199,lstring_value(mid_plot),eh->font(),cmap,eh->bright_color());
    eh->flush_screen();
    time_marker now; while (now.diff_time(&start)<0.18) now.get_time(); start.get_time();
    while (eh->event_waiting() && ev.type!=EV_KEY) eh->get_event(ev);
  }

  if (ev.type!=EV_KEY)
  {
    do
    {
      eh->flush_screen();
      eh->get_event(ev);    
    } while (ev.type!=EV_KEY && ev.type!=EV_MOUSE_BUTTON);
  }

  fade_out(16);
  eh->set_mouse_shape(blank.copy(),0,0);      // don't show mouse  
  show_sell(1);
  eh->push_event(new event(ID_SHOW_SELL,NULL));
}


void show_end()
{
  fade_out(16);
  image blank(2,2); blank.clear();
  eh->set_mouse_shape(blank.copy(),0,0);      // don't show mouse
  screen->clear();

  image *im=cash.img(cash.reg("art/endgame.spe","end.pcx",SPEC_IMAGE,1));

  int dx=(xres+1)/2-320/2,dy=(yres+1)/2-200/2;

  void *end_plot=symbol_value(make_find_symbol("plot_end"));
  p_ref r2(end_plot);


  fade_in(im,32);

  uchar cmap[32];
  int i;
  for (i=0;i<32;i++)
    cmap[i]=pal->find_closest(i*256/32,i*256/32,i*256/32);

  event ev; ev.type=EV_SPURIOUS;
  time_marker start;
  for (i=0;i<320 && ev.type!=EV_KEY;i++)
  {
    im->put_image(screen,dx,dy);

    text_draw(205-i,dx+10,dy,dx+319-10,dy+199,lstring_value(end_plot),eh->font(),cmap,eh->bright_color());
    eh->flush_screen();
    time_marker now; while (now.diff_time(&start)<0.18) now.get_time(); start.get_time();
    while (eh->event_waiting() && ev.type!=EV_KEY) eh->get_event(ev);
  }

  if (ev.type!=EV_KEY)
  {
    do
    {
      eh->flush_screen();
      eh->get_event(ev);    
    } while (ev.type!=EV_KEY && ev.type!=EV_MOUSE_BUTTON);
  }

  delete current_level;
  current_level=NULL;

  fade_out(16);
  screen->clear();

  show_sell(1);

  eh->set_mouse_shape(cash.img(c_normal)->copy(),1,1);
  the_game->set_state(MENU_STATE);
}

