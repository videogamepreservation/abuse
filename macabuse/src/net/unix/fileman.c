#include "macs.hpp"

#include "fileman.hpp"
#include "netface.hpp"
#include "ghandler.hpp"
#include "dprint.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

extern net_protocol *prot;


#if !defined( __WATCOMC__ ) && !defined( __MAC__ )
#include <sys/stat.h>
#endif

#ifdef __MAC__
extern char *macify_name(char *s);
#endif

file_manager *fman=NULL;

file_manager::file_manager(int argc, char **argv, net_protocol *proto) : proto(proto)
{  
  default_fs=NULL;
  no_security=0;
  nfs_list=NULL;

  int i;
  for (i=1;i<argc;i++)
    if (!strcmp(argv[i],"-bastard"))   // this bypasses filename security features
    {
      dprintf("Warning : Security measures bypassed (-bastard)\n");
      no_security=1;
    }
}



void file_manager::process_net()
{
  nfs_client *nc,*last=NULL;
  for (nc=nfs_list;nc;)      // check for nfs request
  {

    int ok=1;

    if (nc->sock->error())
    {
      ok=0;
      //dprintf("Killing nfs client, socket went bad\n");
    } 
    else if (nc->size_to_read && nc->sock->ready_to_write())
      ok=nc->send_read();
    else if (nc->sock->ready_to_read())
      ok=process_nfs_command(nc);    // if we couldn't process the packeted, delete the connection
    
    if (ok)
    {
      last=nc;
      nc=nc->next;
    } else
    {
      if (last) last->next=nc->next;
      else nfs_list=nc->next;
      nfs_client *c=nc;
      nc=nc->next;
      delete c;
    }
  }
}


int file_manager::process_nfs_command(nfs_client *c)
{
  char cmd;
  if (c->sock->read(&cmd,1)!=1) return 0;
  switch (cmd)
  {
    case NFCMD_READ :
    {
      long size;
      if (c->sock->read(&size,sizeof(size))!=sizeof(size)) return 0;
      size=lltl(size);

      c->size_to_read=size;
      return c->send_read();
    } break;
    case NFCMD_CLOSE :
    {
      return 0;
    } break;
    case NFCMD_SEEK :
    {
      long offset;
      if (c->sock->read(&offset,sizeof(offset))!=sizeof(offset)) return 0;
      offset=lltl(offset);
      offset=lseek(c->file_fd,offset,0);
      offset=lltl(offset);
      if (c->sock->write(&offset,sizeof(offset))!=sizeof(offset)) return 0;
      return 1;
    } break;
    case NFCMD_TELL :
    {
      long offset=lseek(c->file_fd,0,SEEK_CUR);
      offset=lltl(offset);
      if (c->sock->write(&offset,sizeof(offset))!=sizeof(offset)) return 0;
      return 1;
    } break;
    
    default :
    { 
    	dprintf("net driver : bad command from nfs client\n");
      return 0;
    }
  } 
}

int file_manager::nfs_client::send_read()   // return 0 if failure on socket, not failure to read
{
  if (file_fd>=0 && sock)
  {
    // first make sure the socket isn't 'full'
    if (sock->ready_to_write())
    {
      char buf[READ_PACKET_SIZE];   
      short read_total;
      short actual;

      do
      {      
				read_total=size_to_read>(READ_PACKET_SIZE-2) ? (READ_PACKET_SIZE-2) : size_to_read;
			
				actual=read(file_fd,buf+2,read_total);
				*((ushort *)buf)=lstl(actual);
			
				int write_amount=sock->write(buf,actual+2);
				if (write_amount!=actual+2) 
				{ 
				  dprintf("write failed\n");
				  return 0;
				}
			
				size_to_read-=actual;
			
				if (!sock->ready_to_write())
				{
				  sock->read_unselectable();
				  sock->write_selectable();
				  return 1;    // not ok to write anymore, try again latter
				}

      } while (size_to_read && actual==read_total);

      sock->read_selectable();
      sock->write_unselectable();

      size_to_read=0;
      return 1;
    } else 
    {
      sock->read_unselectable();
      sock->write_selectable();
      return 1;
    }
  }
  return 0;
}


void file_manager::secure_filename(char *filename, char *mode)
{
  if (!no_security)
  {    
    if (filename[0]=='/') { filename[0]=0; return ; }
    int level=0;
    char *f=filename;
    while (*f)
    {
      if (*f=='/') { f++; level++; }
      else if (*f=='.' && f[1]=='.')
      {
	if (f[3]=='.') while (*f!='.') f++;
	else
	{
	  f+=2;
	  level--;
	}
      } else f++; 
      
    }
    if (level<0)
      filename[0]=0;
  }
}




