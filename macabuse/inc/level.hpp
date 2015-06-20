
#ifndef __LEVEL_HPP_
#define __LEVEL_HPP_

#include "specs.hpp"
#include "macs.hpp"
#include "objects.hpp"
#include "view.hpp"
#include "id.hpp"
#include "timing.hpp"

#include <stdlib.h>
#define ASPECT 4             // foreground scrolls 4 times faster than background


// the following defines the area of activity for objects
// when they are out of this are no processing occurs on them
// region is specified from upper left corner of screen
#define ACTIVE_LEFT 500
#define ACTIVE_RIGHT (280+500)         
#define ACTIVE_TOP 200
#define ACTIVE_BOTTOM (180+200)
#define fgvalue(y) ((y) & 0x3fff)
#define above_tile(y) ((y) & 0x4000)
#define bgvalue(y) (y)

class area_controller
{
  public :
  long x,y,w,h,active;
  long ambient,view_xoff,view_yoff;
  long ambient_speed, view_xoff_speed,view_yoff_speed;
  area_controller *next;
  area_controller(long X, long Y, long W, long H, area_controller *Next);
} ;

extern long last_tile_hit_x,last_tile_hit_y;
extern int dev;
class level        // contain map info and objects
{
  unsigned short *map_fg,        // just big 2d arrays
                 *map_bg,
	         bg_width,bg_height,
	         fg_width,fg_height;  
  char *Name,*first_name;
  long total_objs;
  game_object *first,*first_active,*last;

  game_object **attack_list;                // list of characters for tick which can attack someone
  int attack_list_size,attack_total;
  void add_attacker(game_object *who);

  game_object **target_list;                // list of characters for tick which can be attacked
  int target_list_size,target_total;
  void add_target(game_object *who);

  game_object **block_list;                // list of characters who can block a character
  int block_list_size,block_total;
  void add_block(game_object *who);

  void remove_block(game_object *who);
  void remove_all_block(game_object *who);

  game_object **all_block_list;            // list of characters who can block a character or can be hurt
  int all_block_list_size,all_block_total;
  void add_all_block(game_object *who);

  double previous_time;      // previous time spent playing the level (i.e. previous to this load)
  time_marker start_time;    // time spent playing this level during this sessions
  ulong ctick;
  int am_timing;             // flag set when timing is on so time can be transfered to previous time on save()

public:
  void start_timing();         // reset's start_time
  void stop_timing();          // transfers all time from start_time till now to previous time
  double time_taken();

  char *original_name() { if (first_name) return first_name; else return Name; }
  ulong tick_counter() { return ctick; }
  void set_tick_counter(ulong x);
  area_controller *area_list;
 
  void clear_active_list() { first_active=NULL; }
  char *name() { return Name; }
  game_object *attacker(game_object *who);
  int is_attacker(game_object *who);
  game_object *main_character();

  game_object *first_object() { return first; }
  game_object *first_active_object() { return first_active; }
  unsigned short foreground_width() { return fg_width; }
  unsigned short foreground_height() { return fg_height; }
  unsigned short background_width() { return bg_width; }
  unsigned short background_height() { return bg_height; }
  int load_failed() { return map_fg==NULL; } 
  level(spec_directory *sd, bFILE *fp, char *lev_name);
  void load_fail();             
  level(int width, int height, char *name);  
  int save(char *filename, int save_all);  // save_all includes player and view information (1 = success)
  void set_name(char *name) { Name=strcpy((char *)jrealloc(Name,strlen(name)+1,"map name"),name); }
  void set_size(int w, int h);
  void remove_light(light_source *which);
  void try_pushback(game_object *subject,game_object *target);
  ~level();

  int fg_raised(int x, int y) { CHECK(x>=0 && y>=0 && x<fg_width && y<fg_height);
				 return (*(map_fg+x+y*fg_width))&0x4000; }
  void fg_set_raised(int x, int y, int r) { CHECK(x>=0 && y>=0 && x<fg_width && y<fg_height);
					    ushort v=(*(map_fg+x+y*fg_width))&(0xffff-0x4000);
					    if (r) (*(map_fg+x+y*fg_width))=v|0x4000;
					    else (*(map_fg+x+y*fg_width))=v;
					  }
  void mark_seen(int x, int y) { CHECK(x>=0 && y>=0 && x<fg_width && y<fg_height);
					  (*(map_fg+x+y*fg_width))|=0x8000; }
  void clear_fg(long x, long y) { *(map_fg+x+y*fg_width)&=0x7fff; }

