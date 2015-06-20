#include <malloc.h>
#include "jmalloc.hpp"
#include <stdio.h>
#include "macs.hpp"
#include <string.h>
#include "exitproc.hpp"
#include "dprint.hpp" 


#ifdef __WATCOMC__
#include <dos.h>
#include <new.h>



char *WAT_dos_alloc(long &size)
{
  union REGS r;
  r.x.eax=0x0100;
  r.x.ebx=(size+15)>>4;
  int386(0x31,&r,&r);
 
  if (r.x.cflag)
  {
    size=(long)r.w.bx*16;
    return NULL;
  }
  else 
    return (char *)((r.x.eax&0xffff)<<4);
}

void WAT_dos_free(void *ptr)
{
  union REGS r;
  r.x.eax=0x0101;
  r.x.edx=((long)ptr)>>4;
  printf("free : segment is %d\n",r.w.dx);
  int386(0x31,&r,&r);
  if (r.x.cflag)
    printf("DOS_free failed\n");

}

#endif

struct memory_node
{
  memory_node *next;
  long size;
#ifdef MEM_CHECK
  char *name;                     // name is allocated on regular heap
#endif                            // because it is used for debugging purposes
                                  // and will probably be run on my linux box with VMM
  memory_node *next_free;         // if free (size<0) this is a pointer to the next free block
                                  // otherwise data starts here
};



struct memory_block
{
  int type;
  memory_node *fnode;
} ;


#define JM_SMALL_SIZE 128      // above 128 bytes is considered to be a big block and no hashing is done

memory_node *jm_small[JM_SMALL_SIZE/4];
memory_node *jm_big;

memory_node *last_used=NULL;

#define MAX_BLOCKS 4
long mem_blocks=0;
memory_block blocks[MAX_BLOCKS];

#define REG_MEM    1
#define LOW_MEM    2
#define STATIC_MEM 3


#ifdef MEM_CHECK
void mem_check()
{
  int i,j;
  for (i=0;i<mem_blocks;i++)
  {
    memory_node *last=blocks[i].fnode,*n;
    for (j=0,n=last->next;n;n=n->next,j++)
    {
      if (last>=n)
      {
	printf("Memory corrupted, block #%d\n",j);
	if (last->size>=0)
	  printf("last block name is %s\n",last->name);
	else printf("last block was free\n");
	mem_report("corrupt");
      }
    }    
  }


}
#endif

void add_block(void *addr, long size, int type)
{
  if (mem_blocks<MAX_BLOCKS-1)
  {   
    blocks[mem_blocks].fnode=(memory_node *)addr;
    blocks[mem_blocks].type=type;

    memory_node *f=blocks[mem_blocks].fnode;
    f->size=-size+sizeof(memory_node)-sizeof(memory_node *);
    f->next=NULL;
    f->next_free=jm_big;
    jm_big=f;
    mem_blocks++;
  }
  else
    fprintf(stderr,"added more than MEM_BLOCKS blocks\n");
}

void *operator new( size_t size)
{
  return jmalloc(size,"::new object");
} 

void operator delete(void *ptr)
{
  jfree(ptr);
}

void jmem_cleanup(int ret, void *arg)
{ jmalloc_uninit(); }

