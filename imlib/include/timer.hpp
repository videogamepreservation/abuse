#ifndef __TIMER_HPP_
#define __TIMER_HPP_
typedef void (*int_handler)();
void init_timer(int_handler int_proc, long utime);
#endif

