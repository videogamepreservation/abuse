#include <stdio.h>
#include <stdlib.h>
#ifdef __POWERPC__
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <unistd.h>
#include "timing.hpp"

void timer_init() { ; }
void timer_uninit() { ; }


double time_marker::diff_time(time_marker *other)
{  
  return (double)(seconds-other->seconds)+  (double)(micro_seconds-other->micro_seconds)/1000000;
}

void time_marker::get_time()
{
  micro_seconds = clock();
  seconds = micro_seconds/CLOCKS_PER_SEC;
  micro_seconds = (micro_seconds%CLOCKS_PER_SEC)*1000000/CLOCKS_PER_SEC;
}

time_marker::time_marker() { get_time(); }

void milli_wait(unsigned wait_time)
{
	clock_t tim;
	
	tim = clock();
	tim += (clock_t)(wait_time*CLOCKS_PER_SEC/1000);
	while ( (signed long)tim - (signed long)clock() > 0 ) ;
//		SystemTask();
}

