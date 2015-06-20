#include <math.h>

main()
{
  int i,x;
  printf("long sin_table[360]={\n  ");
  for (i=0;i<360;i++)
  {
    printf("0x%08x",(long)(sin(i/180.0*3.141592654)*65536));
    if (i!=359) printf(", ");
    if ((i%6)==5) printf("\n  ");
  }
  printf("};\n");

  printf("unsigned short atan_table[1662]={\n  ");
 
  for (i=1;i<1662;i++)
  {    
    if (i<29) x=0;
    else x=(long)(atan(i/(double)29)/3.14152654*180)-45;
    if (x<0) x+=360;
    printf("%3d",x);
    if (i!=1661) printf(", ");
    if ((i%12)==11) printf("\n  ");
  }
  printf("};\n");  
}
