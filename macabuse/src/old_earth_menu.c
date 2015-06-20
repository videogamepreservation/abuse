#include "menu.hpp"
#include "lisp.hpp"
#include "loader.hpp"
#include "game.hpp"
#include "timing.hpp"
#include "game.hpp"
#include "id.hpp"
#include "pmenu.hpp"
#include "gui.hpp"
#include "property.hpp"
#include "dev.hpp"
#include "clisp.hpp"
#include "gamma.hpp"
#include "dprint.hpp"
#include "demo.hpp"
#include <math.h>

jwindow *volume_window=NULL;


//percent is 0..256
void tint_area(int x1, int y1, int x2, int y2, int r_to, int g_to, int b_to, int percent)
{
  int x,y;
  short cx1,cy1,cx2,cy2;
  screen->get_clip(cx1,cy1,cx2,cy2);
  if (x1<cx1) x1=cx1;
  if (y1<cy1) y1=cy1;
  if (x2>cx2) x2=cx2;
  if (y2>cy2) y2=cy2;
  if (x2<x1 || y2<y1) return ;

  percent=256-percent;

  for (y=y1;y<=y2;y++)
  {
    unsigned char *sl=screen->scan_line(y)+x1;
    for (x=x1;x<=x2;x++,sl++)
    {
      unsigned char *paddr=(unsigned char *)pal->addr()+(*sl)*3;
      unsigned char r=((*(paddr++))-r_to)*percent/256+r_to;
      unsigned char g=((*(paddr++))-g_to)*percent/256+g_to;
      unsigned char b=((*(paddr++))-b_to)*percent/256+b_to;
      *sl=color_table->lookup_color((r)>>3,(g)>>3,(b)>>3);
    }
  }
  screen->add_dirty(x1,y1,x2,y2);  
}

void darken_area(int x1, int y1, int x2, int y2, int amount)
{
  int x,y;
  short cx1,cy1,cx2,cy2;
  screen->get_clip(cx1,cy1,cx2,cy2);
  if (x1<cx1) x1=cx1;
  if (y1<cy1) y1=cy1;
  if (x2>cx2) x2=cx2;
  if (y2>cy2) y2=cy2;
  if (x2<x1 || y2<y1) return ;

  for (y=y1;y<=y2;y++)
  {
    unsigned char *sl=screen->scan_line(y)+x1;
    for (x=x1;x<=x2;x++,sl++)
    {
      unsigned char *paddr=(unsigned char *)pal->addr()+(*sl)*3;
      unsigned char r=(*(paddr++))*amount/256;
      unsigned char g=(*(paddr++))*amount/256;
      unsigned char b=(*(paddr++))*amount/256;
      *sl=color_table->lookup_color((r)>>3,(g)>>3,(b)>>3);
    }
  }
  screen->add_dirty(x1,y1,x2,y2);
}

void dark_wiget(int x1, int y1, int x2, int y2, int br, int dr, int amount)
{
  screen->add_dirty(x1,y1,x2,y2);
  screen->line(x1,y1,x1,y2,br);
  screen->line(x1+1,y1,x2,y1,br);
  screen->line(x2,y1+1,x2,y2,dr);
  screen->line(x1+1,y2,x2,y2,dr);
  darken_area(x1+1,y1+1,x2-1,y2-1,amount);  
}

char *men_str(void *arg)
{
  switch (item_type(arg))
  {
    case L_STRING : 
    { return lstring_value(arg); } break;
    case L_CONS_CELL : 
    { return lstring_value(CAR(arg)); } break;
    default : 
    {
      lprint(arg);
      printf(" is not a valid menu option\n");
      exit(0);
    }
  }
  return NULL;
}

void main_menu();

