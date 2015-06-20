
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

//#include "dprint.hpp"
#include "shm_fifo.hpp"

int shm_fifo::read(void *buf, int size)
{
  unsigned char *b=(unsigned char *)buf;
  int tread=0;
  while (size)
  {
    while (fc->f_start==fc->f_end)
    {
      if (fc->nattached<2) 
        return tread;
      sleep(0);
    }

    *b=data[fc->f_start];
    if (fc->f_start==fc->size-1)
      fc->f_start=0;
    else fc->f_start++;
    b++;
    size--;
    tread++;
  }
  return tread;
}

int shm_fifo::write(void *buf, int size)
{
  unsigned char *b=(unsigned char *)buf;
  int twriten=0;
  while (size)
  {
    while (fc->f_end==fc->f_start-1 || (fc->f_end==fc->size-1 && fc->f_start==0))   // wait to space to write
    {
      if (fc->nattached<2) return twriten;
      sleep(0);
    }

    data[fc->f_end]=*b;
    if (fc->f_end==fc->size)
      fc->f_end=0;
    else fc->f_end++;
    b++;
    twriten++;
    size--;
  }
  return twriten;
}

int shm_fifo::ready_to_read()
{
  return fc->f_start!=fc->f_end;
}

shm_fifo::shm_fifo()
{
  shm_id=-1;
}

int shm_fifo::create(int size)
{
  if (shm_id==-1)
  {
    shm_id=shmget(IPC_PRIVATE,size+sizeof(fifo_control),IPC_CREAT);
    if (shm_id<0) return 0;
    fc=(fifo_control *)shmat(shm_id,0,0);
    if (!fc)
    {
      shmctl(shm_id,IPC_RMID,NULL);
      return 0;
    }
    data=(unsigned char *)(fc+1);   
    fc->nattached=1;
    fc->f_start=fc->f_end=0;            // clear buffer
    fc->size=size;
    return 1;
  } 
  
  dprintf("shm_fifo already created/attached\n");
  return 0;  
}


int shm_fifo::wait_attach()
{
  while (fc->nattached==1);  
  unsigned char on;
  read(&on,1);
  shmctl(shm_id,IPC_RMID,NULL);     // now that the other process has it, delete the shm id so memory gets freed on exit
  return 1;
}



int shm_fifo::attach(int id)   // attach to already
{
  if (shm_id==-1)
  {
    fc=(fifo_control *)shmat(id,0,0);  
    if (!fc)
      return 0;
    shm_id=id;
    data=(unsigned char *)(fc+1);
    fc->nattached++;               // notify entry
    unsigned char on=1;
    write(&on,1);                  // write one byte
    while (ready_to_read());      // wait for it to get read
    return 1;
  }
  return 0;
}


shm_fifo::~shm_fifo()
{
  if (shm_id!=-1)
  {
    fc->nattached--;
    shmdt(fc);
    shm_id=-1;
  }
}

