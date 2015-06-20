#include "idle.hpp"
#include "system.h"
#include "game.hpp"
#include "dev.hpp"
#include "id.hpp"
#include "joy.hpp"
#include "timing.hpp"
#include "help.hpp"
#include "ability.hpp"
#include "cache.hpp"
#include "loader.hpp"
#include "lisp.hpp"
#include "monoprnt.hpp"
#include "jrand.hpp"
#include "config.hpp"
#include "light.hpp"
#include "scroller.hpp"
#include "dprint.hpp"
#include "nfserver.hpp"
#include "video.hpp"
#include "transp.hpp"
#include "clisp.hpp"
#include "guistat.hpp"
#include "menu.hpp"
#include "gamma.hpp"
#include "lisp_gc.hpp"
#include "demo.hpp"
#include "sbar.hpp"
#include "profile.hpp"
#include "compiled.hpp"
#include "lisp_gc.hpp"
#include "pmenu.hpp"
#include "timing.hpp"
#include "chat.hpp"
#include "demo.hpp"
#include "netcfg.hpp"
#include "specache.hpp"
#include "ant.hpp"
#include "netstat.hpp"


#ifdef __MAC__
//#include <profiler.h>  //prof
#include "macgame.hpp"
#include "macstat.hpp"
#endif

#include <ctype.h>
#include <setjmp.h>

#define SHIFT_RIGHT_DEFAULT 0
#define SHIFT_DOWN_DEFAULT 30

class game;
extern crc_manager *net_crcs;
extern void show_verinfo(int argc, char **argv);
game *the_game;
window_manager *eh=NULL;
int dev,shift_down=SHIFT_DOWN_DEFAULT,shift_right=SHIFT_RIGHT_DEFAULT;
double sum_diffs=1,total_diffs=12;
int total_active=0;
long map_xoff=0,map_yoff=0;
long current_vxadd,current_vyadd;
int frame_panic=0,massive_frame_panic=0;
int demo_start=0,idle_ticks=0;
int req_end=0;

extern palette *old_pal;
char **start_argv;
int start_argc;
int has_joystick=0;
char req_name[100];

int registered=0;

extern int PixMult;

extern uchar chatting_enabled;

extern int confirm_quit();

#ifdef __MAC__
extern char *macify_name(char *s);

#if 1
#include "atalk.hpp"
atalk_protocol atalk;

#else
#include "tcpip.hpp"

tcpip_protocol tcpip;
#endif

#endif


#if !defined (MAC_PROFILE)
#define StartTime(a)
#define DiffTime(a,b)
#else

// Timer code
#include <Timer.h> // kill me (us)

#define KJCOUNTERS 20
long kjcount[KJCOUNTERS];
char kjfie[KJCOUNTERS][256] = { };  // kill me

extern long StartTime(int cnt);         //kill me
extern long DiffTime(char *name, int cnt);   //kill me
extern unsigned lighting_loop_time;

long StartTime(int cnt) //kill me too
{
  UnsignedWide t;
  Microseconds(&t);
	
  return (kjcount[cnt] = t.lo);
}

long DiffTime(char *name, int cnt) //kill me too
{
  UnsignedWide t;
  Microseconds(&t);
	
  sprintf(kjfie[cnt],"%-16s %8d",name,t.lo-kjcount[cnt]);
	
  return (t.lo-kjcount[cnt]);

}
#endif

void fade_in(image *im, int steps);
void fade_out(int steps);

FILE *open_FILE(char *filename, char *mode)
{
  /*  char *prefix=get_filename_prefix() ? get_filename_prefix() : "",*c;
  
      if (get_save_filename_prefix)
      {
      for (c=mode;*c;c++) 
      if (c=='w' || c=='W') 
      {
      }
      } */
      
  
  char tmp_name[200];
  if (get_filename_prefix())
    sprintf(tmp_name,"%s%s",get_filename_prefix(),filename);
  else strcpy(tmp_name,filename);
#ifdef __MAC__
  macify_name(tmp_name);
#endif
  return fopen(tmp_name,mode);
}


void handle_no_space()
{
  char *no_space_msg= "\nYou are out of disk space or the game\n"
    "was unable to write to disk for some reason\n"
    "The game cannot continue, please check this out\n"
    "and try again.\n";
  if (eh)
  {
    jwindow *no_space=eh->new_window(0,0,-1,-1,
        new button(WINDOW_FRAME_LEFT,WINDOW_FRAME_TOP,ID_QUIT_OK,"Quit",
            new info_field(WINDOW_FRAME_LEFT,WINDOW_FRAME_TOP+eh->font()->height()*2,ID_NULL,
                no_space_msg,NULL)),"ERROR");
    event ev;
    do 
    { 
      eh->flush_screen();
      eh->get_event(ev);
    } while (ev.type!=EV_MESSAGE || ev.message.id!=ID_QUIT_OK);
    eh->close_window(no_space);

    close_graphics();
    exit(1);
  } else
  {
    dprintf("%s\n",no_space_msg);
    exit(0);
  }
}

void game::play_sound(int id, int vol, long x, long y)
{
  if (sound_avail&SFX_INITIALIZED)
  {
    if (vol<15) return ;
    if (!player_list) return ;

    ulong mdist=0xffffffff;
    view *cd=NULL;
    for (view *f=player_list;f;f=f->next)
    {
      if (f->local_player())
      {
	long cx=abs(f->x_center()-x),cy=abs(f->y_center()-y),d;
	if (cx<cy)
          d=cx+cy-(cx>>1);
	else d=cx+cy-(cy>>1);

	if (d<mdist)
	{
	  cd=f;
	  mdist=d;
	} 
      }
    }  
    if (mdist>500 || !cd) return ;
    if (mdist<100) mdist=0;
    else mdist-=100;

    int v=(400-mdist)*sfx_volume/400-(127-vol);
    if (v>0 && (sound_avail&SFX_INITIALIZED))
      cash.sfx(id)->play(v);
  }
}

int get_option(char *name)
{
  int i;
  for (i=1;i<start_argc;i++)
    if (!strcmp(start_argv[i],name))
      return i;
  return 0;
}


void make_screen_size(int w, int h)
{
  return ;
  /*
  for (view *f=player_list;f;f=f->next)
  {
    if (f->local_player())
    {
      if (w>=xres-1) w=xres-2;
      if (h>=yres-1) h=yres-2;
      f->suggest.cx1=(xres+1)/2-w/2;
      f->suggest.cx2=(xres+1)/2+w/2;
      f->suggest.cy1=(yres-31)/2+5-h/2;
      f->suggest.cy2=(yres-51)/2+5+h/2;
      f->suggest.shift_down=f->shift_down;
      f->suggest.shift_right=f->shift_right;
      f->suggest.pan_x=f->pan_x;
      f->suggest.pan_y=f->pan_y;

      f->suggest.send_view=1;
    }
  } */
}

void game::grow_views(int amount)
{
  return ;
  /*
  int undo=0;
  view *f=first_view;
  for (;f;f=f->next)
  {
    if (f->local_player())
    {
      f->suggest.cx1=(f->cx1-amount);
      f->suggest.cy1=f->cy1-amount/2;
      f->suggest.cx2=(f->cx2+amount);
      f->suggest.cy2=f->cy2+amount/2;
      f->suggest.shift_down=f->shift_down;
      f->suggest.shift_right=f->shift_right;
      f->suggest.pan_x=f->pan_x;
      f->suggest.pan_y=f->pan_y;

      f->suggest.send_view=1;
    }
  }


  for (f=first_view;f;f=f->next)  
  {
    if (f->local_player())
    {
      if (f->suggest.cx2-f->suggest.cx1<20 || f->suggest.cy2-f->suggest.cy1<15 || 
          f->suggest.cx1<0 || f->suggest.cy1<0) f->suggest.send_view=0;
      if (f->next && f->next->local_player() && f->suggest.cy2>=f->next->cy1) f->suggest.send_view=0;
    }
  } */
}

void game::pan(int xv, int yv) 
{ 
  first_view->pan_x+=xv; 
  first_view->pan_y+=yv; 
}

view *game::view_in(int mousex, int mousey)
{
  for (view *f=first_view;f;f=f->next)
    if (f->drawable() && mousex>=f->cx1 && mousey>=f->cy1 && mousex<=f->cx2 && mousey<=f->cy2)
      return f;
  return NULL;
}

int playing_state(int state)
{ 
  if (state==RUN_STATE || state==PAUSE_STATE) return 1;
  else return 0;
}

void game::ftile_on(int screenx, int screeny, long &x, long &y)
{
  mouse_to_game(screenx,screeny,x,y);
  x/=ftile_width();
  y/=ftile_height();
  /*  view *f=view_in(screenx,screeny);
      if (f)
      {
      x=((long)(screenx)-(long)f->cx1+f->xoff())/(long)f_wid;
      y=((long)(screeny)-(long)f->cy1+f->yoff())/(long)f_hi;
      }
      else
      {
      x=-1; 
      y=-1;
      }*/
}

void game::btile_on(int screenx, int screeny, long &x, long &y)
{
  view *f=view_in(screenx,screeny);
  if (f)
  {
    x=((long)(screenx)-(long)f->cx1+f->xoff()*bg_xmul/bg_xdiv)/(long)b_wid; 
    y=((long)(screeny)-(long)f->cy1+f->yoff()*bg_ymul/bg_ydiv)/(long)b_hi;
  }
  else
  {
    x=-1; 
    y=-1;
  }
}


void game::mouse_to_game(long x, long y, long &gamex, long &gamey, view *f)
{
  if (!f)
  {
    f=view_in(x,y);
    if (!f) f=player_list;  // if not in a view use the first on
  }

  if (f)
  {
    if (dev&MAP_MODE)
    {
      gamex=((x-(long)f->cx1)*ftile_width()/AUTOTILE_WIDTH+map_xoff*ftile_width());
      gamey=((y-(long)f->cy1)*ftile_height()/AUTOTILE_HEIGHT+map_yoff*ftile_height());
    } else
    {
      gamex=(x-(long)f->cx1+f->xoff());
      gamey=(y-(long)f->cy1+f->yoff());
    }
  }
}

