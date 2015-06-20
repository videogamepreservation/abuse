#include "joy.hpp"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
//#include <linux/joystick.h>



int joy_fd=-1,joy_centx=863,joy_centy=705;


int joy_init(int argc, char **argv)
{
  return 0;
  /*  int i,timeout;
  char *joy_dev="/dev/js0";
  
  // see if the user specefied a different joy stick device to use

  for (i=1;i<argc;i++)
  {
    if (!strcmp(argv[i],"-joy_dev"))     
    {
      i++;
      joy_dev=argv[i];
    } else if (!strcmp(argv[i],"-joy"))
      joy_dev="/dev/js0";
    else if (!strcmp(argv[i],"-joy2"))
      joy_dev="/dev/js1";
    else if (!strcmp(argv[i],"-nojoy"))
      return 0;
  
  }
  

  if (!strcmp(joy_dev,"none"))    // they asked not to have one!
    return 0;
  joy_fd=open(joy_dev,O_RDONLY);
 
  if (joy_fd<0)
  {    
    printf("No joystick detected at %s (apply joystick patch)\n",joy_dev);    
    return 0;         // I guess they don't have one
  }


  timeout=10; 

  ioctl (joy_fd, JS_SET_TIMELIMIT, &timeout);
  return 1;  */
}

void joy_status(int &b1, int &b2, int &b3, int &xv, int &yv)
{
  /*  if (joy_fd>=0)
  {
    struct JS_DATA_TYPE j;
    read(joy_fd,&j,sizeof(JS_DATA_TYPE));  
    b1=(j.buttons&1) ? 1 : 0;
    b2=(j.buttons&2) ? 1 : 0;
    b3=0; 
    xv=(j.x-joy_centx)*2/joy_centx;
    yv=(j.y-joy_centy)*2/joy_centy;  
  } else xv=yv=b1=b2=b3=0; */
}


void joy_calibrate()
{
  /*  if (joy_fd>=0)
  {
    struct JS_DATA_TYPE j;
    read(joy_fd,&j,sizeof(JS_DATA_TYPE));  
    joy_centx=j.x;
    joy_centy=j.y;
    if (joy_centx==0) joy_centx=1;   // can't dvide by 0
    if (joy_centy==0) joy_centy=1;   // can't dvide by 0
  } */
}
