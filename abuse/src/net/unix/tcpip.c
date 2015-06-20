#include "tcpip.hpp"
#include <ctype.h>

tcpip_protocol tcpip;

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


int unix_fd::read(void *buf, int size, net_address **addr) 
{
  int tr=::read(fd,buf,size);
  if (addr) *addr=NULL;
  return tr;
}

int unix_fd::write(void *buf, int size, net_address *addr)
{ 
  if (addr) fprintf(stderr,"Cannot change address for this socket type\n");
  return ::write(fd,buf,size); 
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



net_socket *tcpip_protocol::start_notify(int port, void *data, int len)
//{{{
{
	if (responder)
	{
		delete responder;
		delete bcast;
		responder = 0;
	}
	
	int resp_len = strlen(notify_response);
  notify_len = len + resp_len + 1;
  strcpy(notify_data,notify_response);
	notify_data[resp_len] = '.';
  memcpy(notify_data+resp_len+1,data,len);
  
  // create notifier socket
#ifdef TCPIP_DEBUG
	fprintf(stderr,"Creating notifier on port %d\n",port);
#endif
  notifier = create_listen_socket(port, net_socket::SOCKET_FAST);
  
  if (notifier)
  {
    notifier->read_selectable();
    notifier->write_unselectable();
  }
  else
		fprintf(stderr,"Couldn't start notifier\n");
  
  return notifier;
}
//}}}///////////////////////////////////


void tcpip_protocol::end_notify()
//{{{
{
  if (notifier)
    delete notifier;
  notifier = 0;

  notify_len = 0;
}
//}}}///////////////////////////////////

int tcpip_protocol::handle_notification()
//{{{
{
  if (!notifier)
    return 0;
    
  if (notifier->ready_to_read())
  {
    char buf[513];
    int len;
    // got a notification request "broadcast"
    ip_address *addr;

#ifdef TCPIP_DEBUG
		printf("Notifier: ");
#endif

    len = notifier->read(buf, 512, (net_address**)&addr);
#ifdef TCPIP_DEBUG
		if (len>0) {
			buf[len] = 0;
			printf("[%s] ",buf);
		}
#endif
    if (addr && len>0)
    {
			buf[len] = 0;
      if  (strcmp(buf, notify_signature)==0) {
				char s[256];
#ifdef TCPIP_DEBUG
				addr->store_string(s,256);
				printf("responding to %s",s);
#endif
        // send notification data to requester
        notifier->write(notify_data,notify_len,addr);
			}
        
      delete addr;
    }
#ifdef TCPIP_DEBUG
		printf("\n");
#endif
    return 1;
  }
  if (notifier->error())
  {
    fprintf(stderr,"Error on notification socket!\n");
    return 1;
  }

  return 0;
}
//}}}///////////////////////////////////

net_address *tcpip_protocol::find_address(int port, char *name)
//{{{
{
  // name should be a 256 byte buffer
	char s[256];

	end_notify();

  if (!responder) {
//#ifdef TCPIP_DEBUG
		fprintf(stderr,"Creating responder on port %d\n",port);
//#endif
    responder = create_listen_socket(port, net_socket::SOCKET_FAST);
		responder->read_selectable();
		responder->write_unselectable();
    bcast = (ip_address *)get_local_address();
    bcast->set_port(port);
    
//#ifdef TCPIP_DEBUG
		*((unsigned char *)(&bcast->addr.sin_addr)+3) = 255;
		bcast->store_string(s,256);
		fprintf(stderr,"Simulating broadcast to [%s]\n",s);
//#endif

		*((unsigned char *)(&bcast->addr.sin_addr)+3) = 0;		
	}

  if (responder)
  {
    int i;
    
    for (i=0; i<5; i++)
    {
#ifdef TCPIP_DEBUG
			bcast->store_string(s,256);
			fprintf(stderr,"\r[%s]",s);
#endif
	    int found = 0;
	    
	    for (p_request p = servers.begin(); !found && p!=servers.end(); ++p)
	    	if ( *((*p)->addr) == *bcast )
	    		found = 1;
	    for (p_request q = returned.begin(); !found && q!=returned.end(); ++q)
	    	if ( *((*q)->addr) == *bcast )
	    		found = 1;
	    		
			if (!found) {
				responder->write((void*)notify_signature,
												 strlen(notify_signature),bcast);
				select(0);
			}
	    *((unsigned char *)(&bcast->addr.sin_addr)+3) += 1;
	
	    select(0);
	    
	    if (!servers.empty())
	    	break;
		}
  }
  
  if (servers.empty())
    return 0;

  servers.move_next(servers.begin_prev(), returned.begin_prev());
	ip_address *ret = (ip_address*)(*returned.begin())->addr->copy();
	strcpy(name,(*returned.begin())->name);

#ifdef TCPIP_DEBUG
	ret->store_string(s,256);
	fprintf(stderr,"Found [%s]\n",s);
#endif

  return ret;
}
//}}}///////////////////////////////////

void tcpip_protocol::reset_find_list()
//{{{
{
	p_request p;
	
	for (p=servers.begin(); p!=servers.end(); ++p)
		delete (*p)->addr;
	for (p=returned.begin(); p!=returned.end(); ++p)
		delete (*p)->addr;
		
  servers.erase_all();
  returned.erase_all();
}
//}}}///////////////////////////////////