void game::game_to_mouse(long gamex, long gamey, view *which, long &x, long &y)
{
  if (dev&MAP_MODE)
  {
    long x1,y1;
    if (dev&EDIT_MODE)
    {
      x1=map_xoff;
      y1=map_yoff;
    } else
    {
      if (which->focus)
      {
	x1=which->focus->x/ftile_width()-(which->cx2-which->cx1)/AUTOTILE_WIDTH/2;
	y1=which->focus->y/ftile_height()-(which->cy2-which->cy1)/AUTOTILE_HEIGHT/2;
      } else x1=y1=0;
    }
    if (x1<0) x1=0;
    if (y1<0) y1=0;

    x=gamex*AUTOTILE_WIDTH/ftile_width()-x1*AUTOTILE_WIDTH+which->cx1;
    if (x1>0)
      x-=((which->focus->x*AUTOTILE_WIDTH/ftile_width()) %AUTOTILE_WIDTH);

    y=gamey*AUTOTILE_HEIGHT/ftile_height()-y1*AUTOTILE_HEIGHT+which->cy1;
    if (y1>0)
      y-=((which->focus->y*AUTOTILE_HEIGHT/ftile_height()) %AUTOTILE_HEIGHT);
  }
  else
  {
    x=gamex-which->xoff()+which->cx1;
    y=gamey-which->yoff()+which->cy1;
  }
}

int window_state(int state)
{
  switch (state)
  {
    case RUN_STATE : 
    case PAUSE_STATE : 
    case JOY_CALB_STATE :
    { return 1; } break;

    case INTRO_START_STATE :
    case HELP_STATE : 
    case INTRO_MORPH_STATE :
    case MENU_STATE :
    case SCENE_STATE :
    { return 0; } break;
  }
  return 1;
}


// not global for now
extern void make_edit_mode_menu();
extern void kill_edit_mode_menu();

void game::set_state(int new_state) 
{
  int d=0;
  
  reset_keymap();                             // we think all the keys are up right now

  if (!playing_state(new_state) && playing_state(state))
  {
    switch_mode(VMODE_640x480);
  }

  if (playing_state(new_state) && !playing_state(state))
  {
    switch_mode(VMODE_320x200);
    recalc_local_view_space();

    if (first_view && first_view!=player_list)
    {
      while (first_view)
      {
        view *tmp=first_view;
        first_view=first_view->next;
        delete tmp;
      }
      first_view=old_view;
      old_view=NULL;
    }
    first_view=player_list;
    d=1;
  } else if (!playing_state(new_state) && (playing_state(state) || state==START_STATE))
  {
    if (player_list)    
    {
      first_view=new view(player_list->focus,NULL,-1);
      first_view->pan_x=player_list->xoff();
      first_view->pan_y=player_list->yoff();
    }
    else    
      first_view=new view(NULL,NULL,0);

    /*
    first_view->cx1=(xres+1)/2-155;
    first_view->cy1=(yres+1)/2-95;
    first_view->cx2=(xres+1)/2+155;
    if (total_weapons)
      first_view->cy2=(yres+1)/2+68;
    else
      first_view->cy2=(yres+1)/2+95;
      */
    
    d=1;
  }
  


  // switching to/from scene mode cause the screen size to change and the border to change
  // so we need to redraw.
  if (window_state(new_state) && !window_state(state))
    eh->show_windows();
  else if (!window_state(new_state) && window_state(state))
    eh->hide_windows();

  int old_state=state;
  state=new_state;

  pal->load();    // restore old palette

  if (playing_state(state) &&  !(dev&EDIT_MODE))
    eh->set_mouse_shape(cash.img(c_target)->copy(),8,8);
  else
    eh->set_mouse_shape(cash.img(c_normal)->copy(),1,1);

	if (playing_state(new_state) && (dev&EDIT_MODE))
		make_edit_mode_menu();
	else
		kill_edit_mode_menu();
	
  if (old_state==SCENE_STATE && new_state!=SCENE_STATE)
  {
    d=1;
    scene_director.set_abort(0);   // don't skip any more scene stuff
  }
  else if (new_state==SCENE_STATE && old_state!=SCENE_STATE)
    d=1;
  
  if (d)
    draw(state==SCENE_STATE);

  dev_cont->set_state(new_state);
}

void game::joy_calb(event &ev)
{
  if (joy_win)   // make sure the joy stick calibration window is open
  {
    
    if (ev.type==EV_SPURIOUS)   // spurious means we should update your status
    {
      int b1,b2,b3=0,x,y;
      joy_status(b1,b2,b2,x,y);
      int but=b1|b2|b3;
      if (x>0) x=1; else if (x<0) x=-1;
      if (y>0) y=1; else if (y<0) y=-1;
      if (but) but=1;
      int dx=WINDOW_FRAME_LEFT+20,dy=WINDOW_FRAME_TOP+5;
      image *jim=cash.img(joy_picts[but*9+(y+1)*3+x+1]);
      joy_win->screen->bar(dx,dy,dx+jim->width()+6,dy+jim->height()+6,eh->black());
      jim->put_image(joy_win->screen,dx+3,dy+3);

      if (but)
        joy_calibrate();
    } else if (ev.type==EV_MESSAGE && ev.message.id==JOY_OK)
    {
      eh->close_window(joy_win);
      joy_win=NULL;
      set_state(MENU_STATE);
    }        
  }
}

void game::menu_select(event &ev)
{
  state=DEV_MOUSE_RELEASE;
  if (top_menu)
  {
    /*    eh->push_event(new event(men_mess[((pick_list *)ev.message.data)->get_selection()],NULL));
          eh->close_window(top_menu);
          top_menu=NULL;*/
  }
}


void game::show_help(char *st)
{
  strcpy(help_text,st);
  help_text_frames=0;  
  refresh=1;
}

void game::draw_value(image *screen, int x, int y, int w, int h, int val, int max)
{
  screen->bar(x,y,x+w-1,y+h,eh->dark_color());
  screen->bar(x,y+1,x+w*val/max,y+h-1,eh->bright_color());  
}


void game::set_level(level *nl)
{
  if (current_level) 
    delete current_level;  
  current_level=nl;
}

void game::load_level(char *name)
{
  if (current_level) 
    delete current_level;

  bFILE *fp=open_file(name,"rb");

  if (fp->open_failure())
  {
    delete fp;
    current_level=new level(100,100,name);
    char msg[100];
    sprintf(msg,symbol_str("no_file"),name);
    show_help(msg);
  }
  else
  {			      
    spec_directory sd(fp);  

#if 0
    if (state != RUN_STATE)
    {
      // clear screen
      image blank(2,2); blank.clear();
      fade_out(16);
      eh->set_mouse_shape(blank.copy(),0,0);      // don't show mouse
      fade_in(cash.img(cdc_logo),16);
    }
#endif

    spec_entry *e=sd.find("Copyright 1995 Crack dot Com, All Rights reserved"); 
    if (!e)
    { 
      the_game->show_help(symbol_str("missing_c"));
      current_level=new level(100,100,"untitled");
      the_game->need_refresh();
    }
    else 
      current_level=new level(&sd,fp,name);
    delete fp;
  }
  
  base->current_tick=(current_level->tick_counter()&0xff); 

  current_level->level_loaded_notify();
  the_game->help_text_frames=-1;  

}

int game::done() 
{ 
  return finished || (main_net_cfg && main_net_cfg->restart_state());

}

void game::end_session()
{
  finished=1;
  if (main_net_cfg)
  {
    delete main_net_cfg;
    main_net_cfg=NULL;
  }
}

void game::put_block_fg(int x, int y, trans_image *im)
{
  for (view *f=first_view;f;f=f->next)
  {
    if (f->drawable())
    {
      int xoff=f->xoff(),yoff=f->yoff(),viewx1=f->cx1,viewy1=f->cy1,viewx2=f->cx2,viewy2=f->cy2;
      if (xoff/ftile_width()>x || xoff/ftile_width()+(viewx2-viewx1)/ftile_width()+1<x ||
	  yoff/ftile_height()>y || yoff/ftile_height()+(viewy2-viewy1)/ftile_height()+1<y) return;
      short cx1,cy1,cx2,cy2;
      screen->get_clip(cx1,cy1,cx2,cy2);
      screen->set_clip(viewx1,viewy1,viewx2,viewy2);
      im->put_image(screen,(x-xoff/ftile_width())*ftile_width()+viewx1-xoff%ftile_width(),
          (y-yoff/ftile_height())*ftile_height()+viewy1-yoff%ftile_height());
      screen->set_clip(cx1,cy1,cx2,cy2);
    }
  }
}

void game::put_block_bg(int x, int y, image *im)
{
  for (view *f=first_view;f;f=f->next)
  {
    if (f->drawable())
    {
      int xoff=f->xoff(),yoff=f->yoff(),viewx1=f->cx1,viewy1=f->cy1,viewx2=f->cx2,viewy2=f->cy2;
      int xo=xoff*bg_xmul/bg_xdiv;
      int yo=yoff*bg_ymul/bg_ydiv;
      
      if (xo/btile_width()>x || xo/btile_width()+(viewx2-viewx1)/btile_width()+1<x ||
	  yo/btile_height()>y || yo/btile_height()+(viewy2-viewy1)/btile_height()+1<y) return;
      short cx1,cy1,cx2,cy2;
      screen->get_clip(cx1,cy1,cx2,cy2);
      screen->set_clip(viewx1,viewy1,viewx2,viewy2);
      im->put_image(screen,(x-xo/btile_width())*btile_width()+viewx1-xo%btile_width(),
          (y-yo/btile_height())*btile_height()+viewy1-yo%btile_height(),0);
      screen->set_clip(cx1,cy1,cx2,cy2);
    }
  }
}

int need_delay=1;

void game::dev_scroll()
{
  need_delay=0;
  if (dev)
  {
    int xmargin,ymargin;
    if (xres>400)
    {
      xmargin=20;
      ymargin=10;
    }
    else 
    {
      xmargin=10;
      ymargin=5;
    }

    int xs,ys;
    if (mousex<xmargin &&  dev_cont->ok_to_scroll()) xs=-18;
    else if (mousex>(screen->width()-xmargin) &&  dev_cont->ok_to_scroll()) xs=18;
    else if (eh->key_pressed(JK_LEFT) && !last_input && !dev_cont->need_arrows())
      xs=-18;
    else if (eh->key_pressed(JK_RIGHT) && !last_input && !dev_cont->need_arrows())
      xs=18;
    else xs=0;
	     

    if (mousey<ymargin && dev_cont->ok_to_scroll()) ys=-18;
    else if (mousey>(screen->height()-ymargin) &&  dev_cont->ok_to_scroll()) ys=18;
    else if (eh->key_pressed(JK_UP) && !last_input)
      ys=-18;
    else if (eh->key_pressed(JK_DOWN) && !last_input)
      ys=18;
    else ys=0;

    
    if (xs || ys)
    {
      need_delay=1;
      if (dev&MAP_MODE)
      {
        map_xoff+=xs/2;
        map_yoff+=ys/2;
        if (map_xoff<0) map_xoff=0;
        if (map_yoff<0) map_yoff=0;
      } 
      else
      { 
        for (view *v=first_view;v;v=v->next)
        {
          if (xs>=0 || v->xoff()>0)
            v->pan_x+=xs;
          if (ys>=0 || v->yoff()>0)
            v->pan_y+=ys;
        }
      }
      refresh=1;
    }
  }
}

