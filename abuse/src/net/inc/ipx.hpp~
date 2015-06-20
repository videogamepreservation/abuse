#ifndef __IPX_SOCK_HPP_
#define __IPX_SOCK_HPP_

#include "sock.hpp"
#include "macs.hpp"
#include "timing.hpp"
#include <string.h>

#include <string.h>

class ipx_net_address : public net_address
{ 
  public :
  uchar addr[10];
  ushort port;      // stored in intel-format
  virtual protocol protocol_type() { return IPX; }
  virtual int equal(net_address *who)   { return who->protocol_type()==protocol_type() && 
					         memcmp(addr,((ipx_net_address *)who)->addr,10)==0; }
  virtual int set_port(int Port);
  virtual void print() { printf("network = %x.%x.%x.%x, node = %x.%x.%x.%x.%x.%x, port = %d\n",
                         addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7],addr[8],addr[9],port); }  
  ipx_net_address() { memset(addr,0,10); port=0; }
  ipx_net_address(uchar *addr_data, ushort port) : port(port) 
  { memcpy(addr,addr_data,10); }
  int get_port() { return port ; }
  net_address *copy() { return new ipx_net_address(addr,port); }
  void store_string(char *st, int st_length)
  {
    char buf[100];
    sprintf(buf,"%x.%x.%x.%x.%x.%x.%x.%x.%x.%x:%d",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7],addr[8],addr[9],port);
    strncpy(st,buf,st_length);
    st[st_length-1]=0;
  }
} ;

#pragma pack (1)


class ipx_net_socket : public net_socket
{
  public :
  uchar *backup;                   // when a partial read request happens
  int backup_start,backup_end,backup_size;
  int read_backup(void *&buf, int &size);
  void add_backup(uchar *buf, int size);

  ipx_net_socket *next;
  int total_packets;

  int ipx_packet_total() { return total_packets; }

  enum {IPX_PACKET_SIZE=1024} ;      // this amount of data space reserved for each packet
                                    // though the actaul packet sent may be smaller because of IPX limits


  enum { ACK=1,            // acknowedge receipt of packet
	 FIN=2,            // connection has finished
	 CONNECT=4,        // asking to make a connection
	 WRITE_DATA=8,
	 WRITE_CONFIRM=16,
	 ALL_FLAGS=31 };

  enum { SEND_RETRY_TIMEOUT=10,  // .1 seconds timeout before resending data packet
	 RETRY_TOTAL=2400        // give up after retring 2400 times, or 240 seconds =  4 minutes
       } ;



  struct ipx_packet
  {
    struct ECBStructure
    {
      ushort Link[2];
      ushort ESRAddress[2];
      uchar  InUseFlag;
      uchar  CompletionCode;
      ushort ECBSocket;
      uchar  IPXWorkspace[4];
      uchar  DriverWorkspace[12]; 
      uchar  ImmediateAddress[6]; 
      ushort FragmentCount;
      ushort fAddress[2];
      ushort fSize;

      int data_waiting() { return InUseFlag; }
      void set_data_size(int size);
    } ecb;

    struct IPXPacketStructure
    {
      ushort PacketCheckSum;
      ushort PacketLength;
      uchar  PacketTransportControl;
      uchar  PacketType;

      uchar  dAddr[10];
      ushort dSocket;      // flipped intel-format

      uchar  sAddr[10];
      ushort sSocket;     // flipped intel-format

    } ipx;


    uchar data[IPX_PACKET_SIZE];

    int packet_prefix_size()                 { return 6; }    // 2 size
                                                              // 2 byte check sum
                                                              // 1 byte packet order
                                                              // 1 byte flags
	   

    void set_packet_size(unsigned short x)   { *((unsigned short *)data)=lstl(x); }
    unsigned short packet_size()             { unsigned short size=(*(unsigned short *)data); return lstl(size); }
    unsigned char tick_received()            { return data[4]; }  
    void set_tick_received(unsigned char x)  { data[4]=x; }
    unsigned char *packet_data()             { return data+packet_prefix_size(); }
    unsigned short get_checksum()            { unsigned short cs=*((unsigned short *)data+1); return lstl(cs); }
    uchar get_flag(int flag)                 { return data[5]&flag; }
    void set_flag(int flag, int x)           { if (x) data[5]|=flag; else data[5]&=~flag; }
    unsigned short calc_checksum()
    {
      *((unsigned short *)data+1)=0;
      int i,size=packet_prefix_size()+packet_size();
      unsigned char c1=0x12,c2=0x34,*p=data;
      for (i=0;i<size;i++,p++)
      {
	c1+=*p;
	c2+=c1;
      }
      unsigned short cs=( (((unsigned short)c1)<<8) | c2);
      *((unsigned short *)data+1)=lstl(cs);
      return cs;
    }


