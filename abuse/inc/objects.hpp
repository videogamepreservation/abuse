#ifndef _OBJECTS_HPP_
#define _OBJECTS_HPP_
#include "input.hpp"
#include "chars.hpp"
#include "sound.hpp"
#include "morpher.hpp"
#include "loader2.hpp"
#include "view.hpp"
#include "extend.hpp"

class view;

extern char **object_names;
extern int total_objects;

#define NOT_BLOCKED 0
#define BLOCKED_LEFT 1
#define BLOCKED_RIGHT 2
#define BLOCKED_UP 4
#define BLOCKED_DOWN 8



#define FIRST_ATTACK atk_fast
#define LAST_ATTACK attack_special




#define RC_C 0
#define RC_S 1
#define RC_L 2





#define TOTAL_OBJECT_VARS 28
struct obj_desc { char *name; int type; } ;
extern obj_desc object_descriptions[TOTAL_OBJECT_VARS];
int RC_type_size(int type);
//void init_object_offsets();


class game_object : public simple_object
{  
  sequence *current_sequence() { return figures[otype]->get_sequence(state); }
public :
  game_object *next,*next_active;
  long *lvars;

  int size();
  int decide();        // returns 0 if you want to be deleted
  int type() { return otype; }
  ifield *make_fields(int ystart, ifield *Next) { return NULL ; }
  void gather_input(input_manager *inm) { ; }
  int hurt_all() { return figures[otype]->get_cflag(CFLAG_HURT_ALL); }
  int stoppable() { return figures[otype]->get_cflag(CFLAG_STOPPABLE); }
  int can_block() { return figures[otype]->get_cflag(CFLAG_CAN_BLOCK); }

  int hurtable() { return figures[otype]->get_cflag(CFLAG_HURTABLE); }
  int pushable() { return figures[otype]->get_cflag(CFLAG_PUSHABLE); }

  void draw();
  void map_draw();
  void draw_trans(int count, int max);
  void draw_tint(int tint_id);
  void draw_double_tint(int tint_id1, int tint_id2);
  void draw_predator();
  

  void drawer();
  void draw_above(view *v);
  void do_damage(int amount, game_object *from, long hitx, long hity, long push_xvel, long push_yvel);
  void damage_fun(int amount, game_object *from, long hitx, long hity, long push_xvel, long push_yvel);


  void note_attack(game_object *whom);
  void recieve_signal(long singal) { ; }
  int push_range();
  int can_hurt(game_object *who);     // collision checking will ask first to see if you
                              // can hurt this person before calculating weither you actually do

  void load(int type, bFILE *fp, unsigned char *state_remap);
  int tick();  // should be called from decide, does the physics on the people, returns blocked status
  void *float_tick();  // returns T or blocked structure = 
                       // (block_flags 'tile tilex tiley) 
                       // (block_flags 'object obj)

  void next_sequence();

  int facing_attacker(int attackerx);
  void set_state(character_state s, int frame_direction=1);
  int has_sequence(character_state s) { return figures[otype]->has_sequence(s); }

  game_object *try_move(long x, long y, long &xv, long &yv, int checks);  // 1=down,2=up,3=both
  game_object *bmove(int &whit, game_object *exclude);  // ballestic move, return hit object, 
                                                        // or NULL (whit is 1 if hit wall)
  trans_image *picture() { return current_sequence()->get_frame(current_frame,direction); }
												    
  int next_picture();
  long x_center();  
  long height();

  void stop_acel() { set_xacel(0);  set_yacel(0); set_fxacel(0); set_fyacel(0); }
  void stop_vel() {  set_xvel(0);   set_yvel(0); set_fxvel(0);  set_fyvel(0); }
  void stop_x() {  set_xvel(0);  set_fxvel(0); set_xacel(0); set_fxacel(0); }
  void stop()
  { set_xvel(0);   set_yvel(0); set_fxvel(0);  set_fyvel(0);
    set_xacel(0);  set_yacel(0); set_fxacel(0); set_fyacel(0);
  }

  int move(int cx, int cy, int button);  // return blocked status
  int mover(int cx, int cy, int button);
  figure *current_figure() { return current_sequence()->get_figure(current_frame); }
  int total_frames() { return current_sequence()->length(); }
  void picture_space(long &x1, long &y1,long &x2, long &y2);
  int tx(int x) { if (direction>0) return x-x_center(); else return x_center()-x; }
  int ty(int y) { return y-picture()->height()+1; }
  void defaults();
  game_object(int Type, int load=0);
  int is_playable() { return hurtable(); }
  void add_power(int amount);
  void add_hp(int amount);
  int can_morph_into(int type);  
  void morph_into(int type, void (*stat_fun)(int), int anneal, int frames);
  void do_flinch(game_object *from);
  void set_aimemory(game_object *p) { add_object(p); p->set_flags(p->flags()|KNOWN_FLAG); }
  int alive() { if (state==dead || hp()<=0) return 0; else return 1; }
  void frame_advance();
  object_node *make_not_list(object_node *first);
  int reduced_state();
  void reload_notify();

  void change_type(int new_type);
  int set_var_by_name(char *name, long value);
  long get_var_by_name(char *name, int &error);
  game_object *copy();
  void change_aitype(int new_type);
  ~game_object();
} ;

class object_node  // used to create various list of objects
{
  public :
  game_object *me;
  object_node *next;
  object_node(game_object *Me, object_node *Next) { me=Me; next=Next; }
} ; 

extern game_object *current_object;
extern view *current_view;
game_object *create(int type, long x, long y, int skip_constructor=0, int aitype=0);
int base_size();

void delete_object_list(object_node *first);
int          object_to_number_in_list(game_object *who, object_node *list);
game_object *number_to_object_in_list(long x, object_node *list);


#endif




