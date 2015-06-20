#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <netdb.h>
#include "nstruct.hpp"



int setup_udp_port(int port)      // returns fd or -1
{
  int fd=socket(AF_INET,SOCK_DGRAM,0);
  sockaddr_in host;
  memset( (char*) &host,0, sizeof(host));
  host.sin_family = AF_INET;
  host.sin_port = htons(port);
  host.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind(fd, (struct sockaddr *) &host, sizeof(sockaddr_in))==-1)
    return -1;
  else if (listen(fd,1)==-1)
    return -1;
  return fd;
}

int udp_connect(char *rhost, int port)
{
  int fd=socket(AF_INET,SOCK_DGRAM,0);
  hostent *hp=gethostbyname(rhost);
  if (!rhost)
    return -1;

  sockaddr_in host;
  memset( (char*) &host,0, sizeof(host));
  host.sin_family = AF_INET;
  host.sin_port = htons(port);
  host.sin_addr.s_addr = htonl (INADDR_ANY);
  memcpy(&host.sin_addr,hp->h_addr,hp->h_length);
    
  if (connect(fd, (struct sockaddr *) &host, sizeof(host))==-1)
    return -1;

  return fd;
}



int catch_sigs[]={SIGHUP,SIGINT,SIGQUIT,SIGILL,SIGABRT,
		  SIGIOT,SIGFPE,SIGKILL,SIGUSR1,SIGSEGV,
		  SIGUSR2,SIGPIPE,SIGTERM,SIGSTKFLT,SIGCHLD,
		  SIGCONT,SIGSTOP,SIGTSTP,SIGTTIN,SIGTTOU,-1};



int net_init(int argc, char **argv)
{
  for (int i=1;i<argv;i++)
    if (!strcmp(argv[i],"-port"))
    {
    }
  }

}


main(int argc, char **argv)
{
  int fail=0;
  if (argc<2)
    fail=1;
  else 
  {
    int shmid;
    if (sscanf(argv[1],"%d")!=1)
      fail=1;
    else
    {
      
    }
  }

  { fprintf(stderr,
	    "Abuse UNIX network driver is automatically run by abuse\n"
	    "it cannot be run stand-alone\n");
    exit(0);
  }


}