void remap_area(image *screen, int x1, int y1, int x2, int y2, uchar *remap)
{
  uchar *sl=(uchar *)screen->scan_line(y1)+x1;
  int x,y,a=screen->width()-(x2-x1+1);
  uchar c;
  for (y=y1;y<=y2;y++)
  {
    for (x=x1;x<=x2;x++)
    {
      c=*sl;
      *(sl++)=remap[c];
    }
    sl+=a;
  }
}

void post_render()
{
  if (DEFINEDP(symbol_function(l_post_render)))
  {
    screen->dirt_off();
    clear_tmp();
    eval_function((lisp_symbol *)l_post_render,NULL);
    clear_tmp();
    screen->dirt_on();
  }
}
  
void game::draw_map(view *v, int interpolate)
{
  foretile *ft;
  backtile *bt;
  int x1,y1,x2,y2,x,y,xo,yo,nxoff,nyoff;
  short cx1,cy1,cx2,cy2;
  
  StartTime(6); //kill me
  
  screen->get_clip(cx1,cy1,cx2,cy2);

  if (!current_level || state==MENU_STATE)
  {
    if (title_screen>=0)
    {
      if (state==SCENE_STATE)
        screen->set_clip(v->cx1,v->cy1,v->cx2,v->cy2);        
      image *tit=cash.img(title_screen);
      tit->put_image(screen,screen->width()/2-tit->width()/2,
          screen->height()/2-tit->height()/2);
      if (state==SCENE_STATE)
        screen->set_clip(cx1,cy1,cx2,cy2);
      eh->flush_screen();
    }   
    return ;
  }

  refresh=0;  


  // save the dirty rect routines some work by markinging evrything in the 
  // view area dirty alreadt

  if (small_render)
    screen->add_dirty(v->cx1,v->cy1,(v->cx2-v->cx1+1)*2+v->cx1,v->cy1+(v->cy2-v->cy1+1)*2);    
  else
    screen->add_dirty(v->cx1,v->cy1,v->cx2,v->cy2);    

  if (v->draw_solid!=-1)      // fill the screen and exit..
  {
    int c=v->draw_solid;
    for (int y=v->cy1;y<=v->cy2;y++)
      memset(screen->scan_line(y)+v->cx1,c,v->cx2-v->cx1+1);
    v->draw_solid=-1;
    return ;
  }

  long old_cx1,old_cy1,old_cx2,old_cy2;   // if we do a small render, we need to restore these
  image *old_screen=NULL;
  if (small_render && (dev&DRAW_LIGHTS))  // cannot do this if we skip lighting
  {
    old_cx1=v->cx1;
    old_cy1=v->cy1;
    old_cx2=v->cx2;
    old_cy2=v->cy2;

    /*    v->cx1=0;
    v->cy1=0;
    v->cx2=small_render->width()-1;
    v->cy2=small_render->height()-1; */

    old_screen=screen;
    screen=small_render;
  } else
    screen->dirt_off();



  //  long max_xoff=(current_level->foreground_width()-1)*ftile_width()-(v->cx2-v->cx1+1);
  //  long max_yoff=(current_level->foreground_height()-1)*ftile_height()-(v->cy2-v->cy1+1); 

  long xoff,yoff;
  if (interpolate)
  {
    xoff=v->interpolated_xoff();
    yoff=v->interpolated_yoff();
  } else
  {
    xoff=v->xoff();
    yoff=v->yoff();
  }

  //  if (xoff>max_xoff) xoff=max_xoff;
  //  if (yoff>max_yoff) yoff=max_yoff;  

  current_vxadd=xoff-v->cx1;
  current_vyadd=yoff-v->cy1;

  screen->set_clip(v->cx1,v->cy1,v->cx2,v->cy2);

  nxoff=xoff*bg_xmul/bg_xdiv;
  nyoff=yoff*bg_ymul/bg_ydiv;

  //  long max_bg_xoff=(current_level->background_width())*btile_width()-(v->cx2-v->cx1+1);
  //  long max_bg_yoff=(current_level->background_height())*btile_height()-(v->cy2-v->cy1+1);
  //  if (nxoff>max_bg_xoff) nxoff=max_xoff;
  //  if (nyoff>max_bg_yoff) nyoff=max_yoff;  
  
  
  x1=nxoff/btile_width(); y1=nyoff/btile_height();
  x2=x1+(v->cx2-v->cx1+btile_width())/btile_width();
  y2=y1+(v->cy2-v->cy1+btile_height())/btile_height();


  xo=v->cx1-nxoff%btile_width();
  yo=v->cy1-nyoff%btile_height();
  
  int xinc,yinc,draw_x,draw_y;


  DiffTime("      PreDraw",6);//kill me
  StartTime(7); //kill me
  if (!(dev & MAP_MODE) && (dev & DRAW_BG_LAYER))
  {
    xinc=btile_width();  
    yinc=btile_height();  
    
    int bh=current_level->background_height(),bw=current_level->background_width();
    ushort *bl;
    for (draw_y=yo,y=y1;y<=y2;y++,draw_y+=yinc)
    {
      if (y>=bh)
        bl=NULL;
      else
        bl=current_level->get_bgline(y)+x1;

      for (x=x1,draw_x=xo;x<=x2;x++,draw_x+=xinc)
      {
        if (x<bw && y<bh)
        {
          bt=get_bg(*bl);
          bl++;
        }
        else bt=get_bg(0);

        bt->im->put_image(screen,draw_x,draw_y); 
        //        if (!(dev & EDIT_MODE) && bt->next)
        //	  current_level->put_bg(x,y,bt->next);
      }
    }
  }
  //  if (!(dev&EDIT_MODE))
  //    server_check();

  uchar rescan=0;  

  int fw,fh;

  if (dev&MAP_MODE)
  {
    fw=AUTOTILE_WIDTH;
    fh=AUTOTILE_HEIGHT;
    if (dev&EDIT_MODE)
    {
      x1=map_xoff;
      y1=map_yoff;
    } else
    {
      if (v->focus)
      {
        x1=v->focus->x/ftile_width()-(v->cx2-v->cx1)/fw/2;
        y1=v->focus->y/ftile_height()-(v->cy2-v->cy1)/fh/2;
      } else x1=y1=0;
    }
    if (x1>0)
      xo=v->cx1-((v->focus->x*fw/ftile_width()) %fw);
    else xo=v->cx1;
    if (y1>0)
      yo=v->cy1-((v->focus->y*fh/ftile_height()) %fh);
    else yo=v->cy1;
  } else
  {
    fw=ftile_width();
    fh=ftile_height();
    x1=(xoff)/fw; y1=(yoff)/fh;
    xo=v->cx1-xoff%fw;
    yo=v->cy1-yoff%fh;

  }
  if (x1<0) x1=0;
  if (y1<0) y1=0;
  
  x2=x1+(v->cx2-v->cx1+fw)/fw;
  y2=y1+(v->cy2-v->cy1+fh)/fh;
  if (x2>=current_level->foreground_width())
    x2=current_level->foreground_width()-1;
  if (y2>=current_level->foreground_height())
    y2=current_level->foreground_height()-1;


  xinc=fw;
  yinc=fh;

  DiffTime("      BG/MAP",7);//kill me
  StartTime(8); //kill me

  if (dev & DRAW_FG_LAYER)
  {
    short ncx1,ncy1,ncx2,ncy2;
    screen->get_clip(ncx1,ncy1,ncx2,ncy2);

    int scr_w=screen->width();
    if (dev&MAP_MODE)
    {
      if (dev&EDIT_MODE)
        screen->clear(eh->bright_color());
      else
        screen->clear(eh->black());
      for (y=y1,draw_y=yo;y<=y2;y++,draw_y+=yinc)
      {
        if (!(draw_y<ncy1 ||draw_y+yinc>=ncy2))
        {
          ushort *cl=current_level->get_fgline(y)+x1;
          uchar *sl1=screen->scan_line(draw_y)+xo;
          for (x=x1,draw_x=xo;x<=x2;x++,cl++,sl1+=xinc,draw_x+=xinc)
          {
            if (!(draw_x<ncx1 || draw_x+xinc>=ncx2))
            {
              int fort_num;
              //	      if (*cl&0x8000 || (dev&EDIT_MODE))
              fort_num=fgvalue(*cl);
              //	      else fort_num=0;
			
              uchar *sl2=get_fg(fort_num)->micro_image->scan_line(0);
              uchar *sl3=sl1;
              memcpy(sl3,sl2,AUTOTILE_WIDTH); sl2+=AUTOTILE_WIDTH; sl3+=scr_w;
              memcpy(sl3,sl2,AUTOTILE_WIDTH); sl2+=AUTOTILE_WIDTH; sl3+=scr_w;
              memcpy(sl3,sl2,AUTOTILE_WIDTH);
            }
          }
        }
      }

      if (dev&EDIT_MODE)
        current_level->draw_areas(v);
    } else
    {

      int fg_h=current_level->foreground_height(),fg_w=current_level->foreground_width();
      
      for (y=y1,draw_y=yo;y<=y2;y++,draw_y+=yinc)
      {
	
        ushort *cl;
        if (y<fg_h)
          cl=current_level->get_fgline(y)+x1;
        else cl=NULL;
        uchar *sl1=draw_y<ncy1 ? 0 : screen->scan_line(draw_y)+xo;
			
        for (x=x1,draw_x=xo;x<=x2;x++,draw_x+=xinc,cl++,sl1+=xinc)
        {
          if (x<fg_w && y<fg_h)
          {
            if (above_tile(*cl))
              rescan=1;
            else
            {
              int fort_num=fgvalue(*cl);	  
              if (fort_num!=BLACK)
              {
                if (draw_y<ncy1 || draw_y+yinc>=ncy2 || draw_x<ncx1 || draw_x+xinc>=ncx2)
                  get_fg(fort_num)->im->put_image(screen,draw_x,draw_y);
                else
                  get_fg(fort_num)->im->put_image_offseted(screen,sl1);
			
                if (!(dev & EDIT_MODE))
                  *cl|=0x8000;      // mark as has-been-seen
              }	
            }
          }
        }
      }  
    }
    /*        if (dev==0)
              current_level->put_fg(x,y,ft->next);  */       
  }
  
  DiffTime("      FG",8);//kill me

  //  if (!(dev&EDIT_MODE))
  //    server_check();

  long ro=rand_on;
  if (dev & DRAW_PEOPLE_LAYER)
  {
    if (interpolate)
      current_level->interpolate_draw_objects(v);
    else
      current_level->draw_objects(v);
  }

  //  if (!(dev&EDIT_MODE))
  //    server_check();

  StartTime(9); //kill me
  if (!(dev&MAP_MODE))
  {

    int fg_h=current_level->foreground_height(),fg_w=current_level->foreground_width();

    if (dev & DRAW_FG_LAYER && rescan)
    {
      for (y=y1,draw_y=yo;y<=y2;y++,draw_y+=yinc)
      {
        ushort *cl=current_level->get_fgline(y)+x1;
        for (x=x1,draw_x=xo;x<=x2;x++,draw_x+=xinc,cl++)
        {
          if (above_tile(*cl))
          {
            int fort_num=fgvalue(*cl);	  
            if (fort_num!=BLACK)
            {
              if (dev & DRAW_BG_LAYER)
                get_fg(fort_num)->im->put_image(screen,draw_x,draw_y);
              else
                get_fg(fort_num)->im->put_image_filled(screen,draw_x,draw_y,0);
			
              if (!(dev & EDIT_MODE))
                current_level->mark_seen(x,y);
              else
              {
                screen->line(draw_x,draw_y,draw_x+xinc,draw_y+yinc,eh->bright_color());
                screen->line(draw_x+xinc,draw_y,draw_x,draw_y+yinc,eh->bright_color());
              }
            }	
          }
        }
      }   
    }

    
    if (dev & DRAW_FG_BOUND_LAYER)
    {
      int b=eh->bright_color();
      int fg_h=current_level->foreground_height(),fg_w=current_level->foreground_width();

      for (y=y1,draw_y=yo;y<=y2;y++,draw_y+=yinc)
      {
        ushort *cl;
        if (y<fg_h)
          cl=current_level->get_fgline(y)+x1;
        else cl=NULL;
        for (x=x1,draw_x=xo;x<=x2;x++,draw_x+=xinc,cl++)
        {
          if (x<fg_w && y<fg_h)
          {
            int fort_num=fgvalue(*cl);	  
            if (fort_num!=BLACK)
            {
              point_list *p=get_fg(fort_num)->points;
              uchar *d=p->data;	
              if (p->tot)
              {
                for (int i=1;i<p->tot;i++)
                {
                  d+=2;
                  screen->line(draw_x+*(d-2),draw_y+*(d-1),draw_x+*d,draw_y+*(d+1),b);
                }
                screen->line(draw_x+*d,draw_y+*(d-1),draw_x+p->data[0],draw_y+p->data[1],b);
              }
            }
          }
        }
      }
    }

    //    if (!(dev&EDIT_MODE))
    //      server_check();

    if (dev & DRAW_HELP_LAYER)
    {
      if (help_text_frames>=0)
      {
        int color;
				
        if (help_text_frames<10)
          color=2;
        else
          color=2+(help_text_frames-10);
				
        int x1=v->cx1,
          y1=v->cy2-(eh->font()->height()+5),
          x2=v->cx2,
          y2=v->cy2-1;

			
        remap_area(screen,x1,y1,x2,y2,white_light+40*256);
        screen->bar(x1,y1,x2,y1,color);
        screen->bar(x1,y2,x2,y2,color);
			
        eh->frame_font()->put_string(screen,x1+5,y1+5,
            help_text,color);
        if (color>30)
          help_text_frames=-1;      
        else help_text_frames++;
	
      }    
    }
    
    if (dev_cont)
      dev_cont->dev_draw(v);  
    if (cash.in_use())
      cash.img(vmm_image)->put_image(screen,v->cx1,v->cy2-cash.img(vmm_image)->height()+1);  

    if (dev&DRAW_LIGHTS)
    {  
      if (small_render)
      {
        double_light_screen(screen,xoff,yoff,white_light,v->ambient,old_screen,old_cx1,old_cy1);
			
        /*        v->cx1=old_cx1;
        v->cy1=old_cy1;
        v->cx2=old_cx2;
        v->cy2=old_cy2; */
        screen=old_screen;
      } else
      {      
        screen->dirt_on();
        if (xres*yres<=64000)
          light_screen(screen,xoff,yoff,white_light,v->ambient);
        else light_screen(screen,xoff,yoff,white_light,63);            // no lighting for hi-rez
      }

    } else 
      screen->dirt_on();



  }  else
    screen->dirt_on();

  DiffTime("      Other",9); //kill me

  StartTime(15); // kill me
  rand_on=ro;                // restore random start in case in draw funs moved it
  // ... not every machine will draw the same thing

  post_render();

  screen->set_clip(cx1,cy1,cx2,cy2);
    



  if (playing_state(state))        // draw stuff outside the clipping region
    v->draw_character_damage();

  if (profiling())
    profile_update();

  sbar.draw_update();
  DiffTime("      Post",15); //kill me
}

