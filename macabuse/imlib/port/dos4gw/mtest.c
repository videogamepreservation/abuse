#include "i86.h"

main()
{
  union  REGS in,out;
  memset(&in,0,sizeof(in));
  in.w.ax=0;
  int386(0x33,&in,&out);

  printf("Has a mouse = %d\n",((unsigned short)out.w.ax)==0xffff);

  memset(&in,0,sizeof(in));
  in.w.ax=4;
  in.w.cx=40;
  in.w.dx=40;
   
  int386(0x33,&in,&out);
}
