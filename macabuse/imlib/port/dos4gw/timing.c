#include <time.h>
#include "timing.hpp"
#include <dos.h>
#include <stdlib.h>
#include <i86.h>
#include <stdio.h>
#include <conio.h>
#include "dprint.hpp"
#include <string.h>


static void (__interrupt __far *prev_int8)()=NULL;
volatile unsigned long tick_counter,start_tick_counter; // note : if timer handler is installed for more than 497 days then
                                                        // tick_counter will overflow.  ** don't use this in OS code **

void (*install_timer_handler)(void (*fun)())=NULL;   // function to install timer
void (*uninstall_timer_handler)()=NULL;
unsigned char timer_installed=0;


static dostime_t dos_start_time;
static dosdate_t dos_start_date;
static chain_on=0,chain_counter=0;
int total_timer_calls=0;

#define MAX_TIMER_CALLS 10

#define TICKS_PER_SEC 100

struct
{
  int and_call_mask;
  void ( *fun)();  
} timer_calls[MAX_TIMER_CALLS];

void add_timer_call(void ( *fun)(), int and_call_mask)
{
  if (total_timer_calls>=MAX_TIMER_CALLS)
  {
    dprintf("Too many timer calls installed\n");
    exit(0);
  }
  timer_calls[total_timer_calls].fun=fun;
  timer_calls[total_timer_calls].and_call_mask=and_call_mask;
  total_timer_calls++;  
}

void remove_timer_call(void ( *fun)())
{
  for (int i=0;i<total_timer_calls;i++)
  {
    if (timer_calls[i].fun==fun)
    {
      for (int j=i;j<total_timer_calls-1;j++)        
        timer_calls[j]=timer_calls[j+1];
      total_timer_calls--;
      return ;
    }
  }
  dprintf("remove_timer_call : call not installed\n");
  exit(0);
}

void cli();
#pragma aux cli = "cli";

void sti();
#pragma aux sti = "sti";



static int inside=0;  // check for re-entry
void __interrupt __far new_timer_int () 
{ 
  outp(0x20,0x20);          // signal int chip that ints can continue
  tick_counter++;

  if (!inside)
  {    
    inside=1;    
    sti();                  // turn interrupts back on


    for (int i=0;i<total_timer_calls;i++)
      if ((tick_counter&timer_calls[i].and_call_mask)==0)
        timer_calls[i].fun();
    inside=0;
  } else sti();                  // turn interrupts back on

  chain_counter++;               // see if we need to call the normal DOS intr yet
  if (chain_counter==chain_on)
  {
    chain_counter=0;
    _chain_intr(prev_int8);
  }

} 

void timer_stub()  // this should be called int timer_init has been called and install_timer_handler!=NULL
{
  tick_counter++;

  if (!inside)
  {
    inside=1;
    for (int i=0;i<total_timer_calls;i++)
      if ((tick_counter&timer_calls[i].and_call_mask)==0)
        timer_calls[i].fun();
    inside=0;
  }
}

void timer_init()
{
  if (install_timer_handler)
  {
    install_timer_handler(timer_stub);
    timer_installed=1;
  } else
  {
    if (prev_int8)  
      fprintf(stderr,"timer_init called twice (not good)\n");
    else
    {
      _dos_gettime(&dos_start_time);             // get initail time
      _dos_getdate(&dos_start_date);             // get initail date

      prev_int8=_dos_getvect(8);  
      _dos_setvect(0x8008,new_timer_int);       // set protected mode int 8

      int new_divsor;
      if (TICKS_PER_SEC<18.2)
      {
        new_divsor=0xffff;
	chain_on=1;
      }
      else
      {
	chain_on=TICKS_PER_SEC/18.2;
        new_divsor=0xffff/chain_on;
      }

      chain_counter=0;
      cli();
      outp(0x43,0x36);                      // set timer speed
      outp(0x40,(new_divsor&0xff)); 
      outp(0x40,((new_divsor&0xff00)>>8));
      sti();

      timer_installed=1;
    }
    atexit(timer_uninit);
    start_tick_counter=tick_counter=clock();
  }
}

static char dim[12]={31,  // Jan
		     29,  // Feb
		     31,  // March
		     30,  // Apr
		     31,  // May
		     30,  // June
		     31,  // July
		     30,  // Aug
		     31,  // Sept
		     31,  // October
		     30,  // Nov
		     31}; // Dec

static int days_in_month(long month, long year)
{
  if (month==1 && (((year-1)%4)==0))   // check for leap-year
    return 28;
  else return dim[month];
}

void timer_uninit()
{
  chain_on=0;
  if (timer_installed)
  {
    if (uninstall_timer_handler)
      uninstall_timer_handler();
    else
    {
      if (prev_int8)
      {
	outp(0x43,0x36);                      // set timer speed back to 18.2
	outp(0x40,0); 
	outp(0x40,0); 
	_dos_setvect(8,prev_int8);

	unsigned long ticks_passed=tick_counter-start_tick_counter;
	unsigned long hsec=ticks_passed*100/TICKS_PER_SEC;

	// now calculate how much time we stole from DOS and adjust the clock
	dos_start_time.hsecond=((long)dos_start_time.hsecond+hsec)%100;
	dos_start_time.second=((long)dos_start_time.second+hsec/100)%60;
	dos_start_time.minute=((long)dos_start_time.minute+hsec/(100*60))%60;
	dos_start_time.hour=((long)dos_start_time.hour+hsec/(100*60*60))%24;
	
	long days=hsec/(100*60*60*24);
	while (days)
	{
	  dos_start_date.dayofweek=(dos_start_date.dayofweek+2)%7-1;
	  dos_start_date.day=dos_start_date.day+1;
	  if (days_in_month(dos_start_date.month,dos_start_date.year)>dos_start_date.day)
	  {
	    dos_start_date.day=1;
	    dos_start_date.month++;
	    if (dos_start_date.month>12)
	    {
	      dos_start_date.month=1;
	      dos_start_date.year++;
	    }
	  }
	  days--;
	}
	_dos_settime(&dos_start_time);
	_dos_setdate(&dos_start_date);

	prev_int8=NULL;    
      }
    }
    timer_installed=0;
  }
}

void time_marker::get_time()
{
  seconds=0;
  if (timer_installed)
    micro_seconds=tick_counter*1000/TICKS_PER_SEC;
  else
    micro_seconds=clock()*10;
}

time_marker::time_marker()
{
  seconds=0;
  if (timer_installed)
    micro_seconds=tick_counter*1000/TICKS_PER_SEC;  
  else
    micro_seconds=clock()*10;
}

double time_marker::diff_time(time_marker *other)
{
  return (double)(micro_seconds-other->micro_seconds)/1000.0;  
}

void milli_wait(unsigned wait_time)
{
  if (timer_installed)
  {
    unsigned long wait_tick=wait_time*10/TICKS_PER_SEC+tick_counter;
    while (tick_counter<wait_tick) ;
  } else delay(wait_time);                    // timer not installed
}

