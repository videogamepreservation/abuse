#define dprintf printf
#include "shm_fifo.c"
#include <stdio.h>

main()
{  
  shm_fifo a;
  a.create(100);
  shm_fifo b;
  b.create(100);

  printf("ids = %d %d\n",a.shm_id,b.shm_id);
  a.wait_attach();
  b.wait_attach();

  int i;
  for (i=0;i<100;i++)
  {
    char msg[100];
    a.read(msg,12);  
    printf("MSG : %s\n",msg);
    char stat=1;
    b.write(&stat,1);
  }
}