int menu(void *args, JCFont *font)             // reurns -1 on esc
{
  main_menu();
  char *title=NULL;
  if (!NILP(CAR(args)))
    title=lstring_value(CAR(args));
  Cell *def=lcar(lcdr(lcdr(args)));
  args=CAR(CDR(args));

  int options=list_length(args);
  int mh=(font->height()+1)*options+10,maxw=0;

  Cell *c=(Cell *)args;
  for (;!NILP(c);c=CDR(c))
  {
    if (strlen(men_str(CAR(c)))>maxw)
      maxw=strlen(men_str(CAR(c)));
  }
  
  int mw=(font->width())*maxw+20;
  int mx=screen->width()/2-mw/2,
      my=screen->height()/2-mh/2;
  

  screen->add_dirty(mx,my,mx+mw-1,my+mh-1);

  if (title)
  {
    int tl=strlen(title)*font->width();
    int tx=screen->width()/2-tl/2;
    dark_wiget(tx-2,my-font->height()-4,tx+tl+2,my-2,eh->medium_color(),eh->dark_color(),180);
    font->put_string(screen,tx+1,my-font->height()-2,title,eh->bright_color());
  }
  
  dark_wiget(mx,my,mx+mw-1,my+mh-1,eh->medium_color(),eh->dark_color(),200);


  int y=my+5;
  for (c=(Cell *)args;!NILP(c);c=CDR(c))
  {
    char *ms=men_str(CAR(c));
    font->put_string(screen,mx+10+1,y+1,ms,eh->black());
    font->put_string(screen,mx+10,y,ms,eh->bright_color());
    y+=font->height()+1;
  }
  

  eh->flush_screen();
  event ev;
  int choice=0,done=0;
  int bh=font->height()+3;
  image *save=new image(mw-2,bh);
  int color=128,cdir=50;
  
  time_marker *last_color_time=NULL; 
  if (!NILP(def))
    choice=lnumber_value(def);
  do
  {
  
    if (eh->event_waiting())
    {
      eh->get_event(ev);
      if (ev.type==EV_KEY)
      {
	switch (ev.key)
	{
	  case JK_ESC : 
	  { choice=-1; done=1; } break;
	  case JK_ENTER :
	  { done=1; } break;
	  case JK_DOWN : 
	  { if (choice<options-1) 
	    choice++;
	  else choice=0;
	  } break;
	  case JK_UP :
	  {
	    if (choice>0)
	    choice--;
	    else choice=options-1;
	  } break;		      
	}
      } else if (ev.type==EV_MOUSE_BUTTON && ev.mouse_button)
      {
	if (ev.mouse_move.x>mx && ev.mouse_move.x<mx+mw && ev.mouse_move.y>my &&
	    ev.mouse_move.y<my+mh)
	{
	  int msel=(ev.mouse_move.y-my)/(font->height()+1);
	  if (msel>=options) msel=options-1;
	  if (msel==choice)                    // clicked on already selected item, return it
	    done=1;
	  else choice=msel;                    // selects an item
	}
      }
      eh->flush_screen();
    }

    time_marker cur_time;
    if (!last_color_time || (int)(cur_time.diff_time(last_color_time)*1000)>120)
    {       
      if (last_color_time)
        delete last_color_time;
      last_color_time=new time_marker;

      int by1=(font->height()+1)*choice+my+5-2;
      int by2=by1+bh-1;

      screen->put_part(save,0,0,mx+1,by1,mx+mw-2,by2);
      tint_area(mx+1,by1,mx+mw-2,by2,63,63,63,color);

      char *cur=men_str(nth(choice,args));
      font->put_string(screen,mx+10+1,by1+3,cur,eh->black());
      font->put_string(screen,mx+10,by1+2,cur,eh->bright_color());
      screen->rectangle(mx+1,by1,mx+mw-2,by2,eh->bright_color());

      color+=cdir;

      if (color<12 || color>256)
      {
	cdir=-cdir;
	color+=cdir;
      }
      eh->flush_screen();
      save->put_image(screen,mx+1,by1);
    } else milli_wait(10);

  } while (!done);
  if (last_color_time)
    delete last_color_time;
  delete save;
  the_game->draw(the_game->state==SCENE_STATE);

  if (choice!=-1)
  {
    void *val=nth(choice,args);
    if (item_type(val)==L_CONS_CELL)   // is there another value that the user want us to return?
      return lnumber_value(lcdr(val));  
  }
  return choice;
}



