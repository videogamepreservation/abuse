#include "netstat.hpp"
#include "dprint.hpp"
#include "cache.hpp"
#include "jwindow.hpp"

extern palette *pal;
static int old_xres;

extern window_manager *eh;

class net_status_node
{
  public :  
  char *name;
  net_status_node *next;
  visual_object *show;
  int last_update;

  net_status_node(char *Name, visual_object *Show, net_status_node *Next) 
  { 
  	name=strcpy((char *)jmalloc(strlen(Name)+1,"status name"),Name); 
    show=Show;
    next=Next; 
    last_update=0;
  }
  ~net_status_node() { jfree(name); if (show) delete show; }
} ; 

net_status_manager::net_status_manager(char *graphic_file, 
    int x1, int y1,
    int x2, int y2,
    int color1, int color2) : g_file(graphic_file), color1(color1), color2(color2)
{ 
  first_x1=x1;
  first_y1=y1;


  if (x2-x1<y2-y1)
  {
    first_x2=(x1+x2)/2-1;
    first_y2=y2;
    second_x1=(x1+x2)/2+1;
  } else
  {
    first_x2=x2;
    first_y2=(y1+y2)/2-1;
    second_x1=x1;
    second_y1=(y1+y2)/2+1;
  }
  second_x2=x2;
  second_y2=y2;


  first=NULL; 
  level=0; 
}

void load_image_into_screen(char *filename, char *name, int &x_loaded, int &y_loaded);

void net_status_manager::push(char *name, visual_object *show)
{


  level++;
  first=new net_status_node(name,show,first);
  if (level==1)
  {
    screen->clear();
    update_dirty(screen);   // make sure the screen is actually clear before we mess with the palette

    // save a copy of the old palette
    if (pal)
      old_pal=pal->copy();
    else old_pal=0;
    bFILE *fp=open_file(g_file,"rb");
    if (fp->open_failure())
    {
      dprintf("Unable to open art/status.spe\n");
      exit(0);
    }

    old_xres=xres;
    switch_mode(VMODE_640x480);

    spec_directory sd(fp);
    spec_entry *se=sd.find(SPEC_PALETTE);  // find the palette used by this screen
    fp->seek(se->offset,0);
    palette new_pal(fp);
    new_pal.load();
    delete fp;

    // now load the status image direct into our back buffer so that
    // another image is not allocated
    int x,y;
    load_image_into_screen(g_file,"status",x,y);
    xp=x; yp=y;

    //    cash.img(cash.reg("art/status.spe","status", SPEC_IMAGE,1))->put_image(screen,0,0);
    //    xp=0;
    //    yp=0;

    update_dirty(screen);
  }


	
}

void draw_bar(int x1, int y1, int x2, int y2, int percent, int color)
{
  int h=(y2-y1)*percent/100;
  int w=(x2-x1)*percent/100;
  
  if (h>w)
  {
    screen->bar(x1,y2-h,x2,y2,color);
    if (y2-h!=y1)
      screen->bar(x1,y1,x2,y2-h-1,0);
  } else
  {
    screen->bar(x1,y1,x1+w,y2,color);
    if (x2-x1!=w)
      screen->bar(x1+w+1,y1,x2,y2,0);
  }
}


void net_status_manager::update(int percentage)
{
  if (level==last_level && percentage==last_percent)
    return ;
    
  if (level==1)
     first_percent=percentage;
    
  if (level==1 || level==2)
  {
     draw_bar(xp+first_x1,yp+first_y1,xp+first_x2,yp+first_y2,first_percent,color1);   
  }
  
  if (level==2)
  {
    draw_bar(xp+second_x1,yp+second_y1,xp+second_x2,yp+second_y2,percentage,color2);
  }
    
  last_percent=percentage;
  last_level=level;
  
  update_dirty(screen);
}


void net_status_manager::pop()
{
//	return;

  level--;
  if (level==0)
  {

    screen->clear();
    update_dirty(screen);

    if (old_xres<=320)
      switch_mode(VMODE_320x200);

    if (eh)
    {
      event e,*ev=new event;

      ev->type=EV_REDRAW;
      ev->redraw.x1=0;
      ev->redraw.y1=0;
      ev->redraw.x2=xres;
      ev->redraw.y2=yres;

      eh->push_event(ev);
      eh->get_event(e);   // cause the window manager to process this event
    }

    if (old_pal)
    {
      old_pal->load();
      delete old_pal;
    }

  }

  net_status_node *p=first; 
  first=first->next;
  delete p;

  if (level==0)
  {
  }
}





