#include "jnet.hpp"
#include "macs.hpp"
#include "dprint.hpp"
#include "system.h"
#include "ipx.hpp"
#include "bwtcp.hpp"
#include "jmalloc.hpp"


char last_sock_err[200];
int current_sock_err=0;


void set_sock_err(int x) { current_sock_err=x; }
static int prot=NONET_PROTOCOL;


int net_init(int protocol)
{
  if (protocol==TCPIP_PROTOCOL)
  {
    if (bwt_init())
    {
      prot=TCPIP_PROTOCOL;
      return 1;
    }
    return 0;
  }
  else if (protocol==IPX_PROTOCOL)
  {
    if (ipx_init())
    {
      prot=IPX_PROTOCOL;
      return 1;
    }
    return 0;
  }
  else return 0;
}

void net_uninit()
{
  if (prot==TCPIP_PROTOCOL)
    bwt_uninit();
  else if (prot==IPX_PROTOCOL)
    ipx_uninit();
}



in_socket *create_in_socket(int port)
{
  switch (prot)
  {
    case TCPIP_PROTOCOL :
    { return new bwt_in_socket(port); } break;
    case IPX_PROTOCOL :
    { return new ipx_in_socket(port); } break;
  }
  return NULL;
}

out_socket *create_out_socket(char *name, int port)
{
  switch (prot)
  {
    case TCPIP_PROTOCOL :
    { 
      bwt_out_socket *o=new bwt_out_socket();
      if (o->try_connect(name,port))
        return o;
      else { delete o; return NULL; }
    } break;
    case IPX_PROTOCOL :
    { 
      ipx_out_socket *o=new ipx_out_socket(port);
      if (o->try_connect(name,port))
        return o;
      else { delete o; return NULL; }
    } break;
  }
  return NULL;
}



uchar *get_local_address()                            // same format as above (be sure to jfree this)
{
  switch (prot)
  {
    case IPX_PROTOCOL :
    {
      return ipx_get_local_address();
    } break;
    case TCPIP_PROTOCOL :
    {
      uchar *b=(uchar *)jmalloc(4,"TCPIP address"); 
      *((int *)b)=bwt_get_my_ip();
      return b;
    } break;
  }
  return NULL;
}


out_socket::~out_socket()
{ ; }








