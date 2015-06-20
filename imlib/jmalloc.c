#include <stdlib.h>
#include <string.h>
#include <stdio.h>
//#include <new.h>

//#ifdef MEM_CHECK
//#define MEM_CLEAR
//#endif

#ifdef __WATCOMC__
#include "doscall.hpp"
#endif
#include "jmalloc.hpp"
#define uchar unsigned char
#define JM_SMALL_SIZE 128      // above 128 bytes is considered to be a big block and no hashing is done
int alloc_space=ALLOC_SPACE_STATIC;

extern void free_up_memory();

#ifdef MEM_CHECK
long break_mem_point=0;       // can be set in debugger, break mem fun will be called when this address is allocated
void break_mem_fun()
{
  printf("memory breakpoint\n");
}
#endif

struct memory_node
{
  long size;
#ifdef MEM_CHECK
  char *name;                     // name is allocated on regular heap
#endif                            // because it is used for debugging purposes
                                  // and will probably be run on my linux box with VMM
  memory_node *next;
};


struct small_block
{  
  unsigned long size;                     // size of blocks...
  unsigned long alloc_list;               // bit field saying weither each block is allocated or not.
  small_block *next;                      // next small block of same size
#ifdef MEM_CHECK
  char *name[32];
#endif 
} ;

enum { HI_BLOCK, LOW_BLOCK }; 

class block_manager
{
  public :

  long block_size;                             // size of this memory_block
  small_block *sblocks[JM_SMALL_SIZE];
  small_block *cblocks[JM_SMALL_SIZE];
  void *addr;

  memory_node *sfirst,*slast,
              *cfirst;
  unsigned char block_type;

  void init(void *block, long Block_size, uchar type);
  void *static_alloc(long size, char *name);
  void *cache_alloc(long size, char *name);
  void static_free(void *ptr);
  void cache_free(void *ptr);
  long available();
  long allocated();
  long pointer_size(void *ptr);
  void report(FILE *fp);
  void inspect();

  int valid_static_ptr(void *ptr);     // only called from within debugger
  int valid_cache_ptr(void *ptr);
} bmanage[5];

int bmanage_total=0;


void inspect_memory()
{
  for (int i=0;i<bmanage_total;i++)
    bmanage[i].inspect();
}


int block_manager::valid_static_ptr(void *ptr)
{
  void *next=(void *)(*(((long *)ptr)-1));
  if (next && ((small_block *)next)->size<JM_SMALL_SIZE)  // small allocation
  {
    small_block *s=(small_block *)next;
    if (s->size<=0) return 0;

    small_block *c=sblocks[s->size];
    while (c && c!=s) c=c->next;
    if (!c) return 0;
  }

  memory_node *o=(memory_node *)(((char *)ptr)-sizeof(memory_node));
  memory_node *f=sfirst;
  while (f && f!=o) f=f->next;
  if (f) return 1;
  else return 0;
}


int block_manager::valid_cache_ptr(void *ptr)
{
  void *next=(void *)(*(((long *)ptr)-1));
  if (next && ((small_block *)next)->size<JM_SMALL_SIZE)  // small allocation
  {
    small_block *s=(small_block *)next;
    if (s->size<=0) return 0;

    small_block *c=cblocks[s->size];
    while (c && c!=s) c=c->next;
    if (!c) return 0;
  }

  memory_node *o=(memory_node *)(((char *)ptr)-sizeof(memory_node));
  memory_node *f=cfirst;
  while (f && f!=o) f=f->next;
  if (f) return 1;
  else return 0;
}

void small_static_allocation_summary(int &total, int *&static_list, int *&cache_list)
{
  int size=1;
  total=JM_SMALL_SIZE/4;
  static_list=(int *)jmalloc(total*sizeof(int),"small static report");
  cache_list=(int *)jmalloc(total*sizeof(int),"small cache report");
  
  for (;size<total;size++)
  {
    static_list[size]=0;
    cache_list[size]=0;
    int i,x;
    for (i=0;i<bmanage_total;i++)
    {
      small_block *s=bmanage[i].sblocks[size];
      while (s) 
      { 
				for (x=0;x<32;x++)
				  if (s->alloc_list&(1<<x))
				    static_list[size]++; 
			
				s=s->next; 
      }

      s=bmanage[i].cblocks[size];
      while (s) 
      { 
				for (x=0;x<32;x++)
				  if (s->alloc_list&(1<<x))
				    cache_list[size]++; 
			
				s=s->next; 
      }
    }
  }
}