void game::put_fg(int x, int y, int type)
{ 
  if (current_level->get_fg(x,y)!=type)
  {
    current_level->put_fg(x,y,type);
    for (view *f=first_view;f;f=f->next)
      if (f->drawable())
        draw_map(f);    
    /*    put_block_bg(x,y,get_bg(current_level->get_bg(x/ASPECT,y/ASPECT))->im);
          if (type>BLACK)
          put_block_fg(x,y,get_fg(type)->im); */
  }
}

void game::put_bg(int x, int y, int type)
{
  if (current_level->get_bg(x,y)!=type)
  {
    current_level->put_bg(x,y,type);
    for (view *f=first_view;f;f=f->next)
      if (f->drawable())
        draw_map(f);    
    /*    put_block_bg(x,y,get_bg(type)->im);
          if (current_level->get_fg(x,y)>BLACK) 
          put_block_fg(x,y,get_fg(current_level->get_fg(x,y))->im);*/
  }
}

int game::in_area(event &ev, int x1, int y1, int x2, int y2)
{
  return (last_demo_mx>=x1 && last_demo_mx<=x2 &&
      last_demo_my>=y1 && last_demo_my<=y2);
}

void game::request_level_load(char *name)
{
  strcpy(req_name,name);
}

extern int start_doubled;

void fade_in(image *im, int steps)
{
  palette *old_pal=pal->copy();
  int i;
  if (im)
  {
    screen->clear();
    im->put_image(screen,(xres+1)/2-im->width()/2,(yres+1)/2-im->height()/2);
  }

  for (i=0;i<steps;i++)
  {
    uchar *sl1=(uchar *)pal->addr();    
    uchar *sl2=(uchar *)old_pal->addr();    
    int j;
    int r,g,b;
    int v=(i+1)*256/steps;
    for (j=0;j<256;j++)
    {
      *(sl1)=((int)*(sl2))*v/256;  sl1++; sl2++;
      *(sl1)=((int)*(sl2))*v/256;  sl1++; sl2++;
      *(sl1)=((int)*(sl2))*v/256;  sl1++; sl2++;
    }
    pal->load();
    eh->flush_screen();
    milli_wait(25);
  }
  delete pal;
  pal=old_pal;
}

void fade_out(int steps)
{
  palette *old_pal=pal->copy();
  int i;
  for (i=0;i<steps;i++)
  {
    uchar *sl1=(uchar *)pal->addr();    
    uchar *sl2=(uchar *)old_pal->addr();    
    int j;
    int r,g,b;
    int v=(steps-i)*256/steps;
    for (j=0;j<256;j++)
    {
      *(sl1)=((int)*(sl2))*v/256;  sl1++; sl2++;
      *(sl1)=((int)*(sl2))*v/256;  sl1++; sl2++;
      *(sl1)=((int)*(sl2))*v/256;  sl1++; sl2++;
    }
    pal->load();
    eh->flush_screen();
    milli_wait(25);
  }
  screen->clear();
  eh->flush_screen();
  delete pal;
  pal=old_pal;
  
  pal->load();
}

int text_draw(int y, int x1, int y1, int x2, int y2, char *buf, JCFont *font, uchar *cmap, char color);

void do_title()
{
  if (cdc_logo!=-1)
  {
    if (sound_avail&MUSIC_INITIALIZED)
    {
      if (current_song) { current_song->stop(); delete current_song; }
      current_song=new song("music/intro.hmi");
      current_song->play(music_volume);
    }

    void *logo_snd=symbol_value(make_find_symbol("LOGO_SND"));

    if (DEFINEDP(logo_snd) && (sound_avail&SFX_INITIALIZED))
      cash.sfx(lnumber_value(logo_snd))->play(sfx_volume);

    image blank(2,2); blank.clear();
    eh->set_mouse_shape(blank.copy(),0,0);      // don't show mouse
    fade_in(cash.img(cdc_logo),32);
    
    milli_wait(900);

    void *space_snd=symbol_value(make_find_symbol("SPACE_SND"));

    fade_out(32);
    milli_wait(300);

    int i,abort=0;
    char *str=lstring_value(eval(make_find_symbol("plot_start")));
    
    bFILE *fp=open_file("art/smoke.spe","rb");
    if (!fp->open_failure())
    {
      spec_directory sd(fp);
      palette *old_pal=pal;
      pal=new palette(sd.find(SPEC_PALETTE),fp);
      pal->shift(1);

      image *gray=new image(sd.find("gray_pict"),fp);
      image *smoke[5];

      char nm[20];
      for (i=0;i<5;i++)
      {
        sprintf(nm,"smoke%04d.pcx",i+1);
        smoke[i]=new image(sd.find(nm),fp);
      }

      screen->clear();
      pal->load();

      int dx=(xres+1)/2-gray->width()/2,dy=(yres+1)/2-gray->height()/2;
      gray->put_image(screen,dx,dy);
      smoke[0]->put_image(screen,dx+24,dy+5);

      fade_in(NULL,16);
      uchar cmap[32];
      for (i=0;i<32;i++)
        cmap[i]=pal->find_closest(i*256/32,i*256/32,i*256/32);

      event ev; ev.type=EV_SPURIOUS;
      time_marker start;


      for (i=0;i<600 && ev.type!=EV_KEY && ev.type!=EV_MOUSE_BUTTON;i++)
      {
        gray->put_part(screen,dx,dy,0,0,319,199);
        smoke[i%5]->put_image(screen,dx+65,dy+70);
        text_draw(205-i,dx+15,dy,dx+320-15,dy+199,str,eh->font(),cmap,eh->bright_color());
        eh->flush_screen();
        time_marker now;
        while (now.diff_time(&start)<0.15) 
          now.get_time();
        start.get_time();
			
        while (eh->event_waiting() && ev.type!=EV_KEY) eh->get_event(ev);
        if ((i%5)==0 && DEFINEDP(space_snd) && (sound_avail&SFX_INITIALIZED))
          cash.sfx(lnumber_value(space_snd))->play(sfx_volume*90/127);
      }

      the_game->reset_keymap();

      fade_out(16);

      for (i=0;i<5;i++) delete smoke[i];
      delete gray;

      delete pal;
      pal=old_pal;
    }
    delete fp;

    for (i=0;i<100 && !abort;i++)
    {
      
    }



    if (title_screen>=0)
      fade_in(cash.img(title_screen),32);

    eh->set_mouse_shape(cash.img(c_normal)->copy(),1,1);
  }
}

