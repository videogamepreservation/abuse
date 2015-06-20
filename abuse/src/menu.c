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
#include "gamma.hpp"
#include "dprint.hpp"
#include "demo.hpp"
#include "loadgame.hpp"
#include "scroller.hpp"
#include "netcfg.hpp"
#include <math.h>
#include "sock.hpp"

extern net_protocol *prot;
jwindow *volume_window=NULL;
extern int confirm_quit();
extern int registered;

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
    eh->flush_screen();
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


static void draw_vol(image *screen, int x1, int y1, int x2, int y2, int t, int max, int c1, int c2, int slider)
{
  int dx=x1+t*(x2-x1)/max;
  if (t!=0)
  {
    cash.img(slider)->put_image(screen,x1,y1);    
//    screen->bar(x1,y1,dx,y2,c1);
  }
  else dx--;

  if (dx<x2)
    screen->bar(dx+1,y1,x2,y2,c2);
}

static void draw_sfx_vol(int slider)
{
  draw_vol(volume_window->screen,6,16,34,22,sfx_volume,127,pal->find_closest(200,75,19),
	   pal->find_closest(40,0,0),slider);
}

static void draw_music_vol(int slider)
{
  draw_vol(volume_window->screen,6,61,34,67,music_volume,127,pal->find_closest(255,0,0),
	   pal->find_closest(40,0,0),slider);
}

static void create_volume_window()
{
/*  int vx=WINDOW_FRAME_LEFT,vy=WINDOW_FRAME_TOP+eh->font()->height()*2,scroller_height=130,bh=eh->font()->height()+5;

  volume_window=eh->new_window(prop->getd("volume_x",xres/2-20),
			       prop->getd("volume_y",yres/2-50),
			       -1,
			       -1,
			       new scroller(vx,vy,LOWER_SFX,0,scroller_height,0,127,
			       new scroller(vx+30,vy,LOWER_MUSIC,0,scroller_height,0,127,NULL)),symbol_str("VOLUME"));
  event ev;
  int done=0;
  do
  {
    eh->flush_screen();
    eh->get_event(ev);
    if (ev.type==EV_CLOSE_WINDOW && ev.window==volume_window) done=1;    
  } while (!done);
  eh->close_window(volume_window);
  volume_window=NULL; */


  char *ff="art/frame.spe";
  int t=SPEC_IMAGE;
  int u_u=cash.reg(ff,"u_u",t,1),
      u_d=cash.reg(ff,"u_u",t,1),
      u_ua=cash.reg(ff,"u_ua",t,1),
      u_da=cash.reg(ff,"u_da",t,1),

      d_u=cash.reg(ff,"d_u",t,1),
      d_d=cash.reg(ff,"d_u",t,1),
      d_ua=cash.reg(ff,"d_ua",t,1),
      d_da=cash.reg(ff,"d_da",t,1),
      slider=cash.reg(ff,"volume_slide",t,1);
  
  volume_window=eh->new_window(prop->getd("volume_x",xres/2-20),
			       prop->getd("volume_y",yres/2-50),
			       41-WINDOW_FRAME_LEFT-WINDOW_FRAME_RIGHT-2,
			       101-WINDOW_FRAME_TOP-WINDOW_FRAME_BOTTOM,
			     new ico_button(10,27,ID_SFX_DOWN,d_u,d_d,d_ua,d_da,
			     new ico_button(21,27,ID_SFX_UP,u_u,u_d,u_ua,u_da,
                             new info_field(15,42,0,symbol_str("SFXv"),

			     new ico_button(10,72,ID_MUSIC_DOWN,d_u,d_d,d_ua,d_da,
			     new ico_button(21,72,ID_MUSIC_UP,u_u,u_d,u_ua,u_da,
                             new info_field(10,86,0,symbol_str("MUSICv"),
					    NULL)))))));

  cash.img(cash.reg(ff,"vcontrol",t,1))->put_image(volume_window->screen,0,0);
  draw_music_vol(slider);
  draw_sfx_vol(slider);
  volume_window->inm->redraw();
  eh->grab_focus(volume_window);
  eh->flush_screen();

  volume_window->inm->allow_no_selections();
  volume_window->inm->clear_current();

  event ev;
  do
  {
    do { eh->get_event(ev); } while (ev.type==EV_MOUSE_MOVE && eh->event_waiting()); 
    eh->flush_screen();
    if (ev.type==EV_MESSAGE)
    {
      switch (ev.message.id)
      {
      	case ID_SFX_UP :
	{ if (volume_window) 
	  {
	    sfx_volume+=16;
	    if (sfx_volume>127) sfx_volume=127;
	    draw_sfx_vol(slider);
	    char *s="sfx/ambtech1.wav";
	    if (sound_avail&SFX_INITIALIZED) 
	      cash.sfx(cash.reg(s,s,SPEC_EXTERN_SFX,1))->play(sfx_volume);
	  }
	} break;
      	case ID_SFX_DOWN :
	{ if (volume_window) 
	  {
	    sfx_volume-=16;
	    if (sfx_volume<0) sfx_volume=0;
	    draw_sfx_vol(slider);
	    char *s="sfx/ambtech1.wav";
	    if (sound_avail&SFX_INITIALIZED) 
	      cash.sfx(cash.reg(s,s,SPEC_EXTERN_SFX,1))->play(sfx_volume);
	  }
	} break;

      	case ID_MUSIC_UP :
	{ if (volume_window) 
	  {
	    music_volume+=16;
	    if (music_volume>127) music_volume=127;
	    draw_music_vol(slider);
	    if (current_song) current_song->set_volume(music_volume);
	  }
	} break;
      	case ID_MUSIC_DOWN :
	{ if (volume_window) 
	  {
	    music_volume-=16;
	    if (music_volume<0) music_volume=0;
	    draw_music_vol(slider);
	    if (current_song) current_song->set_volume(music_volume);
	  }
	} break;
      }
    } else if (ev.type==EV_CLOSE_WINDOW || (ev.type==EV_KEY && ev.key==JK_ESC))
    {
      eh->close_window(volume_window);
      volume_window=NULL;
    }
  } while (volume_window);
}