void block_manager::inspect()
{
  memory_node *f=sfirst;
  for (;f;f=f->next);               // scan through static big list

  int i,bit;
  for (i=0;i<JM_SMALL_SIZE;i++)
  {
    for (small_block *s=sblocks[i];s;s=s->next)
    {
      char *addr=((char *)(s+1));
      for (int j=0;j<32;j++)
      {
				if (s->alloc_list&bit)
				{
				  void *next=(void *)(*(((long *)addr)));
				  if ((long)next!=(long)s)
				  {
				    fprintf(stderr,"inspect : bad pointer\n");
				    return ;	    
				  }
				}
				bit=bit<<1;
				addr+=s->size+4;
      }	
    }
  }

}

void block_manager::report(FILE *fp)
{
  fprintf(fp,"************** Block size = %d ***************\n",block_size);
  fprintf(fp,"************** STATIC SPACE ******************\n");
  int i=0;
  memory_node *f=sfirst;
  for (;f;f=f->next,i++)
  {    
    fprintf(fp,"%4d   %p (%d) %4d      ",i,f,((char *)f-(char *)sfirst),f->size);
#ifdef MEM_CHECK
    if (f->size>0)
      fprintf(fp,"%s",f->name);
    else fprintf(fp,"FREE");
#endif
    fprintf(fp,"\n");
  }    
  for (i=0;i<JM_SMALL_SIZE;i++)
  {
    for (small_block *s=sblocks[i];s;s=s->next)
    {      
      fprintf(fp,"*** Small Block size = %d ***\n",i);      
      unsigned long x=0,bit=1;
      char *addr=((char *)(s+1));
      for (int j=0;j<32;j++)
      {
				fprintf(fp,"%p   ",addr);
				if (s->alloc_list&bit)
				{
#ifdef MEM_CHECK
				  fprintf(fp,"%s\n",s->name[j]);
#else
				  fprintf(fp,"allocated\n");
#endif	  
				} else fprintf(fp,"FREE\n");
				bit=bit<<1;
				addr+=s->size+4;
      }
    }
  }


  fprintf(fp,"************** CACHE SPACE ******************\n",block_size);
  i=0;
  for (f=cfirst;f;f=f->next,i++)
  {    
    fprintf(fp,"%4d   %p %4d      ",i,f,f->size);
#ifdef MEM_CHECK
    if (f->size>0)
      fprintf(fp,"%s",f->name);
    else fprintf(fp,"FREE");
#endif
    fprintf(fp,"\n");
  }    
  for (i=0;i<JM_SMALL_SIZE;i++)
  {
    for (small_block *s=cblocks[i];s;s=s->next)
    {      
      fprintf(fp,"*** Small Block size = %d ***\n",i);      
      unsigned long x=0,bit=1;
      char *addr=((char *)(s+1));
      for (int j=0;j<32;j++)
      {
	fprintf(fp,"%p   ",addr);
	if (s->alloc_list&bit)
	{
#ifdef MEM_CHECK
	  fprintf(fp,"%s\n",s->name[j]);
#else
	  fprintf(fp,"allocated\n");
#endif	  
	} else fprintf(fp,"FREE\n");
	bit=bit<<1;
	addr+=s->size+4;
      }
    }
  }
}

long block_manager::pointer_size(void *ptr)
{
  void *next=(void *)(*(((long *)ptr)-1));
  if (next>ptr)
    return ((memory_node *)(((char *)ptr)-sizeof(memory_node)))->size;
  else return ((small_block *)next)->size;
}

long block_manager::available()
{
  long size=0;
  memory_node *f;
  for (f=sfirst;f;f=f->next)
    if (f->size<0) size-=f->size;

  for (f=cfirst;f;f=f->next)
    if (f->size<0) size-=f->size;
  return size;
}

long block_manager::allocated()
{
  long size=0;
  memory_node *f;
  for (f=sfirst;f;f=f->next)
    if (f->size>0) size+=f->size;

  for (f=cfirst;f;f=f->next)
    if (f->size>0) size+=f->size;
  return size;
}

void block_manager::init(void *block, long Block_size, uchar type)
{
  block_size=Block_size;
  addr=block;
  /* 
     I'm padding each block, because I'm comparing pointers against size
     in jfree to determine weither a pointer is too a small object or a large alloc
     and it must always be true that the address of the pointer is > JM_SMALL_SIZE 
     All systems I know start pointer address pretty high, but this is a porting consern.     
  */
  
  slast=sfirst=(memory_node *)(((char *)block)+JM_SMALL_SIZE);   
  sfirst->size=-(block_size-sizeof(memory_node)-JM_SMALL_SIZE);
  sfirst->next=NULL;
  cfirst=NULL;
  memset(sblocks,0,sizeof(sblocks));
  memset(cblocks,0,sizeof(cblocks));
  block_type=type;
}

