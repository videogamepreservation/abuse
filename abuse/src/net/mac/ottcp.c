#include "ottcp.hpp"
#include <ctype.h>

ot_tcpip_protocol ot_tcpip;

/*
FILE *log_file=NULL;
extern int net_start();
void net_log(char *st, void *buf, long size)
{
    
  if (!log_file) 
  {
    if (net_start())
      log_file=fopen("client.log","wb");
    else
      log_file=fopen("server.log","wb");
  }


    fprintf(log_file,"%s%d - ",st,size);
    int i;
    for (i=0;i<size;i++) 
      if (isprint(*((unsigned char *)buf+i)))
        fprintf(log_file,"%c",*((unsigned char *)buf+i));
      else fprintf(log_file,"~");

    fprintf(log_file," : ");

    for (i=0;i<size;i++) 
    fprintf(log_file,"%02x, ",*((unsigned char *)buf+i),*((unsigned char *)buf+i));
    fprintf(log_file,"\n");
    fflush(log_file);

} */

pascal void ot_tcp_EventHandler(void*, OTEventCode event, OTResult result, void* cookie)
{
	OTEventCode tempevent = 0;
	ot_tcp_socket *sckt = (ot_tcp_socket*)cookie;

	switch(event)
	{
	case T_OPENCOMPLETE:
		sckt->err = (OSStatus)result;
		sckt->code = event;
		break;
	case T_DATA:
		sckt->ready = 1;
		break;
	case T_BINDCOMPLETE:
//		gBindCompleted = 1;
		break;
	case T_ORDREL:
//		gCallRcvOrdDiscon = 1;
		break;
	default:
		// OTDebugBreak("EventHandler got unexpected event");
//		tempevent = event;
		break;
	}
	return;
}

ot_tcp_socket::ot_tcp_socket()
{
	TEndpointInfo	info;
	OSStatus err;

	// create TCP endpoint structure for MAC
	ep = OTOpenEndpoint(OTCreateConfiguration(kTCPName), 0, &info, &err);
	if ( ep == NULL || err != kOTNoError )
	{
		ep = NULL;
		fprintf(stderr,"ERROR: OpenEndpoint(\"TCP\") failed with %d\n", err);
		return;
	}

	// required by mac?
	err = ep->SetSynchronous();
	if ( err != kOTNoError )
	{
		fprintf(stderr,"ERROR: SetSynchronous() failed with %d\n", err);
		return;
	}

	// Install notifier of communication events
	err = ep->InstallNotifier(ot_tcp_EventHandler, this);
	if ( err != kOTNoError )
	{
		fprintf(stderr, "ERROR: InstallNotifier() failed with %d\n", err);
		return;
	}
	listening = 0;
}

int ot_tcp_socket::read(void *buf, int size, net_address **addr) 
{
  int result;
  
  result = ep->Rcv(buf, size, 0);
  if (addr) *addr=NULL;
  
  if (result>=0)
  	return result;
  else
  {
  	// save error code for next select
  	return 0;
  }
}

int ot_tcp_socket::write(void *buf, int size, net_address *addr)
{ 
  if (addr) fprintf(stderr,"Cannot change address for this socket type\n");
  
	result = ep->Snd(buf, len, 0);
	if (result >= 0)
		return result;
	else if (result != kOTFlowErr) 
	{
		// store error for next select
		return 0;
	}
}

int ot_tcp_socket::listen(int port)
{
  sockaddr_in host;
  memset( (char*) &host,0, sizeof(host));
  host.sin_family = AF_INET;
  host.sin_port = htons(port);
  host.sin_addr.s_addr = htonl(INADDR_ANY);
  if (ep->Bind(ep, (struct sockaddr *) &host, sizeof(sockaddr_in))==-1)
  {
    fprintf(stderr,"net driver : could not bind socket to port %d\n",port);
    return 0;
  }
  if (::listen(fd,5)==-1)
  {
    fprintf(stderr,"net driver : could not listen to socket on port %d\n",port);    
    return 0;
  }
  listening=1;
  return 1;
}

