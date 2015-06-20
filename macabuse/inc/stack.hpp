#ifndef __STACK_HPP_
#define __STACK_HPP_

#ifndef NO_LIBS
#include "jmalloc.hpp"

#include "dprint.hpp"
#else
#include "fakelib.hpp"
#endif

#include <stdio.h>
struct cons_cell;

template<class T> class grow_stack        // stack does not shrink
{ 
  public :
  T **sdata;
  long son;

  grow_stack(int max_size) { sdata=(T **)jmalloc(max_size,"pointer stack");  son=0; }
  void push(T *data) 
  {
    sdata[son]=data;
    son++;
  }
   
  T *pop(long total) 
  { if (total>son) { lbreak("stack underflow\n"); exit(0); }
    son-=total;
    return sdata[son];
  }
  void clean_up() 
  { 
    if (son!=0) dprintf("Warning cleaning up stack and not empty\n");
    jfree(sdata); 
    sdata=NULL;  son=0; 
  }
} ;

#endif