void *block_manager::static_alloc(long size, char *name)
{
  if (size<JM_SMALL_SIZE)
  {
    small_block *s=sblocks[size];
    for (;s && s->alloc_list==0xffffffff;s=s->next);
    if (!s)
    {
      s=(small_block *)static_alloc((size+4)*32+sizeof(small_block),"small_block");
      if (!s) return NULL;   // not enough room for another small block
      s->alloc_list=1;
      s->next=sblocks[size];
      sblocks[size]=s;
      s->size=size;
#ifdef MEM_CHECK
      s->name[0]=strcpy((char *)malloc(strlen(name)+1),name);
      if ((long)s==break_mem_point)
        break_mem_fun();
#endif      
      long *addr=(long *)(((char *)s)+sizeof(small_block));
      *addr=(long)s;
      return (void *)(addr+1);  // return first block
    } else
    {
      int bit=1,i=0,offset=0;
      char *addr=((char *)s)+sizeof(small_block);
      while (1)        // we already know there is a bit free
      {
	if ((s->alloc_list&bit)==0)
	{
	  s->alloc_list|=bit;
#ifdef MEM_CHECK
	  s->name[i]=strcpy((char *)malloc(strlen(name)+1),name);
#endif      	 
	  *((long *)addr)=(long)s;

#ifdef MEM_CHECK
	  if ((long)addr==break_mem_point)
            break_mem_fun();
#endif

	  return (void *)(addr+4);
	}
	i++;
	bit=bit<<1;
	addr+=size+4;
      }      
    }                
  }


  memory_node *s=sfirst;
  for (;s && -s->size<size;s=s->next);
  if (!s) return NULL;
  s->size=-s->size;

  if (s->size-size>sizeof(memory_node)+4)  // is there enough space to split the block?
  {    
    memory_node *p=(memory_node *)((char *)s+sizeof(memory_node)+size);
    if (s==slast)
      slast=p;
    p->size=-(s->size-size-sizeof(memory_node));
#ifdef MEM_CLEAR
//    memset( ((memory_node *)p)+1,0,-p->size);
#endif
    p->next=s->next;
    s->next=p;
    s->size=size;
  }
#ifdef MEM_CHECK
  s->name=strcpy((char *)malloc(strlen(name)+1),name);

  if ((long)s==break_mem_point)
    break_mem_fun();

#endif
  return (void *)(((char *)s)+sizeof(memory_node));
}


void *block_manager::cache_alloc(long size, char *name)
{
  if (size<JM_SMALL_SIZE)
  {
    small_block *s=cblocks[size];
    for (;s && s->alloc_list==0xffffffff;s=s->next);
    if (!s)
    {
      s=(small_block *)cache_alloc((size+4)*32+sizeof(small_block),"small_block");
      if (!s) return NULL;   // not enough room for another small block
      s->alloc_list=1;
      s->next=cblocks[size];
      cblocks[size]=s;
      s->size=size;
#ifdef MEM_CHECK
      s->name[0]=strcpy((char *)malloc(strlen(name)+1),name);

#endif      
      long *addr=(long *)(((char *)s)+sizeof(small_block));
      *addr=(long)s;
#ifdef MEM_CHECK
      if ((long)s==break_mem_point)
        break_mem_fun();
#endif
      return (void *)(addr+1);  // return first block
    } else
    {
      int bit=1,i=0,offset=0;
      char *addr=((char *)s)+sizeof(small_block);
      while (1)        // we already know there is a bit free
      {
	if ((s->alloc_list&bit)==0)
	{
	  s->alloc_list|=bit;
#ifdef MEM_CHECK
	  s->name[i]=strcpy((char *)malloc(strlen(name)+1),name);
	  if ((long)s==break_mem_point)
	    break_mem_fun();
#endif      	 
	  *((long *)addr)=(long)s;
	  return (void *)(addr+4);
	}
	i++;
	bit=bit<<1;
	addr+=size+4;
      }      
    }                
  }


  memory_node *clast=NULL;
  memory_node *s=cfirst;
  for (;s && -s->size<size;s=s->next) clast=s;
  if (!s) // no current cache space for object, see if we can enlarge the cache space
  {
    long size_avail=-slast->size;
    size_avail-=sizeof(memory_node);

    if (slast->size>0 || size_avail<size) // not enough space
      return NULL;
    else
    {
      slast->size+=size+sizeof(memory_node);
      memory_node *nc=(memory_node *)(((char *)(slast)) + (-slast->size+sizeof(memory_node)));
      
      nc->next=NULL;
      nc->size=size;
#ifdef MEM_CHECK
      nc->name=strcpy((char *)malloc(strlen(name)+1),name);      
      if ((long)nc==break_mem_point)
        break_mem_fun();
#endif      
      if (!clast)
        cfirst=nc;
      else clast->next=nc;
      return (void *)(((char *)nc)+sizeof(memory_node));
    }
  }


  s->size=-s->size;

  if (s->size-size>sizeof(memory_node)+4)  // is there enough space to split the block?
  {
    memory_node *p=s;    // store this position
    long psize=s->size-size-sizeof(memory_node);
    s=(memory_node *)(((char *)s)+psize+sizeof(memory_node));
    p->size=-psize;
    s->next=p;
    s->size=size;
    if (cfirst==p) cfirst=s;
    else clast->next=s;
  }
#ifdef MEM_CHECK
  s->name=strcpy((char *)malloc(strlen(name)+1),name);
  if ((long)s==break_mem_point)
    break_mem_fun();
#endif
  return (void *)(((char *)s)+sizeof(memory_node));
}


