#ifndef __HELP_HPP_
#define __HELP_HPP_

#include "event.hpp"

extern int total_help_screens;
extern int *help_screens;
void help_handle_event(event &ev);
void draw_help();        // called from game draw if in help mode

#endif





