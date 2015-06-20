
#include "ipx.hpp"
#include "doscall.hpp"
#include "jmalloc.hpp"
#include <ctype.h>
#include <time.h>

ipx_net_protocol ipx_net;

#define JC_SEG(x) (((long)x)>>4)
#define JC_OFF(x) (((long)x)&0xf)

#define htons(x) (((x&0xff)<<8) | ((x>>8)&0xff))

static rminfo rm;
static uchar b_addr[6]={0xff,0xff,0xff,0xff,0xff,0xff};  // broadcast address

closed_secure_ipx_socket::closed_secure_ipx_socket(ipx_net_socket::ipx_packet *pk, 
						   int total_packets,
						   int fd, 
						   unsigned char final, 
						   closed_secure_ipx_socket *next) : 
						   pk(pk), fd(fd), next(next) 
{ 
  tp=total_packets;
  state=CLOSING;
  final_packet_number=final;
  close_send_count=0; 
  start_counter.get_time();
}

int ipx_net_address::set_port(int Port) 
{ 
  port=Port;
  return 1; 
}


void ipx_idle()
{
  memset(&rm,0,sizeof(rm));
  rm.ebx=0xa;
  RM_intr(0x7a,&rm); 
}

void show_ipx_address(uchar *addr, ushort sock)
{
  fprintf(stderr,"%x.%x.%x.%x.%x.%x.%x.%x.%x.%x:%x",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7],addr[8],addr[9],sock);
}  

static int ipx_listen_to_packet(ipx_net_socket::ipx_packet::ECBStructure *ecb)
{
  memset(&rm,0,sizeof(rm));
  rm.esi=JC_OFF(ecb);
  rm.es=JC_SEG(ecb);

  rm.ebx=4;
  RM_intr(0x7a,&rm);

  if (rm.eax&0xff)
    return 0;
  else return 1;  
}


void ipx_send_data(int size, ipx_net_socket::ipx_packet *pk)
{

  if (ipx_net.debug_level(net_protocol::DB_MINOR_EVENT))
  fprintf(stderr,"(send %d)",size);

  memset(&rm,0,sizeof(rm));
  rm.ebx=3;                      // send packet function
  rm.esi=JC_OFF(&pk[0].ecb);
  rm.es=JC_SEG(&pk[0].ecb);
  pk[0].ecb.set_data_size(size+pk[0].packet_prefix_size());
  
  RM_intr(0x7a,&rm);

}

net_address *ipx_net_protocol::get_local_address()
{
  if (Installed)
  {
    ipx_net_address *a=new ipx_net_address;
    uchar *low_addr=(uchar *)low_paragraph;

    memset(&rm,0,sizeof(rm));
    rm.ebx=9;
    rm.esi=JC_OFF(low_addr);
    rm.es=JC_SEG(low_addr);
    RM_intr(0x7a,&rm);
    
    memcpy(a->addr,low_addr,10);
    return a;
  }
  return NULL;
}

static int total_open_sockets=0;

static void ipx_close_socket(int port)
{
  if (ipx_net.debug_level(net_protocol::DB_IMPORTANT_EVENT))
    fprintf(stderr,"(close socket, port=%d)",port);

  memset(&rm,0,sizeof(rm));
  rm.ebx=1;                      // close socket function
  rm.edx=htons(port);     
  RM_intr(0x7a,&rm);
  total_open_sockets--;
}

static int ipx_open_socket(int port)
{
  if (ipx_net.debug_level(net_protocol::DB_MINOR_EVENT))
    fprintf(stderr,"(open socket, port=%d)",port);

  do
  {
    memset(&rm,0,sizeof(rm));
    rm.ebx=0;                      // Open socket function

    rm.eax=0;                      // close on program exit
    rm.edx=htons(port);     
    RM_intr(0x7a,&rm);

    //  fprintf(stderr,"******* open socket %d\n",htons((rm.edx&0xffff)));

  } while ((rm.eax&0xff) && ipx_net.free_closed_socket());

  if (rm.eax&0xff)
  {
    if (ipx_net.debug_level(net_protocol::DB_MAJOR_EVENT))
      fprintf(stderr,"Unable to created ipx socket on port %d, socket table full\n",port);
    return -1;
  }  

  total_open_sockets++;
  return (ushort)htons((rm.edx&0xffff));
}

int ipx_net_socket::open_socket(int port)
{
  return ipx_open_socket(port);
}

void ipx_net_protocol::show_socks()
{
  ipx_net_socket *f=list;
  for (;f;f=f->next) { fprintf(stderr,"("); f->print(); fprintf(stderr,")"); }

  closed_secure_ipx_socket *c=closed_list;
  for (;c;c=c->next) { fprintf(stderr,"("); c->print(); fprintf(stderr,")"); }
}

int ipx_net_protocol::socket_can_be_allocated(int num_packets)
{
  int fd=ipx_open_socket(0);
  if (fd<0)
  {
    fprintf(stderr,"could not allocate a socket, total open=%d\n",total_open_sockets);
    ipx_net.show_socks();
    return 0;    
  }
  else
  {
    ipx_close_socket(fd);
    
    // now make sure enough low memory is available to allocate space for the packets    
    void *mem;
    do
    {
      mem=alloc_low_memory(sizeof(ipx_net_socket::ipx_packet)*num_packets);
    } while (mem==0 && ipx_net.free_closed_socket());
    if (!mem) 
    {
      fprintf(stderr,"could not allocate memory for a socket, size=%d (t=%d), total open=%d, low avail=%d\n",
	      sizeof(ipx_net_socket::ipx_packet)*num_packets,num_packets,total_open_sockets,low_memory_available());
      ipx_net.show_socks();
      return 0;
    }
    free_low_memory(mem);    // we don't really need it
      

    return 1;
  }
}