    void reset()    { set_packet_size(0); set_flag(ALL_FLAGS,0); }     // 2 bytes for size, 1 byte for tick
    
    void add_to_packet(void *buf, int size);

    void write_byte(unsigned char x) { add_to_packet(&x,1); }
    void write_short(unsigned short x) { x=lstl(x); add_to_packet(&x,2); }
    void write_long(unsigned long x) { x=lltl(x); add_to_packet(&x,4); }


  } *pk;

	 

  int fd,er;
  void idle();   // call this if you are bored
  
  enum { INIT=0,
         READ_CHECKING=1,
	 WRITE_CHECKING=2,
	 READ_SELECTED=4,
	 WRITE_SELECTED=8 
       } ;
  uchar status;

  void send_data(int data_size);
  void send_ping_packet();
  int listen_to_packet(int packet_num);
  int open_socket(int port);

  virtual int error()                                              { return er; }
  virtual int ready_to_read();
  virtual int ready_to_write() { if (fd>=0) return pk[0].ecb.InUseFlag; else return 1; }

  virtual int write(void *buf, int size, net_address *addr=NULL) { return 0; }

  virtual int read(void *buf, int size, net_address **addr=0) { if (addr) *addr=0; return 0; }
  virtual int get_fd()    { return fd; }
  virtual void read_selectable()     { status|=READ_CHECKING; }
  virtual void read_unselectable()   { status&=~READ_CHECKING; }
  virtual void write_selectable()    { status|=WRITE_CHECKING; }
  virtual void write_unselectable()  { status&=~WRITE_CHECKING; }
  virtual int listen(int port)       { return 0; }
  virtual net_socket *accept(net_address *&from) { from=NULL; return NULL; }
  virtual int from_port() { return -1; }

  virtual void clear();

  void set_socket_dest(uchar *addr, ushort port);
  ipx_net_socket(int port, int total_read_packets, ipx_net_address *dest);
  virtual void print() { fprintf(stderr,"ipx net socket");  }
  virtual ~ipx_net_socket();
} ;

#pragma pack (0)


class ipx_secure_listening_socket;

class ipx_secure_socket : public ipx_net_socket
{  
  uchar packet_on,write_ack_needed,write_ack_checksum;
  ushort parent_port;

  enum { ESTABLISHED, 
	 CLOSE_WAIT,        // waiting for user to close
	 CLOSING,           // waiting for remote response to close
	 TIME_WAIT,         // waiting till 'counter' expires to ensure remote socket has gotten close message
       } state;

  int counter,closed;
  
  public :
  enum { SECURE_TOTAL_READ_PACKETS=2 } ;

  virtual int ready_to_read();
  virtual int ready_to_write() { return 1; }

  virtual int write(void *buf, int size, net_address *addr=NULL);
  virtual int read(void *buf, int size, net_address **addr=0);

  void clear_packet(int i);
  ipx_secure_socket(int parent_port, int port, ipx_net_address *dest) : 
    ipx_net_socket(port,SECURE_TOTAL_READ_PACKETS,dest),parent_port(parent_port)
  { packet_on=1; write_ack_needed=0; closed=0; }
  int from_port() { return parent_port; }
  int verify_sync();
  virtual void print() { fprintf(stderr,"ipx secure socket");  }
  ~ipx_secure_socket();
} ;


// the closed_secure_ipx_packet is not to be seen by anyone other than the ipx_protocol
// and it's is polled during calls to select()

class closed_secure_ipx_socket
{
  public :
  closed_secure_ipx_socket *next;

  enum { CLOSING,                       // keep send close message, until receive ack, then goto TIME_WAIT
	 TIME_WAIT                      // after ack for close message, wait for TIME_WAIT_EXPIRE before closing
       } state;

  enum { CLOSE_MSG_RETRY_TIMEOUT=20,    // .2 seconds between each close() message retry
         CLOSE_TOTAL_RETRY=10,          // 10 total retries for sending a close() message before giving up
	 TIME_WAIT_EXPIRE=1000          // 10 seconds before a socket expires and can be reused
       } ;

  time_marker start_counter;      // if CLOSING, then the clock() of the last close message sent
                                  // if TIME_WAIT, then clock() of the entering this state

  int close_send_count;           // number of times close socket msg has been sent (from CLOSING state)
                                  
