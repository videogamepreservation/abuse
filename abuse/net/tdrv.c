#include "../src/unixnfc.c"

main(int argc, char **argv)
{
  if (net_init(argc,argv))
  {

    printf("try open %s\n",argv[1]);
    int fd=NF_open_file(argv[1],"rb");    
    printf("open returned %d\n",fd);
    if (fd>0)
    {
      long size=NF_filelength(fd);
      printf("sizeof file is %d\n",size);


//      printf("seek returned %d\n",NF_seek(fd,2));

      char *buffer=(char *)malloc(size);
      long tr=NF_read(fd,buffer,size);
      

      printf("read %d bytes = \n",tr);
      fwrite(buffer,size,1,stderr);
    }

    kill_net();
  }
}