int ipx_net_protocol::free_closed_socket()   // returns 1 if able to close one socket from the closed (close pool) list
{
  closed_secure_ipx_socket *f=closed_list,*l=NULL;
  for (;f;f=f->next)   // these sockets are just sitting listening for close pings
  {
    if (f->state==closed_secure_ipx_socket::TIME_WAIT)
    {
      if (l) l->next=f->next;
      else closed_list=f->next;
      delete f;
      return 1;
    }
    l=f;
  }


  if (closed_list)  // gonna have to try to close a socket that's sending closed packets
  {
    closed_secure_ipx_socket *f=closed_list;
    closed_list=closed_list->next;
    delete f;
    return 1;
  }

  return 0;
}


ipx_net_socket::~ipx_net_socket()
{
  if (backup) jfree(backup);
  if (fd>=0)  
    ipx_close_socket(fd);
  ipx_net.remove_socket_from_list(this);
  if (pk)
    free_low_memory(pk);
}

closed_secure_ipx_socket::~closed_secure_ipx_socket()
{
  if (pk)
    free_low_memory(pk);

  if (fd>=0)
    ipx_close_socket(fd);
}


void ipx_net_socket::ipx_packet::add_to_packet(void *buf, int size)
{
  if (size && size+packet_prefix_size()<=ipx_net.max_packet_data_size())
  {
    memcpy(data+packet_size()+packet_prefix_size(),buf,size);
    set_packet_size(packet_size()+size);
  }
}

int closed_secure_ipx_socket::poll()                      // called by select, return 0 when socket is expired
{
  int i;
  for (i=1;i<tp;i++)
  {
    if (!pk[i].ecb.InUseFlag)
    {
      if (pk[i].get_flag(ipx_net_socket::FIN))           // other socket is finished?
      {
	if (pk[i].get_flag(ipx_net_socket::ACK))         // do they wnat an acknowedgement?
	{
	  if (ipx_net.debug_level(net_protocol::DB_MINOR_EVENT))
	    fprintf(stderr,"(FIN->ACK)");
	  while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
	  pk[0].reset();
	  pk[0].set_flag(ipx_net_socket::FIN,1);         // tell other side we have closed as well
	  pk[0].set_tick_received(final_packet_number);
	  ipx_send_data(0,pk);
	}

	if (state==CLOSING)                              // if we were waiting for this then go to timeout state
	{
	  if (ipx_net.debug_level(net_protocol::DB_MINOR_EVENT))
	    fprintf(stderr,"(CLOSING->TIME_WAIT)");
	  state=TIME_WAIT;
	  start_counter.get_time();
	}
      } else if (pk[i].get_flag(ipx_net_socket::WRITE_DATA) &&  // they want a acknowedgement of the last packet we read?
		 pk[i].tick_received()==(uchar)(final_packet_number-1))
      {
	if (ipx_net.debug_level(net_protocol::DB_MINOR_EVENT))
	  fprintf(stderr,"(write ACK)");

	while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
	pk[0].reset();
	pk[0].set_flag(ipx_net_socket::WRITE_CONFIRM,1);          // acknowedge the data
	pk[0].set_tick_received((uchar)(final_packet_number-1));
	ipx_send_data(0,pk);
      }
      ipx_listen_to_packet(&pk[i].ecb);                              // reset the packet for listening
    }
  }

  time_marker now;  
  if (state==CLOSING)
  {
    if (close_send_count==0 || now.diff_time(&start_counter)*10>CLOSE_MSG_RETRY_TIMEOUT)
    {
      if (ipx_net.debug_level(net_protocol::DB_MINOR_EVENT))
        fprintf(stderr,"(send FIN|ACK)");
      close_send_count++;
      while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
      pk[0].reset();
      pk[0].set_flag(ipx_net_socket::FIN | ipx_net_socket::ACK,1);
      pk[0].set_tick_received(final_packet_number);
      ipx_send_data(0,pk);
      start_counter.get_time();
    }       
  } else if (state==TIME_WAIT && now.diff_time(&start_counter)*10>TIME_WAIT_EXPIRE)
    return 0;
  
  return 1;
}

ipx_net_socket::ipx_net_socket(int port, int total_read_packets, ipx_net_address *dest)
{  
  backup=NULL; backup_size=backup_end=backup_start=0;
  pk=NULL;
  er=0;
  status=INIT;
  fd=open_socket(port);
  ipx_net.add_socket_to_list(this);
  total_packets=total_read_packets+1;

  if (fd>=0)
  {
    void *mem;
    do
    {
      mem=alloc_low_memory(sizeof(ipx_net_socket::ipx_packet)*ipx_packet_total());
    } while (mem==0 && ipx_net.free_closed_socket());
    if (!mem)
    {
      fprintf(stderr,"unable to allocate low memory for packets, size = %d (t=%d), total open=%d, low_avail=%d\n",
	      sizeof(ipx_packet)*ipx_packet_total(),ipx_packet_total(),total_open_sockets,low_memory_available()); 
      ipx_net.show_socks();
      ipx_close_socket(fd);
      fd=-1;
      return ;
    }

    pk=(ipx_packet *)mem;
    memset(pk,0,sizeof(ipx_packet)*ipx_packet_total());
    

    // setup an outgoing packet structure
    pk[0].ecb.ECBSocket = htons(fd);
    pk[0].ecb.FragmentCount = 1;
    pk[0].ecb.fAddress[0] = JC_OFF(&pk[0].ipx);
    pk[0].ecb.fAddress[1] = JC_SEG(&pk[0].ipx);


    memcpy(pk[0].ipx.sAddr,ipx_net.local_addr,10);
    pk[0].ipx.sSocket=htons(fd);

    set_socket_dest(dest->addr,dest->port);

    for (int i=1;i<ipx_packet_total();i++)
    {
      // setup incoming packet structure
      pk[i].ecb.InUseFlag = 0x1d;
      pk[i].ecb.ECBSocket = htons(fd);
      pk[i].ecb.FragmentCount = 1;
      pk[i].ecb.fAddress[0] = JC_OFF(&pk[i].ipx);
      pk[i].ecb.fAddress[1] = JC_SEG(&pk[i].ipx);
      pk[i].ecb.fSize = sizeof(pk[0].ipx)+sizeof(pk[0].data);

      if (listen_to_packet(i)<0)
        fprintf(stderr,"listen failed on packet %d\n",i);
    }
  }
}