file_manager::nfs_client::nfs_client(net_socket *sock, int file_fd, nfs_client *next) : 
  sock(sock),file_fd(file_fd),next(next),size_to_read(0)
{ 
  sock->read_selectable();
}  


file_manager::nfs_client::~nfs_client() 
{ 
  delete sock;
  if (file_fd>=0)
    close(file_fd);
}


void file_manager::add_nfs_client(net_socket *sock)
{
  uchar size[2];
  char filename[300],mode[20],*mp;
  if (sock->read(size,2)!=2) { delete sock; return ; }
  if (sock->read(filename,size[0])!=size[0]) { delete sock; return ; }
  if (sock->read(mode,size[1])!=size[1]) { delete sock; return ; }
 

  secure_filename(filename,mode);  // make sure this filename isn't a security risk
  if (filename[0]==0) { dprintf("(denied)\n"); delete sock; return ; }

  mp=mode;
  int flags=0;

#ifdef __WATCOMC__
    flags|=O_BINARY;
#endif

  while (*mp)
  {
    if (*mp=='w') flags|=O_CREAT|O_RDWR;
    else if (*mp=='r') flags|=O_RDONLY;
    mp++;
  }

#ifdef __MAC__
  int f=open(macify_name(filename),flags);
#else      
  int f=open(filename,flags,S_IRWXU | S_IRWXG | S_IRWXO);
#endif

  FILE *fp=fopen("open.log","ab"); 
  fprintf(fp,"open file %s, fd=%d\n",filename,f);
  fclose(fp);
  
  if (f<0) 
    f=-1;  // make sure this is -1


  long ret=lltl(f);
  if (sock->write(&ret,sizeof(ret))!=sizeof(ret)) { delete sock; return ; }

  if (f<0)    // no file, sorry
    delete sock;
  else
  {
    long cur_pos=lseek(f,0,SEEK_CUR);
    long size=lseek(f,0,SEEK_END);
    lseek(f,cur_pos,SEEK_SET);
    size=lltl(size);
    if (sock->write(&size,sizeof(size))!=sizeof(size)) {  close(f); delete sock; sock=NULL; return ; }

    nfs_list=new nfs_client(sock,f,nfs_list);
    nfs_list->size=size;
  }
}



void file_manager::remote_file::r_close(char *reason) 
{ 
  if (reason) dprintf("remote_file : %s\n",reason);

  if (sock)
  {
    delete sock;
    sock=NULL;
  }

}

file_manager::remote_file::remote_file(net_socket *sock, char *filename, char *mode, remote_file *Next) : sock(sock)
{
  next=Next;
  open_local=0;

  uchar sizes[3]={CLIENT_NFS,strlen(filename)+1,strlen(mode)+1};
  if (sock->write(sizes,3)!=3) { r_close("could not send open info"); return ; }
  if (sock->write(filename,sizes[1])!=sizes[1]) { r_close("could not send filename"); return ; }
  if (sock->write(mode,sizes[2])!=sizes[2]) { r_close("could not send mode"); return ; }

  long remote_file_fd;
  if (sock->read(&remote_file_fd,sizeof(remote_file_fd))!=sizeof(remote_file_fd)) 
  { r_close("could not read remote fd"); return ; }   
  remote_file_fd=lltl(remote_file_fd);
  if (remote_file_fd<0) { r_close("remote fd is bad"); return ; }

  if (sock->read(&size,sizeof(size))!=sizeof(size)) { r_close("could not read remote filesize"); return ; } 

  size=lltl(size);
}

int file_manager::remote_file::unbuffered_read(void *buffer, size_t count)
{
  if (sock && count)
  {
    uchar cmd=NFCMD_READ;
    if (sock->write(&cmd,sizeof(cmd))!=sizeof(cmd)) { r_close("read : could not send command"); return 0; }

    long rsize=lltl(count);
    if (sock->write(&rsize,sizeof(rsize))!=sizeof(rsize)) { r_close("read : could not send size"); return 0; }

    long total_read=0,total;
    char buf[READ_PACKET_SIZE];
    ushort size;

    ushort packet_size;    
    do
    {
      if (sock->read(&packet_size,sizeof(packet_size))!=sizeof(packet_size)) 
      {
				dprintf("could not read packet size\n");
				return 0;
      }

      packet_size=lstl(packet_size);

      ushort size_read=sock->read(buf,packet_size); 

      if (size_read!=packet_size) 
      { 
				if (sock->read(buf+2+size_read,packet_size-size_read)!=packet_size-size_read)
				{
				  dprintf("incomplete packet\n"); 
				  return 0; 
				}
      }

      memcpy(buffer,buf,packet_size);
      buffer=(void *)(((char *)buffer)+packet_size);

      total_read+=packet_size;
      count-=packet_size;
    } while (packet_size==READ_PACKET_SIZE-2 && count);     
    return total_read;
  }
  return 0;
}

