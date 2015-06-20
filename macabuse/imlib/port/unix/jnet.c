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

#ifdef __sgi
#include <bstring.h>
#endif

#ifdef _AIX
#include <sys/select.h>
#include <strings.h>
extern "C" {
#endif
#include <netdb.h>

#ifdef _AIX
} ;
#endif

#include "jnet.hpp"
#include "macs.hpp"
#include "jmalloc.hpp"

int net_init(int protocol) 
{ 
  if (protocol==TCPIP_PROTOCOL)    // UNIX always has TCPIP!
    return 1;
  else return 0;
}


out_socket::~out_socket()
{ ; }

void net_uninit() { ; }

char last_sock_err[200];
int current_sock_err=0;

void set_sock_err(int x) { current_sock_err=x; }


#define PK_BUFFER_SIZE 2000



class unix_out_socket : public out_socket
{
  int fd;
  int type;      // SOCK_DGRAM, or SOCK_STREAM
  public :
  uchar pk_buffer[PK_BUFFER_SIZE];
  long pk_buffer_ro,pk_buffer_last;
  unix_out_socket(int FD) { fd=FD; pk_buffer_ro=pk_buffer_last=0; }
  unix_out_socket();
  void fill_buffer();
  int get_fd() { return fd; }
  virtual int try_connect(char *rhost, int port);
  virtual int ready_to_read();
  virtual int ready_to_write();
  virtual int send(packet &pk);
  virtual int get(packet &pk);
  virtual ~unix_out_socket();
} ;


class unix_in_socket : public in_socket
{
  int fd;
  sockaddr_in host;
  int port;
  public :
  unix_in_socket(int Port);
  virtual out_socket *check_for_connect();
  virtual ~unix_in_socket();
} ;


int unix_out_socket::ready_to_read()
{
  if (pk_buffer_last>pk_buffer_ro) return 1;
  struct timeval tv={0,0};
  fd_set set,ex_set;
  FD_ZERO(&set);
  FD_SET(fd,&set);

  FD_ZERO(&ex_set);
  FD_SET(fd,&ex_set);
  select(FD_SETSIZE,&set,NULL,&ex_set,&tv);                // check for exception
  return (FD_ISSET(fd,&set) || FD_ISSET(fd,&ex_set));
}

int unix_out_socket::ready_to_write()
{
  struct timeval tv={0,0};
  fd_set set,ex_set;
  FD_ZERO(&set);
  FD_SET(fd,&set);
  select(FD_SETSIZE,NULL,&set,NULL,&tv);                // check for exception
  return (FD_ISSET(fd,&set));
}

unix_in_socket::unix_in_socket(int Port)
{
  port=Port;
  fd=socket(AF_INET,SOCK_STREAM,0);
  memset( (char*) &host,0, sizeof(host));
  host.sin_family = AF_INET;
  host.sin_port = htons(port);
  host.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind(fd, (struct sockaddr *) &host, sizeof(sockaddr_in))==-1)
  {
    set_sock_err(SOCK_BIND_FAIL);
    sprintf(last_sock_err,"Unable to bind to port %d",port);
  }
  else
  {
    if (listen(fd,1)==-1)
    {
      set_sock_err(SOCK_LISTEN_FAIL);
      sprintf(last_sock_err,"Unable to listen to port %d",port);      
    }
  }

}


out_socket *unix_in_socket::check_for_connect()
{ 
  struct timeval tv={0,0};
  fd_set set,ex_set;
  FD_ZERO(&set);
  FD_SET(fd,&set);


  select(FD_SETSIZE,&set,NULL,NULL,&tv);                // check for exception

  if (FD_ISSET(fd,&set))
  {
    int len=sizeof(sockaddr_in);
    int new_fd=accept(fd, (struct sockaddr *) &host, &len);
    if (new_fd<0)
    {
      set_sock_err(SOCK_ACCEPT_FAIL);
      sprintf(last_sock_err,"Accept failure on port %d",port);      
      return NULL;
    }

    unsigned long gaddr=ntohl(host.sin_addr.s_addr);
    return new unix_out_socket(new_fd);
  } else return NULL;
}
  

unix_out_socket::unix_out_socket()
{
  pk_buffer_ro=pk_buffer_last=0;
  fd=-1;
}
    

