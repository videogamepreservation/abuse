#ifndef MACS__
#define MACS__
#include "system.h"
#include "dprint.hpp"
#include <stdio.h>

#define ERROR(x,st) { if (!(x)) \
   { dprintf("Error on line %d of %s : %s\n", \
     __LINE__,__FILE__,st); exit(1); } }

// These macros should be removed for the non-debugging version
#ifdef NO_CHECK
#define CONDITION(x,st) 
#define CHECK(x) 
#else
#define CONDITION(x,st) ERROR(x,st)
#define CHECK(x) CONDITION(x,"Check stop");
#endif


#ifndef min
#define min(x,y) (x<y ? x:y)
#endif
#ifndef max
#define max(x,y) (x>y ? x:y)
#endif

#define uchar  unsigned char
#define schar  signed char
#define ushort unsigned short
#define sshort signed short
#define ulong  unsigned long

#endif
