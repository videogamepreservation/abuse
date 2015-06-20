#include "sock.hpp"
#include "dprint.hpp"

class bwip_address : public net_address
{

  public :
  unsigned long ip;
  unsigned short port;


  virtual protocol protocol_type() { return net_address::IP; }
  virtual equal(net_address *who)
  {
    if (who->protocol_type()==IP && ((bwip_address *)who)->ip==ip)
      return 1;
    else return 0;
  }
  virtual int set_port(int Port)  { port=Port; }
  bwip_address(unsigned long ip, unsigned short port) : port(port), ip(ip) {};
  virtual void print()
  {
    unsigned char *c=(unsigned char *) (&ip);
    dprintf("%d.%d.%d.%d",c[0],c[1],c[2],c[3]);
  }
  bwip_address() {} ;    
} ;

class bw_tcpip_protocol : public net_protocol
{
  unsigned long bw_get_server_ip(char *sn);
  int 
  public :

  
  bw_tcpip_protocol();
  net_address *get_node_address(char *&server_name, int def_port, int force_port);
  net_socket *connect_to_server(net_address *addr, 
				net_socket::socket_type sock_type=net_socket::SOCKET_SECURE);
  net_socket *create_listen_socket(int port, net_socket::socket_type sock_type);
  int select_sockets();
} ;

extern bw_tcpip_protocol tcpip;

class unix_fd : public net_socket
{
  protected :
  int fd;
  public :
  unix_fd(int fd) : fd(fd) { };
  virtual int error()                             { return FD_ISSET(fd,&tcpip.exception_set); }
  virtual int ready_to_read()                     { return FD_ISSET(fd,&tcpip.read_set); }
  virtual int ready_to_write()                    
  { 
    struct timeval tv={0,0};     // don't wait
    fd_set write_check;  
    FD_ZERO(&write_check);  
    FD_SET(fd,&write_check);     
    select(FD_SETSIZE,NULL,&write_check,NULL,&tv);
    return FD_ISSET(fd,&write_check); 
  }
  virtual int write(void *buf, int size)          { return ::write(fd,buf,size); }
  virtual int read(void *buf, int size, net_address **addr)
  {
    int tr=::read(fd,buf,size);
    if (addr) *addr=NULL;
    return tr;
  }

  virtual ~unix_fd()                            { read_unselectable();  write_unselectable(); close(fd); }
  virtual void read_selectable()                   { FD_SET(fd,&tcpip.master_set); }
  virtual void read_unselectable()                 { FD_CLR(fd,&tcpip.master_set); }
  virtual void write_selectable()                  { FD_SET(fd,&tcpip.master_write_set); }
  virtual void write_unselectable()                { FD_CLR(fd,&tcpip.master_write_set); }
  int get_fd() { return fd; }
} ;