int unix_out_socket::try_connect(char *rhost, int port)
{
  if (strlen(rhost)>5 && 
      rhost[0]=='t' && 
      rhost[1]=='c' && 
      rhost[2]=='p' &&
      rhost[1]=='/')
    type=SOCK_STREAM;
  else type=SOCK_DGRAM;

  if (fd==-1)
    fd=socket(AF_INET,type,0);
  if (fd==-1)
  {
    set_sock_err(SOCK_CREATE_FAIL);
    sprintf(last_sock_err,"Failed to create socket (too many file descriptors open?)");
    return 0; 
  }

  hostent *hp=gethostbyname(rhost);
  if (!rhost)
  {
    set_sock_err(SOCK_NAMELOOKUP_FAIL);
    sprintf(last_sock_err,"Unable to get address for name %s",rhost);      
    return 0; 
  } else
  {
    sockaddr_in host;
    memset( (char*) &host,0, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(port);
    host.sin_addr.s_addr = htonl (INADDR_ANY);
    memcpy(&host.sin_addr,hp->h_addr,hp->h_length);
    
    if (connect(fd, (struct sockaddr *) &host, sizeof(host))==-1)
    {
      set_sock_err(SOCK_CONNECT_FAIL);
      sprintf(last_sock_err,"Unable to connect to address %s on port %d",rhost,port);
      return 0; 
    }

    return 1;
  }
}



unix_out_socket::~unix_out_socket()
{  close(fd); }

unix_in_socket::~unix_in_socket()
{  close(fd);  }


void unix_out_socket::fill_buffer()
{
  while (!ready_to_read()) ;
  pk_buffer_last=read(get_fd(),pk_buffer,PK_BUFFER_SIZE);
  pk_buffer_ro=0;
}

int unix_out_socket::get(packet &pk)
{ 
  pk.ro=pk.wo=2;

  ushort size=0;
  if (pk_buffer_last==pk_buffer_ro)
    fill_buffer();

  if (pk_buffer_last-pk_buffer_ro<2)  // make sure the two byte for the size are in the same packet
  {
    uchar two[2];
    two[0]=pk_buffer[pk_buffer_ro];
    fill_buffer();
    if (pk_buffer_last-pk_buffer_ro<2)  // if still not enough info, something is screwy
    {
      printf("Incomplete packet\n");
      return 0;
    }

    two[1]=pk_buffer[pk_buffer_ro];
    pk_buffer_ro++;
    size=lstl((*((ushort *)two)));
  } else
  {
    memcpy(&size,pk_buffer+pk_buffer_ro,2); pk_buffer_ro+=2;
    size=lstl(size);
  }
  pk.rend=size+2;
  pk.make_bigger(pk.rend);

  uchar *pk_off=pk.buf+2;
  int rs;
  while (size)
  {
    if (pk_buffer_last==pk_buffer_ro)
      fill_buffer();    
    if (pk_buffer_last-pk_buffer_ro>size)
      rs=size;
    else
      rs=pk_buffer_last-pk_buffer_ro;
    
    memcpy(pk_off,pk_buffer+pk_buffer_ro,rs);
    pk_buffer_ro+=rs;
    size-=rs;
    pk_off+=rs;         
  }
  
  return 1;
}

int unix_out_socket::send(packet &pk)
{  
  while (!ready_to_write()) ;
  ushort size=lstl(pk.wo-2);
  memcpy(pk.buf,&size,2);
  return write(fd,pk.buf,pk.wo)==pk.wo;
}

in_socket *create_in_socket(int port)
{ return new unix_in_socket(port); }

out_socket *create_out_socket(char *name, int port)
{ unix_out_socket *s=new unix_out_socket();
  if (s->try_connect(name,port))
    return s;
  else   
    delete s;
  return NULL;
}

uchar *get_local_address()                             // same format as above (be sure to jfree this)
{
  char name[80];
  uchar *ip=(uchar *)jmalloc(4,"IP address");

  if (gethostname(name,80)!=0)
  { ip[0]=127; ip[1]=ip[2]=0; ip[3]=1; }
  else
  {
    struct hostent *me=gethostbyname(name);
    memcpy(ip,me->h_addr,4);
  }
}