void ipx_net_socket::send_ping_packet()
{
  while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
  pk[0].reset();
  send_data(0);   // send a 0 length data packet to remote host
}

ipx_net_socket *ipx_net_protocol::socket_origination(uchar *addr,         // their address
						     ushort other_port)
{
  ipx_net_socket *p=list;
  for (;p;p=p->next)
  {
    if (p->pk && p->from_port()!=-1)
    {
      int correct_machine=memcmp(p->pk[0].ipx.dAddr,addr,10)==0;
      int correct_port=htons(p->pk[0].ipx.dSocket);
      if (correct_machine && correct_port==other_port)
        return p;
    }
  }
  return NULL;
}

void ipx_net_socket::clear()
{
  if (pk && ready_to_read())
  {
    for (int i=1;i<ipx_packet_total();i++)
      if (!pk[i].ecb.InUseFlag) 
        listen_to_packet(i);
  }
}

void ipx_net_socket::set_socket_dest(uchar *addr, ushort port)
{
  uchar *low_addr=(uchar *)ipx_net.low_paragraph;

  memcpy(pk[0].ipx.dAddr,addr,10);
  pk[0].ipx.dSocket=htons(port);

  int fail=1;       // if we can't get the local target, use a broadcast
  if (low_addr)
  {
    memset(&rm,0,sizeof(rm));
    rm.ebx=2;
    memcpy(low_addr,addr,10);
    
    rm.es=JC_SEG(low_addr);
    rm.esi=JC_OFF(low_addr);
    rm.edi=rm.esi+10;
    RM_intr(0x7a,&rm);
    
    if ((rm.eax&0xff)==0)  // was ipx able to get the local target?
    {
      memcpy(pk[0].ecb.ImmediateAddress,low_addr+10,6);
      fail=0;
    }

  }
  if (fail)
    memset(pk[0].ecb.ImmediateAddress,0xff,6);
}

int ipx_secure_listening_socket::ready_to_read()
{
  // the following packets are checked for

  //    locate  : dest address is (broadcast), send responce packet
  //    connect : store remote address in connect_request and return True

  for (int i=1;i<ipx_packet_total();i++)
  {
    if (!pk[i].ecb.InUseFlag)
    {
      if (memcmp(pk[i].ipx.sAddr,ipx_net.local_addr,10)!=0)
      {
	if (memcmp(pk[i].ipx.dAddr+4,b_addr,6)==0)                   // is this a 'locate' packet?
	{
	  if (ipx_net.debug_level(net_protocol::DB_MINOR_EVENT))
	    fprintf(stderr,"(listen::rtr->locate responce)");
	  while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
	  pk[0].reset();
	  uchar nm_len=strlen(name)+1;
	  pk[0].add_to_packet(&nm_len,1);
	  pk[0].add_to_packet(name,nm_len);
	  pk[0].calc_checksum();
	  
	  set_socket_dest(pk[i].ipx.sAddr,htons(pk[i].ipx.sSocket));

	  send_data(strlen(name)+2);                                              // send response packet
	  listen_to_packet(i);
	}
	else if (pk[i].get_flag(CONNECT))
	{
	  // is this address already allocated a socket?
	  ipx_net_socket *c=ipx_net.socket_origination(pk[i].ipx.sAddr,                // their address
						       htons(pk[i].ipx.sSocket));      // their port

	  if (c) 
	  {
	    if (ipx_net.debug_level(net_protocol::DB_MINOR_EVENT))
	      fprintf(stderr,"(listen::rtr->connect response, me=%d, other=%d)",
		      htons(pk[i].ipx.dSocket),htons(pk[i].ipx.sSocket));

	    ((ipx_secure_socket *)c)->send_ping_packet();                                   // tell this socket to tell it's address
	    listen_to_packet(i);
	  }
	  else 
	  {
	    if (ipx_net.debug_level(net_protocol::DB_MINOR_EVENT))
	      fprintf(stderr,"(listen::rtr->see real data)");

	    if (ipx_net.socket_can_be_allocated(ipx_secure_socket::SECURE_TOTAL_READ_PACKETS+1))
	      return 1;
	    else	    
	      listen_to_packet(i);	    	    	      	    
	  }
	} else
	{
	  if (ipx_net.debug_level(net_protocol::DB_IMPORTANT_EVENT))
	    fprintf(stderr,"(listen::see unknown packet, flags=%d)",pk[i].get_flag(ALL_FLAGS));
	  listen_to_packet(i);
	}
      } else 
      {
	if (ipx_net.debug_level(net_protocol::DB_MINOR_EVENT))
	  fprintf(stderr,"(listen::rtr->see self packet)");
	listen_to_packet(i);
      }
    } 
  }
  return 0;
}


