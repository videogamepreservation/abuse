#ifndef ABILITIES_HPP_
#define ABILITIES_HPP_

enum ability
{	start_hp,
	start_accel,
	stop_accel,
        jump_xvel,
	jump_yvel,
	run_top_speed,
	jump_top_speed,

	tint_color,
	push_xrange,
	walk_top_speed                  // keep as last entry!
} ;

#define TOTAL_ABILITIES (walk_top_speed+1)
extern char *ability_names[TOTAL_ABILITIES];
long get_ability(int who, ability a);
long get_ability_default(ability a);

#endif


