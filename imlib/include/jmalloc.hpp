#ifndef __jmalloc_hpp_
#define __jmalloc_hpp_


#include <stdlib.h>

#ifdef MANAGE_MEM
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





