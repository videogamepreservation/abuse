#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

main(int argc, char **argv)
{
  struct hostent *hn=gethostbyname(argv[1]);
  if (hn)
  {
    printf("host %s\n",hn->h_name);
    char **a=hn->h_addr_list;
    while (*a)
    {
      char *b=*a;
      printf("address %d.%d.%d.%d\n",b[0],b[1],b[2],b[3]);
      a++;
    }
  }
            
}