void jmalloc_init(long min_size)  // allocates as much memory as possible, craps if min_size too big
{
  if (mem_blocks)
    dprintf("warning : jmalloc_init called twice\n");
  else
  {
    memset(jm_small,0,sizeof(memory_node *)*JM_SMALL_SIZE/4);      // clear out old free stacks
    jm_big=NULL;

    exit_proc(jmem_cleanup,jmalloc_uninit);          // make sure memory gets freed up on exit

    void *mem_start;
    long mem_size; 

    mem_start=NULL;

    for (mem_size=4000000;!mem_start && mem_size>0x4000;mem_size-=0x100)  // allocate 4 MB
      mem_start=malloc(mem_size);
    if (mem_start)
    {
      free(mem_start);
      mem_size-=0x4000;
      mem_start=malloc(mem_size);     // save some space on regular heap
      dprintf("Memory subsystem : added high mem block (%d bytes)\n",mem_size);
      add_block(mem_start,mem_size,REG_MEM);
    }

#ifdef __WATCOMC__                      // allocate low memory from DOS
    long dos_size=0xA0000;
    char *dmem=(char *)WAT_dos_alloc(dos_size);
    if (dmem) { printf("expecting dos_alloc to fail for %d bytes\n",0xa0000); }
    if (dos_size<12000)
      dprintf("Memory subsystem : low memory not used.. only %d bytes available\n",dos_size);
    else
    {
      dos_size-=10000;   // 10k in case we need to for something else
      dmem=(char *)WAT_dos_alloc(dos_size);
      if (dmem)
      {
	add_block(dmem,dos_size,LOW_MEM);
	dprintf("Memory subsystem : using %d bytes of low memory\n",dos_size);
      } else dprintf("error in jmalloc_init\n");
    }
#endif

    
    if (j_available()<min_size)
    {
      fprintf(stderr,"available memory = %d bytes, need %d\n",j_available(),min_size);
      fprintf(stderr,"You do not have enough memory available!\n"
	      "  DOS users  : Make sure you have himem.sys or other extended memory\n"
	      "               manager running. (look in your config.sys)\n"
	      "               Can you remove any TSR/driver programs?\n"
	      "  UNIX users : Do you have a swap file/partition installed?\n"
	      "  MAC users  : I don't think so....... :)    -JC\n"); 
      exit(0);
    } 
  }
}

void jmalloc_uninit()
{ 
  if (mem_blocks)
  {
    int i;
    for (i=0;i<mem_blocks;i++)
    {
      switch (blocks[i].type)
      {
	case REG_MEM : 
	{ 
	  free((void *)blocks[i].fnode); } break;
/*#ifdef __WATCOMC__         don't do this, because we don't know the segment numer
                             and the memory manager will clean up for us...
	case LOW_MEM : 
	{ WAT_dos_free(blocks[i].fnode); break; }
#endif*/
	case STATIC_MEM : break;
	default :
	  dprintf("Memory subsystem : Unknow memory block type\n");
      }
    }
    mem_blocks=0;
  } else
    dprintf("jmalloc_uninit :: jmalloc_init not called\n");
}


int join_blocks()
{
  int i,j=0;
  memory_node *f=NULL;

  memset(jm_small,0,sizeof(memory_node *)*JM_SMALL_SIZE/4);      // clear out old free stacks
  jm_big=NULL;

  for (i=0;!f && i<mem_blocks;i++)      
  {
    for (f=blocks[i].fnode;f;)
    {
      if (f->size<0)
      {
	if (!f->next || f->next->size>0)  // if next bock is not free and to stack
	{
	  if (-f->size<JM_SMALL_SIZE)
	  {
	    f->next_free=jm_small[-f->size/4];
	    jm_small[-f->size/4]=f;
	  } else
	  {
	    f->next_free=jm_big;
	    jm_big=f;
	  }
	  f=f->next;
	} else if (f->next && f->next->size<0)
	{
	  f->size+=f->next->size-sizeof(memory_node)+sizeof(memory_node *);
	  f->next=f->next->next;
	  j=1;
	}	 	 
      }
      else f=f->next;
    }
  }
  return j;
}