  unsigned short *get_fgline(int y) { CHECK(y>=0 && y<fg_height); return map_fg+y*fg_width; }
  unsigned short *get_bgline(int y) { CHECK(y>=0 && y<bg_height); return map_bg+y*bg_width; }
  unsigned short get_fg(int x, int y) { if (x>=0 && y>=0 && x<fg_width && y<fg_height)
        			          return fgvalue(*(map_fg+x+y*fg_width)); 
	                                else return 0;
				      }
  unsigned short get_bg(int x, int y) { if (x>=0 && y>=0 && x<bg_width && y<bg_height)
					  return *(map_bg+x+y*bg_width); 
	                                 else return 0;
					}
  void put_fg(int x, int y, unsigned short tile) { *(map_fg+x+y*fg_width)=tile; }   
  void put_bg(int x, int y, unsigned short tile) { *(map_bg+x+y*bg_width)=tile; }
  void draw_objects(view *v); 
  void interpolate_draw_objects(view *v);
  void draw_areas(view *v);
  int tick();                                // returns false if character is dead
  void check_collisions();
  void wall_push();
  void add_object(game_object *new_guy);
  void add_object_after(game_object *new_guy, game_object *who);
  void delete_object(game_object *who);
  void remove_object(game_object *who);      // unlinks the object from level, but doesn't delete it
  void load_objects(spec_directory *sd, bFILE *fp);
  void load_cache_info(spec_directory *sd, bFILE *fp);
  void old_load_objects(spec_directory *sd, bFILE *fp);
  void load_options(spec_directory *sd, bFILE *fp);
  void write_objects(bFILE *fp, object_node *save_list);
  void write_options(bFILE *fp);
  void write_thumb_nail(bFILE *fp, image *im);
  void write_cache_prof_info();
  void restart();


  void unactivate_all();
  // forms all the objects in processing range into a linked list
  int add_actives(long x1, long y1, long x2, long y2);  //returns total added
  void pull_actives(game_object *o, game_object *&last_active, int &t);
  int add_drawables(long x1, long y1, long x2, long y2);  //returns total added

  game_object *find_object(long x, long y);

  game_object *damage_intersect(long x1, long y1, long &x2, long &y2, game_object *exclude);
  game_object *boundary_setback(game_object *subject, long x1, long y1, long &x2, long &y2);
  game_object *all_boundary_setback(game_object *subject, long x1, long y1, long &x2, long &y2);
  int crush(game_object *by_who, int xamount, int yamount);
  int push_characters(game_object *by_who, int xamount, int yamount);  // return 0 if fail on any.
  int platform_push(game_object *by_who, int xamount, int yamount);
  void foreground_intersect(long x1, long y1, long &x2, long &y2);
  void vforeground_intersect(long x1, long y1, long &y2);

  void hurt_radius(long x, long y,long r, long m, game_object *from, game_object *exclude,
		   int max_push);
  void send_signal(long signal);
  void next_focus();
  void to_front(game_object *o);
  void to_back(game_object *o);
  game_object *find_closest(int x, int y, int type, game_object *who);
  game_object *find_xclosest(int x, int y, int type, game_object *who);
  game_object *find_xrange(int x, int y, int type, int xd);
  game_object *find_self(game_object *me);


  void write_links(bFILE *fp, object_node *save_list, object_node *exclude_list);
  void load_links(bFILE *fp, spec_directory *sd, object_node *save_list, object_node *exclude_list);


  game_object *find_type(int type, int skip);
  void insert_players();   // inserts the players into the level


  game_object *get_random_start(int min_player_dist, view *exclude); 
//  game_object *find_enemy(game_object *exclude1, game_object *exclude2); 

  bFILE *create_dir(char *filename, int save_all,
		    object_node *save_list, object_node *exclude_list);
  view *make_view_list(int nplayers);
  long total_light_links(object_node *list);
  long total_object_links(object_node *save_list);
  game_object *find_object_in_area(long x, long y, long x1, long y1, 
				   long x2, long y2, Cell *list, game_object *exclude);
  game_object *find_object_in_angle(long x, long y, long start_angle, long end_angle,
				    void *list, game_object *exclude);
  object_node *make_not_list(object_node *list);
  int load_player_info(bFILE *fp, spec_directory *sd, object_node *save_list);
  void write_player_info(bFILE *fp, object_node *save_list);
  void write_object_info(char *filename);
  void level_loaded_notify();
} ;

extern level *current_level;
void pull_actives(game_object *o, game_object *&last_active, int &t);



#endif

















