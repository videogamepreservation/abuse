#ifndef __UNIX_SHM_FIFO_HPP_
#define __UNIX_SHM_FIFO_HPP_


struct fifo_control
{
  long nattached;
  long f_start,f_end;
  long size;
} ;

class shm_fifo
{
  unsigned char *data;
  fifo_control *fc;
public :
  int shm_id;
  shm_fifo();
  int create(int size);   // create a fifo of size
  int wait_attach();      // returns 1 if successful


  int attach(int shm_id);   // attach to already
  int ready_to_read();
  int read(void *buf, int size);
  int write(void *buf, int size);
  ~shm_fifo();
} ;



#endif


