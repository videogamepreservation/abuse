#ifndef __GAME_HPP_
#define __GAME_HPP_

#include "loader2.hpp"

#include "macs.hpp"
#include "image.hpp"
#include "video.hpp"
#include "mdlread.hpp"
#include "event.hpp"
#include "fonts.hpp"
#include "loader.hpp"
#include "monoprnt.hpp"
#include "items.hpp"
#include "jwindow.hpp"
#include "filter.hpp"
#include "level.hpp"
#include "cache.hpp"
#include "director.hpp"
#include "view.hpp"
#include "id.hpp"

#define MAPFW				100
#define MAPFH				100
#define MAPBW				100
#define MAPBH				100

#define RUN_STATE 0
#define PAUSE_STATE 1
#define HELP_STATE 2
#define INTRO_START_STATE 3
#define INTRO_MORPH_STATE 4
#define JOY_CALB_STATE    5
#define MENU_STATE        6
#define SCENE_STATE       7
#define START_STATE       8
#define BLACK 0

#define tile_type unsigned short
class game;
extern game *the_game;
extern window_manager *eh;
extern int dev;
extern int morph_sel_frame_color;

extern char **start_argv;
extern int start_argc;
extern long current_vxadd,current_vyadd;
extern int frame_panic,massive_frame_panic;
extern int demo_start,idle_ticks;

class game
{
  JCFont *fnt;
  int finished;
  int bg_top,fg_top;                         // in the fg/bg window which tile is at the top?
  int bright_color,med_color,dark_color,     // for boundaries and windows, etc
      morph_bright_color,morph_med_color,morph_dark_color;

  long last_time,fps;
  char mapname[100],command[200],help_text[200];
  int refresh,mousex,mousey,help_text_frames;
  int has_joystick,no_delay;


  jwindow *top_menu,*joy_win,*last_input;
  JCFont *game_font;
  uchar keymap[512/8];

public : 
  int key_down(int key) { return keymap[key/8]&(1<<(key%8)); }
  void set_key_down(int key, int x) { if (x) keymap[key/8]|=(1<<(key%8)); else keymap[key/8]&=~(1<<(key%8)); }
  void reset_keymap() { memset(keymap,0,sizeof(keymap)); }

  int nplayers;
  view *first_view,*old_view;
  int state,zoom;


  game(int argc, char **argv);
  void step();
  void show_help(char *st);
  void draw_value(image *screen, int x, int y, int w, int h, int val, int max);
  unsigned char get_color(int x) { return x; }
  int done();
  void draw(int scene_mode=0);

  backtile *get_bg(int x) { if (x<0 || x>=nbacktiles || backtiles[x]<0) 
                           return cash.backt(backtiles[BLACK]); 
                           else return cash.backt(backtiles[x]); }
  foretile *get_fg(int x) { if (x<0 || x>=nforetiles || foretiles[x]<0) 
                           return cash.foret(foretiles[BLACK]); else 
			   return cash.foret(foretiles[x]); }

  void ftile_on(int screenx, int screeny, long &x, long &y);
  void btile_on(int screenx, int screeny, long &x, long &y);
  void toggle_delay();
  void set_delay(int on) { no_delay=!on; }
  void pan(int xv, int yv);

  void mouse_to_game(long x, long y, long &gamex, long &gamey, view *v=NULL);
  void game_to_mouse(long gamex, long gamey, view *which, long &x, long &y);
  view *view_in(int mousex, int mousey);

  int calc_speed();
  int ftile_width()  { return f_wid; }
  int ftile_height() { return f_hi; }

  int btile_width()  { return b_wid; }
  int btile_height() { return b_hi; }


  void put_fg(int x, int y, int type);
  void put_bg(int x, int y, int type);
  void draw_map(view *v, int interpolate=0);
  void dev_scroll();
  void put_block_fg(int x, int y, trans_image *im);
  void put_block_bg(int x, int y, image *im);


  int in_area(event &ev, int x1, int y1, int x2, int y2);
  void load_level(char *name);
  void set_level(level *nl);
  void show_time();
  tile_type get_map_bg(int x, int y) { return current_level->get_bg(x,y); }
  tile_type get_map_fg(int x, int y) { return current_level->get_fg(x,y); }
  void end_session();
  void need_refresh() { refresh=1; }       // for development mode only
  palette *current_palette() { return pal; }

  void update_screen();
  void get_input();
  void do_intro();
  void joy_calb(event &ev);
  void menu_select(event &ev2);
  int can_morph_into(int type);
  void morph_into(int type);
  void set_state(int new_state);
  int game_over();
  void grow_views(int amount);
  void play_sound(int id, int vol, long x, long y);
  void request_level_load(char *name);
  void request_end();
  ~game();
} ;

extern int playing_state(int state);
#endif