extern int start_edit;

void game::request_end()
{
  req_end=1;
}

extern void fast_load_start_recording(char *name);
extern void fast_load_stop_recording();
extern void fast_load_start_reloading(char *name);
extern void fast_load_stop_reloading();

game::game(int argc, char **argv)
{
  int i;
  req_name[0]=0;
  bg_xmul=bg_ymul=1;
  bg_xdiv=bg_ydiv=8;
  last_input=NULL;
  current_level=NULL;
  refresh=1;  
  the_game=this;  
  top_menu=joy_win=NULL;
  old_view=first_view=NULL;
  nplayers=1;

  help_text_frames=-1;  
  strcpy(help_text,"");

  for (i=1;i<argc;i++)
    if (!strcmp(argv[i],"-no_delay"))
    {
      no_delay=1;
      dprintf("Frame delay off (-nodelay)\n");
    }
  
  image_init();  
  zoom=15;  
  no_delay=0;

  if (get_option("-use_joy"))  
  {
    has_joystick=joy_init(argc,argv);
    dprintf("Joystick : ");
    if (has_joystick) dprintf("detected\n");
    else dprintf("not detected\n");
  }
  else has_joystick=0;

  //	fast_load_start_recording("fastload.dat");
  load_data(argc,argv);  
  //	fast_load_stop_recording();

  get_key_bindings();


  reset_keymap();                   // we think all the keys are up right now
  finished=0;

  calc_light_table(pal);

  if (current_level==NULL && net_start())  // if we joined a net game get level from server
  {
    if (!request_server_entry())
    {
      exit(0);
    }
    net_reload();
    //    load_level(NET_STARTFILE); 
  }


  if (get_option("-2") && (xres<639 || yres<399))
  {
    close_graphics();
    dprintf("Resolution must be > 640x400 to use -2 option\n");    
    exit(0);
  }
  pal->load();
  
  recalc_local_view_space();   // now that we know what size the screen is...

  dark_color=get_color(cash.img(window_colors)->pixel(2,0));
  bright_color=get_color(cash.img(window_colors)->pixel(0,0));
  med_color=get_color(cash.img(window_colors)->pixel(1,0));

  morph_dark_color=get_color(cash.img(window_colors)->pixel(2,1));
  morph_bright_color=get_color(cash.img(window_colors)->pixel(0,1));
  morph_med_color=get_color(cash.img(window_colors)->pixel(1,1));
  morph_sel_frame_color=pal->find_closest(255,255,0);
  light_connection_color=morph_sel_frame_color;

  if (NILP(symbol_value(l_default_font)))
  {
    dprintf("No font defined, set symbol default-font to an image name\n");
    exit(0);
  }

  int font_pict=big_font_pict;
  
  if (console_font_pict==-1) console_font_pict=font_pict;
  game_font=new JCFont(cash.img(big_font_pict));
  window_font=new JCFont(cash.img(small_font_pict));

  console_font=new JCFont(cash.img(console_font_pict));

  eh=new window_manager(screen,pal,bright_color,
      med_color,
      dark_color,
      game_font);  
  idle_man=new idle_manager;


  eh->set_frame_font(window_font);

  chat=new chat_console(eh,console_font,50,6);

  if (!eh->has_mouse())
  {
    close_graphics();
    image_uninit();
    dprintf("No mouse driver detected, please rectify.\n");
    exit(0);
  }

  gamma_correct(pal);

  if (main_net_cfg==NULL || (main_net_cfg->state!=net_configuration::SERVER &&
      main_net_cfg->state!=net_configuration::CLIENT))
  {
    if (!start_edit && !net_start())
      do_title();
  } else if (main_net_cfg && main_net_cfg->state==net_configuration::SERVER)
  {
    the_game->load_level(level_file);
    start_running=1;
  }
    
  dev|= DRAW_FG_LAYER | DRAW_BG_LAYER | DRAW_PEOPLE_LAYER | DRAW_HELP_LAYER | DRAW_LIGHTS | DRAW_LINKS;

  if (dev & EDIT_MODE)
    set_frame_size(0);
  //  do_intro();
  state=START_STATE;         // first set the state to one that has windows

  if (start_running)
    set_state(RUN_STATE);
  else
  {
    screen->clear();  
    if (title_screen>=0)
    {
      image *tit=cash.img(title_screen);
      tit->put_image(screen,screen->width()/2-tit->width()/2,
          screen->height()/2-tit->height()/2);
    }   
    set_state(MENU_STATE);   // then go to menu state so windows will turn off
  }
}



time_marker *led_last_time=NULL,*fps_mark_start=NULL;
double avg_fps=15.0,possible_fps=15.0;

void game::toggle_delay()
{
  no_delay=!no_delay;
  if (no_delay)
    show_help(symbol_str("delay_off"));
  else show_help(symbol_str("delay_on"));
  avg_fps=possible_fps=15.0;
}

void game::show_time()
{
  if (first_view && fps_on)
  {
    char str[10];
    sprintf(str,"%d",(long)(avg_fps*10.0));
    console_font->put_string(screen,first_view->cx1,first_view->cy1,str);

    sprintf(str,"%d",total_active);
    console_font->put_string(screen,first_view->cx1,first_view->cy1+10,str);
  }

#ifdef MAC_PROFILE
  for (int i=0; i<KJCOUNTERS; i++)
    console_font->put_string(screen,10,8*i+8,kjfie[i]); // kill me
#endif
}

void game::update_screen()
{
  if (state==HELP_STATE)
    draw_help();
  else if (current_level)
  {    
    if (!(dev & EDIT_MODE) || refresh)    
    {    
      view *f=first_view;
      current_level->clear_active_list();
      for (;f;f=f->next)
      {
        if (f->focus)           
        {
          int w,h;
			
          w=(f->cx2-f->cx1+1);
          h=(f->cy2-f->cy1+1);
			
          total_active+=current_level->add_drawables(f->xoff()-w/4,f->yoff()-h/4,
              f->xoff()+w+w/4,f->yoff()+h+h/4);
			
        }
      }

      for (f=first_view;f;f=f->next)
      {
        if (f->drawable())
        {
          StartTime(5); // kill me
          if (interpolate_draw)
          {
            draw_map(f,1);
            eh->flush_screen();
          }
          draw_map(f,0);
          DiffTime("    Drawmap",5); // kill me
        }
      }

    }  
    if (state==PAUSE_STATE)
    {
      for (view *f=first_view;f;f=f->next)
        cash.img(pause_image)->put_image(screen,(f->cx1+f->cx2)/2-cash.img(pause_image)->width()/2,
            f->cy1+5,1);
    }
    
    show_time();
  }

  if (state==RUN_STATE && cash.prof_is_on())
    cash.prof_poll_end();

  StartTime(16); //kill me
  eh->flush_screen();
  DiffTime("    Flush",16); //kill me
}

void game::do_intro()
{

}

#if 1
int game::calc_speed()
{
  int ret=0;
  if (fps_mark_start)
  {

    time_marker t;

    // find average fps for last 10 frames
    double td=t.diff_time(fps_mark_start);
    if (td<0.001)     // something is rotten in the state of demark
      td=0.001;

    avg_fps=avg_fps*9.0/10.0+1.0/(td*10.0);  
    possible_fps=possible_fps*9.0/10.0+1.0/(td*10.0);  

    if (avg_fps>14)
    {
      if (massive_frame_panic>20)
        massive_frame_panic=20;
      else if (massive_frame_panic)
        massive_frame_panic--;
    }

    if (avg_fps>15 && ((dev&EDIT_MODE)==0 || need_delay))
    {
      frame_panic=0;
      long stime=(long)((1/15.0-1.0/possible_fps)*1000.0); 
      if (stime>0 && !no_delay)
      {
        milli_wait(stime);
        avg_fps-=1.0/(td*10.0);        // subtract out old estimate
			
        time_marker t;
				
        // find average fps for last 10 frames
        double td=t.diff_time(fps_mark_start);
        if (td<0.00001)     // something is rotten in the state of demark
          td=0.00001;
			
        avg_fps+=1.0/(td*10.0);       // add in new estimate
      }
    } else if (avg_fps<14)
    {
      if (avg_fps<10)
        massive_frame_panic++;
      frame_panic++;
      ret=1;
    }
    
    delete fps_mark_start;    
  }
  fps_mark_start=new time_marker;
  return ret;
}
#else
int game::calc_speed()
{
  static UnsignedWide last_t = {0,0};
  UnsignedWide t;
	
  if (dev & EDIT_MODE)
    return 0;
	
  do 
  {
    Microseconds(&t);
  } while( t.lo - last_t.lo < 60000);
	
  last_t = t;
	
  return 0;
}
#endif

extern int start_edit;

void single_render();
void double_render();