void *jmalloc(long size, char *what_for)
{
  if (!mem_blocks)              // if not initialized, then use real malloc
    return malloc(size);
#ifdef MEM_CHECK
  if (size<=0)     
  {
    size=4;
    printf("jmalloc : asking for 0 or less\n");    
  }
#endif
  size=(size+3)&(0xffffffff-3);      // make sure the size is word alligned

  while (1)     // loop until we find a block to return
  {
    if (size<JM_SMALL_SIZE && jm_small[size/4])  // see if we have a block this size already waiting
    {
      memory_node *find=jm_small[size/4];
      find->size=-find->size;                 // mark as being used
#ifdef MEM_CHECK
      find->name=strcpy((char *)malloc(strlen(what_for)+1),what_for);
#endif	   
      jm_small[size/4]=find->next_free;                       // pop the block from the free stack
      return (void *)&find->next_free;
    } else   
    {
      // find first block which will accomodate this size
      // save the last pointer so we can compact the stack
      memory_node *find=NULL,*f,*last=NULL;
      for (f=jm_big;!find && f;f=f->next_free)
        if (-f->size>=size)
	  find=f;
        else last=f;

      if (find)
      {
	find->size=-find->size;                 // mark as being used
#ifdef MEM_CHECK
	find->name=strcpy((char *)malloc(strlen(what_for)+1),what_for);
#endif	        
	if (last)
	  last->next_free=find->next_free;
	else
	  jm_big=find->next_free;                    // pop the block from the free stack
	if (find->size-size>sizeof(memory_node))     // enough space for free block?
	{
	  memory_node *new_free=(memory_node *)(((char *)(&find->next_free))+size);
	  new_free->size=(find->size+sizeof(memory_node *)-size-sizeof(memory_node));
	  find->size=size;
	  if (new_free->size<JM_SMALL_SIZE)
	  {
	    new_free->next_free=jm_small[new_free->size/4];
	    jm_small[new_free->size/4]=new_free;
	  } else
	  {
	    new_free->next_free=jm_big;
	    jm_big=new_free;	    
	  }	  
	  new_free->next=find->next;
	  find->next=new_free;
	  new_free->size=-new_free->size;           // mark this block as free
	}
	return (void *)&find->next_free;
      } else if (!join_blocks())
        free_up_memory();
    }
  }
}


/*    // start at the last spot we used and see if we can find a block near there
    if (last_used && (last_used->size<0) && ((-last_used->size-reserve)>=size))
        f=last_used;

    if (!f)     // no block yet, scan for one.
    {
      int i;
      for (i=0;!f && i<mem_blocks;i++)      
      {
        for (f=blocks[i].fnode;f && (f->size>=0 || -f->size-reserve<=size);f=f->next);      
      }
    }
    if (!f && !join_blocks())
      free_up_memory();                       // user defined function to free memory
    else
    {
      if (size>=-f->size-reserve)             // allocating the whole block?
      {
	f->size=-f->size;                     // chain stays the same, but mark memory as used
#ifdef MEM_CHECK
	f->name=strcpy((char *)malloc(strlen(what_for)+1),what_for);
#endif	  
	last_used=f->next;                    // use next spot as refrence spot
      }
      else                                    // else create a new free node
      {
	memory_node *new_free;
	new_free=(memory_node *)(((unsigned char *)f)+size+sizeof(memory_node));	
	new_free->next=f->next;
	new_free->size=f->size+size+sizeof(memory_node);

	f->next=new_free;
	f->size=size;
	last_used=new_free;
#ifdef MEM_CHECK
	f->name=strcpy((char *)malloc(strlen(what_for)+1),what_for);
#endif	  
      }
      return (void *)(((unsigned char *)(f))+sizeof(memory_node));
    }
  } 
  return NULL; // while never happen 
} */


void jfree(void *ptr)
{
  if (!mem_blocks)
    free(ptr);
  else
  {
    memory_node *f=(memory_node *)(((char *)ptr)+sizeof(memory_node *)-sizeof(memory_node));   
#ifdef MEM_CHECK
    if (f->size<0)
    {
      printf("Bad pointer\n");
      return ;
    }
    free(f->name);
#endif
    if (f->size<JM_SMALL_SIZE)    // insert into small block chain?
    {
      f->next_free=jm_small[f->size/4];
      jm_small[f->size/4]=f;
    }
    else
    {
      f->next_free=jm_big;
      jm_big=f;
    }
    f->size=-f->size;   // mark free and join blocks later
  }
}