net_socket *ipx_secure_listening_socket::accept(net_address *&from)
{
  // first check to make sure there is a pending connection
  if (ipx_net.debug_level(net_protocol::DB_IMPORTANT_EVENT))
    fprintf(stderr,"(accept)");

  int i=1;
  for (;i<ipx_packet_total();i++)
  {
    if (!pk[i].ecb.InUseFlag &&                     // active packet
	memcmp(pk[i].ipx.dAddr+4,b_addr,6)!=0 &&    // not a 'ping' packet

	!ipx_net.socket_origination(pk[i].ipx.sAddr,                // their address
				    htons(pk[i].ipx.sSocket)))      // their port
                                                                    // no active connections with this address on this port
    {
      if (!ipx_net.socket_can_be_allocated(ipx_secure_socket::SECURE_TOTAL_READ_PACKETS+1)) 
      {from=NULL; return NULL; }   // no more sockets, can't do anything..

      from=new ipx_net_address(pk[i].ipx.sAddr,htons(pk[i].ipx.sSocket));
      ipx_secure_socket *s=new ipx_secure_socket(htons(pk[i].ipx.sSocket),
						 0,(ipx_net_address *)from);    // dynamically allocate port number
      if (s->fd<0)
      { delete s;
	from=NULL;
	return NULL;
      }

      s->send_ping_packet();                                                    // tell remote host which port we are on
      listen_to_packet(i);
      if (ipx_net.debug_level(net_protocol::DB_MAJOR_EVENT))
      {
        fprintf(stderr,"(accepting ");
	if (from) from->print(); 
	fprintf(stderr,")");
      }

      return s;
    }
  }
  from=NULL;
  return NULL;
}

ipx_secure_socket::~ipx_secure_socket()
{
  if (!closed)
  {
    if (ipx_net.debug_level(net_protocol::DB_MAJOR_EVENT))
       fprintf(stderr,"(close secure %d)",fd);

    ipx_net.add_closed(pk,2,fd,packet_on);
    pk=NULL;
    fd=-1;
  }
}

ipx_secure_listening_socket::~ipx_secure_listening_socket()
{
;
}

void ipx_net_protocol::add_closed(ipx_net_socket::ipx_packet *pk, int total_packets, int fd, int packet_on)
{
  closed_list=new closed_secure_ipx_socket(pk,total_packets, fd,packet_on,closed_list);
}


net_address *ipx_net_protocol::find_host(int port, int timeout)
{
  ipx_net_address *dest=new ipx_net_address;
  memcpy(dest->addr,local_addr,4);      // locate on the same net as us
  memset(dest->addr+4,0xff,6);               // broadcast the packet and wait for a response
  dest->port=port;

  if (ipx_net.debug_level(DB_MAJOR_EVENT))
    fprintf(stderr,"(find_host)");

  int found=0;
  {
    int tp=ipx_secure_socket::SECURE_TOTAL_READ_PACKETS+1;
    if (!ipx_net.socket_can_be_allocated(tp)) return NULL;   // no more sockets, can't do anything..

    ipx_net_socket s(0,tp,dest);                  // allocate any abritray port, to use to send and recieve a response
    if (s.fd<0) return NULL;
    memset(s.pk[0].ecb.ImmediateAddress,0xff,6);  // broadcast the packet

    while (s.pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
    s.pk[0].reset();
    do
    {
      fprintf(stderr,".");
      s.send_data(0);
      
      timeout--;
      time_marker start;


      int old_time=timeout;
      
      do
      {
	time_marker now;
	if (now.diff_time(&start)>0.1)
	{
	  timeout--;
	  start.get_time();
	}

	int i=1;
	for (;i<s.ipx_packet_total();i++)
	{
	  if (!s.pk[i].ecb.InUseFlag)
	  {
	    if (ipx_net.debug_level(DB_MINOR_EVENT))
	      fprintf(stderr,"(find_host->see packet)");

	    if (memcmp(s.pk[i].ipx.sAddr,ipx_net.local_addr,10)!=0 &&     // from source address differs from ours?
		htons(s.pk[i].ipx.sSocket)==port)                         // and from the port is the one we are pinging?
	    {
	      memcpy(dest->addr,s.pk[i].ipx.sAddr,10);
	      dest->port=htons(s.pk[i].ipx.sSocket);
	      found=1;
	    }

	    s.listen_to_packet(i);
	  }	    
	} 
      } while (timeout==old_time && found==0);
    } while (timeout>0 && found==0);
  }

  if (found)
    return dest;
    
  delete dest;
  return NULL;
}


net_socket *ipx_net_protocol::connect_to_server(net_address *addr, 
						net_socket::socket_type sock_type)
{
  if (ipx_net.debug_level(DB_MAJOR_EVENT))
  {
    fprintf(stderr,"(connect to server "); addr->print(); fprintf(stderr,")");
  }

  if (addr->protocol_type()!=net_address::IPX)
  {
    fprintf(stderr,"cannot connect a non-ipx address on the ipx protocol\n");
    return NULL;
  }
  
  ipx_net_socket *s;


  if (sock_type!=net_socket::SOCKET_FAST)    // fast socket are assumed to be connected
  {
    if (!ipx_net.socket_can_be_allocated(ipx_secure_socket::SECURE_TOTAL_READ_PACKETS+1)) return NULL;   // no more sockets, can't do anything..
    s=new ipx_secure_socket(-1,0,(ipx_net_address *)addr);    // allocate a dynamic port
    if (s->fd<0) { delete s; return NULL; }
    s->pk[0].reset();
    s->pk[0].set_flag(ipx_net_socket::CONNECT,1);


    int retry=0;
    time_marker start_clock;
    do
    {
      time_marker now;
      if (retry==0 || now.diff_time(&start_clock)*10>CONNECT_PING_RETRY)
      {
	retry++;
	s->send_data(0);                   // send a packet to the server, packet has our port to respond to
	start_clock.get_time();
      }

      for (int i=1;i<ipx_secure_socket::SECURE_TOTAL_READ_PACKETS+1;i++)    // check for a response
      {
	if (!s->pk[i].ecb.InUseFlag)                                           // incomming packet?
	{
	  if (memcmp(s->pk[i].ipx.sAddr,((ipx_net_address *)addr)->addr,10)==0 &&    // from expected host?
	      htons(s->pk[i].ipx.sSocket)!=((ipx_net_address *)addr)->port)          // not from connection port?
	  {
	    if (ipx_net.debug_level(DB_MINOR_EVENT))
	      fprintf(stderr,"(got connect reply from port %d)",htons(s->pk[i].ipx.sSocket));
	    s->set_socket_dest(s->pk[i].ipx.sAddr,htons(s->pk[i].ipx.sSocket));
	    s->listen_to_packet(i);
	    return s;                                      // return the established connection
	  } else
	  {
	    if (ipx_net.debug_level(DB_IMPORTANT_EVENT))
	      fprintf(stderr,"(got connect reply from wrong addr/port)");
	  }

	  s->listen_to_packet(i);
	}
      }

      ipx_net.clear_sockets(NULL);
    } while (retry<CONNECT_RETRY_TOTAL);
    delete s;
    s=NULL;
  } else
  {
    if (!ipx_net.socket_can_be_allocated(16+1)) return NULL;   // no more sockets, can't do anything..
    s=new ipx_fast_socket(0,(ipx_net_address *)addr);
    if (s->fd<0) { delete s; return NULL; }    
  }

  return s;
}

net_address *ipx_net_protocol::get_node_address(char *&server_name, int def_port, int force_port)
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

  if (name[0]==0 || stricmp(name,"any")==0)
    return find_host(def_port,1000);       // timeout 1 second
  else 
  {
    int addr[10];
    ipx_net_address *ret=new ipx_net_address;
    ret->port=def_port;
    int t=sscanf(name,"%x.%x.%x.%x.%x.%x.%x.%x.%x.%x",
		 addr,addr+1,addr+2,addr+3,addr+4,addr+5,addr+6,addr+7,addr+8,addr+9);
    if (t==6)
    {
      memcpy(ret->addr,local_addr,4); // assume on same network
      for (int i=0;i<6;i++) ret->addr[i+4]=addr[i];
    } else if (t==10)
    {
      for (int i=0;i<10;i++) 
        ret->addr[i]=addr[i];     // convert int to uchar
    } else                        // must be a bad address
    {
      delete ret;
      ret=NULL;
    }
    return ret;
  }

  return NULL;  // stupid compilers
}