struct mask_line
{
  int x,size;
  ushort *remap;
} ;



void scan_map(image *screen, int sx, int sy, image *im, image *clouds, mask_line *p, int mask_height, 
	      int xoff, int coff)
{  
  int x1=10000,x2=0;
  int iw=im->width();  
  ushort r,co,off,cc;
  int y=0;
  for (;y<mask_height;y++)
  {
    mask_line *n=p+y;
    uchar *sl=screen->scan_line(y+sy)+sx+n->x;
    uchar *sl2=im->scan_line(y);
//    uchar *sl3=clouds->scan_line(y);
    ushort *rem=n->remap;
    if (sx+n->x<x1) x1=sx+n->x;    
    int x=0;
    for (;x<n->size;x++,sl++,rem++)   
    {
      r=*rem;
//      co=(r+coff);
//      if (co>=iw) co-=iw;     
//      cc=sl3[co];

      off=(r+xoff);
      if (off>=iw) off-=iw;
//      if (cc)
//        *sl=*(white_light+(r/4+(x+y)%2)*256+*(white_light+((256-cc)/2-64)*256+sl2[off])); 
//        *sl=*(white_light+(r/4+(x+y)%2)*256+*(white_light+(48+cc/16)*256+sl2[off])); 
//      else
        *sl=*(white_light+(r/4+(x+y)%2)*256+sl2[off]); 
      
    }
    if (sx+n->x+x>x2) x2=sx+n->x+x;
    
  }
  screen->add_dirty(x1,sy,x2,sy+mask_height-1);

}

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
    while (*sl!=0 && x<mask->width()) { sl++; x++; size++; }
    p[y].size=size;

    // now calculate remap for line
    p[y].remap=(ushort *)jmalloc(size*2,"mask remap");
    ushort *rem=p[y].remap;
    for (x=0;x<size;x++,rem++)
    {
      if (x<=size/2)
        *rem=(int)(sqrt(x/(double)size)*map_width/2.0);
      else *rem=(int)(mask->width()-(sqrt((size-x)/(double)size)*map_width/2.0)+mask->width()/2);
    }
  }
  return p;
}

static void draw_vol(image *screen, int x1, int y1, int x2, int y2, int t, int max, int c1, int c2)
{
  int dx=x1+t*(x2-x1)/max;
  if (t!=0)
    screen->bar(x1,y1,dx,y2,c1);
  else dx--;

  if (dx<x2)
    screen->bar(dx+1,y1,x2,y2,c2);
}

static void draw_sfx_vol()
{
  draw_vol(volume_window->screen,5,17,34,23,sfx_volume,127,pal->find_closest(255,0,0),
	   pal->find_closest(90,0,0));
}

static void draw_music_vol()
{
  draw_vol(volume_window->screen,5,72,34,78,music_volume,127,pal->find_closest(255,0,0),
	   pal->find_closest(90,0,0));
}

static void create_volume_window()
{
  char *ff="art/frame.spe";
  int t=SPEC_IMAGE;
  int u_u=cash.reg(ff,"u_u",t,1),
      u_d=cash.reg(ff,"u_u",t,1),
      u_ua=cash.reg(ff,"u_ua",t,1),
      u_da=cash.reg(ff,"u_da",t,1),

      d_u=cash.reg(ff,"d_u",t,1),
      d_d=cash.reg(ff,"d_u",t,1),
      d_ua=cash.reg(ff,"d_ua",t,1),
      d_da=cash.reg(ff,"d_da",t,1);
  
  volume_window=eh->new_window(prop->getd("volume_x",xres/2-20),
			       prop->getd("volume_y",yres/2-50),
			       41-WINDOW_FRAME_LEFT-WINDOW_FRAME_RIGHT,
			       101-WINDOW_FRAME_TOP-WINDOW_FRAME_BOTTOM,
			     new ico_button(10,27,ID_SFX_DOWN,d_u,d_d,d_ua,d_da,
			     new ico_button(21,27,ID_SFX_UP,u_u,u_d,u_ua,u_da,

			     new ico_button(10,63,ID_MUSIC_DOWN,d_u,d_d,d_ua,d_da,
			     new ico_button(21,63,ID_MUSIC_UP,u_u,u_d,u_ua,u_da,
					    NULL)))));
  cash.img(cash.reg(ff,"vcontrol",t,1))->put_image(volume_window->screen,0,0);
  draw_music_vol();
  draw_sfx_vol();
  volume_window->inm->redraw();
  eh->grab_focus(volume_window);
}


