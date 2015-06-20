#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <termios.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/keyboard.h>
#include <sys/ioctl.h>
#include <sys/stat.h>


int fd_ready_to_read(int fd)
{
  struct timeval tv={0,0};
  fd_set kbd_set;
  FD_ZERO(&kbd_set);
  FD_SET(fd,&kbd_set);
  select(FD_SETSIZE,&kbd_set,NULL,NULL,&tv);
  return (FD_ISSET(fd,&kbd_set));
}





int key_fifo()
{
  int fd,child_pid; 
  FILE *fp;
  fp=popen("keydrv","r");
  fscanf(fp,"%d",&child_pid);  
  pclose(fp);
  do
  { usleep(10000);
  } while (access("/tmp/jckey-driver",R_OK)); 
  fd=open("/tmp/jckey-driver",O_RDONLY);
  return fd;
}


main()
{
  unsigned char key;
  
  int console_fd=key_fifo();
  while (1)
  {
    if (fd_ready_to_read(console_fd))
    {
      read(console_fd,&key,1);      
      printf("read key (%d)\n\r",(int)key);
    } else usleep(10000);          
  }  
}