void *jrealloc(void *ptr, long size, char *what_for)
{
  if (!mem_blocks)
  {
    if (ptr)                     // some platforms don't do this!
      return realloc(ptr,size);
    else return malloc(size);
  }

  if (!ptr)
    return jmalloc(size,what_for);
  else
  {
    if (size==0)                    // if the new size needed is zero then we can throw this away.
    {      
      jfree(ptr);
      return NULL;
    }
    else
    {
      memory_node *f=(memory_node *)(((char *)ptr)+sizeof(memory_node *)-sizeof(memory_node));
      long old_size=f->size;       // for now we are not going to be very smart about our re-allocation
                                  // i.e. just allocate another block and copy this into it.
      void *new_loc=jmalloc(size,what_for);  

      if (size>old_size)
        memcpy(new_loc,ptr,old_size);
      else
        memcpy(new_loc,ptr,size);

      jfree(ptr);
      return new_loc;
    }
  }
  return NULL;
}


long j_allocated()
{
  memory_node *f;
  long s=0,i;
  for (i=0;i<mem_blocks;i++)
  {
    for (f=blocks[i].fnode;f;f=f->next)
    {
      if (f->size>0)
      s+=f->size;
    }
  }
  return s;
}

long j_available()
{
  memory_node *f;
  long s=0,i;
  for (i=0;i<mem_blocks;i++)
  {
    if (!blocks[i].fnode)
      printf("block #%d is NULL\n",i);

    for (f=blocks[i].fnode;f;f=f->next)
    {
      if (f->size<0)
        s+=-f->size;
    }
  }
  return s;
}



void mem_report(char *filename)
{
  long tot=0;
  FILE *fp=fopen(filename,"wb");
  int size_count[64];
  memset(size_count,0,sizeof(size_count));
  if (fp)
  {
    long i,size,tblocks=0;
    for (i=0;i<mem_blocks;i++)
    {
      fprintf(fp,"*************** MEMORY BLOCK #%d ****************\n",i);
      long offset=0;
      memory_node *p=blocks[i].fnode;
      while (p)
      {
	if (p->size>=0)
	{
	  tblocks++;
	  if (p->size<64)
	    size_count[p->size]++;
	}

	if (p==last_used)
	  fprintf(fp,"-> ");
#ifdef MEM_CHECK
	if (p->size<0)
          fprintf(fp,"%10d %d  %s\n",offset,p->size,"FREE");
	else
          fprintf(fp,"%10d %d  %s\n",offset,p->size,p->name);
#else
	fprintf(fp,"%10d %d\n",offset,p->size);
#endif
	offset+=(abs(p->size)+sizeof(memory_node));
	tot+=abs(p->size);
	p=p->next;
      }
    }
    fprintf(fp,"##  Total = %d bytes\n",tot);
    fprintf(fp,"##  Total allocated = %d bytes\n",j_allocated());
    fprintf(fp,"##  Total blocks = %d\n",tblocks);


    for (i=0;i<JM_SMALL_SIZE/4;i++)
    {
      memory_node *f=jm_small[i];
      if (f)
      { 
	int t;
	fprintf(fp,"Size %d : Free = ",i*4);
	for (t=0;f;f=f->next_free,t++)
          if (-f->size!=i*4)
	    fprintf(fp,"  bad size! (%d)\n",f->size);

	int tb=0;
	for (int j=0;j<mem_blocks;j++)
	{
	  memory_node *p=blocks[j].fnode;
	  for (;p;p=p->next)
	    if (p->size==i*4)
	      tb++;
	}
	
	fprintf(fp,"%d, Used = %d\n",t,tb);

      }
    }

  }
  fclose(fp);
}







