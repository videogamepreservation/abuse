#include "ability.hpp"
#include "chars.hpp"

/*int abil[TOTAL_OBJECTS*TOTAL_ABILITIES]= { 
/                 hp starta stopa jumpxv  jumpyv crouch_tops run_tops jmp_tops wlk_tops    /
/ human    /     50,     3,    4,    8,     -24,     3,        6,      8,        4,
/ iguana   /     4,      3,    4,    10,     -34,    0,        8,      10,       6, 
/ who      /     2,      2,    4,    20,      0,     0,        18,      20,      18,

/ account1 /     3,      3,    4,    10,     -20,    0,        6,       8,       5,
/ cop      /     6,      3,    4,    10,     -14,    0,        6,       20,      6,
/ laser    /     2,      0,    0,    0,       0,     0,        0,       0,       0,
/ elevator /     2,      2,    2,    0,       0,     0,        1,       1,       1,
/ sensor  /      0,      0,    0,    0,       0,     0,        0,       0,       0,
/ elcontrol /    0,      0,    0,    0,       0,     0,        0,       0,       0,
/ no type  /     0,      0,    0,    0,       0,     0,        0,       0,       0
} ;*/


char *ability_names[TOTAL_ABILITIES]=
   {"start_hp","start_accel","stop_accel",
    "jump_xvel","jump_yvel",
    "run_top_speed","jump_top_speed",
    "tint_color",
    "push_xrange",
    "walk_top_speed"};


long abil_def[TOTAL_ABILITIES]=
   {6,   3,     4,    2,    -16,   10,       6,         0,          
/* hp starta stopa jumpxv  jumpyv run_tops jmp_tops blood_type */
   
/* push_xrange wlk_tops */
      0,            6 };

long get_ability_default(ability a)
{
  return abil_def[a];
}

long get_ability(int who, ability a)
{
  return figures[who]->abil[a];
}










