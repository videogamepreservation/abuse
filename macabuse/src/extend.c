/*



  Simple object             (power ups, non-moving objects)
    long x,y;
    schar direction;
    ushort otype,state
    ushort current_frame;
    extension *


  Moving object             (simple lisp controlled characters)
     uchar flags;
     long xvel,yvel,xacel,yacel;
     uchar fx,fy,fxvel,fyvel,fxacel,fyacel,aitype;
     ushort aistate,aistate_time;
     unsigned short hp,mp,
     extension *


  Complex objects          (can controll lights, other characters, and have a neural net ai)
    uchar tobjs,tlights;
    object_list *                       
    light_list *
    nnet_info *
    schar fade_dir, frame_dir;        
    unsigned char fade_count,fade_max;
    morph_char *morph_status;


*/
#include "extend.hpp"
#include "view.hpp"
#include "objects.hpp"
#include "lisp.hpp"


void simple_object::add_light(light_source *ls)
{ 
  if (!ls) return ;
  ls->known=1;
  for (int i=0;i<tlights;i++) if (lights[i]==ls) return;
  tlights++; 
  lights=(light_source **)jrealloc(lights,sizeof(light_source *)*tlights,"Light list");
  lights[tlights-1]=ls;
}

void simple_object::add_object(game_object *o)
{
  if (!o) return ;
  for (int i=0;i<tobjs;i++) if (objs[i]==o) return;
  o->set_flags(o->flags()|KNOWN_FLAG);
  tobjs++;
  objs=(game_object **)jrealloc(objs,sizeof(game_object *)*tobjs,"Object list");
  objs[tobjs-1]=o;
}


void simple_object::remove_light(light_source *ls)
{
  for (int i=0;i<tlights;i++) 
  {
    if (lights[i]==ls)
    {
      tlights--;
      for (int j=i;j<tlights;j++)     // don't even think about it :)
        lights[j]=lights[j+1];
      lights=(light_source **)jrealloc(lights,sizeof(light_source *)*tlights,"object's lights");
      return ;
    }
  }
}

void simple_object::remove_object(game_object *o)
{
  for (int i=0;i<tobjs;i++) 
  {
    if (objs[i]==o)
    {
      tobjs--;
      for (int j=i;j<tobjs;j++)     // don't even think about it :)
        objs[j]=objs[j+1];
      objs=(game_object **)jrealloc(objs,sizeof(game_object *)*tobjs,"object's lights");
      return ;
    }
  }
}


simple_object::simple_object()
{
  
  x=y=0;
  direction=1;
  otype=0;
  state=stopped;
  current_frame=0;

  Fade_dir=0;
  Fade_count=0;
  Fade_max=16;


  tobjs=tlights=0;
  objs=NULL;
  lights=NULL;
  Frame_dir=1;
  mc=NULL;
  Controller=NULL;

  Flags=0;
  Xvel=Yvel=Xacel=Yacel=0;
  Fx=Fy=Fxvel=Fyvel=Fxacel=Fyacel=Aitype=0;
  Aistate=Aistate_time=0;
  Hp=Mp=Fmp=0;
  grav_on=1;
  targetable_on=1;
}



void simple_object::set_morph_status(morph_char *Mc)
{
  mc=Mc;
}

void simple_object::clean_up()
{
  if (tlights) jfree(lights);
  if (tobjs)   jfree(objs);
  if (Controller)
    Controller->focus=NULL;
}


simple_object default_simple;