net_socket *ipx_net_protocol::create_listen_socket(int port, net_socket::socket_type sock_type, char *sock_name)
{
  if (!ipx_net.socket_can_be_allocated(3)) return NULL;   // no more sockets, can't do anything..

  if (ipx_net.debug_level(DB_MAJOR_EVENT))
    fprintf(stderr,"opening listening socket : port = %d, sock_type =%d\n",port,sock_type);

  ipx_net_address a;
  if (sock_type==net_socket::SOCKET_SECURE)
    return new ipx_secure_listening_socket(port,&a,sock_name);
  else return new ipx_fast_socket(port,&a);

  return NULL;
}

int ipx_net_socket::listen_to_packet(int packet_num)
{
  if (ipx_listen_to_packet(&pk[packet_num].ecb))
    return 1;
  else
  {
    er=1;
    return 0;
  }
}


int ipx_net_socket::ready_to_read()     
{ 
  for (int i=1;i<ipx_packet_total();i++)
  {
    if (!pk[i].ecb.InUseFlag)
    {
      if (memcmp(pk[i].ipx.dAddr+4,b_addr,6)==0)                   // is this a 'locate' packet?
      {
/*	while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
	memcpy(pk[0].ipx.dAddr,pk[i].ipx.sAddr,10);                // address to send back to
	pk[0].ipx.dSocket=pk[i].ipx.sSocket;
	pk[0].reset();
	send_data(0); */                                             // send response packet
	listen_to_packet(i);
      } else if (memcmp(pk[i].ipx.sAddr,ipx_net.local_addr,10)==0) // if this packet was from us, reset listen and ignore
	  listen_to_packet(i);
      else  
        return 1;
    }
  }
  return 0;
}


int ipx_fast_socket::ready_to_read()
{
  for (int i=1;i<ipx_packet_total();i++)
  {
    if (!pk[i].ecb.InUseFlag)
    {
      if (memcmp(pk[i].ipx.dAddr+4,b_addr,6)==0)                   // is this a 'locate' packet?
      {
/*	while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
	memcpy(pk[0].ipx.dAddr,pk[i].ipx.sAddr,10);                // address to send back to
	pk[0].ipx.dSocket=pk[i].ipx.sSocket;
	pk[0].reset();
	send_data(0); */                                              // send response packet 
	listen_to_packet(i);
      } else if (memcmp(pk[i].ipx.sAddr,ipx_net.local_addr,10)==0) // if this packet was from us, reset listen and ignore
	  listen_to_packet(i);
      else if (memcmp(pk[i].ipx.dAddr,ipx_net.local_addr,10)!=0)
      {
	if (ipx_net.debug_level(net_protocol::DB_MAJOR_EVENT))
	{
	  fprintf(stderr,"see packet not addressed to us!\n");
	  show_ipx_address(pk[i].ipx.dAddr,pk[i].ipx.dSocket);
	  fprintf(stderr,",");
	  show_ipx_address(ipx_net.local_addr,pk[0].ipx.sSocket);
	  fprintf(stderr,"\n");
	}

	
	listen_to_packet(i);
      }
      else  
      {
        return 1;
      }
    }
  }
  return 0;
}



