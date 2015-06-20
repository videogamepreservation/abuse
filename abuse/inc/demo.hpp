#ifndef __DEMO_HPP_
#define __DEMO_HPP_

#include "jwindow.hpp"

class demo_manager
{
  void *initial_difficulty;
  bFILE *record_file;
  int skip_next;

  public :
  enum demo_state { NORMAL,
		    RECORDING,
		    PLAYING    } state;
  int set_state(demo_state new_state, char *filename=NULL);
  demo_state current_state() { return state; }
  int save_packet(void *packet, int packet_size);   // returns non 0 if actually saved
  int get_packet(void *packet, int &packet_size);   // returns non 0 if actually loaded
  
  int start_playing(char *filename);
  int start_recording(char *filename);
  void reset_game();
  int demo_skip() { if (skip_next) { skip_next--; return 1; } else return 0; }
  demo_manager() { state=NORMAL; skip_next=0; }
  void do_inputs();
} ;

extern demo_manager demo_man;

extern void get_event(event &ev, window_manager *wm);
extern int event_waiting(window_manager *wm);



extern int last_demo_mx,last_demo_my,last_demo_mbut;
//extern ulong demo_tick_on;
#endif