#define MENU_TICONS 19
int menu_icons[MENU_TICONS*3];
int menu_icons_ids[MENU_TICONS]={ID_START_GAME,ID_VOLUME,ID_NULL,ID_NULL,ID_NULL,ID_NULL,
				 ID_NULL,ID_NULL,ID_QUIT,ID_NULL,ID_EASY,ID_NULL,
				 ID_MEDIUM,ID_NULL,ID_HARD,ID_NULL,ID_LIGHT_ON,ID_LIGHT_OFF,
			         ID_EXTREME };

static jwindow *ico_win;

void save_difficulty()
{
  FILE *fp=fopen("hardness.lsp","wb");
  if (!fp)
    dprintf("Unable to write to file hardness.lsp\n");
  else 
  {
    fprintf(fp,"(setf difficulty '");
    if (DEFINEDP(l_difficulty))
    {
      if (symbol_value(l_difficulty)==l_extreme)
        fprintf(fp,"extreme)\n");
      else if (symbol_value(l_difficulty)==l_hard)
        fprintf(fp,"hard)\n");
      else if (symbol_value(l_difficulty)==l_easy)
        fprintf(fp,"easy)\n");
      else 
        fprintf(fp,"medium)\n");
    } else 
       fprintf(fp,"medium)\n");
    fclose(fp);
  }
}

void menu_handler(event &ev, jwindow *ico_win)
{
  switch (ev.type)
  {
    case EV_MESSAGE :
    {
      switch (ev.message.id)
      {
	case ID_LIGHT_OFF :
	{
	  gamma_correct(pal,1);
	} break;
	case ID_START_GAME :
	{        
	  the_game->load_level(level_file);
	  the_game->set_state(RUN_STATE);
	} break;

	case ID_VOLUME : 
	{ create_volume_window(); } break;
      	case ID_SFX_UP :
	{ if (volume_window) 
	  {
	    sfx_volume+=16;
	    if (sfx_volume>127) sfx_volume=127;
	    draw_sfx_vol();
	  }
	} break;
      	case ID_SFX_DOWN :
	{ if (volume_window) 
	  {
	    sfx_volume-=16;
	    if (sfx_volume<0) sfx_volume=0;
	    draw_sfx_vol();
	  }
	} break;

      	case ID_MUSIC_UP :
	{ if (volume_window) 
	  {
	    music_volume+=16;
	    if (music_volume>127) music_volume=127;
	    draw_music_vol();
	  }
	} break;
      	case ID_MUSIC_DOWN :
	{ if (volume_window) 
	  {
	    music_volume-=16;
	    if (music_volume<0) music_volume=0;
	    draw_music_vol();
	  }
	} break;
	case ID_MEDIUM :
	{
	  set_symbol_value(l_difficulty,l_medium);
	  save_difficulty();
	} break;
	case ID_HARD :
	{
	  set_symbol_value(l_difficulty,l_hard);
	  save_difficulty();
	} break;
	case ID_EXTREME :
	{
	  set_symbol_value(l_difficulty,l_extreme);
	  save_difficulty();
	} break;
	case ID_EASY :
	{
	  set_symbol_value(l_difficulty,l_easy);
	  save_difficulty();
	} break;
		      
			 
      } break;
    } break;
    case EV_CLOSE_WINDOW :
    {
      if (ev.window==volume_window)
      { eh->close_window(volume_window); volume_window=NULL; }
    } break;
  }
}