FILE *open_FILE(char *filename, char *mode);

void save_difficulty()
{
  FILE *fp=open_FILE("hardness.lsp","wb");
  if (!fp)
    dprintf("Unable to write to file hardness.lsp\n");
  else 
  {
    fprintf(fp,"(setf difficulty '");
    if (DEFINEDP(symbol_value(l_difficulty)))
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

void fade_out(int steps);
void fade_in(image *im, int steps);


void show_sell(int abortable)
{
  void *ss=make_find_symbol("sell_screens");
  if (!DEFINEDP(symbol_value(ss)))
  {
    int sp=current_space;
    current_space=PERM_SPACE;
//    char *prog="((\"art/help.spe\" . \"sell2\")(\"art/help.spe\" . \"sell4\")(\"art/help.spe\" . \"sell3\")(\"art/endgame.spe\" . \"credit\"))";
//    char *prog="((\"art/endgame.spe\" . \"credit\") (\"art/help.spe\" . \"sell6\"))";
    char *prog="((\"art/endgame.spe\" . \"credit\"))";
    set_symbol_value(ss,compile(prog));
    current_space=sp;
  }

  if (DEFINEDP(symbol_value(ss)))
  {
    image blank(2,2); blank.clear();
    eh->set_mouse_shape(blank.copy(),0,0);      // don't show mouse

    ss=symbol_value(ss);
    int quit=0;
    while (ss && !quit)
    {
      int im=cash.reg_object("art/help.spe",CAR(ss),SPEC_IMAGE,1);
      fade_in(cash.img(im),16);

      event ev;
      do
      { eh->flush_screen();
	eh->get_event(ev);
      } while (ev.type!=EV_KEY);
      if (ev.key==JK_ESC && abortable)
        quit=1;
      fade_out(16);
      ss=CDR(ss);
    }
    eh->set_mouse_shape(cash.img(c_normal)->copy(),1,1);
  }
}


void menu_handler(event &ev, input_manager *inm)
{
  switch (ev.type)
  {
    case EV_MESSAGE :
    {
      switch (ev.message.id)
      {
	case ID_LIGHT_OFF :
	if (!volume_window)
	{
	  gamma_correct(pal,1);
	} break;
	case ID_RETURN :
	if (!volume_window)
	{
	  the_game->set_state(RUN_STATE);
	} break;
	case ID_START_GAME :
	if (!volume_window)
	{        
	  the_game->load_level(level_file);
	  the_game->set_state(RUN_STATE);
	  view *v;
	  for (v=player_list;v;v=v->next)
	    if (v->focus)
	      v->reset_player();
	   
	} break;

   
        case ID_LOAD_PLAYER_GAME :
	if (!volume_window)
	{
	  int got_level=load_game(0,symbol_str("LOAD"));
	  the_game->reset_keymap();
	  if (got_level)
	  {
	    char name[50];
	    sprintf(name,"save%04d.spe",got_level);
	    the_game->load_level(name);
	    the_game->set_state(RUN_STATE);	    
	  }
	} break;


	case ID_VOLUME : 
	if (!volume_window)
	{ create_volume_window(); } break;

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

	case ID_NETWORKING :
	{
	  if (!volume_window)
	  {
	    net_configuration *cfg=new net_configuration;
	    if (cfg->input())
	    {
	      if (main_net_cfg) delete main_net_cfg;
	      main_net_cfg=cfg;
	    } else delete cfg;
	    the_game->draw(0);
	    inm->redraw();
	  }
	} break;
		      
	case ID_SHOW_SELL :
	if (!volume_window)
	{ 
	  show_sell(1); 
	  screen->clear();
	  if (title_screen>=0)
	  {
	    image *tit=cash.img(title_screen);
	      tit->put_image(screen,screen->width()/2-tit->width()/2,
					      screen->height()/2-tit->height()/2);
	  }
	  inm->redraw();
	  fade_in(NULL,8);
	  eh->flush_screen(); 

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

static ico_button *load_icon(int num, int id, int x, int y, int &h, ifield *next, char *key)
{
  char name[20];
  char *base="newi";
  int a,b,c;
  sprintf(name,"%s%04d.pcx",base,num*3+1);
  a=cash.reg("art/icons.spe",name,SPEC_IMAGE,1);

  sprintf(name,"%s%04d.pcx",base,num*3+2);
  b=cash.reg("art/icons.spe",name,SPEC_IMAGE,1);

  sprintf(name,"%s%04d.pcx",base,num*3+3);
  c=cash.reg("art/icons.spe",name,SPEC_IMAGE,1);

  h=cash.img(a)->height();

  return new ico_button(x,y,id,b,b,a,c,next,-1,key);
}

ico_button *make_default_buttons(int x,int &y, ico_button *append_list)
{
  int h;
  int diff_on;

  if (DEFINEDP(symbol_value(l_difficulty)))
  {
    if (symbol_value(l_difficulty)==l_extreme)
      diff_on=3;
    else if (symbol_value(l_difficulty)==l_hard)
      diff_on=2;
    else if (symbol_value(l_difficulty)==l_easy)
      diff_on=0;
    else 
      diff_on=1;
  } else  diff_on=3;

  
  ico_button *start=load_icon(0,ID_START_GAME,x,y,h,NULL,"ic_start");                         y+=h;

  ico_switch_button *set=NULL;
  if (!main_net_cfg || (main_net_cfg->state!=net_configuration::SERVER && main_net_cfg->state!=net_configuration::CLIENT))
  {
    set=new ico_switch_button(x,y,ID_NULL,diff_on,
						 load_icon(3,ID_EASY,x,y,h,
						 load_icon(8,ID_MEDIUM,x,y,h,
					         load_icon(9,ID_HARD,x,y,h,
			                         load_icon(10,ID_EXTREME,x,y,h,NULL,"ic_extreme"),
				                  "ic_hard"),"ic_medium"),"ic_easy"),NULL);         y+=h;

  }

  ico_button *color=load_icon(4,ID_LIGHT_OFF,x,y,h,NULL,"ic_gamma");                          y+=h;  
  ico_button *volume=load_icon(5,ID_VOLUME,x,y,h,NULL,"ic_volume");                            y+=h;
  ico_button *sell=NULL;

  if (registered && prot)
  {
    sell=load_icon(11,ID_NETWORKING,x,y,h,NULL,"ic_networking");
    y+=h;  
  } else
  {
    sell=load_icon(2,ID_SHOW_SELL,x,y,h,NULL,"ic_sell");                           
    y+=h;  
  }
  ico_button *quit=load_icon(6,ID_QUIT,x,y,h,NULL,"ic_quit");                                y+=h;

  if (set)
  {
    start->next=set;
    set->next=color;
  }
  else start->next=color;


  color->next=volume;  
  if (sell)
  {
    volume->next=sell;  
    sell->next=quit;
  } else volume->next=quit;

  ico_button *list=append_list;

  if (append_list) 
  { 
    while (append_list->next) 
      append_list=(ico_button *)append_list->next; 
    append_list->next=start;
  } else list=start;
  
  return list;  
}


ico_button *make_conditional_buttons(int x,int &y)
{  
  ico_button *start_list=NULL;
  int h;
  if (current_level)       // should we include a return icon?
  {
    start_list=load_icon(7,ID_RETURN,x,y,h,NULL,"ic_return");                       y+=h;
  }
    
  
  ico_button *load;
  if (show_load_icon())
  { load= load_icon(1,ID_LOAD_PLAYER_GAME,x,y,h,NULL,"ic_load");                     y+=h;}
  else load=NULL;

  if (start_list) start_list->next=load;
  else start_list=load;

  return start_list;   
}

void main_menu()
{
  int y=yres/2-100;
  ico_button *list=make_conditional_buttons(xres-33,y);
  list=make_default_buttons(xres-33,y,list);


  input_manager *inm=new input_manager(screen,eh,list);
  inm->allow_no_selections();
  inm->clear_current();

  time_marker old_time;

  screen->add_dirty(0,0,319,199);

  int eoff=0,coff=0;
  event ev;

  int state=0,stop_menu=0;
  time_marker start;
  eh->flush_screen(); 
  do
  {
    time_marker new_time;

    if (eh->event_waiting())
    {
      do { eh->get_event(ev); } while (ev.type==EV_MOUSE_MOVE && eh->event_waiting()); 
      inm->handle_event(ev,NULL,eh);
      if (ev.type==EV_KEY && ev.key==JK_ESC)
        eh->push_event(new event(ID_QUIT,NULL));

      menu_handler(ev,inm);
      start.get_time();
      
      eh->flush_screen();
    }

    if (new_time.diff_time(&start)>10)
    {
      if (volume_window)
        start.get_time();
      else
      {
	if (!current_demo)
	{
	  void *d=make_find_symbol("demos");
	  if (DEFINEDP(symbol_value(d)))
	  current_demo=symbol_value(d);
	}
	if (current_demo)
	{
	  demo_man.set_state(demo_manager::PLAYING,lstring_value(CAR(current_demo)));
	  stop_menu=1;
	  current_demo=CDR(current_demo);
	}
      }
    }

    if (volume_window) stop_menu=0;  // can't exit with colume window open
    else if (main_net_cfg && main_net_cfg->restart_state()) stop_menu=1;
    else if (the_game->state==RUN_STATE) stop_menu=1;
    else if (ev.type==EV_MESSAGE)
    {
      if (ev.message.id==ID_START_GAME || ev.message.id==ID_RETURN) stop_menu=1;
      else if (ev.message.id==ID_QUIT)
      {
	if (confirm_quit()) stop_menu=1;
	  else { ev.type=EV_SPURIOUS;       start.get_time(); }
      }
    }

  } while (!stop_menu);

  delete inm;


  if (ev.type==EV_MESSAGE && ev.message.id==ID_QUIT)   // propogate the quit message
    the_game->end_session();


}