void game::get_input()
{
  event ev;  
  idle_ticks++;
  while (event_waiting(eh))
  {
    get_event(ev,eh);

    if (ev.type==EV_MOUSE_MOVE) last_input=ev.window;
    // don't process repeated keys in the main window, it will slow down the game handle such
    // useless events. However in other windows it might be useful, such as in input windows
    // where you want to repeatedly scroll down..
    if (ev.type!=EV_KEY || !key_down(ev.key) || ev.window || (dev&EDIT_MODE))
    {
      if (ev.type==EV_KEY)     
      {
      	int input_key = ev.key;
      	input_key = (input_key>='A' && input_key<='Z') ? input_key - 'A' + 'a' : input_key;
        set_key_down(input_key,1);
        if (playing_state(state))
        {	 
          if (ev.key<256)
          {
            if (chat && chat->chat_event(ev))
              base->packet.write_byte(SCMD_CHAT_KEYPRESS);
            else
              base->packet.write_byte(SCMD_KEYPRESS);
          }
          else
            base->packet.write_byte(SCMD_EXT_KEYPRESS);
          base->packet.write_byte(client_number());
          if (ev.key>256)
            base->packet.write_byte(ev.key-256);
          else
            base->packet.write_byte(ev.key);	  
        }
      }
      else if (ev.type==EV_KEYRELEASE)
      {
      	int input_key=ev.key;
      	input_key = (input_key>='A' && input_key<='Z') ? input_key - 'A' + 'a' : input_key;
        set_key_down(input_key,0);
        if (playing_state(state))
        {	 
          if (ev.key<256)
            base->packet.write_byte(SCMD_KEYRELEASE);
          else
            base->packet.write_byte(SCMD_EXT_KEYRELEASE);
          base->packet.write_byte(client_number());
          if (ev.key>255)
            base->packet.write_byte(ev.key-256);
          else
            base->packet.write_byte(ev.key);	  
        }	
      }
      
      if ((dev&EDIT_MODE) || start_edit || ev.type==EV_MESSAGE)
        dev_cont->handle_event(ev);

      view *v=first_view;
      for (;v;v=v->next)
        if (v->local_player() && v->handle_event(ev))
          ev.type=EV_SPURIOUS;       // if the event was used by the view, gobble it up


      
      help_handle_event(ev);    
      mousex=last_demo_mx;
      mousey=last_demo_my;

      if (ev.type==EV_MESSAGE)
      {
        switch (ev.message.id)
        {
          case CALB_JOY : 
          { 
            if (!joy_win)
            {
              int wx=WINDOW_FRAME_LEFT,wy=WINDOW_FRAME_TOP;
				      
              joy_win=eh->new_window(80,50,-1,-1,
                  new button(wx+70,wy+9,JOY_OK,"OK",
                      new info_field(wx,wy+30,DEV_NULL,
                          " Center joystick and\n"
                          "press the fire button",NULL)),
			
                  "Joystick");
              set_state(JOY_CALB_STATE); 
            }
          }
          case TOP_MENU :
          { menu_select(ev); } break;
          case DEV_QUIT :
          { finished=1; } break;
			
        }
      }	    
      else if (ev.type==EV_CLOSE_WINDOW && ev.window==top_menu)
      {
        eh->close_window(top_menu);
        top_menu=NULL;
      }
    
      switch (state)
      {
        case JOY_CALB_STATE :
        { joy_calb(ev); } break;
        case INTRO_START_STATE : 
        { do_intro(); 
        if (dev & EDIT_MODE)
          set_state(RUN_STATE);
        else
          set_state(MENU_STATE); 
        } break;     
        case PAUSE_STATE : if (ev.type==EV_KEY && (ev.key==JK_SPACE || ev.key==JK_ENTER)) 
        { set_state(RUN_STATE); } break;
        case RUN_STATE : 
        {
          if (ev.window==NULL)
          {
				  
            switch (ev.type)
            {
              case EV_KEY : 
              { switch (ev.key)
              {
                case 'm' : 
                { 
                  if (dev&MAP_MODE) dev-=MAP_MODE;
                  else if ((player_list && player_list->next) || dev&EDIT_MODE) dev|=MAP_MODE;
				
                  if (!(dev&MAP_MODE))
                  {
                    if (dev_cont->tbw) dev_cont->toggle_toolbar();
                    edit_mode=ID_DMODE_DRAW;
                  }
                  need_refresh();	 
                } break;
                case 'v' : 
                { eh->push_event(new event(DO_VOLUME,NULL)); } break;
                case 'p' : 
                { if (!(dev&EDIT_MODE) && (!main_net_cfg || 
                    (main_net_cfg->state!=net_configuration::SERVER &&
                        main_net_cfg->state!=net_configuration::CLIENT)))  
                  set_state(PAUSE_STATE); 
                } break;
                case 'S' : 
                  if (start_edit)
                  { eh->push_event(new event(ID_LEVEL_SAVE,NULL)); } break;
                case JK_TAB : 
                { if (start_edit) toggle_edit_mode(); need_refresh(); } break;
                case 'c' :		  
                { if (chatting_enabled && (!(dev&EDIT_MODE) && chat))
                  chat->toggle();
                } break;
                case '9' : 
                { dev=dev^PERFORMANCE_TEST_MODE; need_refresh(); } break;
				
                /*
                  case '=' :
                  case '+' : 
                  { if (!dev_cont->need_plus_minus())
                  { 
                  if (eh->key_pressed(JK_CTRL_L))
                  grow_views(20);
                  else
                  grow_views(5); 
                  draw(state==SCENE_STATE);
                  }
                  } break;
                  case JK_F10 : make_screen_size(311,160); break;
                  case '_' :
                  case '-' : 
                  {
                  if (!dev_cont->need_plus_minus())		    
                  { 
                  if (eh->key_pressed(JK_CTRL_L))
                  grow_views(-20);
                  else
                  grow_views(-5); 
                  draw(state==SCENE_STATE); 
                  } 
                  } break; */
				
              }
  	      } break;	  	  
              case EV_RESIZE : 
              { 
                view *v;
                for (v=first_view;v;v=v->next)  // see if any views need to change size
                {
                  if (v->local_player())
                  {
                    int w=(xres-10)/(small_render ? 2 : 1);
                    int h=(yres-10)/(small_render ? 2 : 1);
				
                    /*                    v->suggest.send_view=1;
                    v->suggest.cx1=5;
                    v->suggest.cx2=5+w;
                    v->suggest.cy1=5;
                    v->suggest.cy2=5+h;
                    v->suggest.pan_x=v->pan_x;
                    v->suggest.pan_y=v->pan_y;
                    v->suggest.shift_down=v->shift_down;
                    v->suggest.shift_right=v->shift_right; */
                  }		  
                }
                draw(); 
              } break;
              case EV_REDRAW : 
                screen->add_dirty(ev.redraw.x1,ev.redraw.y1,
                    ev.redraw.x2,ev.redraw.y2); 
                break;
              case EV_MESSAGE :	    
                switch (ev.message.id)
                {
                  case RAISE_SFX :
                  case LOWER_SFX :
                  case RAISE_MUSIC :
                  case LOWER_MUSIC :
                    if (ev.message.id==RAISE_SFX && sfx_volume!=127) sfx_volume=min(127,sfx_volume+16);
                    if (ev.message.id==LOWER_SFX && sfx_volume!=0) sfx_volume=max(sfx_volume-16,0);
                    if (ev.message.id==RAISE_MUSIC && music_volume!=126) 
                    {		    
                      music_volume=min(music_volume+16,127);
                      if (current_song && (sound_avail&MUSIC_INITIALIZED))
                        current_song->set_volume(music_volume);
                    }
			  
                    if (ev.message.id==LOWER_MUSIC && music_volume!=0) 
                    {		    
                      music_volume=max(music_volume-16,0);
                      if (current_song && (sound_avail&MUSIC_INITIALIZED))
                        current_song->set_volume(music_volume);
                    }		    
			  
                    ((button *)ev.message.data)->push();	      
                    break;
                }
		    		  
            }
          }
      	} break;
      }
    }
  }
}


void net_send(int force=0)
{
  if ( (!(dev&EDIT_MODE)) || force)
  {
    if (demo_man.state==demo_manager::PLAYING)
    {
      base->input_state=INPUT_PROCESSING;
    } else
    {
      


      if (!player_list->focus)
      {
        dprintf("Players have not been created\ncall create_players");
        exit(0);
      }


      view *p=player_list;
      for (;p;p=p->next)
        if (p->local_player())
          p->get_input();


      base->packet.write_byte(SCMD_SYNC);
      base->packet.write_short(make_sync());

      if (base->join_list)
        base->packet.write_byte(SCMD_RELOAD);

      //      dprintf("save tick %d, pk size=%d, rand_on=%d, sync=%d\n",current_level->tick_counter(),
      //	     base->packet.packet_size(),rand_on,make_sync());
      send_local_request();
    }
  }
}

void net_receive()
{
  if (!(dev&EDIT_MODE) && current_level)
  {
    uchar buf[PACKET_MAX_SIZE+1];
    int size;

    if (demo_man.state==demo_manager::PLAYING)
    { 
      if (!demo_man.get_packet(buf,size))
        size=0;
      base->packet.packet_reset();
      base->mem_lock=0;
    } else
    {
      size=get_inputs_from_server(buf);
      if (demo_man.state==demo_manager::RECORDING)
        demo_man.save_packet(buf,size);
    }

    process_packet_commands(buf,size);
  }
}

void game::step()
{
  clear_tmp();
  if (current_level)
  {
    current_level->unactivate_all();
    total_active=0;
    for (view *f=first_view;f;f=f->next)
    {
      if (f->focus)           
      {
        f->update_scroll();
        int w,h;
			
        w=(f->cx2-f->cx1+1);
        h=(f->cy2-f->cy1+1);
        total_active+=current_level->add_actives(f->xoff()-w/4,f->yoff()-h/4,
            f->xoff()+w+w/4,f->yoff()+h+h/4);
      }
    }
  }

  if (state==RUN_STATE)
  {    
    if ((dev&EDIT_MODE) || (main_net_cfg && (main_net_cfg->state==net_configuration::CLIENT ||
        main_net_cfg->state==net_configuration::SERVER)))
      idle_ticks=0;

    if (demo_man.current_state()==demo_manager::NORMAL && idle_ticks>420 && demo_start)
    {
      idle_ticks=0;
      set_state(MENU_STATE);    
    }
    else if (!(dev & EDIT_MODE))               // if edit mode, then don't step anything
    {
#ifdef __MAC__
      if (key_down(JK_ESC) || (key_down('q') && key_down(JK_COMMAND)))
#else
        if (key_down(JK_ESC))
#endif
        {
          int r=rand_on;
          set_state(MENU_STATE);
          main_menu();  
          rand_on=r;
          
          //          set_key_down(JK_ESC,0);
          //          return ;
        }
      ambient_ramp=0;
      view *v;
      for (v=first_view;v;v=v->next)
        v->update_scroll();

      cash.prof_poll_start();
      current_level->tick();
      sbar.step();
    } else    
      dev_scroll();  
  } else if (state==JOY_CALB_STATE)
  {
    event ev;
    joy_calb(ev);
  } else if (state==MENU_STATE)  
    main_menu();  
    
  if (key_down('x') && (key_down(JK_ALT_L) || key_down(JK_ALT_R)) && confirm_quit()) finished=1;
}