/************************** CACHE FREE ****************************/
/*    should be called to free a pointer in the cache heap        */
/*    i.e. end of the heap                                        */
/******************************************************************/
void block_manager::cache_free(void *ptr)
{
  // see if this was a small_block allocation
  void *next=(void *)(*(((long *)ptr)-1));
  if (next && ((small_block *)next)->size<JM_SMALL_SIZE)  // small allocation
  {
    small_block *s=(small_block *)next;
    if (s->size<=0)
    {
      fprintf(stderr,"jfree : bad pointer\n");
      return ;
    }

    int field=(((char *)ptr)-((char *)s)-sizeof(small_block))/(s->size+4);
#ifdef MEM_CHECK
    free(s->name[field]);
#endif
    s->alloc_list&=(0xffffffff-(1<<field));
    if (s->alloc_list==0)
    {
      small_block *l=NULL;
      small_block *n=cblocks[s->size];
      for (;n!=s;n=n->next) l=n;
#ifdef MEM_CHECK
      if (!n) 
      { printf("Free small block error\n"); }
#endif
      if (!l)
      cblocks[s->size]=s->next;
      else l->next=s->next;
      cache_free(s);
    }      
  } else
  {
    memory_node *o=(memory_node *)(((char *)ptr)-sizeof(memory_node)),*last=NULL;
    memory_node *n=cfirst;
    for (;n && n!=o;n=n->next) last=n;
#ifdef MEM_CHECK
    if (!n) 
    { printf("Free cached big block error\n"); }
    free(o->name);
#endif
    
    if (last && last->size<0)   // can we add into last block
    {
      memory_node *prev=NULL;
      for (memory_node *n=cfirst;n && n!=last;n=n->next) prev=n;   // find previous to last pointer
      if (prev)
        prev->next=o;
      else cfirst=o;

      o->size=last->size-o->size-sizeof(memory_node);
      last=prev;
    } else o->size=-o->size;
    
    if (!o->next)           // if no next block, then we should add back into static memory
    {
      if (last) last->next=NULL;  // unlink from cache chain
      else cfirst=NULL;

      if (slast->size>0)    // if last static is allocated then create a new free static at end of list
      {
	slast->next=o;
	slast=o;
      } else      
	slast->size+=o->size-sizeof(memory_node);  // else just increase the size of last block
    } else if (o->next->size<0)   // see if we can add into next block
    {
      o->next->size+=o->size-sizeof(memory_node);
      if (last)      
	last->next=o->next;
      else
        cfirst=o->next;
    }
  }  
}