int ipx_secure_socket::ready_to_read()     
{ 
  if (closed) return 1;

  if (backup_start<backup_end) return 1;

  for (int i=1;i<ipx_packet_total();i++)
  {
    if (!pk[i].ecb.InUseFlag)
    {

      if (memcmp(pk[i].ipx.dAddr+4,b_addr,6)==0)                   // is this a 'locate' packet?
      {
	listen_to_packet(i);
      } else if (memcmp(pk[i].ipx.sAddr,ipx_net.local_addr,10)==0) // if this packet was from us, reset listen and ignore
	  listen_to_packet(i);
      else if ( ((pk[i].tick_received()+1)&0xff)==packet_on && // old packet, send confirm crc anyway
		 pk[i].get_flag(WRITE_DATA)                   &&
		 pk[i].get_checksum()==pk[i].calc_checksum())
      {
	while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
	pk[0].reset();
	pk[0].set_flag(WRITE_CONFIRM,1);
	pk[0].set_tick_received(pk[i].tick_received());
	send_data(0);
	listen_to_packet(i);	
      } else if (pk[i].tick_received()==packet_on &&
		 pk[i].get_flag(WRITE_DATA))
        return 1;
      else if (pk[i].tick_received()==packet_on &&
	       pk[i].get_flag(ipx_net_socket::FIN))
      {

	if (pk[i].get_flag(ipx_net_socket::ACK))
	{
	  while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
	  pk[0].reset();
	  pk[0].set_flag(ipx_net_socket::WRITE_CONFIRM,1);          // acknowedge the data
	  pk[0].set_tick_received((uchar)(packet_on-1));
	  ipx_send_data(0,pk);
	}
	listen_to_packet(i);
	closed=1;
	ipx_net.add_closed(pk,2,fd,packet_on);
	pk=NULL;
	fd=-1;
      
      } else listen_to_packet(i);
    }
  }

  return 0;
}



void ipx_net_socket::ipx_packet::ECBStructure::set_data_size(int size)
{
  fSize=sizeof(ipx_net_socket::ipx_packet::IPXPacketStructure)+size;
}


void paddr(uchar *addr)
{
  for (int i=0;i<10;i++) fprintf(stderr,"%x.",*(addr++));
}

virtual int ipx_secure_socket::write(void *buf, int size, net_address *addr)
{
  if (addr)
  {
    fprintf(stderr,"Cannot alter address for secure sockets\n");
    return 0;
  }

  if (ipx_net.debug_level(net_protocol::DB_IMPORTANT_EVENT))
    fprintf(stderr,"(FW %d,%d)",size,fd);

  if (closed) { er=1; return 0; }

  int ss=0,i;
  if (ipx_net.debug_level(net_protocol::DB_IMPORTANT_EVENT))
    fprintf(stderr,"(W %d,%d)",size,fd);

  while (size && er==0)
  {
    while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
    pk[0].reset();
    int ws;

    if (size<ipx_net.max_packet_data_size()-pk[0].packet_prefix_size())
      ws=size;
    else ws=ipx_net.max_packet_data_size()-pk[0].packet_prefix_size();

    int verify=0,retry_total=0;
    time_marker start_clock;
    do
    {
      time_marker now;
      if (retry_total==0 || (int)(now.diff_time(&start_clock)*100.0)>SEND_RETRY_TIMEOUT)
      {
	while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
	pk[0].reset();
	pk[0].add_to_packet(buf,ws);
	pk[0].set_tick_received(packet_on);
	pk[0].set_flag(WRITE_DATA,1);
	pk[0].calc_checksum();

	send_data(ws);

	if (ipx_net.debug_level(net_protocol::DB_IMPORTANT_EVENT))
	  fprintf(stderr,"w");

	start_clock.get_time();
	retry_total++;
	if (retry_total>RETRY_TOTAL)
	  er=1;       
      }

    
      for (i=1;i<ipx_packet_total();i++)    // wait till we get a packet back with the same packet_on
        if (!pk[i].ecb.InUseFlag)
	{
	  if (pk[i].tick_received()==packet_on && pk[i].get_flag(ipx_net_socket::WRITE_CONFIRM))
	  {
	    size-=ws;
	    ss+=ws;
	    buf=(void *)((uchar *)buf+ws);
	    packet_on++;
	    verify=1;
	    listen_to_packet(i);

	  } else clear_packet(i);
	  
	}

      ipx_net.clear_sockets(this);

    } while (verify==0 && er==0 && closed==0);
    
  }

  if (ipx_net.debug_level(net_protocol::DB_IMPORTANT_EVENT))
    fprintf(stderr,"(~W)");

  return ss;
}

void ipx_net_socket::send_data(int data_size)
{
  ipx_send_data(data_size,pk);
}


void ipx_secure_socket::clear_packet(int i)
{
  if (pk[i].tick_received()==packet_on && pk[i].get_flag(ipx_net_socket::FIN)) // other socket is finished?
  {	      
    if (pk[i].get_flag(ipx_net_socket::ACK))         // do they wnat an acknowedgement?
    {
      while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
      pk[0].reset();
      pk[0].set_flag(ipx_net_socket::FIN,1);         // tell other side we have closed as well
      pk[0].set_tick_received(packet_on);
      ipx_send_data(0,pk);
    }
    
    listen_to_packet(i);

    closed=1;
    ipx_net.add_closed(pk,2,fd,packet_on);
    pk=NULL;
    fd=-1;

  } else if ( ((pk[i].tick_received()+1)&0xff)==packet_on && pk[i].get_flag(WRITE_DATA))  // old packet, send confirm crc anyway
  {	  
    while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet    
    pk[0].reset();
    pk[0].set_flag(WRITE_CONFIRM,1);
    pk[0].set_tick_received(pk[i].tick_received());
    send_data(0);
    listen_to_packet(i);

  } else listen_to_packet(i);


}