extern void *current_demo;

game::~game()
{  
  delete idle_man;
  idle_man=0;

  current_demo=NULL;
  if (first_view==player_list) first_view=NULL;
  while (player_list)
  {
    view *p=player_list;
    game_object *o=p->focus;
    player_list=player_list->next;
    delete p;
    o->set_controller(NULL);
    if (current_level && o)
      current_level->delete_object(o);
    else delete o;
  }

  if (current_level) { delete current_level; current_level=NULL; }

  if (first_view!=player_list)
  {
    while (player_list)
    {
      view *p=player_list;
      player_list=player_list->next;
      delete p;
    }
  }

  while (first_view)
  {
    view *p=first_view;
    first_view=first_view->next;
    delete p;
  }

  player_list=NULL;  

  if (old_view)
  {
    first_view=old_view;
    while (first_view)
    {
      view *p=first_view;
      first_view=first_view->next;
      delete p;
    }
  }
  old_view=NULL;

  int i=0;
  for (;i<total_objects;i++)
  {
    jfree(object_names[i]);
    delete figures[i];
  }
  if (fps_mark_start) delete fps_mark_start; fps_mark_start=NULL;
  delete pal;
  jfree(object_names);
  jfree(figures);

  jfree(backtiles);
  jfree(foretiles);
  if (total_weapons)
    jfree(weapon_types);

  config_cleanup();
  delete color_table;
  delete eh;
  eh = 0;
  
  delete game_font;
  delete window_font;
  delete big_font;
  delete console_font;
  if (total_help_screens)
    jfree(help_screens);
    

  //  image_uninit();
}



void game::draw(int scene_mode)
{
  screen->add_dirty(0,0,xres,yres);
  image *bt=cash.img(border_tile);
  int tw=bt->width(),th=bt->height(),dx,dy=0;
  int xt=screen->width()/tw+1,yt=screen->height()/th+1,x,y;
  screen->clear();
  //  for (y=0;y<yt;y++,dy+=th)
  //    for (x=0,dx=0;x<xt;x++,dx+=tw)
  //      bt->put_image(screen,dx,dy); 
  int lr=eh->bright_color(),
    mr=eh->medium_color();

  if (scene_mode)
  {
    char *helpstr="ARROW KEYS CHANGE TEXT SPEED";
    eh->font()->put_string(screen,screen->width()/2-(eh->font()->width()*strlen(helpstr))/2+1,
        screen->height()-eh->font()->height()-5+1,helpstr,eh->dark_color());    
    eh->font()->put_string(screen,screen->width()/2-(eh->font()->width()*strlen(helpstr))/2,
        screen->height()-eh->font()->height()-5,helpstr,eh->bright_color());    
  }
  /*  else
      {
      char *helpstr="PRESS h FOR HELP";
      eh->font()->put_string(screen,screen->width()-eh->font()->width()*strlen(helpstr)-5,
      screen->height()-eh->font()->height()-5,helpstr);
      }*/
  /*  int dc=cash.img(window_colors)->pixel(0,2);
      int mc=cash.img(window_colors)->pixel(1,2);
      int bc=cash.img(window_colors)->pixel(2,2);
      screen->line(0,0,screen->width()-1,0,dc);
      screen->line(0,0,0,screen->height()-1,dc);
      screen->line(0,screen->height()-1,screen->width()-1,screen->height()-1,bc);
      screen->line(screen->width()-1,0,screen->width()-1,screen->height()-1,bc);*/
  for (view *f=first_view;f;f=f->next)
    draw_map(f,0);

  sbar.redraw(screen);
}

int external_print=0;

void start_sound(int argc, char **argv)
{
  sfx_volume=music_volume=127;

  for (int i=1;i<argc;i++)
    if (!strcmp(argv[i],"-sfx_volume"))
    {
      i++;
      if (atoi(argv[i])>=0 && atoi(argv[i])<127)
        sfx_volume=atoi(argv[i]);
      else dprintf("Bad sound effects volume level, use 0..127\n");      
    }
    else if (!strcmp(argv[i],"-music_volume"))
    {
      i++;
      if (atoi(argv[i])>=0 && atoi(argv[i])<127)
        music_volume=atoi(argv[i]);
      else dprintf("Bad music volume level, use 0..127\n");      
    }

  sound_avail=sound_init(argc,argv);
}

void game_printer(char *st)
{

#if 0
	static FILE *f = 0;
	if (!f)
		f = fopen("abuse.out","wt");
	fprintf(f,st);
	fflush(f);
#endif

  if (dev_console && !external_print)
  {
    dev_console->put_string(st);
  }

#ifndef __MAC__
  else fprintf(stderr,"%s",st);
#endif
}

void game_echoer(char *st)
{
  #ifdef __MAC__
	static FILE *f = 0;
	if (!f)
		f = fopen("abuse.out","wt");
	fprintf(f,st);
	fflush(f);
#else
       
  if (dev_console && !external_print)
  {
    dev_console->put_string(st);
  } else if (external_print)
    fprintf(stderr,"%s",st);

#endif
}

void game_getter(char *st, int max)
{
  if (!max) return ;
  max--;
  *st=0;
  if (dev_console && !external_print)
  {    
    dev_console->show();
    int t=0;
    event ev;
    do
    {
      get_event(ev,eh);
      if (ev.type==EV_KEY)
      {
        if (ev.key==JK_BACKSPACE)
        {
          if (t) 
          {
            dev_console->printf("%c",ev.key);
            t--;
            st--;
            *st=0;
            max++;
          }
        } else if (ev.key>=' ' && ev.key<='~')
        {
          dev_console->printf("%c",ev.key);
          *st=ev.key;
          t++;
          max--;
          st++;
          *st=0;
        }			  
      }
      eh->flush_screen();
    } while (ev.type!=EV_KEY || ev.key!=JK_ENTER);    
    dprintf("\n");
  }
  else 
  {
    if (fgets(st,max,stdin))
    {
      if (*st)
        st[strlen(st)-1]=0;
    }
  }
}


void show_startup()
{
  show_verinfo(start_argc,start_argv);
}

char *get_line(int open_braces)
{
  char *line=(char *)jmalloc(1000,"lstring");
  fgets(line,1000,stdin);

  char prev=' ';
  for (char *s=line;*s && (prev!=' ' || *s!=';');s++)
  {
    prev=*s;
    if (*s=='(') open_braces++;
    else if (*s==')') open_braces--;
  }
  if (open_braces<0)	
    dprintf("\nToo many )'s\n");
  else if (open_braces>0)
  {
    char *s2=get_line(open_braces);
    line=(char *)jrealloc(line,strlen(line)+strlen(s2)+1,"lstring");
    strcat(line,s2);
    jfree(s2);    
  }  
  return line;
}

void check_for_upgrade(int argc, char **argv)
{
  for (int i=1;i<argc;i++)
    if (!strcmp(argv[i],"-upgrade"))
    {
      lisp_init(0xf000,0x30000);
      char *prog="(load \"lisp/upgrade.lsp\")",*cs;
      cs=prog;
      if (!eval(compile(cs)))
	dprintf("file does not exists : lisp/upgrade.lsp, cannot upgrade\n");

      exit(0);
    }        
}

void check_for_lisp(int argc, char **argv)
{
  for (int i=1;i<argc;i++)
  {
    if (!strcmp(argv[i],"-lisp"))
    {
      lisp_init(0xf000,0x30000);

#ifdef __WATCOMC__
      char *eof_char="CTRL-Z";
#else
      char *eof_char="CTRL-D";
#endif
      dprintf(
              " CLIVE (C) 1995 Jonathan Clark, all rights reserved\n"
              "   (C LISP interpreter and various extentions)\n"
              "Type (%s) to exit\n",eof_char);

      while (!feof(stdin))
      {
	dprintf("Lisp> ");
	char *l=get_line(0);
	char *s=l;
	while (*s)
	{
	  void *prog=compile(s);
	  l_user_stack.push(prog);
	  while (*s==' ' || *s=='\t' || *s=='\r' || *s=='\n') s++;
	  lprint(eval(prog));
	  l_user_stack.pop(1);
	}
	jfree(l);
      }      

      dprintf("End of input : bye\n");
      exit(0);
    }
  }
}


void music_check()
{
  if (sound_avail&MUSIC_INITIALIZED)
  {
    if (current_song && !current_song->playing())
    {
      current_song->play();
      dprintf("song finished\n");
    }
    if (!current_song)
    {

      current_song=new song("music/intro.hmi");
      current_song->play(music_volume);

      /*      if (DEFINEDP(symbol_function(l_next_song)))  // if user function installed, call it to load up next song
              {
              int sp=current_space;
              current_space=PERM_SPACE;
              eval_function((lisp_symbol *)l_next_song,NULL);
              current_space=sp;
              } */
    } 
  }
}

void setup(int argc, char **argv);

void share_end();
void show_end();

void show_sell(int abortable);

extern pmenu *dev_menu;


extern int jmalloc_max_size;
extern int jmalloc_min_low_size;

extern int (*verify_file_fun)(char *,char *);

int registered_ok(char *filename, char *mode)
{
  if (registered) return 1;

  char name[256],*c;
  c=name;
  while (*filename) { *c=*(filename++); *c=toupper(*c); c++; } *c=0;
  if (strstr(name,"REGISTER"))
    return 0;
  else return 1;
}

void game_net_init(int argc, char **argv)
{
  int nonet=!net_init(argc, argv);
  if (nonet)
    dprintf("No network driver, or network driver returned failure\n");
  else 
  {
    set_file_opener(open_nfs_file);
    if (main_net_cfg && main_net_cfg->state==net_configuration::CLIENT)
    {
      if (set_file_server(net_server))
        start_running=1;
      else
      {
        dprintf("Unable to attach to server, quiting\n");
        exit(0);
      }
    } else
    {
      int i;
      for (i=1;i<argc-1;i++)
        if (!strcmp(argv[i],"-fs"))
          if (!set_file_server(argv[i+1]))
            dprintf("could not set defualt file server to %s\n",argv[i+1]);
    }
  }	

}

