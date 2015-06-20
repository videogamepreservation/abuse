/* Jonathan Clark April 5, 93   Converts Unix file for to DOS & vice versa. */
#include <stdio.h>

#define STReq(x,y) (!strcmp(x,y))
main(int argc, char **argv)
{
  FILE *fp,*o;
  int i,strip,add,c;
  char st[100];
  if (argc<3 || !(STReq(argv[1],"2unix") || STReq(argv[1],"2dos")))
  { printf("Usage : convert [2unix]|[2dos] files\n");
    exit(0);
  }
  if (STReq(argv[1],"2unix"))
  { strip=1; add=0; }
  else {strip=0; add=1; }
  printf("Converting...\n");
  for (i=2;i<argc;i++)
  {
    printf("  %s\n",argv[i]);
    fp=fopen(argv[i],"r"); 
    o=fopen("testXDF.out","w");
    while (!feof(fp))
    {
      c=fgetc(fp);
      if (c>=0)
      {
        if (c=='\n' && add) { fputc('\r',o); }
        if (!(c=='\r') || !strip)
          fputc(c,o); 
      }
    }
    fclose(o);
    fclose(fp);
    sprintf(st,"cp testXDF.out %s",argv[i]);
    system(st);
    unlink("testXDF.out"); 
  } 
}