/************************** STATIC FREE ***************************/
/*    should be called to free a pointer in the static heap       */
/*    i.e. begining of the heap                                   */
/******************************************************************/
void block_manager::static_free(void *ptr)
{
  // see if this was a small_block allocation
  void *next=(void *)(*(((long *)ptr)-1));
  if (next && next<ptr)  // small allocation
  {
    small_block *s=(small_block *)next;
    if (s->size<=0)
    {
      fprintf(stderr,"jfree : bad pointer\n");
      return ;
    }
#ifdef MEM_CLEAR
    memset(ptr,0,s->size);
#endif

    int field=(((char *)ptr)-((char *)s)-sizeof(small_block))/(s->size+4);
#ifdef MEM_CHECK
    free(s->name[field]);
#endif
    s->alloc_list&=(0xffffffff-(1<<field));
    if (s->alloc_list==0)
    {
      small_block *l=NULL;
      small_block *n=sblocks[s->size];
      for (;n!=s;n=n->next) l=n;
#ifdef MEM_CHECK
      if (!n) { printf("Free static small block error\n"); }
#endif
      if (!l)
      sblocks[s->size]=s->next;
      else l->next=s->next;
      static_free(s);
    }      
  } else
  {
    memory_node *o=(memory_node *)(((char *)ptr)-sizeof(memory_node)),*last=NULL;
#ifdef MEM_CHECK
    free(o->name);
#endif
#ifdef MEM_CLEAR
    memset(ptr,0,o->size);
#endif

    if (o->next && o->next->size<0)   // see if we can add into next block
    {
      if (o->next==slast)
        slast=o;
      o->size+=-o->next->size+sizeof(memory_node);
      o->next=o->next->next;
    }

    memory_node *n=sfirst;
    for (;n && n!=o;n=n->next) last=n;
#ifdef MEM_CHECK
    if (!n) { printf("Free static big block error\n"); }
#endif
    
    if (last && last->size<0)
    {
      if (o==slast) slast=last;
      last->next=o->next;
      last->size-=o->size+sizeof(memory_node);	
    } else o->size=-o->size;            
  }  
}


void jmalloc_uninit()
{
  for (int i=0;i<bmanage_total;i++)
  {
    switch (bmanage[i].block_type)
    {
      case HI_BLOCK : 
      { free(bmanage[i].addr); } break;
#ifdef __WATCOMC__      
      case LOW_BLOCK :
      { free_low_memory(bmanage[i].addr); } break;
#endif      
    }
  }
  bmanage_total=0;
}

void jmem_cleanup(int ret, void *arg)
{ jmalloc_uninit(); }

 
int jmalloc_max_size=3072000;
int jmalloc_min_low_size=0x1000;
char *not_enough_total_memory_message="Memory manager : Sorry you do not have enough memory available to\n"
                                       "                 run this program.\n"
				       "    DOS users  : Remove any TSR's and device drivers you can.\n"
				       "    UNIX users : Do you have a swapfile/partition setup?\n";
char *not_enough_low_memory_message="Memory Manager : Not enough low memory available (%d : need %d)\n"
                                   "  Suggestions...\n"
				   "    - make a boot disk\n"
				   "    - remove TSR's  & drivers not needed by ABUSE\n"
				   "    - add memory to your system\n";

void jmalloc_init(long min_size)
{
  if (bmanage_total)
    fprintf(stderr,"warning : jmalloc_init called twice\n");
  else
  {
//    exit_proc(jmem_cleanup,jmalloc_uninit);          // make sure memory gets freed up on exit
    void *mem;

#ifdef __POWERPC__
    long size=jmalloc_max_size-0x10000;
    for (mem=NULL;!mem && size>0x10000;)
    {
      mem=malloc(size+0x10000);
      if (!mem) size-=0x100;        
    }
    free(mem);
    mem = malloc(size);
#else
    long size=jmalloc_max_size;
    for (mem=NULL;!mem && size>0x4000;)
    {
      mem=malloc(size);
      if (!mem) size-=0x100;        
    }
#endif
    if (mem)
    {
			bmanage[bmanage_total].init(mem,size,HI_BLOCK);
			bmanage_total++;      
			fprintf(stderr,"Added himem block (%d bytes)\n",size);
    }

/*    bmanage[bmanage_total].init(malloc(2039552),2039552,HI_BLOCK);
    bmanage_total++;      
    bmanage[bmanage_total].init(malloc(150224),150224,HI_BLOCK);
    bmanage_total++;      */



#ifdef __WATCOMC__
    if (size!=jmalloc_max_size)
    {
      do
      {
				size=low_memory_available();
				if (size>jmalloc_min_low_size+0x1000)              // save 64K for misc low memory needs
				{
				  bmanage[bmanage_total].init(alloc_low_memory(size-jmalloc_min_low_size-0x1000),size-jmalloc_min_low_size-0x1000,LOW_BLOCK);
				  bmanage_total++; 
				  fprintf(stderr,"Added low memory block (%d bytes)\n",size);
				}
      } while (size>jmalloc_min_low_size+0x1000);
      if (size<jmalloc_min_low_size)
      {
				fprintf(stderr,not_enough_low_memory_message,size,jmalloc_min_low_size);
				exit(0);
      }
    }
#endif
 

    fprintf(stderr,"Memory available : %d\n",j_available());
    if (j_available()<min_size)
    {
      fprintf(stderr,not_enough_total_memory_message);
      exit(0);
    }

  }
}


