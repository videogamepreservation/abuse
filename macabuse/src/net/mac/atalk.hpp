#include "sock.hpp"
#include "dprint.hpp"
//#define dprintf printf

#include <string.h>
#include <stdio.h>
#include <AppleTalk.h>
#include <ADSP.h>

#include "isllist.hpp"

#define MAXSOCKS 30
#define QSIZE 1024

class atalk_address : public net_address
{
public :
  AddrBlock addr;

	atalk_address(short net, short node)
	{ 
		addr.aNet = net; 
		addr.aNode = node;
		addr.aSocket = 0;
	}
  atalk_address(AddrBlock *Addr) 
  {
  	memcpy(&addr,Addr,sizeof(addr));
  }
  atalk_address() {}

  virtual protocol protocol_type() const { return net_address::ATALK; }
  int get_port() { return addr.aSocket; }

  virtual int equal(const net_address *who) const
  {
    if (who->protocol_type()==ATALK &&
				(addr.aNet == ((atalk_address *)who)->addr.aNet) &&
				(addr.aNode == ((atalk_address *)who)->addr.aNode))
			return 1;
    else
    	return 0;
  }
  virtual int set_port(int port)  { return addr.aSocket=port; }

  virtual void print()
  {
    dprintf("%d:%d:%d",addr.aNet,addr.aNode,addr.aSocket);
  }
  net_address *copy()  { return new atalk_address(&addr); }
  void store_string(char *st, int st_length)
  {
    char buf[100];
    sprintf(buf,"%d:%d:%d",addr.aNet,addr.aNode,addr.aSocket);
    strncpy(st,buf,st_length);
    st[st_length-1]=0;    
  }
};

class atalk_base : public net_socket
{
protected:
  int num;
  int select_flags;
public:
	int select_count()
	{
		return ( ((select_flags&1 && ready_to_read())? 1 : 0)
					 + ((select_flags&2 && ready_to_write())? 1 : 0) );
	}

  atalk_base(int fd);
  virtual ~atalk_base();

  virtual void read_selectable() { select_flags |= 1; }
  virtual void read_unselectable() { select_flags &= ~1; }
  virtual void write_selectable() { select_flags |= 2; }
  virtual void write_unselectable() { select_flags &= ~2; }
  
  int get_fd() { return num; }  
};

class adsp_listener;
class atalk_protocol;

class adsp_socket : public atalk_base
{
	friend adsp_listener;
private:
	TRCCB	ccb;
protected:
	DSPParamBlock	dsp;
	char send_queue[QSIZE];
	char recv_queue[QSIZE];
	char attn_buff[attnBufSize];
	short	conn;
public :
  adsp_socket(int fd);
	int open(atalk_address *addr);
	void cleanup();
	virtual ~adsp_socket() { cleanup(); }

  virtual int error();
  virtual int ready_to_read();
  virtual int ready_to_write();                  

  virtual int read(void *buf, int size, net_address **addr);
  virtual int write(void *buf, int size, net_address *addr=NULL);
};

class adsp_listener : public atalk_base
{
	friend atalk_protocol;
private:
	TRCCB	ccb;
protected:
	DSPParamBlock	dsp;
	short	conn;
	int listening;
public:
  adsp_listener(int fd);
	void cleanup();
	virtual ~adsp_listener() { cleanup(); }

  virtual int error() { return 0; }
  virtual int ready_to_read();
  virtual int ready_to_write() { return 0; }

  virtual int listen(int port);
  virtual net_socket *accept(net_address *&addr);

  virtual void write_selectable() { return; }
  virtual void write_unselectable() { return; }

  virtual int read(void *buf, int size, net_address **addr) { return 0; }
  virtual int write(void *buf, int size, net_address *addr=NULL) { return 0; }
};

class ddp_socket : public atalk_base
{
protected:
	ATDDPRec ddp;
	char buffer[602 + 20 + 602*16];
	short socket;

	struct sPacket
	{
		long Count;
		AddrBlock Addr;
		long Data;
	};
	struct sDDPBuff
	{
		struct sPacket *Start,*End,*Head,*Tail;
		long Count;
		long Data;
	};
public :

	AddrBlock def_addr;
	short get_socket() const { return socket; }

  ddp_socket(int fd);
	void cleanup();
  virtual ~ddp_socket() { cleanup(); }

  virtual int error();
  virtual int ready_to_read();
  virtual int ready_to_write() { return 1; }

  virtual int read(void *buf, int size, net_address **addr);
  virtual int write(void *buf, int size, net_address *addr=NULL);
};

class atalk_protocol : public net_protocol
{
	friend adsp_listener;
	friend ddp_socket;
	
protected:
	MPPParamBlock mpp,nbp;
	NamesTableEntry	NTEName;
	Str32 Name;
	atalk_base *socket[MAXSOCKS];
	int usage[MAXSOCKS];
	int free;
	int ok;	
	short listener;
	int registered;
	int querying;
	char Buff[(586+2)*10 + 20];
	
	// Zone information
	enum { MAXZONENAMES = 1024 };
	struct sZoneBlock
	{
		char Name[1024];
	};
	typedef isllist<sZoneBlock*>::iterator p_zoneblock;
	isllist<sZoneBlock*> zones;
	char *(ZoneName[MAXZONENAMES]);
	long last_pos;
	sZoneBlock *last_block;
	int num_zones;
	Str255 MyZone;
	
  // Request Data
  struct RequestItem
  {
  	atalk_address *addr;
  	char name[256];   //name
  };
  typedef isllist<RequestItem *>::iterator p_request;
  isllist<RequestItem*> servers,returned;
public :
	int new_socket();
	void free_socket(int num);

  atalk_protocol();
  virtual ~atalk_protocol() { cleanup(); }

  net_address *get_local_address();
  net_address *get_node_address(char *&server_name, int def_port, int force_port);
  net_socket *connect_to_server(net_address *addr, 
				net_socket::socket_type sock_type=net_socket::SOCKET_SECURE);
  net_socket *create_listen_socket(int &port, net_socket::socket_type sock_type);
  int installed() { return ok; }
  char *name() { return "AppleTalk"; }
  void cleanup();
  int select(int block);          // return # of sockets available for read & writing

  // Notification methods
  virtual net_socket *start_notify(int port, void *data, int len);
  virtual void end_notify();

  // Find notifiers methods
  virtual net_address *find_address(int port, char *name);   // name should be a 256 byte buffer
  virtual void reset_find_list();
  
  // Specialized Zone stuff
  char **GetZones(int &num);
  int GetMyZone(char *);
  void SetZone(char *);
  void AddZone(unsigned char *name);   // pascal string
  void FreeZones();
};

extern atalk_protocol atalk;