net_socket *ot_tcp_socket::accept(net_address *&addr);
{ 
  if (listening)
  {
    struct sockaddr_in from;
    int addr_len=sizeof(from);
    int new_fd=::accept(fd,(sockaddr *)&from,&addr_len);
    if (new_fd>=0)
    {
			addr=new ip_address(&from);
			return new tcp_socket(new_fd);
    }
    else
    { addr=NULL; return 0; }
  }
}

int ot_tcp_socket::connect(ot_ip_address addr);
{ 
	TBind ret;
	InetAddr retsin;

	ret.addr.maxlen = sizeof(struct InetAddress);
	ret.addr.buf = (unsigned char *) &retsin;

	// bind TCP to current address and port
	err = fd->Bind(nil, &ret);
	if ( err != kOTNoError )
	{
		printf("ERROR: Bind() failed with %d\n", err);
		break;
	}
	err = ep->SetSynchronous();
	if ( err != kOTNoError )
	{
		printf("ERROR: SetSynchronous() failed with %d\n", err);
		break;
	}

	def_addr.addr.len = sizeof(struct InetAddress);
	def_addr.addr.buf = (unsigned char *) &sndsin;

	err = ep->Connect(&sndcall, nil);
	if ( err != kOTNoError )
	{
		printf("ERROR: Connect() failed with %d\n", err);
		break;
	}
}

pascal void ot_udp_EventHandler(void*, OTEventCode event, OTResult, void*)
{
	if (event == T_DATA)
	{
		gDataToRead = 1;
		return;
	}
	if (event == T_BINDCOMPLETE)
	{
		gBindCompleted = 1;
		return;
	}
	return;
}

ot_udp_socket::ot_udp_socket()
{
	TEndpointInfo	info;
	OSStatus err;

	fd = OTOpenEndpoint(OTCreateConfiguration(kUDPName), 0, &info, &err);
	if ( ep == NULL || err != kOTNoError )
	{
		ep = NULL;
		fprintf(stderr,"ERROR: OpenEndpoint(\"UDP\") failed with %d\n", err);
		break;
	}
	
	// Install notifier
	err = ep->InstallNotifier(ot_udp_EventHandler, this);
	if ( err != kOTNoError )
	{
		fprintf(stderr, "ERROR: InstallNotifier() failed with %d\n", err);
		break;
	}

}

int ot_udp_socket::read(void *buf, int size, net_address **addr);
{
  int tr;
  if (addr) 
  {
    *addr=new ip_address;
    int addr_size=sizeof(ip_address::addr);
    tr=recvfrom(fd,buf,size,0, (sockaddr *) &((ip_address *)(*addr))->addr,&addr_size);
  } else
    tr=recv(fd,buf,size,0);
  return tr;
}

int ot_udp_socket::write(void *buf, int size, net_address *addr);
{
	OSStatus	err = kOTNoError;
	TUnitData	unitdata;
	char		mystr[255];

	unitdata.addr.len = sizeof (struct InetAddress);
  if (addr)
 		unitdata.addr.buf = (UInt8*) &((ot_ip_address *)addr->addr);
  else
 		unitdata.addr.buf = (UInt8*) &def_addr;
	unitdata.opt.len = 0;
	unitdata.opt.buf = 0;
	unitdata.udata.len = size;
	unitdata.udata.buf = (UInt8*) buf;

	err = ep->SndUData( &unitdata);

	if ( err != kOTNoError )
	{
		fprintf(stderr, "SndUData() returns %d\n", err);
	}
	else 
		return size;
}

int ot_udp_socket::listen(int port);
{
	sockaddr_in host;
	memset( (char*) &host,0, sizeof(host));
	host.sin_family = AF_INET;
	host.sin_port = htons(port);
	host.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(fd, (struct sockaddr *) &host, sizeof(sockaddr_in))==-1)
	{
	  fprintf(stderr,"net driver : could not bind socket to port %d\n",port);
	  return 0;
	}
	return 1;
}

