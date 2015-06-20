#include "human.hpp"
#include "game.hpp"


void human::do_damage(int amount, game_object *from)
{
  game_object::do_damage(amount,from);   
  the_game->show_help("Ouch, that really hurt!");
    
//  if (!gravity())
//    set_state(flinch);  
}


int human::move(int cx, int cy, int button)
{
/*  if (state==hanging)
  {
    int y_moveup[15]
    if (end_of_sequence())
    {
      int xv=direction,yv
      
      
    }*/
  return game_object::move(cx,cy,button);
  
  
     
}