int ipx_secure_socket::read(void *buf, int size, net_address **addr)
{
  if (closed) { er=1; return 0; }

  if (addr) *addr=0;
  int rs=0;

  if (ipx_net.debug_level(net_protocol::DB_IMPORTANT_EVENT))
    fprintf(stderr,"(R %d,%d)",size,fd);

  rs+=read_backup(buf,size);

  while (size && !er && !closed)
  {
    int got_packet=0;
    time_marker start_clock;
    do
    {
      for (int i=1;size && i<ipx_packet_total();i++)    // wait till we get a packet back with the same packet_on
      {
	if (!pk[i].ecb.InUseFlag)
	{
	  if (pk[i].get_flag(WRITE_DATA) && pk[i].tick_received()==packet_on)
	  {
	    if (pk[i].get_checksum()==pk[i].calc_checksum())
	    {
	      int s=pk[i].packet_size();
	      if (s)
	      {
		if (s>size)
		{
		  memcpy(buf,pk[i].packet_data(),size);  
		  add_backup(pk[i].packet_data()+size,s-size);      // add the rest to a backup packet	      
		  rs+=size;
		  size=0;
		} else 
		{
		  memcpy(buf,pk[i].packet_data(),s);
		  buf=(void *)((uchar *)buf+s);
		  size-=s;
		  rs+=s;
		}
		while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet

		memcpy(pk[0].data,pk[i].data,pk[i].packet_prefix_size());   // copy packet prefix info into packet
		pk[0].set_flag(ALL_FLAGS,0);
		pk[0].set_flag(WRITE_CONFIRM,1);

		send_data(0);
		packet_on++;
		got_packet=1; 
	      } else fprintf(stderr,"received 0 sized packet\n");
	    } else fprintf(stderr,"bad checksum\n");
	    listen_to_packet(i);
	  
	  } else clear_packet(i);

	}
      }

      time_marker now;
      if (now.diff_time(&start_clock)*10>RETRY_TOTAL*SEND_RETRY_TIMEOUT)  // if we don't get a packet after this long, report an error
        er=1;
      ipx_net.clear_sockets(this);

    } while (!got_packet && !er && !closed);

  }

  if (ipx_net.debug_level(net_protocol::DB_IMPORTANT_EVENT))
    fprintf(stderr,"(~R)");
  
  return rs;
}


ipx_net_protocol::ipx_net_protocol()
{
  locator_socket=NULL;
  find_list=NULL;
  low_paragraph=NULL;
  memset(&rm,0,sizeof(rm));
  rm.eax=0x7a00;
  RM_intr(0x2f,&rm);
  list=NULL;
  closed_list=NULL;

  if ((rm.eax&0xff)==0xff)      // drivers installed?
  {
    memset(&rm,0,sizeof(rm));   // get the maximum packet size
    rm.ebx=0xd;
    RM_intr(0x7a,&rm);    
    max_pksz=rm.eax&0xffff;

    if (max_pksz<1024) 
    {
      fprintf(stderr,"IPX : Maximum packet size is too small, needs to be at least 1024 (is %d)\n",max_pksz);
      Installed=0;
    } else 
    {
      low_paragraph=alloc_low_memory(16);    // need this to store address when looking up immediate address
      if (!low_paragraph)
      {
	fprintf(stderr,"IPX : Unable to allocate 16 bytes of low memory!\n");
	Installed=0;
      } else
      {
	Installed=1;
	net_address *a=get_local_address();
	if (a)
	{
	  memcpy(local_addr,((ipx_net_address *)a)->addr,10);
	  delete a;
	} else Installed=0;
      }

    }
  }
  else
  {
    Installed=0;
    max_pksz=0;
  }

}



void ipx_net_protocol::add_socket_to_list(ipx_net_socket *who)
{
  who->next=list;
  list=who;
}

void ipx_net_protocol::remove_socket_from_list(ipx_net_socket *who)
{
  if (who==list)
    list=list->next;
  else if (list && list->next)
  {
    ipx_net_socket *last=(ipx_net_socket *)list;
    for (;last->next && last->next!=who;last=last->next);
    if (last->next==who)
      last->next=who->next;         
  }
}

int ipx_net_protocol::select(int block)
{
  int t=0;
  do
  {
    ipx_net_socket *p=list;
    for (;p;p=p->next)
    {
      if ((p->status & ipx_net_socket::READ_CHECKING) && (p->ready_to_read())) 
      {
	t++;
      }
      if ((p->status & ipx_net_socket::WRITE_CHECKING) && (p->ready_to_write())) 
      {
	t++;
      }
      if (p->er) t++;
    }

    closed_secure_ipx_socket *c=closed_list,*last=NULL,*q;
    for (;c;)
    {
      if (!c->poll())
      {
	if (last) last->next=c->next;
	else closed_list=c->next;
	q=c;
	c=c->next;
	delete q;
      } else
      {
	last=c;
	c=c->next;
      }
    }
    

  } while (t==0 && block);  

  return t;
}

void ipx_net_protocol::cleanup()
{
  reset_find_list();

  ipx_net_socket *first=list;
  list=NULL;

  while (first)
  {
    ipx_net_socket *q=first;
    first=first->next;
    delete q;
  }

  closed_secure_ipx_socket *c=closed_list;
  int need_wait=0;
  for (;c;c=c->next)  
    if (c->state==closed_secure_ipx_socket::CLOSING)
      need_wait=1;

  time_marker now,start;
  while (need_wait)
  {
    if (now.diff_time(&start)>3.0) need_wait=0;  // wait a max of 3 seconds
    else 
    {
      now.get_time();
      select(0);

      need_wait=0;
      c=closed_list;
      for (;c;c=c->next)  
        if (c->state==closed_secure_ipx_socket::CLOSING)
	   need_wait=1;
    }    
  }
  c=closed_list;

  while (c)
  {
    closed_secure_ipx_socket *q=c;
    c=c->next;
    delete q;
  }
  closed_list=NULL;
}


