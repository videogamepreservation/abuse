#include "sock.hpp"
#include <stdlib.h>
#include "dprint.hpp"
//#include "stdio.h"
//#define dprintf printf

const char notify_signature[] = "I wanna play ABUSE!";
const char notify_response[] = "Yes!";

net_protocol *net_protocol::first=0;

// connect to an explictedly named address
// first try to get the address and then try to connect
// return NULL if either fail.  This method does not need to be implemented
// in sub-classes

net_socket *net_protocol::connect_to_server(char *&server_name, int port, int force_port, net_socket::socket_type sock_type)
{
	dprintf("Don't execute this!\n");
	return 0;

  net_address *a=get_node_address(server_name,port,force_port);
  if (!a) return NULL;
  net_socket *s=connect_to_server(a,sock_type);
  delete a;
  return s;
}

