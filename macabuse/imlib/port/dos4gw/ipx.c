#include "ipx.hpp"
#include "jnet.hpp"
#include "macs.hpp"
#include "dprint.hpp"
#include "system.h"
#include <i86.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "jmalloc.hpp"
#include "doscall.hpp"
#include "timing.hpp"

#define JC_SEG(x) (((long)x)>>4)
#define JC_OFF(x) (((long)x)&0xf)

static rminfo rm;

class in_socket_dispatcher
{
  
  ipx_in_socket **ins;
  long *ports;            // ports which the in sockets are supposed to be on
} ;


int ipx_init()
{
  memset(&rm,0,sizeof(rm));
  rm.eax=0x7a00;
  RM_intr(0x2f,&rm);
  return (rm.eax&0xff)==0xff;
}

void ipx_uninit() { ; }

void close_ipx_socket(int fd)
{
  memset(&rm,0,sizeof(rm));
  rm.ebx=1;
  rm.edx=(ushort)bstl(fd);
  RM_intr(0x7a,&rm);
}


void ipx_idle()
{
  memset(&rm,0,sizeof(rm));
  rm.ebx=0xa;
  RM_intr(0x7a,&rm); 
}

uchar *ipx_get_local_address()                // same format as above (be sure to jfree this)
{
  uchar *addr=(uchar *)jmalloc(10,"IPX address");  // normal memory return
  uchar *low_addr=(uchar *)alloc_low_memory(10);
  memset(&rm,0,sizeof(rm));
  rm.ebx=9;
  rm.esi=JC_OFF(low_addr);
  rm.es=JC_SEG(low_addr);
  RM_intr(0x7a,&rm);

  memcpy(addr,low_addr,10);
  free_low_memory(low_addr);
  return addr;
}


void ipx_listen(ECBStructure *ecb)
{
  memset(&rm,0,sizeof(rm));
  rm.esi=JC_OFF(ecb);
  rm.es=JC_SEG(ecb);

  rm.ebx=4;
  RM_intr(0x7a,&rm);

  if (rm.eax&0xff)
  {
    current_sock_err=SOCK_LISTEN_FAIL;
    sprintf(last_sock_err,"IPX : listen failed");
  }
}

ipx_out_socket::ipx_out_socket(int port)
{
  remote_socket=-1;              // mark not connected
  memset(&rm,0,sizeof(rm));
  rm.ebx=0;                      // Open socket function
  rm.eax=0;                      // close on program exit
  rm.edx=(ushort)(bstl(port));
  RM_intr(0x7a,&rm);
  if (rm.eax&0xff)
  {
    current_sock_err=SOCK_CREATE_FAIL;
    sprintf(last_sock_err,"Unable to open IPX socket\n");
    return ;
  }
  local_socket=(ushort)bstl((rm.edx&0xffff));
  printf("created socket %x\n",local_socket);


  pk=(JC_ipx_packet *)alloc_low_memory(sizeof(JC_ipx_packet)*MAX_PACKETS);
  if (!pk)
  {
    printf("unable to allocate low memory for packets\n");
    exit(0);
  }
  memset(pk,0,sizeof(JC_ipx_packet)*MAX_PACKETS);


  // setup an outgoing packet structure
  pk[0].ecb.ECBSocket = (ushort)(bstl(local_socket));
  pk[0].ecb.FragmentCount = 1;
  pk[0].ecb.fAddress[0] = JC_OFF(&pk[0].ipx);
  pk[0].ecb.fAddress[1] = JC_SEG(&pk[0].ipx);
//  pk[0].ipx.PacketCheckSum=0xffff;
 
  uchar *my_addr=ipx_get_local_address();
  memcpy(pk[0].ipx.sNetwork,my_addr,10);
  pk[0].ipx.sSocket=(ushort)(bstl(local_socket));
  jfree(my_addr);
  
  for (int i=1;i<MAX_PACKETS;i++)
  {
    // setup incoming packet structure
    pk[i].ecb.InUseFlag = 0x1d;
    pk[i].ecb.ECBSocket = (ushort)(bstl(local_socket));
    pk[i].ecb.FragmentCount = 1;
    pk[i].ecb.fAddress[0] = JC_OFF(&pk[i].ipx);
    pk[i].ecb.fAddress[1] = JC_SEG(&pk[i].ipx);
    pk[i].ecb.fSize = sizeof(JC_ipx_packet)-sizeof(ECBStructure);
    ipx_listen(&pk[i].ecb);
    printf("added listening ecb\n");
  }  

}

