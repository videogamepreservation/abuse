#ifndef __jmalloc_hpp_
#define __jmalloc_hpp_


#include <stdlib.h>

#ifdef MANAGE_MEM


#ifdef MEM_CHECK
#include <stdlib.h>
extern void *operator new( size_t size, char *file, unsigned long line);
#define new new(__FILE__,__LINE__)
#endif


enum {ALLOC_SPACE_STATIC,ALLOC_SPACE_CACHE};
extern int alloc_space;
void *jmalloc(long size, char *what_for);
void *jrealloc(void *ptr, long size, char *what_for);
void jfree(void *ptr);
void mem_report(char *filename);
void jmalloc_init(long min_size);
void jmalloc_uninit();
long j_allocated();
long j_available();
extern void free_up_memory();
#else
#define jmalloc(x,y) malloc(x)
#define jrealloc(x,y,z) realloc(x,y)
#define jfree(x) free(x)
#endif




#endif





