#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
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
  struct timezone tz={0,DST_USA};     
  gettimeofday((struct timeval *)&seconds,&tz);
}

time_marker::time_marker()
{ get_time(); }

void milli_wait(unsigned wait_time)
{
  usleep((long)wait_time*(long)1000);
}