long j_available()
{
  long size=0;
  for (int i=0;i<bmanage_total;i++) 
    size+=bmanage[i].available();
  return size;
}

long j_allocated()
{
  long size=0;
  for (int i=0;i<bmanage_total;i++) 
    size+=bmanage[i].allocated();
  return size;
}


void *jmalloc(long size, char *name)
{  
  if (!bmanage_total)
    return malloc(size);

  size=(size+3)&(0xffffffff-3);
  do
  {
    for (int i=0;i<bmanage_total;i++)
    {
      void *a;
      if (alloc_space==ALLOC_SPACE_STATIC)
        a=bmanage[i].static_alloc(size,name);
      else
        a=bmanage[i].cache_alloc(size,name);
      if (a) return a;
    }
    free_up_memory();
  } while (1);  
}

void jfree(void *ptr)
{
  if (ptr == NULL)
    return;
  if (!bmanage_total) 
  { 
    free(ptr); 
    return ; 
  }
  for (int i=0;i<bmanage_total;i++)
    if (ptr>=(void *)bmanage[i].sfirst)  // is the pointer in this block?
    {
      if (ptr<=(void *)bmanage[i].slast)  // is it in static space?
      {
	bmanage[i].static_free(ptr);
	return ;
      } else if (ptr<=(void *)(((char *)bmanage[i].sfirst)+bmanage[i].block_size))  // or cache space?
      {
	bmanage[i].cache_free(ptr);
	return ;
      }
    }

  free (ptr);  
//  fprintf(stderr,"jfree : bad pointer\n");
}


void *jrealloc(void *ptr, long size, char *name)
{  
  if (!ptr) return jmalloc(size,name);
  if (!bmanage_total) { return realloc(ptr,size); }

  if (size==0) { jfree(ptr); return NULL; }

  long old_size=0;
  for (int i=0;i<bmanage_total;i++)
    if (ptr>=(void *)bmanage[i].sfirst && 
	ptr<=(void *)(((char *)bmanage[i].sfirst)+bmanage[i].block_size))
    {
      old_size=bmanage[i].pointer_size(ptr);  
      if (ptr<=(void *)bmanage[i].slast)
      {
	int sp=alloc_space; sp=ALLOC_SPACE_STATIC;
	void *nptr=jmalloc(size,name);
	if (size>old_size)
	  memcpy(nptr,ptr,old_size);
 	else memcpy(nptr,ptr,size);
	bmanage[i].static_free(ptr);
	alloc_space=sp;
	return nptr;
      } else
      {
	int sp=alloc_space; sp=ALLOC_SPACE_CACHE;
	void *nptr=jmalloc(size,name);
	if (size>old_size)
	  memcpy(nptr,ptr,old_size);
	else memcpy(nptr,ptr,size);
	bmanage[i].cache_free(ptr);
	alloc_space=sp;
	return nptr;
      }
    }
  fprintf(stderr,"jrealloc : bad pointer\n");
  return NULL;
}

void dmem_report()
{
  mem_report("debug.mem");
}


void mem_report(char *filename)
{
  FILE *fp=fopen(filename,"wb");
  for (int i=0;i<bmanage_total;i++)
    bmanage[i].report(fp);
  fclose(fp);
}

void *operator new( size_t size)
{  
  return jmalloc(size,"::new object");
} 

void operator delete(void *ptr)
{
  jfree(ptr);
}


long small_ptr_size(void *ptr)
{
  return ((small_block *)(((long *)ptr)[-1]))->size;
}


int valid_ptr(void *ptr)
{
  if (!bmanage_total) { return 0; }
  for (int i=0;i<bmanage_total;i++)
    if (ptr>=(void *)bmanage[i].sfirst)  // is the pointer in this block?
    {
      if (ptr<=(void *)bmanage[i].slast)  // is it in static space?
      {
	return bmanage[i].valid_static_ptr(ptr);
      } else if (ptr<=(void *)(((char *)bmanage[i].sfirst)+bmanage[i].block_size))  // or cache space?
      {
	return bmanage[i].valid_cache_ptr(ptr);
      }
    }

  return 0;
}

