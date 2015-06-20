#include "lisp.hpp"
#include "lisp_gc.hpp"
#include "compiled.hpp"
#include "objects.hpp"
#include "level.hpp"
#include "game.hpp"
#include "jrand.hpp"
#include "clisp.hpp"

enum { un_offable };     // vars

void *sensor_ai()
{
  game_object *o=current_object,*b;
  if (o->aistate()==0)                     // turned off, what for player to enter
  {
    if (player_list->next)                 // find closest player
      b=current_level->attacker(current_object);
    else b=player_list->focus;
    if (abs(b->x-o->x)<o->xvel() && abs(b->y-o->y)<o->yvel())  // inside area?
    {
      if (!o->hp())
        o->set_aistate(1);
      else
        o->set_aistate(o->hp());
      o->set_state((character_state)S_blocking);      
    } else if (o->state!=stopped)
      o->set_state(stopped);    
  } else if (!o->lvars[un_offable])
  {
    if (!o->hp())
    {
      if (player_list->next)
        b=current_level->attacker(current_object);
      else b=player_list->focus;
      if (abs(o->x-b->x)>o->xacel() || abs(o->y-b->y)>o->yacel())
        o->set_aistate(0);
    } else o->set_aistate(o->aistate()-1);
  }
  return true_symbol;
}
