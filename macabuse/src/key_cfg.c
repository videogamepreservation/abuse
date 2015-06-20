#include "lisp.hpp"
#include "jwindow.hpp"
#include "image.hpp"
#include "specs.hpp"
#include "clisp.hpp"
#include "input.hpp"
#include "cache.hpp"

char *symbol_str(char *name);
#ifdef __MAC__
extern char *macify_name(char *s);
#endif

void load_image_into_screen(char *filename, char *name, int &x_loaded, int &y_loaded)
{
  bFILE *fp=open_file(filename,"rb");
  if (fp->open_failure())
  {
    lbreak("file not found %s\n",filename);
    exit(0);
  }
  spec_directory sd(fp);
  spec_entry *se=sd.find(name);
  if (!se)
  {
    lbreak("Unable to find %s in %s\n",name,filename);
  }
  fp->seek(se->offset,0);
  unsigned short w,h;
  w=fp->read_short();
  h=fp->read_short();

  screen->clear();
  int x,y,ytot;
  int read_width,skip_width;
  if (w>screen->width())
  {
    read_width=screen->width();
    skip_width=screen->width()-w;
    x_loaded=0;
  } else
  {
    read_width=w;
    skip_width=0;
    x_loaded=screen->width()/2-w/2;
  }

  if (h>screen->height())
  {
    ytot=screen->height();
    y_loaded=0;
  } else
  {
    ytot=h;
    y_loaded=screen->height()/2-h/2;
  }

  unsigned char *sl=screen->scan_line(y_loaded)+x_loaded;

  for (y=0;y<ytot;y++)
  {
    fp->read(sl,read_width);
    if (skip_width)
      fp->seek(skip_width,SEEK_CUR);
    sl+=screen->width();
  }

  delete fp;  
}

class key_field : public ifield
{
  int key;
public:

  virtual void area(int &x1, int &y1, int &x2, int &y2, window_manager *wm) 
  { 
    x1=x; 
    y1=y; 
    x2=x+wm->font()->width()*11;
    y2=y+wm->font()->height()+2;
  }

  virtual void draw_first(image *screen, window_manager *wm)
  {
    draw(0,screen,wm);
  }

  virtual void draw(int active, image *screen, window_manager *wm)
  {
    int x1,y1,x2,y2;
    area(x1,y1,x2,y2,wm);
    int bc,tc;

    if (active)
    {
      bc=wm->bright_color();
      tc=wm->black();
    } else
    {
      tc=wm->bright_color();
      bc=wm->black();
    }

    screen->bar(x1,y1,x2,y2,bc);
    screen->rectangle(x1,y1,x2,y2,wm->medium_color());
    char st[50];
    key_name(key,st);

    wm->font()->put_string(screen,(x1+x2)/2-wm->font()->width()*strlen(st)/2,y1+2,st,tc);
  }

  virtual void handle_event(event &ev, image *screen, window_manager *wm, input_manager *im)
  {
    if (ev.type==EV_KEY)
    {
      key=ev.key;
      im->next_active(screen,wm);
    }
  }

  virtual int selectable() { return 1; }
  virtual char *read() { return (char *)(&key); }
  key_field(int X, int Y, int Id, int Key, ifield *Next)
  {
    x=X; y=Y; id=Id; key=Key; next=Next;
  }
  
} ;
                               

enum { field_total=7 } ;

void do_key_config(window_manager *wm)
{
  int xp,yp;
  load_image_into_screen("art/keys.spe","key_config",xp,yp);

  ifield *first,**p;
  p=&first;

  int field_pos[field_total*2+2*2],*fieldp;
  fieldp=field_pos;
  for (int y=0;y<screen->height();y++)
  {
    unsigned char *sl=screen->scan_line(y);
    for (int x=0;x<screen->width();x++,sl++)
      if (*sl==242)
      {
        *fieldp=x; fieldp++;
        *fieldp=y; fieldp++;
      }
  }
 
  image *ok_image=cash.img(cash.reg("art/frame.spe","dev_ok",SPEC_IMAGE,1))->copy(),
    *cancel_image=cash.img(cash.reg("art/frame.spe","cancel",SPEC_IMAGE,1))->copy();


  int key_defaults[field_total]= { lnumber_value(symbol_value(l_up_key)),
                                   lnumber_value(symbol_value(l_left_key)),
                                   lnumber_value(symbol_value(l_right_key)),
                                   lnumber_value(symbol_value(l_down_key)),
                                   lnumber_value(symbol_value(l_special_key)),
                                   lnumber_value(symbol_value(l_weapon_left_key)),
                                   lnumber_value(symbol_value(l_weapon_right_key)) };
                                   
                                   

  for (int i=0;i<field_total;i++)
  {
    *p=new key_field(field_pos[i*2],field_pos[i*2+1],i,key_defaults[i],0);
    p=&((*p)->next);
  }

  *p=new button(field_pos[field_total*2],field_pos[field_total*2+1],20,ok_image,0);
  p=&((*p)->next);  
  *p=new button(field_pos[field_total*2+2],field_pos[field_total*2+3],21,cancel_image,0);
  p=&((*p)->next);  

  input_manager inm(screen,wm,first);
  int done=0;
  do
  {
    event ev;
    wm->flush_screen();
    wm->get_event(ev);
    if (ev.type==EV_MESSAGE)
    {
      if (ev.message.id==20)
        done=1;
      else if (ev.message.id==21)
        done=2;
    }

    if (ev.type==EV_KEY && ev.key==JK_ESC)
      done=2;

    inm.handle_event(ev,0,wm);
    
  } while (!done);
  if (done==1)
  {
    for (int i=0;i<field_total;i++)
    {
      key_defaults[i]=*((int *)first->read()); 
      first=(key_field *)first->next;
    }
    
    set_symbol_number(l_up_key,key_defaults[0]);
    set_symbol_number(l_left_key,key_defaults[1]);
    set_symbol_number(l_right_key,key_defaults[2]);
    set_symbol_number(l_down_key,key_defaults[3]);
    set_symbol_number(l_special_key,key_defaults[4]);
    set_symbol_number(l_weapon_left_key,key_defaults[5]);
    set_symbol_number(l_weapon_right_key,key_defaults[6]);

		char *st = "lisp/input.lsp";

#ifdef __MAC__
		FILE *fp=fopen(macify_name(st),"wb");
#else
    FILE *fp=fopen(st,"wb");
#endif
    if (fp)
    {
      int *key=key_defaults;
      enum { u,l,r,d,sp,wl,wr };
      fprintf(fp,"(setq up_key %d)\n",key[u]);
      fprintf(fp,"(setq left_key %d)\n",key[l]);
      fprintf(fp,"(setq right_key %d)\n",key[r]);
      fprintf(fp,"(setq down_key %d)\n",key[d]);
      fprintf(fp,"(setq special_key %d)\n",key[sp]);
      fprintf(fp,"(setq weapon_left_key %d)\n",key[wl]);
      fprintf(fp,"(setq weapon_right_key %d)\n",key[wr]);
      fclose(fp);
    }								
  }

  delete ok_image;
  delete cancel_image;

  screen->clear();
  wm->flush_screen();
}