  int fd;                          // the port this socket is using
  uchar final_packet_number;
  ipx_net_socket::ipx_packet *pk;  // pointer to low packet memory taken from a ipx_secure_socket
  int poll();                      // called by select, return 0 when socket is expired
  ~closed_secure_ipx_socket();
  int tp;
  closed_secure_ipx_socket(ipx_net_socket::ipx_packet *pk, 
			   int total_packets,
			   int fd, 
			   unsigned char final_packet_number, 
			   closed_secure_ipx_socket *next);

  void print() { fprintf(stderr,"closed secure socket, state=%d",(int)state);  }
} ;


// this class can only sit and listen, it will respond to 'ping' and 'connect' packets
// this class will return 1 for ready_to_read and then create a ipx_secure_socket on accept()

class ipx_secure_listening_socket : public ipx_net_socket
{
  enum { NAME_MAX=30 };
  char name[NAME_MAX];
  public :

  virtual int ready_to_write() { return 1; }
  virtual int ready_to_read();
  virtual int write(void *buf, int size, net_address *addr=NULL) { return 0; }
  virtual int read(void *buf, int size, net_address **addr=0) { if (addr) *addr=0; return 0; }

  virtual net_socket *accept(net_address *&from);
  ipx_secure_listening_socket(int port, ipx_net_address *dest, char *sock_name) : ipx_net_socket(port,2,dest) 
  { strncpy(name,sock_name,NAME_MAX-1); name[NAME_MAX-1]=0; }
  ~ipx_secure_listening_socket();
  virtual void print() { fprintf(stderr,"ipx secure listening socket");  }
} ;

class ipx_fast_socket : public ipx_net_socket
{
  public :
  int write(void *buf, int size, net_address *addr=NULL);
  int read(void *buf, int size, net_address **addr=0);
  virtual int ready_to_read();
  ipx_fast_socket(int port, ipx_net_address *dest) : ipx_net_socket(port,16,dest) { ; }
  void clear() { ; }  // no need to clear these
  virtual void print() { fprintf(stderr,"ipx fast socket");  }

} ;

class ipx_net_protocol : public net_protocol
{
  int Installed;
  ushort max_pksz;
  int max_packet_size() { return max_pksz; }

  net_address *find_host(int port, int timeout);
  ipx_net_socket *list;

  class locator_node
  {
    public :
    ipx_net_address *addr;
    locator_node *next;
    locator_node(ipx_net_address *addr, locator_node *next) : addr(addr), next(next) { ; }
    ~locator_node() { delete addr; }
  } *find_list;

  ipx_net_socket *locator_socket;
  time_marker last_ping;
  enum { LOCATOR_PACKETS=16 } ;

  public :
  void *low_paragraph;

  enum { CONNECT_PING_RETRY=10,     // retry connetc command every .1 seconds
	 CONNECT_RETRY_TOTAL=2400  // retry for 4 mins.
       } ;
	 
  closed_secure_ipx_socket *closed_list;

  uchar local_addr[10];

  virtual net_address *get_local_address();
  virtual net_address *get_node_address(char *&server_name, int def_port, int force_port);
  virtual net_socket *connect_to_server(net_address *addr, 
					net_socket::socket_type sock_type=net_socket::SOCKET_SECURE
					);  
  virtual net_socket *create_listen_socket(int port, net_socket::socket_type sock_type, char *sock_name);

  int select(int block);

  int installed();

  char *name()    { return "IPX driver"; }
  void add_socket_to_list(ipx_net_socket *who);
  void remove_socket_from_list(ipx_net_socket *who);

  int max_packet_data_size() { int x=max_pksz-sizeof(ipx_net_socket::ipx_packet::IPXPacketStructure);  // max IPX will allow
			       if (x>ipx_net_socket::IPX_PACKET_SIZE)                      // max space we have
			          return ipx_net_socket::IPX_PACKET_SIZE; else return x; 
			     }
  void clear_sockets(ipx_net_socket *exclude);
  ipx_net_socket *socket_origination(uchar *addr, ushort other_port);     // finds a socket connected to address, started from port 'port'
  void add_closed(ipx_net_socket::ipx_packet *pk, int total_packets, int fd, int packet_on);
  void cleanup();
  int free_closed_socket();       // returns 1 if able to close 1 closed socket in FIN state
  int socket_can_be_allocated(int num_packets);
  ipx_net_protocol();
  void show_socks();
  ~ipx_net_protocol();

  net_address *find_address(int port, char *name);
  void reset_find_list();
} ;

extern ipx_net_protocol ipx_net;


#endif

