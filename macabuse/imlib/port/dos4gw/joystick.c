#include "joy.hpp"
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <stdlib.h>


int use_joy=1,joy_centx=50,joy_centy=50;    // which joystick do you want to use?
               
int joy_init(int argc, char **argv)
{
  int i;  
  for (i=1;i<argc;i++)
    if (!strcmp(argv[i],"-joy"))
      use_joy=1;
    else if (!strcmp(argv[i],"-joy2"))
      use_joy=2;
    else if (!strcmp(argv[i],"-nojoy"))
      use_joy=0;

  return use_joy;
}             

void joy_status(int &b1, int &b2, int &b3, int &xv, int &yv)
{
  if (use_joy)   // make sure that we detected one before
  {
    union REGS inregs,outregs;
    inregs.w.ax=0x8400;
    inregs.w.dx=0;
    int386(0x15,&inregs,&outregs);

    int b;
    if (use_joy==1)    
      b=((0xff^outregs.w.ax)&0x30)>>4;
    else
      b=((0xff^outregs.w.ax)&0xc0)>>4;
    b1=b&1;
    b2=b&2;      
    b3=0;


    inregs.w.ax=0x8400;
    inregs.w.dx=1;
    int386(0x15,&inregs,&outregs);   // now read the movement

    if (use_joy==1)
    {
      xv=((int)inregs.w.ax-(int)joy_centx)*2/joy_centx;
      yv=((int)inregs.w.bx-(int)joy_centy)*2/joy_centy;
    } else
    {
      xv=((int)inregs.w.cx-(int)joy_centx)*2/joy_centx;
      yv=((int)inregs.w.dx-(int)joy_centy)*2/joy_centy;
    }    
  } 
  else xv=yv=b1=b2=b3=0;
}


void joy_calibrate()
{
  if (use_joy)
  {
    union REGPACK regs;
    regs.w.ax=0x8400;
    regs.w.dx=0x0001;
    intr(0x15,&regs);   // now read the movement

    if (use_joy==1)
    {
      joy_centx=(int)regs.w.ax;
      joy_centy=(int)regs.w.bx;
    } else
    {
      joy_centx=(int)regs.w.cx;
      joy_centy=(int)regs.w.dx;
    }    
  }
}