int ipx_fast_socket::write(void *buf, int size, net_address *addr)
{
  while (pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the previous packet

  if (size>ipx_net.max_packet_data_size())   // make sure this size will fit in IPX max allowable packet size
    size=ipx_net.max_packet_data_size();

  pk[0].reset();
  pk[0].add_to_packet(buf,size);

  if (addr)
    set_socket_dest( ((ipx_net_address *)addr)->addr,
		     ((ipx_net_address *)addr)->port);

  send_data(size);

  return size;
}

int ipx_fast_socket::read(void *buf, int size, net_address **addr)
{
  if (ipx_net.debug_level(net_protocol::DB_IMPORTANT_EVENT))
    fprintf(stderr,"(FR %d,%d)",size,fd);

  while (1)
  {
    int i;
    for (i=1;i<ipx_packet_total();i++)
      if (!pk[i].ecb.InUseFlag)
      {
	int s=pk[i].packet_size();	
	if (size<s) s=size;
	memcpy(buf,pk[i].packet_data(),s);
	if (addr) *addr=new ipx_net_address(pk[i].ipx.sAddr,htons(pk[i].ipx.sSocket));
	listen_to_packet(i);
	return s;
      }
    ipx_net.clear_sockets(this);
  }
  return 0;
}



int ipx_net_socket::read_backup(void *&buf, int &size)    // read previously buffer data
{ 
  int s=backup_end-backup_start;
  if (s && size)
  {					    
    if (size>=s)
    {
      memcpy(buf,backup+backup_start,s);
      buf=(void *)((uchar *)buf+s);
      backup_start=backup_end=0;
      size-=s;
      return s;
    } else
    {
      memcpy(buf,backup+backup_start,size);
      buf=(void *)((uchar *)+size);
      int ret=size;
      backup_start+=size;
      size=0;
      return ret;
    }
  } else return 0;
}



void ipx_net_socket::add_backup(uchar *buf, int size)
{
  if (size)
  {
    if (backup_size-backup_end>=size)
    {
      memcpy(backup+backup_end,buf,size);
      backup_end+=size;
    } else 
    {
      backup_size+=backup_end+size-backup_size;
      backup=(uchar *)jrealloc(backup,backup_size,"backup buffer");
      add_backup(buf,size);
    }
  }
}


void ipx_net_protocol::clear_sockets(ipx_net_socket *exclude)
{
  ipx_net_socket *p=list;
  for (;p;p=p->next)
    if (p!=exclude)
      p->clear();

  closed_secure_ipx_socket *c=closed_list,*last=NULL,*q;
  for (;c;)
  {
    if (!c->poll())
    {
      if (last) last->next=c->next;
      else closed_list=c->next;
      q=c;
      c=c->next;
      delete q;
    } else
    {
      last=c;
      c=c->next;
    }
  }

}

ipx_net_protocol::~ipx_net_protocol()
{
  if (low_paragraph)
    free_low_memory(low_paragraph);
}


net_address *ipx_net_protocol::find_address(int port, char *name) 
{ 

  if (!locator_socket) 
  { 
    if (!ipx_net.socket_can_be_allocated(LOCATOR_PACKETS)) return NULL;   // no more sockets, can't do anything..

    ipx_net_address *dest=new ipx_net_address;
    memcpy(dest->addr,local_addr,4);      // locate on the same net as us
    memset(dest->addr+4,0xff,6);               // broadcast the packet and wait for a response
    dest->port=port;


    locator_socket=new ipx_net_socket(0,LOCATOR_PACKETS,dest);  // allocate abritraty socket #
    delete dest;

    if (locator_socket->fd<0) { delete locator_socket; locator_socket=NULL; return NULL; }


    memset(locator_socket->pk[0].ecb.ImmediateAddress,0xff,6);  // broadcast the packet
  } else locator_socket->pk[0].ipx.dSocket=htons(port);


  int i;
  for (i=1;i<LOCATOR_PACKETS;i++)
  {
    if  (!locator_socket->pk[i].ecb.InUseFlag)  // see something?
    {
      if (memcmp(locator_socket->pk[i].ipx.sAddr,ipx_net.local_addr,10)!=0 &&     // from source address differs from ours?
		htons(locator_socket->pk[i].ipx.sSocket)==port)                         // and from the port is the one we are pinging?
      {
	locator_node *p=find_list,*found=NULL;
	for (;p && !found;p=p->next)
	  if (memcmp(p->addr->addr,locator_socket->pk[i].ipx.sAddr,10)==0 && p->addr->get_port()==htons(locator_socket->pk[i].ipx.sSocket))
	    found=p;

	if (!found)
	{
	  uchar *c=locator_socket->pk[i].packet_data();
	  int len=*(c++);
	  memcpy(name,c,len);

	  find_list=new locator_node(new ipx_net_address(locator_socket->pk[i].ipx.sAddr,htons(locator_socket->pk[i].ipx.sSocket)),find_list);
	  locator_socket->listen_to_packet(i);
	  return find_list->addr->copy();
	}
      }
      locator_socket->listen_to_packet(i);
    }
  }
  
  time_marker now;
  if (now.diff_time(&last_ping)>0.3)
  {
    last_ping.get_time();
    
    while (locator_socket->pk[0].ecb.InUseFlag) ipx_idle();    // wait for ipx to clear the packet
    locator_socket->pk[0].reset();
    locator_socket->send_data(0);
  }

  return NULL;
}


void ipx_net_protocol::reset_find_list()
{
  if (locator_socket) { delete locator_socket; locator_socket=NULL; }

  while (find_list)
  {
    locator_node *q=find_list;
    find_list=find_list->next;
    delete q;    
  }
}

int ipx_net_protocol::installed() 
{ 
  if (!Installed) return 0;
  else if (low_memory_available()<150000)
    return 0;
  else return 1;
}
