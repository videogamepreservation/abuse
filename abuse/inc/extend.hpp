#ifndef EXTEND_HPP_
#define EXTEND_HPP_

#define NNET_HISTSIZE 10
#define FLAG_JUST_HIT     1
#define FLAG_JUST_BLOCKED 2
#define FLOATING_FLAG     4
#define KNOWN_FLAG        8


#include "jmalloc.hpp"
#include "macs.hpp"
#include "morpher.hpp"
#include "chars.hpp"
#include "lisp.hpp"


class view;
class game_object;
class light_source;

class simple_object
{
  public :
  schar Fade_dir;
  uchar Fade_count,Fade_max;
  uchar Flags,grav_on,targetable_on;
  long Xvel,Yvel,Xacel,Yacel;
  uchar Fx,Fy,Fxvel,Fyvel,Fxacel,Fyacel;
  uchar Aitype;
  ushort Aistate,Aistate_time;
  unsigned short Hp,Mp,Fmp;
  schar Frame_dir;        


  uchar tobjs,tlights;
  game_object **objs,*link;
  light_source **lights;

  view *Controller;
  morph_char *mc;
  int total_vars();
  char *var_name(int x);
  int   var_type(int x);
  void set_var(int x, ulong v);
  long get_var(int x);

  // leave these public, so I don't have monster code changes.
  simple_object();
  long x,y,
       last_x,last_y;              // used for frame interpolation on fast machines
  schar direction,active;
  ushort otype;
  character_state state;
  short current_frame;

  int targetable()           { return targetable_on; }
  int gravity()              { return grav_on; }
  int floating()             { return flags()&FLOATING_FLAG; }

  int keep_ai_info()         { return 1; }
  uchar flags()              { return Flags; }
  long xvel()                { return Xvel; }
  long yvel()                { return Yvel; }
  long xacel()               { return Xacel; }
  long yacel()               { return Yacel; }

  uchar fx()                 { return Fx; }
  uchar fy()                 { return Fy; }
  uchar fxvel()              { return Fxvel; }
  uchar fyvel()              { return Fyvel; }
  uchar fxacel()             { return Fxacel; }
  uchar fyacel()             { return Fyacel; }

  uchar sfx()                { return Fx; }  // x & y should always be positive
  uchar sfy()                { return Fy; }
  uchar sfxvel()             { if (Xvel>=0) return Fxvel; else return -Fxvel; }
  uchar sfyvel()             { if (Yvel>=0) return Fyvel; else return -Fyvel; }
  uchar sfxacel()            { if (Xacel>=0) return Fxacel; else return -Fxacel; }
  uchar sfyacel()            { if (Yacel>=0) return Fyacel; else return -Fyacel; }

  uchar aitype()             { return Aitype; }
  ushort aistate()           { return Aistate; }
  ushort aistate_time()      { return Aistate_time; }
  ushort hp()                { return Hp;         }
  ushort mp()                { return Mp;         }
  ushort fmp()               { return Fmp;        }
  schar fade_dir()           { return Fade_dir;   }
  schar frame_dir()          { return Frame_dir;  }
  uchar fade_count()         { return Fade_count; }
  uchar fade_max()           { return Fade_max;   }
  uchar total_objects()      { return tobjs;      }
  uchar total_lights()       { return tlights;    }

  morph_char *morph_status()     { return mc; }
  light_source *get_light(int x)     
  { if (x>=tlights) { lbreak("bad x for light\n"); exit(0); } return lights[x]; }
  game_object *get_object(int x)    
  { if (x>=tobjs) { lbreak("bad x for object\n"); exit(0); } return objs[x]; }
  view *controller()             { return Controller; }

  void set_targetable(uchar x)    { targetable_on=x; }
  void set_flags(uchar f)         { Flags=f; }
  void set_xvel(long xv)          { Xvel=xv; }
  void set_yvel(long yv)          { Yvel=yv; }
  void set_xacel(long xa)         { Xacel=xa; }
  void set_yacel(long ya)         { Yacel=ya; }
  void set_fx(uchar x)            { Fx=x; }
  void set_fy(uchar y)            { Fy=y; }
  void set_fxvel(uchar xv)        { Fxvel=abs(xv); }
  void set_fyvel(uchar yv)        { Fyvel=abs(yv); }
  void set_fxacel(uchar xa)       { Fxacel=abs(xa); }
  void set_fyacel(uchar ya)       { Fyacel=abs(ya); }
  void set_aitype(uchar t)        { Aitype=t; }
  void set_aistate(ushort s)      { Aistate=s; }
  void set_aistate_time(ushort t) { Aistate_time=t; }
  void set_hp(ushort h)           { Hp=h; }
  void set_mp(ushort m)           { Mp=m; }
  void set_fmp(ushort m)          { Fmp=m; }



  void set_fade_count(uchar f)          { Fade_count=f; }
  void set_fade_max(uchar m)            { Fade_max=m;  }
  void set_fade_dir(schar d)            { Fade_dir=d; }

  void set_frame_dir(schar d)           { Frame_dir=d; }
  void add_light(light_source *ls);
  void add_object(game_object *o);

  void remove_object(game_object *o);
  void remove_light(light_source *ls);
  void set_morph_status(morph_char *mc);
  void set_controller(view *v)          { Controller=v; }

  void set_gravity(int x)               { grav_on=x; }
  void set_floating(int x)
  { if (x) 
      set_flags(flags()|FLOATING_FLAG); 
    else 
      set_flags(flags()&(0xff-FLOATING_FLAG)); 
  }

  void clean_up();
} ;


extern simple_object default_simple;

#endif