long file_manager::remote_file::unbuffered_tell()   // ask server where the offset of the file pointer is
{
  if (sock)
  {
    uchar cmd=NFCMD_TELL;
    if (sock->write(&cmd,sizeof(cmd))!=sizeof(cmd)) { r_close("tell : could not send command"); return 0; }

    long offset;
    if (sock->read(&offset,sizeof(offset))!=sizeof(offset)) { r_close("tell : could not read offset"); return 0; }    
    return lltl(offset);
  }    
  return 0;
}

long file_manager::remote_file::unbuffered_seek(long offset)  // tell server to seek to a spot in a file
{
  if (sock)
  {
    uchar cmd=NFCMD_SEEK;
    if (sock->write(&cmd,sizeof(cmd))!=sizeof(cmd)) { r_close("seek : could not send command"); return 0; }

    long off=lltl(offset);
    if (sock->write(&off,sizeof(off))!=sizeof(off)) { r_close("seek : could not send offset"); return 0; }

    if (sock->read(&offset,sizeof(offset))!=sizeof(offset)) { r_close("seek : could not read offset"); return 0; }    
    return lltl(offset);
  }    
  return 0;
}


file_manager::remote_file::~remote_file()
{ r_close(NULL); }

int file_manager::rf_open_file(char *&filename, char *mode)
{
  net_address *fs_server_addr=NULL;

  if (filename[0]=='/' && filename[1]=='/')   // passive server file reference?
  {
    filename+=2;

    fs_server_addr=prot->get_node_address(filename,DEFAULT_COMM_PORT,0);
    if (!fs_server_addr)
    {
      dprintf("couldn not get address for %s\n",filename);
      return -1;
    }
  } else if (default_fs)
    fs_server_addr=default_fs->copy();
  
  if (fs_server_addr)
  {
    net_socket *sock=proto->connect_to_server(fs_server_addr,net_socket::SOCKET_SECURE);
    delete fs_server_addr;

    if (!sock)
    { 
      dprintf("unable to connect\n");
      return -1;
    }

    remote_file *rf=new remote_file(sock,filename,mode,remote_list);
    if (rf->open_failure())
    {
      delete rf;
      return -1;
    }
    else 
    {
      remote_list=rf;
      return rf->sock->get_fd();
    }      
  }

  secure_filename(filename,mode);
  if (filename[0]==0) return -1;

  int flags=0;
  while (*mode)
  {
    if (*mode=='w') flags|=O_CREAT|O_RDWR;
    else if (*mode=='r') flags|=O_RDONLY;
    mode++;
  }

#ifdef __MAC__
  int f=open(macify_name(filename),flags);
#else
  int f=open(filename,flags,S_IRWXU | S_IRWXG | S_IRWXO);
#endif
  if (f>=0)
  { close(f);
    return -2;
  }
  
  return -1;
}


file_manager::remote_file *file_manager::find_rf(int fd)
{
  remote_file *r=remote_list;
  for (;r && r->sock->get_fd()!=fd;r=r->next)
  {
    if (r->sock->get_fd()==-1)
    {
      dprintf("bad sock\n");
    }
  }
  if (!r) { dprintf("Bad fd for remote file %d\n",fd); }
  return r;
}


long file_manager::rf_tell(int fd)
{
  remote_file *rf=find_rf(fd);
  if (rf) return rf->unbuffered_tell();
  else return 0;
}

long file_manager::rf_seek(int fd, long offset)
{
  remote_file *rf=find_rf(fd);
  if (rf) return rf->unbuffered_seek(offset);
  else return 0;
}

int file_manager::rf_read(int fd, void *buffer, size_t count)
{
  remote_file *rf=find_rf(fd);
  if (rf) return rf->unbuffered_read(buffer,count);
  else return 0;
}

int file_manager::rf_close(int fd)
{
  remote_file *rf=remote_list,*last=NULL;
  while (rf && rf->sock->get_fd()!=fd) rf=rf->next;
  if (rf)
  {
    if (last) last->next=rf->next;
    else remote_list=rf->next;
    delete rf;
    return 1;
  } else 
  {
    dprintf("Bad fd for remote file %d\n",fd); 
    return 0;
  }  
}

long file_manager::rf_file_size(int fd)
{
  remote_file *rf=find_rf(fd);
  if (rf) return rf->file_size();
  else return 0;
}
