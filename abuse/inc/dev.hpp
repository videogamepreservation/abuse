#ifndef __DEV_HPP_
#define __DEV_HPP_

#include "game.hpp"
#include "light.hpp"
#include "console.hpp"
#include "timing.hpp"


extern int scale_mult,scale_div;
extern char level_file[100];
extern image *small_render;

void dev_init(int argc, char **argv);
void dev_cleanup();
void toggle_edit_mode();
char *symbol_str(char *name);

class pal_win
{
  long scale,w,h,x,y,last_selected;
  unsigned short *pat;
  void draw();

public : 
  jwindow *me;
  void close_window();
  void open_window();
  char *name;
  void handle_event(event &ev);
  pal_win(void *args);
  void resize(int xa, int ya);
  int get_pat(int x, int y) { return pat[y*w+x]; }
  int width() { return w; }
  int height() { return h; }
  void save(FILE *fp);
  ~pal_win();
} ;

enum dev_state { DEV_CREATE_OBJECT, 
		 DEV_MOVE_OBJECT, 
		 DEV_SELECT, 
		 DEV_MOUSE_RELEASE, 
		 DEV_MOVE_LIGHT, 
		 DEV_CREATE_LIGHT,
	         DEV_DRAG_AREA_TOP,
	         DEV_DRAG_AREA_BOTTOM };

extern char backw_on,forew_on,show_menu_on,ledit_on,pmenu_on,omenu_on,commandw_on,tbw_on,searchw_on,
            small_render_on,interpolate_draw,disable_autolight,fps_on,profile_on,show_names,fg_reversed,
	    raise_all;


class dev_controll
{
  game_object *edit_object,*selected_object,*ai_object,*search_object,
              *link_object;
  light_source *edit_light,*selected_light;
  pal_win **pal_wins;
  char **pwin_list;
  int total_pals;
  dev_state state;
  int area_x1,area_y1,area_x2,area_y2;
  area_controller *current_area;
  time_marker last_area_click;
public :
  jwindow *backw,*forew,*commandw,*modew,*omenu,*oedit,*ledit,
          *music_window,*pmenu,*show_menu,*lightw,*aiw,*ambw,*tbw,*area_win,
          *search_window,*memprof;

  int fg_w,bg_w,fg_scale,bg_scale,yellow;
  void save();
  void fg_fill(int color, int x, int y, pal_win *p);
  void add_palette(void *args);
  void toggle_memprof();
  void search_forward();
  void search_backward();
  void toggle_toolbar();
  void toggle_fgw();
  void toggle_bgw();
  void toggle_omenu();
  void toggle_music_window();
  void toggle_pmenu();
  void toggle_show_menu();
  void toggle_light_window();
  void toggle_search_window();
  void show_char_mem(char *name);
  void close_oedit_window();
  void show_mem();
  dev_controll();
  void handle_event(event &ev);
  void do_command(char *st, event &ev);
  int is_pal_win(jwindow *win);
  void dev_draw(view *v);
  void load_stuff();
  int repeat_key_mode();
  int need_plus_minus();
  int need_arrows();
  void make_ai_window(game_object *o);
  void close_ai_window();
  void make_ambient();
  int ok_to_scroll();
  long snap_x(long x);
  long snap_y(long y);
  void area_handle_input(event &ev);
  void pick_handle_input(event &ev);
  void close_area_win(int read_values);
  void notify_deleted_object(game_object *o);
  void notify_deleted_light(light_source *l);
  void set_state(int new_state);
  void update_memprof();   
  ~dev_controll();
} ;

class dev_term : public shell_term
{
  dev_controll *dv;
  public :
  dev_term(int width, int height, dev_controll *dev) : shell_term(eh,console_font,width,height,"dev")
  {
    dv=dev;
  }
  virtual void execute(char *st);
} ;

extern dev_term *dev_console;

extern dev_controll *dev_cont;


#endif

