int ipx_send_packet(void *packet)
{
  memset(&rm,0,sizeof(rm));
  rm.ebx=3;                      // send packet function
  rm.esi=JC_OFF(packet);
  rm.es=JC_SEG(packet);
  RM_intr(0x7a,&rm); 
  return 1;
}


int ipx_out_socket::try_connect(char *rhost, int port)
{
  uchar *addr=(uchar *)name_to_address(rhost,IPX_PROTOCOL);

  printf("try connect to ");
  uchar *me=addr;
  for (int i=0;i<10;i++,me++)
    printf("%02x:",*me);
  printf("\n");

  if (addr)
  {
    while (pk[0].ecb.InUseFlag)                       // make sure packet is not being used!
      dprintf("Packet already in use (try connect)\n");

    memcpy(&pk[0].ipx.dNetwork[0],addr,10);     // assume they are playing on the same network
    memcpy(&pk[0].ecb.ImmediateAddress[0],addr+4,6);

    memset(&pk[0].ipx.dNetwork[0],0,10);

    memset(&pk[0].ipx.dNode[0],0xff,6);   
    memset(&pk[0].ecb.ImmediateAddress[0],0xff,6);

    pk[0].ipx.dSocket=(ushort)bstl(port);
    pk[0].verify_stamp=0xcdc;
    pk[0].ipx.PacketLength=(ushort)bstl(sizeof(IPXPacketStructure)+6);

    pk[0].ecb.ECBSocket = bstl(port);
    pk[0].ecb.FragmentCount = 1;
    pk[0].ecb.fAddress[0] = JC_OFF(&pk[0].ipx);
    pk[0].ecb.fAddress[1] = FP_SEG(&pk[0].ipx);
    pk[0].ipx.PacketCheckSum=0xffff;

//    for (int j=0 ; j<4 ; j++)
//      pk[0].ipx.dNetwork[j] = localadr.network[j];
    pk[0].ipx.dSocket = bstl(port);
    

    jfree(addr);
    int found_reply=-1;
    for (int tries=0;found_reply==-1 && tries<10;tries++)
    {
      ipx_send_packet(&pk[0].ecb);
      printf("sent request packet\n");
      ipx_idle();
      milli_wait(500);           // wait 1/2 second for a response
      for (int i=1;i<MAX_PACKETS;i++)
      {
	if (!pk[i].ecb.InUseFlag && pk[i].verify_stamp==0xcdc)
	{
	  if (found_reply!=-1)    // we found two replies, this shouldn't happen
	  {
	    dprintf("Two replies found for connection request, what's going on?\n");
	    found_reply=-1;
	    tries=10;
	  }
	  found_reply=i;
	  printf("found reply on packet %d\n",i);
	}
      }
    }


    if (found_reply==-1)
      return 0;
    else 
    { 
      remote_socket=pk[found_reply].time_stamp;
      local_time=remote_time=0;
      pk[0].ipx.dSocket=(ushort)bstl(remote_socket);
      return 1;
    }
  } else dprintf("Unable to get an IPX address for hostname '%s'\n",rhost);
  return 0;
}

int ipx_out_socket::ready_to_read()
{
  if (remote_socket==-1)  return 1;  // not connected, return true so program doesn't block for a
                                     // packet that will never come

  for (int i=1;i<MAX_PACKETS;i++)
  {
    if (!pk[i].ecb.InUseFlag && pk[i].verify_stamp==0xcdc)
      return 1;
  } 
  return 0;
}


int ipx_out_socket::ready_to_write()            // true if send packet is clear
{
  return pk[0].ecb.InUseFlag==0;
}



int ipx_out_socket::send(packet &pak)
{
  if (remote_socket==-1) return 0;  // not connected, cannot send packet

  ushort size=lstl(pak.wo-2);        // attach the packet size to the front of the packet
  memcpy(pak.buf,&size,2);
  

  int bytes_to_write=size+2;
  uchar *pk_off=pak.buf;
  while (bytes_to_write)           // break the packet up into 512 byte segments and send till through
  {
    while (!ready_to_write()) ;       // wait till last packet finishes

    int size_to_send=bytes_to_write>512 ? 512 : bytes_to_write;

    memcpy(pk[0].buffer,pk_off,size_to_send);
    pk[0].time_stamp=local_time;
    local_time++;
    pk[0].verify_stamp=0xcdc;
    pk[0].ipx.PacketLength=(ushort)bstl(sizeof(IPXPacketStructure)+6+size_to_send);

    ipx_send_packet(&pk[0]);
    
    pk_off+=size_to_send;
    bytes_to_write-=size_to_send;
  }

  return 1;
}


