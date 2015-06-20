#include "system.h"
#include "macs.hpp"
#include <stdio.h>

void unpackgl(char *fn, int pict_only)
{
  unsigned short dir_length,sl,x,amread,bufsize;
  unsigned long offset,ret_ofs,length;
  char name[14],st[50],*buf;
  FILE *fp,*ot;
  CHECK(fn);
  fp=fopen(fn,"rb");
  if (!fp) return ;

  if (fread(&dir_length,2,1,fp)!=1) return ;
  dir_length=int_to_local(dir_length)/17-1;
  ret_ofs=ftell(fp);
  while (dir_length--)
  {
    fseek(fp,ret_ofs,SEEK_SET);
    if (fread(&offset,4,1,fp)!=1) return ;
    offset=long_to_local(offset);
    if (fread(name,13,1,fp)!=1)  return ;
    name[13]=0;
    sl=strlen(name);
    ret_ofs=ftell(fp);  // save the current spot in the file so we can come back to it later
    if (toupper(name[sl-3])=='P' || toupper(name[sl-3])=='C' || !pict_only)
    {
      fseek(fp,offset,SEEK_SET);
      fread(&length,1,4,fp);
      length=long_to_local(length);
      if (length>1000000)
        exit(1);
      ot=fopen(name,"rb");
      if (ot)
      { printf("File (%s) already exsist, overwrite (y/n)?\n",name);
	fgets(st,49,stdin);
	if (!(st[0]=='Y' || st[0]=='y'))
	  length=0;
	fclose(ot);
      }
      if (length)
	ot=fopen(name,"wb");
      if (!ot) return ;
      bufsize=0xf000;
      do {
	buf=(char *)jmalloc(bufsize,"unpack_gl::buffer");
	if (!buf) bufsize-=100;
      } while (!buf);

      while (length)
      {
	if (length>bufsize)
	  amread=bufsize;
	else amread=length;
	fread(buf,1,amread,fp);
	fwrite(buf,1,amread,ot);
	length-=amread;
      }
      jfree(buf);
      if (ot) fclose(ot);
    }
  }
  fclose(fp);
}

main(int argc, char **argv)
{
  FILE *fp;
  char fn[100];
  int t,i,x;
  if (argc<2)
  { printf("Usage : unpackgl filename[.gl] [-p]\n");
    printf("  -p only unpacks picture files\n");
    printf("  Unpackes the files contain within a GL file (GRASP format)\n");
    printf("  Does not change the orginal file\n");
    exit(0);
  }
  strcpy(fn,argv[1]);
  if (fn[strlen(fn)-3]!='.')
    strcat(fn,".gl");
  if (*argv[2]=='-' && *(argv[2]+1)=='p')
    unpackgl(fn,1);
  else unpackgl(fn,0);
  return 0;
}