void *current_demo=NULL;

void main_menu()
{
  image *Earth=cash.img(earth);
  image *Emap=cash.img(earth_mask);
	
  char name[20];
  ico_button *buts[MENU_TICONS];

  long maxx=0,maxy=0;
  int i=0;
  for (;i<MENU_TICONS;i++)
  {
    sprintf(name,"icon%04d.pcx",i*3+1);
    menu_icons[i*3]=cash.reg("art/icons.spe",name,SPEC_IMAGE,1);
    sprintf(name,"icon%04d.pcx",i*3+2);
    menu_icons[i*3+1]=cash.reg("art/icons.spe",name,SPEC_IMAGE,1);
    sprintf(name,"icon%04d.pcx",i*3+2);
    menu_icons[i*3+2]=cash.reg("art/icons.spe",name,SPEC_IMAGE,1);

    long x=WINDOW_FRAME_LEFT+(i%9)*cash.img(menu_icons[0])->width();
    long y=WINDOW_FRAME_TOP+(i/9)*cash.img(menu_icons[0])->height();
    if (x>maxx) maxx=x;
    if (y>maxy) maxy=y;
    buts[i]=new ico_button(x,y,menu_icons_ids[i],
			   menu_icons[i*3],menu_icons[i*3],
			   menu_icons[i*3+1],menu_icons[i*3+2],NULL);
  }

  buts[0]->next=buts[1];

  int b1,b2,b3,b4;
  
  if (DEFINEDP(symbol_value))
  {
    if (symbol_value(l_difficulty)==l_extreme)
    { b1=18; b2=10;  b3=12; b4=14;  }
    else if (symbol_value(l_difficulty)==l_hard)
    { b1=14; b2=18;  b3=10; b4=12;  }
    else if (symbol_value(l_difficulty)==l_easy)
    { b1=10; b2=12; b3=14; b4=18; }
    else 
    { b1=12; b2=14; b3=18; b4=10; }
  } else  
  { b1=12; b2=14; b3=18; b4=10; }
  

  buts[b1]->next=buts[b2];
  buts[b2]->next=buts[b3];
  buts[b3]->next=buts[b4];


  buts[1]->next=new ico_switch_button(buts[0]->X(),
				      buts[0]->Y()+cash.img(menu_icons[0])->height()*2,
				      ID_NULL, 
				      buts[b1],buts[17]);

 

  
  buts[17]->next=buts[8];

  buts[1]->set_xy(buts[0]->X(),
		  buts[0]->Y()+cash.img(menu_icons[0])->height()*1);
  buts[12]->set_xy(buts[0]->X(),
		  buts[0]->Y()+cash.img(menu_icons[0])->height()*2);
  buts[17]->set_xy(buts[0]->X(),
		  buts[0]->Y()+cash.img(menu_icons[0])->height()*3);
  buts[8]->set_xy(buts[0]->X(),
		  buts[0]->Y()+cash.img(menu_icons[0])->height()*4);



  ico_win=eh->new_window(-1,yres/2-80,-1,-1,buts[0],"Menu");
  
  
//  pmenu *main_pm=new pmenu(0,0,game_sub,screen,eh);
  time_marker old_time;

  screen->add_dirty(0,0,319,199);

  
  // create sphere map
  mask_line *p=make_mask_lines(Emap,Earth->width());

  int eoff=0,coff=0;
  event ev;
//  main_pm->draw(screen,eh,1);
  long x=84,y=60;
  Cell *v=find_symbol("earth_x");
  if (v && DEFINEDP(v)) x=lnumber_value(symbol_value(v));

  v=find_symbol("earth_y");
  if (v && DEFINEDP(v)) y=lnumber_value(symbol_value(v));
  int state=0,stop_menu=0;
  time_marker start;
  do
  {
    time_marker new_time;
    if (state || new_time.diff_time(&old_time)>0.15)
    {
      old_time.get_time();
      scan_map(screen,x,y,Earth,NULL,p,Emap->height(),eoff,coff);      
      if (state)
      { eoff+=8; coff+=4; }
      else
      {
        eoff+=2; coff+=1;
      }

      if (eoff>=320) eoff-=320;
      if (coff>=320) coff-=320;      
      eh->flush_screen();
    }

    if (eh->event_waiting())
    {
      eh->get_event(ev);    
      start.get_time();      // reset time till demo starts up

      menu_handler(ev,ico_win);
      if (ev.type==EV_MOUSE_BUTTON && ev.mouse_button && ev.mouse_move.x>=x && ev.mouse_move.y>=y && 
	  ev.mouse_move.x<=x+Emap->width() && ev.mouse_move.y<=y+Emap->height())
      {
	state=1;
      } else if (ev.type==EV_MOUSE_BUTTON && !ev.mouse_button) state=0;

	
      
      eh->flush_screen();
    }

    if (new_time.diff_time(&start)>10)
    {
      if (!current_demo)
      {
	void *d=make_find_symbol("demos");
	if (DEFINEDP(d))	
	  current_demo=symbol_value(d);
      }
      if (current_demo)
      {
	if (set_demo_mode(DEMO_PLAY,lstring_value(CAR(current_demo)),eh))
	  stop_menu=1;
	current_demo=CDR(current_demo);
      }
    }
   
  } while (!stop_menu && 
	   (ev.type!=EV_MESSAGE || (ev.message.id!=ID_START_GAME && ev.message.id!=ID_QUIT)));

  for (i=0;i<MENU_TICONS;i++)
  {
    ifield *ic=ico_win->inm->unlink(menu_icons_ids[i]);
    if (i) delete ic;
    else delete buts[i];
  }
  
  eh->close_window(ico_win);
  for (int xx=0;xx<Emap->height();xx++)
    jfree(p[xx].remap);
  jfree(p);

  if (ev.message.id==ID_QUIT)   // propogate the quit message
    the_game->end_session();

//  delete main_pm;
}