#ifdef __MAC__
//extern int PixMult;
#if 1
char cmdline[256];
#elif 1
char cmdline[] = "";
#elif 1
char cmdline[] = "abuse -server -a deathmat";
#else
char cmdline[] = "abuse -net 193.246.40.9";
#endif
char delims[] = " ";
char *tmp_argv[255];

void GetArgs(int &argc, char **(&argv))
{
  char *s;

  dprintf(	"Usage:\n"
      "  abuse [-options]\n\n"
      "  Options:\n"
      "    -server -a deathmat        become a server for deathmatch game\n"
      "    -net <dotted ip address>   connect to a server\n\n"
      "Options for mac:\n"
      "  Hold down <control> for single pixel mode\n"
      "  Hold down <option> for edit mode\n"
      "  Hold down <left shift> for double size mode\n\n"
      "If started with no command line options, networking will attempt\n"
      "  to search the local network for servers\n\n"
                );
  dprintf("Enter command line:\n");
  gets(cmdline);
	
  argc = 0;
  argv = tmp_argv;
  s = strtok(cmdline, delims);
  while (s)
  {
    argv[argc] = s;
    argc++;
    s = strtok(0, delims);
  }
  argv[argc] = 0;
}

extern void InitMacScreen();
extern void QuitMacScreen();

#endif

int main(int argc, char **argv)
{
#ifdef __MAC__
  InitMacScreen();
#if 1
  //	if (!MacMenu())
  //		return 0;

  unsigned char km[16];

  GetKeys((unsigned long*)&km);
  if ((km[ 0x37 >>3] >> (0x37 & 7)) &1 != 0)
	  set_dprinter(game_echoer);
	else
	  set_dprinter(game_printer);

#else
  unsigned char km[16];

  GetArgs(argc,argv);
	
  dprintf("Mac Options: ");
  GetKeys((unsigned long*)&km);
  if ((km[ 0x3a >>3] >> (0x3a & 7)) &1 != 0)
  {
    dev|=EDIT_MODE;    
    start_edit=1;
    start_running=1;
    disable_autolight=1;
    dprintf("Edit Mode...");
  }
  if ((km[ 0x3b >>3] >> (0x3b & 7)) &1 != 0)
  {
    PixMult = 1;
    dprintf("Single Pixel...");
  }
  else
  {
    PixMult = 2;
    dprintf("Double Pixel...");
  }
  if ((km[ 0x38 >>3] >> (0x38 & 7)) &1 != 0)
  {
    xres *= 2;  yres *= 2;
    dprintf("Double Size...");
  }
  dprintf("\n");
	
  if (tcpip.installed())
    dprintf( "Using %s\n", tcpip.name());
#endif

#else

  set_dprinter(game_printer);

#endif

  start_argc=argc;
  start_argv=argv;
  
  { for (int i=0;i<argc;i++)
  {
    if (!strcmp(argv[i],"-cprint"))
      external_print=1;
    if (!strcmp(argv[i],"-min_low"))
    {
      i++;
      jmalloc_min_low_size=atoi(argv[i]);
    }
  }
  }

  set_dgetter(game_getter);
  set_no_space_handler(handle_no_space);

  setup(argc,argv);

  show_startup();

  //  jmalloc_max_size=0x150000;
  jmalloc_init(0x150000); 
  //  jmalloc_init(100000); 

  start_sound(argc,argv);

//#ifdef __MAC__
  //stat_man=new mac_status_manager();
//#else
  stat_man=new text_status_manager();
//#endif
  if (!get_option("-no_timer"))
  {
    timer_init();
  }


  if (!get_option("-share"))
  {
    jFILE *fp=new jFILE("register/micron.vcd","rb");
    if (!fp->open_failure())
    {
      spec_directory sd(fp);
      if (sd.find("Copyright (C) 1995 Crack dot Com, All Rights reserved"))
        registered=1;
    }
    delete fp;
  }

  verify_file_fun=registered_ok;



  jrand_init();
  jrand();  // so compiler doesn't complain


  set_spec_main_file("abuse.spe");

  if (getenv("ABUSE_PATH"))       // look to see if we are supposed to fetch the data elsewhere
    set_filename_prefix(getenv("ABUSE_PATH"));   

  if (getenv("ABUSE_SAVE_PATH"))       // look to see if we are supposed to fetch the data elsewhere
    set_save_filename_prefix(getenv("ABUSE_SAVE_PATH"));   

  check_for_lisp(argc,argv);
  check_for_upgrade(argc,argv);

  set_mode(VMODE_640x480,argc,argv);

  if (stat_man)
    delete stat_man;
  stat_man=new net_status_manager("art/loading.spe",
      413,428,608,451,
      54,104);


 
  do
  {
    if (main_net_cfg) 
    {


      if (!main_net_cfg->notify_reset())
      {
        if (!get_option("-no_timer"))
          timer_uninit();
        sound_uninit();
        exit(0);
      }
    }

    game_net_init(argc,argv);
    lisp_init(0x16000,0x94000);
    //  lisp_init(0x100000,0x10000);

    dev_init(argc,argv);


    game *g=new game(argc,argv);

    delete stat_man;
    stat_man=new net_status_manager("art/status.spe",
        295,203,304,366,
        46,222);



    dev_cont=new dev_controll();
    dev_cont->load_stuff();



    g->get_input();      // prime the net

    int xx;
    for (xx=1;xx<argc;xx++)
      if (!strcmp(argv[xx],"-server"))
      {
        xx++;
        if (!become_server(argv[xx]))
        {      
          dprintf("unable to become a server\n");
          exit(0);
        }
        xx=argc+1;
      }

    if (main_net_cfg) 
      wait_min_players();

    net_send(1);
    if (net_start())
    {
      g->step();                        // process all the objects in the 
      g->calc_speed();
      g->update_screen();               // redraw the screen with any changes
    }

    while (!g->done())
    {
      StartTime(0);//kill me
    	
      StartTime(1);//kill me
      //      music_check();

      if (req_end)
      {
        delete current_level;
        current_level=NULL;
				
        if (!registered)
          share_end();
        else show_end();
			
        the_game->set_state(MENU_STATE);
        req_end=0;
      }
      

      if (demo_man.current_state()==demo_manager::NORMAL)
      {
        net_receive();
      }

      if (req_name[0])            // see if a request for a level load was made during the last tick
      {
        //					ProfilerDump("\pabuse.prof");  //prof    
        //					ProfilerTerm();
        int show;
        if (!current_level)
          show=0;
        else if (strcmp(req_name,current_level->name()) && memcmp(req_name,"save",4)!=0)
          show=1;
        else show=0;

        if (show)
          show_stats();
        g->load_level(req_name);
        switch_mode(VMODE_320x200);
   			recalc_local_view_space();
        if (show)
          end_stats();

        //					ProfilerInit(collectDetailed, bestTimeBase, 2000, 200); //prof
        req_name[0]=0;
        g->draw(g->state==SCENE_STATE);
      }

      //    if (demo_man.current_state()!=demo_manager::PLAYING)
      g->get_input();
      
      if (demo_man.current_state()==demo_manager::NORMAL)
      	net_send();
      else
      	demo_man.do_inputs();

      service_net_request();

      DiffTime("  PreStuff",1); //kill me
      StartTime(2);	//kill me
			
      g->step();                        // process all the objects in the 

      DiffTime("  Step",2);  // kill me
      StartTime(3);  // kill me
      server_check();

      g->calc_speed();
      DiffTime("  MidStuff",3);  // kill me
      StartTime(4);  // kill me
      if (!req_name[0])                // see if a request for a level load was made during the last tick
        g->update_screen();               // redraw the screen with any changes
      DiffTime("  Update",4);  // kill me

      DiffTime("GameLoop",0);
    }
    switch_mode(VMODE_640x480);

    if (main_net_cfg && main_net_cfg->restart_state())
    {
      fade_out(1);
      image blank(2,2); blank.clear();
      eh->set_mouse_shape(blank.copy(),0,0);      // don't show mouse

      char *msg="Wait while network data is collected...";
      screen->clear(255);
      eh->font()->put_string(screen,
          xres/2-strlen(msg)*eh->font()->width()/2,
          yres/2-eh->font()->height()/2,
          msg);
      eh->flush_screen();
      fade_in(0,8);
    }

    net_uninit();

    if (net_crcs)
    {
      net_crcs->clean_up();
      delete net_crcs;
      net_crcs=NULL;
    }

    delete chat;

    if (!registered)
      show_sell(0);
    else milli_wait(500);


    if (small_render) { delete small_render; small_render=NULL; }

    if (current_song) 
    { current_song->stop();
    delete current_song; 
    current_song=NULL; 
    }


    cash.empty();


    if (dev_console)
    {
      delete dev_console;
      dev_console=NULL;
    }

    if (dev_menu)
    {
      delete dev_menu;
      dev_menu=NULL;
    }

    delete g;
    if (old_pal) delete old_pal; old_pal=NULL;
    compiled_uninit();
    delete_all_lights();
    jfree(white_light_initial);

    for (int i=0;i<TTINTS;i++) jfree(tints[i]);


    dev_cleanup();
    delete dev_cont; dev_cont=NULL;
   

    if (!(main_net_cfg && main_net_cfg->restart_state()))
    {
      void *end_msg=make_find_symbol("end_msg");
      if (DEFINEDP(symbol_value(end_msg)))
        dprintf("%s\n",lstring_value(symbol_value(end_msg)));
    }

    lisp_uninit();
    sd_cache.clear();

    base->packet.packet_reset();
    mem_report("end.mem");
  } while (main_net_cfg && main_net_cfg->restart_state());

   close_graphics();

  if (main_net_cfg) { delete main_net_cfg; main_net_cfg=NULL; }
  set_filename_prefix(NULL);  // dealloc this mem if there was any
  set_save_filename_prefix(NULL);

  if (!get_option("-no_timer"))
    timer_uninit();

  if (stat_man)
  {
    delete stat_man;
    stat_man=0;
  }


  mem_report("end.mem");
  jmalloc_uninit();
  l_user_stack.clean_up();
  l_ptr_stack.clean_up();

  sound_uninit();

#ifdef __MAC__
  QuitMacScreen();
#endif

  exit(0);

  return 0;  
}


