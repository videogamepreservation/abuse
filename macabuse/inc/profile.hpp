#ifndef __JPROF_HPP_
#define __JPROF_HPP_

#include "event.hpp"

void profile_init();
void profile_reset();
void profile_uninit();
void profile_add_time(int type, float amount);
void profile_update();
void profile_toggle();
int profile_handle_event(event &ev);
int profiling(); 


#endif
