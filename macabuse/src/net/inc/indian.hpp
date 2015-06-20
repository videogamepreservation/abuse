#ifndef lltl

#ifdef __sgi
#define BIGUNS
#endif


// these macros swap the "endians" of a word to intel form... this should be done for anything sent
// across the net as the other computer might have a different endianess

#ifdef BIGUNS
#define lstl(x) (((((unsigned short) (x)))<<8)|((((unsigned short) (x)))>>8))
#define lltl(x) \
   ((( ((unsigned long)(x)) )>>24)|((( ((unsigned long)(x)) )&0x00ff0000)>>8)| \
   ((( ((unsigned long)(x)) )&0x0000ff00)<<8)|(( ((unsigned long)(x)) )<<24))
#else
#define lstl(x) (x)
#define lltl(x) (x)

#endif

#define uchar unsigned char

#endif





