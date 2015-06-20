#include "system.h"

#ifdef __UNIX_XWIN
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <signal.h>


//#ifndef setitimer
//extern "C" {
//int setitimer(int Which, struct itimerval *Value,
//				struct itimerval *Ovalue);
//} ;
//#endif

struct sigaction sa;
typedef void (*int_handler)();
#define SETSIG(sig,fun) (int_handler)sa.sa_handler=fun; \
			sa.sa_flags=0; \
			sigemptyset(&sa.sa_mask); \
			sigaddset(&sa.sa_mask,SIGALRM); \
			sigaction(sig,&sa,NULL);



//void timer_handler()
//{
//  SETSIG(SIGALRM,timer_handler);
//  printf("%ld\n",jclock++);
//  fflush(stdout);
//}


void init_timer(int_handler int_proc, long utime)
{
  itimerval newt;
  SETSIG(SIGALRM,int_proc);
  newt.it_interval.tv_sec=0;
  newt.it_interval.tv_usec=utime;
  newt.it_value.tv_sec=0;
  newt.it_value.tv_usec=utime;
  setitimer(ITIMER_REAL,&newt,NULL);
}
#else
  #ifdef __BCPLUSPLUS__
  #include <stdio.h>
  #include <dos.h>
  #include <conio.h>

  #define INTR 0X1C    /* The clock tick interrupt */
  void interrupt ( *oldhandler)(...);

  int count=0;

  void interrupt handler(...)
  {
  /* increase the global counter */
     count++;

  /* call the old routine */
     oldhandler();
  }

  int main(void)
  {
  /* save the old interrupt vector */
     oldhandler = getvect(INTR);

  /* install the new interrupt handler */
     setvect(INTR, handler);

  /* loop until the counter exceeds 20 */
     while (count < 20)
	printf("count is %d\n",count);

  /* reset the old interrupt handler */
     setvect(INTR, oldhandler);

     return 0;
  }



  #else
  #error Timer not supported for this compiler! Use Borland or gcc
  #endif
#endif












