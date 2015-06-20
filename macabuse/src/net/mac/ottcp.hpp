#include "sock.hpp"


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "OpenTransport.h"
#include "OpenTptInternet.h"

//extern fd_set master_set,master_write_set,read_set,exception_set,write_set;

class ot_ip_address : public net_address
{
public :
  InetAddress addr;

  virtual protocol protocol_type() { return net_address::IP; }
  virtual equal(net_address *who)
  {
    if (who->protocol_type()==IP &&
				!memcmp(&addr.fHost,& ((ot_ip_address *)who)->addr.fHost,sizeof(addr.fHost)))
      return 1;
    else return 0;
  }
  virtual int set_port(int port)  { addr.fPort=port;  return port; }
  ot_ip_address(InetAddress *Addr) { memcpy(&addr,Addr,sizeof(addr)); }
  virtual void print()
  {
    unsigned char *c=(unsigned char *) (&addr.fHost);
    fprintf(stderr,"%d.%d.%d.%d",c[0],c[1],c[2],c[3]);
  }
  int get_port() { return addr.fPort; }
  net_address *copy()  { return new ot_ip_address(&addr); }
  ot_ip_address() {} ;
  void store_string(char *st, int st_length)
  {
    char buf[100];
    unsigned char *c=(unsigned char *) (&addr.fHost);
    sprintf(buf,"%d.%d.%d.%d:%d",c[0],c[1],c[2],c[3],addr.fPort);
    strncpy(st,buf,st_length);
    st[st_length-1]=0;
  }
};

class ot_tcpip_protocol : public net_protocol
{
protected:
  int HaveOT;
public :
//  fd_set master_set,master_write_set,read_set,exception_set,write_set;

  ot_tcpip_protocol();
  net_address *get_local_address();
  net_address *get_node_address(char *&server_name, int def_port, int force_port);
  net_socket *connect_to_server(net_address *addr, 
				net_socket::socket_type sock_type=net_socket::SOCKET_SECURE);
  net_socket *create_listen_socket(int port, net_socket::socket_type sock_type, char *name);
  int installed() { return 1; }
  char *name() { return "Mac Open Transport TCPIP"; }
  void cleanup() { ; } 
  int select(int block);          // return # of sockets available for read & writing
};

extern ot_tcpip_protocol ot_tcpip;

class ot_socket : public net_socket
{
public :
	// all public since callback function must be able to access
  EndpointRef ep;
  ot_ip_address def_addr;
  int complete;
	OTEventCode code;				/* event code */
	OTResult result;				/* result */
	TCall *call;						/* pointer to call structure */
	TBind *bindReq;					/* pointer to bind request structure */
	TBind *bindRet;					/* pointer to bind return structure */
	OSErr err;
	int ready;

  ot_socket() {}
  virtual int error()                             { return (err != noErr); }
  virtual int ready_to_read()                     { return ready; }
  virtual int ready_to_write()                   	{ return ready; } 

  virtual ~ot_socket();
  int get_fd() { return 0; }
};

class ot_tcp_socket : public ot_socket
{
	friend pascal void ot_tcp_EventHandler(void*, OTEventCode event, OTResult result, void* cookie);
  int listening;
public :
  ot_tcp_socket();
  virtual int write(void *buf, int size, net_address *addr=NULL);
  virtual int read(void *buf, int size, net_address **addr);
  virtual int listen(int port);
  virtual net_socket *accept(net_address *&addr);
  
  int connect(ot_ip_address addr);
};

class ot_udp_socket : public ot_socket
{
public :
  ot_udp_socket() : ot_socket() {}
  virtual int read(void *buf, int size, net_address **addr);
  virtual int write(void *buf, int size, net_address *addr=NULL);
  virtual int listen(int port);

  int connect(ot_ip_address addr);
};

