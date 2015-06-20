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
#include "bwtcp.hpp"

static rminfo rm;

void set_sock_err(int x);
int net_ok=0;
ushort low_sock_mem_seg,low_sock_mem_off;
uchar *low_sock_mem;


int bwt_socket();
void bwt_close(int fd);


int bwt_ioctl_read(int fd);
int bwt_read(int fd, uchar *buffer, long size)
{
  memset(&rm,0,sizeof(rm));
  int t=0;                    // counter for bytes read
  int breakup;                // set if low buffer is not big enought o read everything
  do
  {
    rm.eax = 0x3f00;
    rm.ebx = fd;
    int rs;
    if (size>1024)
    {
      rs=1024;
      breakup=1;
    } else 
    {
      rs=size;
      breakup=0;
    }

    rm.ecx=rs;
    rm.edx=low_sock_mem_off;
    rm.ds=low_sock_mem_seg;
    RM_intr(0x21,&rm);

    if (rm.flags & 1)
    {
      printf("socket : read error\n");
    }
    int ret=rm.eax&0xffff;  
    t+=ret;
    memcpy(buffer,low_sock_mem,ret);
    if (ret!=rs) return t;
    size-=ret;
    buffer+=ret;    
  } while (breakup && size);
  return t;
}

int bwt_write(int fd, uchar *buffer, long size)
{
  memset(&rm,0,sizeof(rm));
  int t=0,breakup;
  do
  {
    rm.eax = 0x4000;
    rm.ebx = fd;
    int rs;
    if (size>1024)
    {
      rs=1024;
      breakup=1;
    } else
    {
      rs=size;
      breakup=0;
    }

    rm.ecx=rs;
    rm.edx=low_sock_mem_off;
    rm.ds=low_sock_mem_seg;
    memcpy(low_sock_mem,buffer,rs);
    RM_intr(0x21,&rm);
    if (rm.flags & 1)
    {
      printf("socket : write error\n");
    }
    int ret=rm.eax&0xffff;  
    t+=ret;
    if (ret!=rs) return t;
    size-=ret;
    buffer+=ret;    
  } while (breakup && size);
  return t;
}


int bwt_init()
{
  low_sock_mem=(uchar *)alloc_low_memory(3520);
  if (!low_sock_mem)
  {
    printf("failed to allocate memory for network driver\n");
    exit(0);
  }
  low_sock_mem_seg=((long)low_sock_mem)>>4;
  low_sock_mem_off=((long)low_sock_mem)&0x0f;

  int fd=bwt_socket();  // test to see if we can open a socket
  
  if (current_sock_err || fd==-1)
  {
    free_low_memory(low_sock_mem);
    return 0;
  } else 
  {
    bwt_close(fd);
    return 1;
  }
  return 0;
}

void bwt_uninit()
{
  free_low_memory(low_sock_mem);
}



int bwt_ioctl_write(int fd, int size)
{
  memset(&rm,0,sizeof(rm));
  rm.eax = 0x4403;
  rm.ebx = fd;
  rm.ecx = size;
  rm.edx = low_sock_mem_off;
  rm.ds = low_sock_mem_seg;


  RM_intr(0x21,&rm);
  if (rm.flags & 1)
  {
    current_sock_err=SOCK_WRITE_FAIL;
    sprintf(last_sock_err,"Error writing ioctl command\n");
    return -1;
  }
  return 0;
}


int bwt_ioctl_read(int fd)
{
  memset(&rm,0,sizeof(rm));
  rm.eax = 0x4402;
  rm.ebx = fd;
  rm.ecx = 21;
  rm.edx = low_sock_mem_off;
  rm.ds = low_sock_mem_seg;



  RM_intr(0x21,&rm);

  if (rm.flags & 1)
  {
    current_sock_err=SOCK_WRITE_FAIL;
    sprintf(last_sock_err,"Error reading ioctl status, flags=%d\n",rm.flags);
    return -1;
  }
  return 0;
}

int bwt_out_socket::ready_to_write()
{
  memset(&rm,0,sizeof(rm));
  rm.eax=0x4407;
  rm.ebx=fd;
  RM_intr(0x21,&rm);
  return (rm.eax&0xff)==0xff;
}


int bwt_out_socket::ready_to_read()
{
  if (pk_buffer_last>pk_buffer_ro) return 1;
  memset(&rm,0,sizeof(rm));
  rm.eax=0x4406;
  rm.ebx=fd;
  RM_intr(0x21,&rm);
  return (rm.eax&0xff)==0xff;
//  bwt_ioctl_read(fd);
//  return *((ushort *)(low_sock_mem+9));
}


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

bwt_out_socket::bwt_out_socket()
{
  pk_buffer_last=pk_buffer_ro=0;
  fd=bwt_socket();
}


int bwt_get_my_ip()
{
  int s;
  s = bwt_socket();
  if (bwt_ioctl_read(s)) return 0;
    close(s);
  return *(int*)(&low_sock_mem[3]);
}

