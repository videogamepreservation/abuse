#ifndef __ANT_HPP_
#define __ANT_HPP_

enum { ANT_START,
       ANT_HANGING=15,
       ANT_HIDING=16,
       ANT_FALL_DOWN=1,
       ANT_LANDING=9,
       ANT_RUNNING=2,
       ANT_POUNCE_WAIT=4,
       ANT_JUMP=6,
       ANT_FIRE=8,
       ANT_JUMP_ROOF=12,
       ANT_ROOF_WALK=13,
       ANT_CEIL_SHOOT=14};

void *ant_ai();
void show_stats();
void end_stats();

#endif
