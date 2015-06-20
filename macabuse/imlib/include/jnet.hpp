#ifndef __NET_HPP_
#define __NET_HPP_

#include "macs.hpp"
#include "packet.hpp"

enum { SOCK_BAD_HOST       = -1,
       SOCK_CONNECT_FAIL   = -2,
       SOCK_BIND_FAIL      = -3, 
       SOCK_LISTEN_FAIL    = -4,
       SOCK_NAMELOOKUP_FAIL= -5,
       SOCK_ACCEPT_FAIL    = -6,
       SOCK_WRITE_FAIL     = -7,
       SOCK_CREATE_FAIL    = -8,
       SOCK_SELECT_FAIL    = -9
     };

enum { NONET_PROTOCOL, IPX_PROTOCOL, TCPIP_PROTOCOL } ;

extern char last_sock_err[200];
extern int current_sock_err;

class out_socket
{
  public :
  virtual int ready_to_read()  = 0;
  virtual int ready_to_write() = 0;
  virtual int send(packet &pk) = 0;
  virtual int get(packet &pk)  = 0;
  virtual ~out_socket();
} ;


class in_socket
{
  public :
  virtual out_socket *check_for_connect()    = 0;
  virtual ~in_socket() { ; }
} ;


in_socket *create_in_socket(int port);
out_socket *create_out_socket(char *name, int port);
uchar *name_to_address(char *name, int protocol);       // should be defined externally
                                                        // returns 4 bytes for TCPIP
                                                        // 4 (net number) 6 node number for IPX

uchar *get_local_address();                             // same format as above (be sure to jfree this)


int net_init(int protocol);
void net_uninit();

#endif

