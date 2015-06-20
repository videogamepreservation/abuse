#ifndef __TIMING_HPP_
#define __TIMING_HPP_

class time_marker
{
public :
  long seconds;
  long micro_seconds;  
  void get_time();
  time_marker();  
  double diff_time(time_marker *other);    // returns time diff in seconds                   
} ;

void milli_wait(unsigned wait_time);  // sleeps for a milli second 1000 millies=1 sec
void timer_init();
void timer_uninit();

#endif

