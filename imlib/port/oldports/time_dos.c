#include <time.h>
#include "timing.hpp"
#include <dos.h>

time_marker::time_marker()
{
  micro_seconds=(long)clock();  
}

double time_marker::diff_time(time_marker *other)
{
  return (double)(micro_seconds-other->micro_seconds)/(double)(CLOCKS_PER_SEC);  
}

void milli_wait(unsigned wait_time)
{
  delay(wait_time);
}

