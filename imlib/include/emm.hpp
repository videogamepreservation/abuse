#ifndef __EMM_H_
#define __EMM_H_
#include "system.h"
#ifdef __DOS            // this stuff is DOS specific, don't include
			// on other systems
#include "linked.hpp"

class emblock : public linked_node
{
public :
  unsigned beg,end;
  int emhandle;
  emblock *nextblk;
} ;


class expandedmm
{
public :
  unsigned frame_ad;
  long frmsz;
  linked_list allocblks,freeblks;
  expandedmm();

  emblock *alloc(unsigned size);
  void free(emblock *block);
  void free_all();
  long avail() { return frmsz; }
  void set(emblock *block, char *dat);
  void get(emblock *block, char *dat);
  ~expandedmm() { free_all(); }
} ;

extern expandedmm epmem;
extern int  emm_avail;
int      EMM_detect();
int      EMM_num_free_pages();
int      EMM_version();
int      EMM_num_active_handles();
long     EMM_free_memory();
int      EMM_alloc(int pages);  // remeber pages are 16K, 16384 bytes
		// returns handle or -1
int      EMM_free(int handle);   // returns true for success
unsigned EMM_frame_address();
void     EMM_map_page(int handle);

#endif
#endif

