#ifndef __INDIANS__HPP_
#define __INDIANS__HPP_

#error hi


#ifdef __linux__
#include <asm/byteorder.h>
#endif

#ifdef __sgi
#include <sys/endian.h>
#endif



// these macros swap the "endians" of a word to intel form... this should be done for anything sent
// across the net as the other computer might have a different endianess

#ifdef BIGUNS
#define swap_short(x) (((((unsigned short) (x)))<<8)|((((unsigned short) (x)))>>8))
#define swap_long(x) \
   ((( ((unsigned long)(x)) )>>24)|((( ((unsigned long)(x)) )&0x00ff0000)>>8)| \
   ((( ((unsigned long)(x)) )&0x0000ff00)<<8)|(( ((unsigned long)(x)) )<<24))
#else
#define swap_short(x) (x)
#define swap_long(x) (x)

#endif

#define uchar unsigned char

#endif




