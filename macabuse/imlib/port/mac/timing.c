#include "system.h"
#include "timing.hpp"
#include <stdio.h>
#include <stdlib.h>
#ifdef __MAC__
#include <Timer.h>
#else
#include <sys/time.h>
#endif
#include <unistd.h>

void timer_init() { ; }
void timer_uninit() { ; }


double time_marker::diff_time(time_marker *other)
{  
  return ((double)(micro_seconds - other->micro_seconds))/1000000.0;
}

void time_marker::get_time()
{
	UnsignedWide tm;
	
  Microseconds(&tm);
  micro_seconds = tm.lo;
}

time_marker::time_marker() { get_time(); }

void milli_wait(unsigned wait_time)
{
	UnsignedWide tim,tim2;
	
	Microseconds(&tim);
	Microseconds(&tim2);
	while ( (tim2.lo - tim.lo) < wait_time*1000)
		Microseconds(&tim2);
//		SystemTask();
}