int ipx_out_socket::get(packet &pak)
{
  pak.ro=pak.wo=2;
  int start=-1;
  time_marker then;
  do
  {
    for (int i=1;start==-1 && i<MAX_PACKETS;i++)
      if (!pk[i].ecb.InUseFlag && pk[i].time_stamp==remote_time && pk[i].verify_stamp==0xcdc)
        start=i;
    time_marker now;

    if (now.diff_time(&then)>20)  // twenty second timeout
      return 0;

  } while (start==-1);     // wait until we get the packet we are looking for

  remote_time++;

  long data_size=pk[start].ipx.PacketLength-sizeof(IPXPacketStructure)-6;

  if (data_size<2)   // even enough space to read packet size?
    return 0;
 
  ushort size;
  memcpy(&size,pk[start].buffer,2);
  data_size-=2;

  size=lstl(size);  
  pak.rend=size+2;
  pak.make_bigger(pak.rend);

  uchar *buf=pak.buf+2;
  uchar *source=pk[start].buffer+2;

  int bytes_left=size;
  do
  {
    memcpy(buf,source,data_size);
    buf+=data_size;
    bytes_left-=data_size;
    ipx_listen(&pk[start].ecb);    // this packet is done and available for listening again
    pk[start].ecb.InUseFlag = 0x1d;

    if (bytes_left)              // see if we need to get more packets
    {
      time_marker then;
      do
      {
	for (int i=1;start==-1 && i<MAX_PACKETS;i++)
          if (pk[i].time_stamp==remote_time && pk[i].verify_stamp==0xcdc)
	    start=i;
	time_marker now;

	if (now.diff_time(&then)>20)  // twenty second timeout
          return 0;
      } while (start==-1);     // wait until we get the packet we are looking for

      remote_time++;
      source=pk[start].buffer;
      data_size=pk[start].ipx.PacketLength-sizeof(IPXPacketStructure)-6;
    }
  } while (bytes_left>0);

  return 1;
}


ipx_out_socket::~ipx_out_socket()
{ 
  close_ipx_socket(local_socket);
  free_low_memory(pk);
}

ipx_in_socket::ipx_in_socket(int Port)
{
  port=Port;
  listener=new ipx_out_socket(port);
}

out_socket *ipx_in_socket::check_for_connect()
{
  int start=-1;
  ipx_idle();
  for (int i=1;i<MAX_PACKETS;i++)
  {
    for (int i=1;start==-1 && i<MAX_PACKETS;i++)
      if (!listener->pk[i].ecb.InUseFlag)
      {
	printf("found a connection\n");
	if (listener->pk[i].verify_stamp==0xcdc)      
          start=i;
	else 
	{
	  listener->pk[i].ecb.InUseFlag=0x1d;
	  ipx_listen(&listener->pk[i].ecb);  // this packet is done and available for listening again

	  printf("bad stamp, getting packets from someone else!\n");
	}
      }
  }

  if (start!=-1)
  {
    ipx_out_socket *nsock=new ipx_out_socket(0);  // create a new socket and dynamicly allocate port
    listener->pk[0].verify_stamp=0xcdc;
    listener->pk[0].ipx.PacketLength=sizeof(IPXPacketStructure)+6;
    listener->pk[0].time_stamp=nsock->local_socket;

    uchar tmp_addr[10];
    memcpy(tmp_addr,listener->pk[0].ipx.dNetwork,10);    
    memcpy(listener->pk[0].ipx.dNetwork,
	   listener->pk[start].ipx.sNetwork,12);  // source <- dest
    memcpy(listener->pk[start].ipx.sNetwork,tmp_addr,10);  // dest <- source

    ipx_listen(&listener->pk[start].ecb);    // this packet is done and available for listening again
    listener->pk[start].ecb.InUseFlag = 0x1d;
    
    ipx_send_packet(&listener->pk[0]);        // send new port info back to caller
    return nsock;
  }
  return NULL;
}



ipx_in_socket::~ipx_in_socket()
{
  delete listener;
}





