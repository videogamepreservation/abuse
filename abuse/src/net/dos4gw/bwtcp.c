
unsigned long bw_tcp_protocol::bw_get_server_ip(char *sn)
{
  int a,b,c,d;
  if (sscanf(sn,"%d.%d.%d.%d",&a,&b,&c,&d)==4 && 
      a>=0 && b>=0 && c>=0 && d>=0 &&
      a<256 && b<256 && c<256 && d<256)
  {
    unsigned char c[4];
    c[0]=a;  c[1]=b;  c[2]=c;  c[3]=d;
    return ((unsigned long *)c);
  }
  return 0;
}


bw_tcp_protocol::bw_tcpip_protocol()
{
}

net_address *bw_tcp_protocol::get_node_address(char *&server_name, int def_port, int force_port)
{
  char name[256],*np;
  np=name;
  while (*server_name && *server_name!=':' && *server_name!='/')
    *(np++)=*(server_name)++;
  *np=0;
  if (*server_name==':')
  {
    server_name++;
    char port[256],*p;
    p=port;
    while (*server_name && *server_name!='/')
      *(p++)=*(server_name++);
    *p=0;
    int x;
    if (!force_port)
    {
      if (sscanf(port,"%d",&x)==1) def_port=x;
      else return 0;
    }
  }

  if (*server_name=='/') server_name++;
  unsigned long x=bw_get_server_ip(server_name);
  if (x) 
    return new bwip_address(x,def_port);
  else return NULL;
}


net_socket *bw_tcp_protocol::connect_to_server(net_address *addr, 
					       net_socket::socket_type sock_type=net_socket::SOCKET_SECURE)
{
  if (sock_type==net_socket::SOCKET_SECURE)
  {
    
int bwt_socket()
{
  memset(&rm,0,sizeof(rm));

  strcpy((char *)low_sock_mem, "TCP-IP10");
  rm.eax = 0x3d42;
  rm.ds = low_sock_mem_seg;
  rm.edx = low_sock_mem_off;
  RM_intr(0x21,&rm);
  if (rm.flags & 1)
  {
    current_sock_err=SOCK_CREATE_FAIL;
    sprintf(last_sock_err,"Unable to create socket");    
    return -1;
  }  
  int fd = (rm.eax&0xffff);


  rm.eax=0x4401;
  rm.edx=0x60;
  rm.ebx=fd; 
  RM_intr(0x21,&rm);


  // internal layer must be told to reclaim internal buffers
  low_sock_mem[0]=6;
  low_sock_mem[1]=0x80;
  bwt_ioctl_write(fd, 2);
  return fd;

}


  } else
  {
  }
}

  net_socket *create_listen_socket(int port, net_socket::socket_type sock_type);
  int select_sockets();