int bwt_out_socket::try_connect(char *rhost, int port)
{
  int ip[4];
  if (sscanf(rhost, "%d.%d.%d.%d", ip, ip+1, ip+2, ip+3) != 4)
  {
    set_sock_err(SOCK_NAMELOOKUP_FAIL);
    sprintf(last_sock_err,"Connect failed, bad ip address '%s'",rhost);   
    return 0;
  } else
  {
    low_sock_mem[0]=6;
    low_sock_mem[1]=0x10;
    bwt_ioctl_write(fd, 2);

    low_sock_mem[0]=1;
    ulong mip=(ip[0]<<24)+(ip[1]<<16)+(ip[2]<<8)+(ip[3]);
    ulong net_order_ip=long_swap(mip);
    memcpy(low_sock_mem+1,&net_order_ip,4);

    ushort host_order_port=port;
    memcpy(&low_sock_mem[5],&host_order_port,2);

    bwt_ioctl_write(fd, 7);
    int last_stat=-1;
    low_sock_mem[0]=1;
    low_sock_mem[7]=0;
    do
    {
      bwt_ioctl_read(fd);
      if (low_sock_mem[0]==0 || low_sock_mem[0]==8)
      {
	current_sock_err=SOCK_CONNECT_FAIL;
	sprintf(last_sock_err,"Connect to %s failed",rhost);
	return 0;
      } else if (low_sock_mem[7]==5)
      {
	current_sock_err=SOCK_CONNECT_FAIL;
	sprintf(last_sock_err,"Connect to %s refused",rhost);
	return 0;
      } 
    } while (low_sock_mem[0]!=4);
  }
  return 1;
} 



static int bwt_bind(int fd, ushort port)
{
  low_sock_mem[0] = 0;
  memcpy(low_sock_mem+1, &port, 2);
  return bwt_ioctl_write(fd, 3);
}

static int bwt_listen(int fd)
{
  low_sock_mem[0] = 2;
  return bwt_ioctl_write(fd, 1);
}

bwt_in_socket::bwt_in_socket(int Port)
{
  port=Port;
  for (int i=0;i<5;i++)
  {
    listeners[i]=new bwt_out_socket(bwt_socket());
    bwt_bind(listeners[i]->get_fd(),port);
    bwt_listen(listeners[i]->get_fd());
  }
}


static int bwt_accept(int fd, ulong &ip, ulong &port)
{
  bwt_ioctl_read(fd);
  if (low_sock_mem[0] < 4) return -1;
  ip=big_long_to_local(*(int*)(&low_sock_mem[11]));
  port=(int)*(short*)(&low_sock_mem[15]);
  low_sock_mem[0]=6;
  low_sock_mem[1]=4;
  bwt_ioctl_write(fd, 2);
  return 0;
}

out_socket *bwt_in_socket::check_for_connect()
{
  for (int i=0;i<5;i++)
  {
    if (listeners[i]->ready_to_read())
    {
      ulong Ip,Port;

      if (bwt_accept(listeners[i]->get_fd(),Ip,Port)<0)
      {
	set_sock_err(SOCK_ACCEPT_FAIL);
	sprintf(last_sock_err,"Accept failed!");
	return NULL;
      } else
        dprintf("connection from IP %d.%d.%d.%d, on port %d\n",(Ip>>24)&0xff,(Ip>>16)&0xff,
		(Ip>>8)&0xff, Ip&0xff,Port);
      bwt_out_socket *ret=listeners[i];
      
      int nl=bwt_socket();
      listeners[i]=new bwt_out_socket(nl);
      bwt_bind(listeners[i]->get_fd(),port);
      bwt_listen(listeners[i]->get_fd());

      return ret;
    }
  }
  return NULL;

}

void bwt_close(int fd)
{
  memset(&rm,0,sizeof(rm));
  rm.eax=0x3e00;
  rm.ebx=fd;
  RM_intr(0x21,&rm);
}

bwt_out_socket::~bwt_out_socket()
{
  bwt_close(fd);
}

bwt_in_socket::~bwt_in_socket()
{
  for (int i=0;i<5;i++)
    delete listeners[i];
}



void bwt_out_socket::bwt_fill_buffer()
{
  while (!ready_to_read()) ;
  pk_buffer_last=bwt_read(get_fd(),pk_buffer,PK_BUFFER_SIZE);
  pk_buffer_ro=0;
}

int bwt_out_socket::get(packet &pk)
{ 
  pk.ro=pk.wo=2;

  ushort size=0;
  if (pk_buffer_last==pk_buffer_ro)
    bwt_fill_buffer();

  if (pk_buffer_last-pk_buffer_ro<2)  // make sure the two byte for the size are in the same packet
  {
    uchar two[2];
    two[0]=pk_buffer[pk_buffer_ro];
    bwt_fill_buffer();
    if (pk_buffer_last-pk_buffer_ro<2)  // if still not enough info, something is screwy
    {
      printf("Incomplete packet\n");
      return 0;
    }

    two[1]=pk_buffer[pk_buffer_ro];
    pk_buffer_ro++;
    size=lstl((*((ushort *)two)));
  } else
  {
    memcpy(&size,pk_buffer+pk_buffer_ro,2); pk_buffer_ro+=2;
    size=lstl(size);
  }
  pk.rend=size+2;
  pk.make_bigger(pk.rend);
  uchar *pk_off=pk.buf+2;
  int rs;
  while (size)
  {
    if (pk_buffer_last==pk_buffer_ro)
      bwt_fill_buffer();    
    if (pk_buffer_last-pk_buffer_ro>size)
      rs=size;
    else
      rs=pk_buffer_last-pk_buffer_ro;
    
    memcpy(pk_off,pk_buffer+pk_buffer_ro,rs);
    pk_buffer_ro+=rs;
    size-=rs;
    pk_off+=rs;         
  }
  
  return 1;
}

int bwt_out_socket::send(packet &pk)
{  
  int ret;
//  pk.make_bigger(pk.wo-2+2047-((pk.wo-2)%2048));
  ushort size=lstl(pk.wo-2);
  memcpy(pk.buf,&size,2);
  ret=bwt_write(fd,pk.buf,pk.wo)==pk.wo;
  return ret;
}
