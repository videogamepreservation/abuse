#include "macs.hpp"

struct rminfo {
  long edi,esi,ebp,resv,ebx,edx,ecx,eax;
  ushort flags, es,ds,fs,gs,ip,cs,sp,ss;
} ;


extern void RM_intr(int intr, struct rminfo *rm);      // do a real-mode interrupt
extern void *alloc_low_memory(long size);              // size in bytes
extern void free_low_memory(void *ptr);
extern long low_memory_available();                     // returns size of largest allocatable block
