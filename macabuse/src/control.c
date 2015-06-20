#include "game.hpp"
#include "control.hpp"
#include "specs.hpp"
#include "loader.hpp"
#include "hpoint.hpp"
#include "ability.hpp"

#define FADING_FRAMES 20
#define FADING_MAX 30

sequence *morph_table[MORPH_TABLE_WIDTH*MORPH_TABLE_WIDTH];


void controlled_character::note_hp_change(int new_hp, game_object *who)
{ 
  if (cur_char==who)  
    the_game->set_hit_points(hp);  
}


void controlled_character::change_character(int char_type)
{
/*  switch (char_type)
  {
    case CHAR_HUMAN : my_figure=human_figure; curbreak;    
    case CHAR_IGUANA : my_figure=iguana_figure; break;
    default : CONDITION(0,"change_character, bad character type\n");
  }
  cur_char=char_type;
  set_state(stopped);
  
  the_game->change_logos(abilities,cur_char);*/
  
}  


   

controlled_character::controlled_character(game_object *who)
{
  cur_char=who;
  abilities=0;  
  morphing_to=-1;
}


int controlled_character::decide()
{
  int button,c_x,c_y;
  the_game->get_movement(button,c_x,c_y);
  cur_char->move(c_x,c_y,button);  
  return 1;
}












