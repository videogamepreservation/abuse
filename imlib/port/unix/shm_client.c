#define dprintf printf
#include "shm_fifo.c"
#include <stdio.h>


main()
{
  int d1,d2;
  printf("enter ids\n");
  scanf("%d%d",&d1,&d2);
  
  shm_fifo a,b;
  int i;

  a.attach(d1);
  b.attach(d2);
  for (i=0;i<100;i++)
  {
    char *msg="Hellow world";
    a.write(msg,strlen(msg)+1);  
    char stat;
    b.read(&stat,1);       // wait for ok
  }
}