net_address *tcpip_protocol::get_local_address()
{
  char my_name[100];                              // now check to see if this address is 'hostname'
  gethostname(my_name,100);
  struct hostent *l_hn=gethostbyname(my_name);  
  ip_address *a=new ip_address();
  memset(&a->addr,0,sizeof(a->addr));
  memcpy(&a->addr.sin_addr,*l_hn->h_addr_list,4);  
}

net_address *tcpip_protocol::get_node_address(char *&server_name, int def_port, int force_port)
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

  hostent *hp=gethostbyname(name);
  if (!hp)
  { 
    fprintf(stderr,"unable to locate server named '%s'\n",name);
    return 0;
  }
  

  sockaddr_in host;
  memset( (char*) &host,0, sizeof(host));
  host.sin_family = AF_INET;
  host.sin_port = htons(def_port);
  host.sin_addr.s_addr = htonl(INADDR_ANY);
  memcpy(&host.sin_addr,hp->h_addr,hp->h_length);
  return new ip_address(&host);
}

net_socket *tcpip_protocol::connect_to_server(net_address *addr, net_socket::socket_type sock_type)
{
  if (addr->protocol_type()!=net_address::IP)
  {
    fprintf(stderr,"Procotol type not supported in the executable\n");
    return NULL;
  }

  int socket_fd=socket(AF_INET,sock_type==net_socket::SOCKET_SECURE ? SOCK_STREAM : SOCK_DGRAM,0);
  if (socket_fd<0) 
  {
    fprintf(stderr,"unable to create socket (too many open files?)\n");
    return 0;
  }

  int zz;
  if (setsockopt(socket_fd,SOL_SOCKET,SO_REUSEADDR,(char *)&zz,sizeof(zz))<0)
  {
    fprintf(stderr,"could not set socket option reuseaddr");
    return 0;
  }

    
  if (connect(socket_fd, (struct sockaddr *) &((ip_address *)addr)->addr, sizeof( ((ip_address *)addr)->addr ))==-1)
  { 
    fprintf(stderr,"unable to connect\n");
    close(socket_fd);
    return 0;
  }

  if (sock_type==net_socket::SOCKET_SECURE)
    return new tcp_socket(socket_fd);
  else
    return new udp_socket(socket_fd);
}


net_socket *tcpip_protocol::create_listen_socket(int port, net_socket::socket_type sock_type, char *name)
{
  int socket_fd=socket(AF_INET,sock_type==net_socket::SOCKET_SECURE ? SOCK_STREAM : SOCK_DGRAM,0);
  if (socket_fd<0) 
  {
    fprintf(stderr,"unable to create socket (too many open files?)\n");
    return 0;
  }
/*  int zz;
  if (setsockopt(socket_fd,SOL_SOCKET,SO_REUSEADDR,(char *)&zz,sizeof(zz))<0)
  {
    fprintf(stderr,"could not set socket option reuseaddr");
    return 0;
  } */


  net_socket *s;
  if (sock_type==net_socket::SOCKET_SECURE)
    s=new tcp_socket(socket_fd);
  else s=new udp_socket(socket_fd);
  if (s->listen(port)==0)
  {   
    delete s;
    return 0;
  }

  return s;
}


tcpip_protocol::tcpip_protocol()
{
  FD_ZERO(&master_set);  
  FD_ZERO(&master_write_set);  
  FD_ZERO(&read_set);
  FD_ZERO(&exception_set);
  FD_ZERO(&write_set); 
}


int tcpip_protocol::select(int block)
{
  memcpy(&read_set,&master_set,sizeof(master_set));
  memcpy(&exception_set,&master_set,sizeof(master_set));
  memcpy(&write_set,&master_write_set,sizeof(master_set));
  if (block)
    return ::select(FD_SETSIZE,&read_set,&write_set,&exception_set,NULL);
  else
  {
    timeval tv={0,0};
    return ::select(FD_SETSIZE,&read_set,&write_set,&exception_set,&tv);
  }
}