/*  pmenu_item *net_sub=new pmenu_item("Multiplayer",
	new psub_menu(
		      new pmenu_item(ID_MODEM,        "Modem",-1,
		      new pmenu_item(ID_TCPIP,        "TCP/IP (internet)",-1,
		      new pmenu_item(ID_IPX,          "IPX",-1,
		      new pmenu_item(ID_SPLIT_SCREEN, "Split screen",-1,
				     NULL)))),NULL),NULL,200);


  pmenu_item *graphics_sub=new pmenu_item("Options",
	new psub_menu(
		      new pmenu_item(ID_KEY_SETUP,    "Keyboard",-1,
		      new pmenu_item(ID_MOUSE_SETUP,  "Mouse",-1,
		      new pmenu_item(CALB_JOY,        "Joystick",-1,
		      new pmenu_item(0,       NULL,        -1,
		      new pmenu_item(ID_LIGHT_DETAIL, "Lighting detail",-1,
		      new pmenu_item(ID_SCREEN_SIZE,  "Screen Size",-1,
		      new pmenu_item(ID_VOLUME,       "Volume",-1,
		      new pmenu_item(ID_SFX_CHANNELS, "Sfx Channels",-1,
				     NULL)))))))),NULL),net_sub,100);


  pmenu_item *game_sub=new pmenu_item("Game", 				      
	new psub_menu(
		      new pmenu_item(ID_NEW_GAME,  "New Game",-1,
		      new pmenu_item(ID_DIFFICULTY,"Difficulty",-1,
		      new pmenu_item(ID_LOAD_GAME, "Load Game",-1,
		      new pmenu_item(ID_QUIT,      "Quit",-1,NULL)))),NULL),graphics_sub,27);
*/
