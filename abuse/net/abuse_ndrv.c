#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>



#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>




#include <bstring.h>

#include <netdb.h>


#include "netface.hpp"      // net interface structures to the engine will use

// these are the names of the fifos to create in tmp
// that communitcate with the engine

#ifdef __sgi
#define BIGUNS             // big endian word ordering
#endif


// these macros swap the "endians" of a word to intel form... this should be done for anything sent
// across the net as the other computer might have a different endianess

#ifdef BIGUNS
#define swap_short(x) (((((unsigned short) (x)))<<8)|((((unsigned short) (x)))>>8))
#define swap_long(x) \
   ((( ((unsigned long)(x)) )>>24)|((( ((unsigned long)(x)) )&0x00ff0000)>>8)| \
   ((( ((unsigned long)(x)) )&0x0000ff00)<<8)|(( ((unsigned long)(x)) )<<24))
#else
#define swap_short(x) (x)
#define swap_long(x) (x)

#endif


#define DIN_NAME "/tmp/.abuse_ndrv_in"
#define DOUT_NAME "/tmp/.abuse_ndrv_out"


// the lock files is used in case a previous net driver is
// already running

#define DLOCK_NAME "/tmp/.abuse_ndrv_lock"


#define MAX_CLIENTS 32  // change this if you need more
#define MAX_JOINERS 32  // maximum clients that can join at the same time

#define DEFAULT_COMM_PORT 20202
#define DEFAULT_GAME_PORT 20203

#define uchar unsigned char

void net_watch();
void setup_ports(int comm_port, int game_port);

int no_security=0;
int driver_out_fd,driver_in_fd;
int shm_seg_id=-1;
void *shm_addr=(void *)-1;  // shmat returns -1 on failure
base_memory_struct *base;   // points to shm_addr
int comm_fd=-1,             // listening socket for commincation port
    game_fd=-1;
char *active_server=NULL;   // if -net option, fetch all files from "active server"
int stand_alone=0;          // if we are running this stand-alone (not interfacing with the engine)

fd_set master_set;
fd_set master_write_set;    // set a socket here if you detect a write_full

client_struct *client_array;  // points to an array of possible clients in shared memory
join_struct *join_array;      // points to an array of possible joining clients in shared memory
char default_fs_name[256];    // default file server name (set with parm -fs)


void clean_up()      // on exit unattach all shared memory links
{  
  fprintf(stderr,"clean up......\n");
  if (shm_seg_id!=-1)
    shmctl(shm_seg_id,IPC_RMID,NULL);

  if (shm_addr!=(void *)-1) 
  {
    shmdt((char *)shm_addr);
    shm_addr=(void *)-1;
  }

  unlink(DIN_NAME);
  unlink(DOUT_NAME);
  unlink(DLOCK_NAME);
}

#ifdef __sgi
void die(...)
#else
void die(int why)
#endif
{ 
  fprintf(stderr,"dieing\n");
  clean_up();
  exit(0);
}

void mdie(char *reason)
{
  fprintf(stderr,"net driver : %s\n",reason);
  die(0);
}

void comm_failed()  // general communication failure with engine
{
  fprintf(stderr,"net driver : Error occured while trying to communicate with the engine\n");
  clean_up();
  exit(0);
}


main(int argc, char **argv)
{
  int i;
  strcpy(default_fs_name,"");          // initially no default file server

  for (i=1;i<argc;i++)
    if (!strcmp(argv[i],"-bastard"))   // this bypasses filename security features
    {
      fprintf(stderr,"Warning : Security measures bypassed (-bastard)\n");
      no_security=1;
    }


  // make sure this program was run by the abuse engine
  if (argc<2 || strcmp(argv[1],"runme"))   
  { 
    stand_alone=1;
    fprintf(stderr,"%s is normally run by abuse, running stand-alone file server\n"
	           "Server will be killed by running abuse\n",argv[0]);
  }


  // see if we are already running, if so kill old driver
  FILE *fp=fopen(DLOCK_NAME,"rb");
  if (fp)
  {
    int pid;
    if (fscanf(fp,"%d",&pid)==1)
    {
      struct stat st;
      char proc_path[50];
      sprintf(proc_path,"/proc/%d",pid);
      if (!stat(proc_path,&st))
      {
	fprintf(stderr,"net driver : warning, %s already running, attempting to kill...\n",argv[0]);
	if (kill(pid,SIGKILL))
	{
	  fprintf(stderr,"net driver : unable to kill process %d, cannot run net-abuse\n",pid);
	  fclose(fp);
	  return 0;
	}
	fprintf(stderr,"killed process %d\n",pid);
      }
    }
    fclose(fp);
    unlink(DLOCK_NAME);    
  }


  unlink(DIN_NAME);    // remove any previous files if they exsists
  unlink(DOUT_NAME);


  if (!stand_alone)
  {
    if (mkfifo(DIN_NAME,S_IRWXU | S_IRWXG | S_IRWXO))
    { perror("Net driver : unable to make fifo in /tmp");
      return 0;
    }
    chmod(DIN_NAME,S_IRWXU | S_IRWXG | S_IRWXO);   // just to be sure umask doesn't screw us

    if (mkfifo(DOUT_NAME,S_IRWXU | S_IRWXG | S_IRWXO))
    { perror("Net driver : unable to make fifo in /tmp");
      return 0;
    }
    chmod(DOUT_NAME,S_IRWXU | S_IRWXG | S_IRWXO);

    int i,no_fork=0;
    for (i=1;i<argc;i++)
    if (!strcmp(argv[i],"-no_fork"))    // use this to debug easier
      no_fork=1;
    
    if (!no_fork)      // use this for debugging
    {
      int child_pid=fork();
      if (child_pid)
      {
	FILE *fp=fopen(DLOCK_NAME,"wb");
	if (!fp)
	{ 
	  fprintf(stderr,"Unable to open %s for writing, killing child\n",DLOCK_NAME);
	  kill(child_pid,SIGKILL);
	  return 0;
	}
	fprintf(fp,"%d\n",child_pid);
	fclose(fp);
	printf("%d\n",child_pid);         // tell parent the sound driver's process number
	return 0;                         // exit, child will continue
      }
    }  

    driver_out_fd=open(DOUT_NAME,O_RDWR);  // open the pipe
    if (driver_out_fd<0)
    { perror(DOUT_NAME); 
      exit(1);
    }

    driver_in_fd=open(DIN_NAME,O_RDWR);
    if (driver_in_fd<0)
    { perror(DIN_NAME); 
      exit(1);
    }
  } else driver_in_fd=driver_out_fd=-1;



  int catch_sigs[]={SIGHUP,SIGINT,SIGQUIT,SIGILL,SIGABRT,
		  SIGIOT,SIGFPE,SIGKILL,SIGUSR1,SIGSEGV,
		  SIGUSR2,SIGPIPE,SIGTERM,SIGCHLD,
		  SIGCONT,SIGSTOP,SIGTSTP,SIGTTIN,SIGTTOU,-1};

  for (i=0;catch_sigs[i]!=-1;i++)     // catch all signals in case we get
    signal(catch_sigs[i],die);            // interrupted before we remove shmid


  int alloc_size=sizeof(client_struct)*MAX_CLIENTS+
                 sizeof(join_struct)*MAX_JOINERS+
                 sizeof(base_memory_struct);

  shm_seg_id=shmget(IPC_PRIVATE,alloc_size,IPC_CREAT | 0777);
  if (shm_seg_id==-1)
    mdie("Unable to allocate shared memory");


  shm_addr=shmat(shm_seg_id,NULL,0);  // attach as read/write
  if (shm_addr==(void *)-1) 
    mdie("could not attach shm seg");

  base=(base_memory_struct *)shm_addr;
  base->active_clients=NULL;
  base->join_list=NULL;
  base->mem_lock=0;
  base->calc_crcs=0;

  if (!stand_alone)
  {
    // see if we can attach this memory with the abuse engine
    if (write(driver_out_fd,&shm_seg_id,sizeof(shm_seg_id))!=sizeof(shm_seg_id))
      comm_failed();

    // wait for engine to ack it has attached
    uchar ack=0;
    if (read(driver_in_fd,&ack,1)!=1 || ack!=1)
      comm_failed();
  }

  
  if (shmctl(shm_seg_id,IPC_RMID,NULL))  // remove the shm id
    mdie("could not remove shm id");

  shm_seg_id=-1;                      // mark as not allocated


  int comm_port=DEFAULT_COMM_PORT;
  int game_port=-1;
  for (i=1;i<argc-1;i++)
    if (!strcmp(argv[i],"-port"))
    {
      comm_port=atoi(argv[i+1]);
      if (game_port==-1)
        game_port=comm_port+1;
    }
    else if (!strcmp(argv[i],"-game_port"))
      game_port=atoi(argv[i+1]);
    else if (!strcmp(argv[i],"-net"))
      active_server=argv[i+1];

  if (game_port==-1) game_port=DEFAULT_GAME_PORT;

  setup_ports(comm_port,game_port);


  net_watch();                        // now go into infinite block/read/process cycle

  return 0;
}


void secure_filename(char *filename, char *mode)
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

int total_clients=0;
int total_joiners=0;

class client
{
public :
  int socket_fd;
  int client_id;       // index into client_struct
  int has_joined;
  client *next;
  client(int sock, int id, client *Next) { socket_fd=sock; client_id=id; next=Next; has_joined=0; }
} *first_client=NULL;


class nfs_client    // this is a client only read's a file
{
  public :
  int socket_fd;
  int file_fd;

  long size_to_read;  
  long size;
  nfs_client *next;
  nfs_client(int Socket_fd, int File_fd, nfs_client *Next) 
  { 
    socket_fd=Socket_fd; 
    file_fd=File_fd; 
    next=Next; 
    size_to_read=0;
    FD_SET(socket_fd,&master_set);
  }  
  int send_read();             // flushes as much of size_to_read as possible
  ~nfs_client() 
  { 
    if (socket_fd>=0) 
      close(socket_fd); 
    if (file_fd>=0)
      close(file_fd);
    FD_CLR(socket_fd,&master_set);
  }
} *first_nfs_client=NULL;


void setup_ports(int comm_port, int game_port)
{
  // the comminication socket is a STREAM
  comm_fd=socket(AF_INET,SOCK_STREAM,0);
  if (comm_fd==-1)
    mdie("net driver : could not create a socket.  (too many open files?)");


  sockaddr_in host;
  memset( (char*) &host,0, sizeof(host));
  host.sin_family = AF_INET;
  host.sin_port = htons(comm_port);
  host.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind(comm_fd, (struct sockaddr *) &host, sizeof(sockaddr_in))==-1)
  {
    fprintf(stderr,"net driver : could not bind socket to port %d",comm_port);
    die(0);
  }

  if (listen(comm_fd,5)==-1)
  {
    fprintf(stderr,"net driver : could not listen to socket on port %d\n",comm_port);    
    die(0);
  }


  game_fd=socket(AF_INET,SOCK_DGRAM,0);
  if (game_fd==-1)
    mdie("net driver : could not create a socket.  (too many open files?)");

  memset( (char*) &host,0, sizeof(host));
  host.sin_family = AF_INET;
  host.sin_port = htons(game_port);
  host.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind(game_fd, (struct sockaddr *) &host, sizeof(sockaddr_in))==-1)
  {
    fprintf(stderr,"net driver : could not bind socket to port %d\n",game_port);
    die(0);
  }
}


void delete_client(client *c)
{
  FD_CLR(c->socket_fd,&master_set);   // don't listen to this client anymore

  client_array[c->client_id].input_state=WAIT_DELETE;  // inform server to delete this client
}


int aquire_mem_lock()
{
  if (base->mem_lock==0 || base->mem_lock==1)
  {
    base->mem_lock=1;
    if (base->mem_lock==1)
      return 1;
  }
  sleep(0);   // probably just gonna loop until we get the lock so halt for next preocess
  return 0;
}

int local_address(char *server_name)    // returns 1 if server name is ourself
{
  struct hostent *hn=gethostbyname(server_name);    // first check to see if this address is 127.0.0.1
  if (!hn) return 0;                                // if bad server_name, return false
  char **ip_address=hn->h_addr_list;
  while (*ip_address) 
  {
    char *a=*ip_address;
    if (a[0]==127 && a[1]==0 && a[2]==0 && a[3]==1)
      return 1;
    ip_address++;
  }
  
  char my_name[100];                              // now check to see if this address is 'hostname'
  gethostname(my_name,100);
  struct hostent *l_hn=gethostbyname(my_name);  
  char **l_ip_address=l_hn->h_addr_list;

  while (*l_ip_address)  // local ip_address
  {
    char *a=*l_ip_address;         // scan through all local ip's
    ip_address=hn->h_addr_list;
    while (*ip_address)            // scan through all ip's for server_name
    {
      char *b=*ip_address;
      if (a[0]==b[0] && a[1]==b[1] && a[2]==b[2] && a[3]==b[3])    // check for match
        return 1;
      ip_address++;
    }
    l_ip_address++;
  }

  return 0;       // didn't match localhost nor hostname, must be somewhere else
}

int nfs_client::send_read()   // return 0 if failure on socket, not failure to read
{
  if (file_fd>=0 && socket_fd>=0)
  {
    // first make sure the socket isn't 'full'

    struct timeval tv={0,0};     // don't wait
    fd_set write_check;  
    FD_ZERO(&write_check);  
    FD_SET(socket_fd,&write_check);     
    select(FD_SETSIZE,NULL,&write_check,NULL,&tv);

    if (FD_ISSET(socket_fd,&write_check))            // ready to write?
    {
      char buf[READ_PACKET_SIZE];   
      short read_total;
      short actual;

      do
      {      
	read_total=size_to_read>(READ_PACKET_SIZE-2) ? (READ_PACKET_SIZE-2) : size_to_read;
	actual=read(file_fd,buf,read_total);
	actual=swap_short(actual);
	if (write(socket_fd,&actual,sizeof(actual))!=sizeof(actual))
	{
	  fprintf(stderr,"write failed\n");
	  return 0;
	}
	actual=swap_short(actual);

	int write_amount=write(socket_fd,buf,actual);
	if (write_amount!=actual) 
	{
	  fprintf(stderr,"write failed\n");
	  return 0;
	}

	size_to_read-=actual;

	FD_ZERO(&write_check);  
	FD_SET(socket_fd,&write_check);     
	select(FD_SETSIZE,NULL,&write_check,NULL,&tv);

	if (!FD_ISSET(socket_fd,&write_check))
	{
	  FD_SET(socket_fd,&master_write_set);      // socket is full, wait for it empty
	  FD_CLR(socket_fd,&master_set);            // don't check for reading or process commands till read is done
	  return 1;    // not ok to write anymore, try again latter
	}

      } while (size_to_read && actual==read_total);
      size_to_read=0;
      FD_CLR(socket_fd,&master_write_set);       // don't check this socket for write ok
      FD_SET(socket_fd,&master_set);             // check it for reading
      return 1;
    } else 
    {
      FD_SET(socket_fd,&master_write_set);      // socket is full, wait for it empty
      FD_CLR(socket_fd,&master_set);            // don't check for reading or process commands till read is done
      return 1;
    }
  }
  return 0;
}

class remote_file
{

  public :
  int socket_fd;
  void r_close(char *reason)
  { 
    if (reason)
       fprintf(stderr,"remote_file : %s\n",reason);
    if (socket_fd>=0) 
    {
      uchar cmd=NFCMD_CLOSE;
      write(socket_fd,&cmd,1);
      close(socket_fd); 
    }
    socket_fd=-1; 
  }

  long size;   // server tells us the size of the file when we open it
  remote_file *next; 
  remote_file(char *server, int port, char *filename, char *mode, remote_file *Next);

  int unbuffered_read(int out_fd, size_t count);
  int unbuffered_write(void *buf, size_t count) { return 0; } // not supported
  int unbuffered_tell();
  int unbuffered_seek(long offset);
  int file_size() { return size; }
  int open_failure() { return socket_fd<0; }
  ~remote_file() { r_close(NULL); }
  int fd() { return socket_fd; }
} ;

remote_file *remote_file_list;

remote_file::remote_file(char *server, int port, char *filename, char *mode, remote_file *Next)
{
  next=Next;
  socket_fd=socket(AF_INET,SOCK_STREAM,0);
  if (socket_fd<0) 
  {
    fprintf(stderr,"unable to open socket\n");
    return ;
  }

  hostent *hp=gethostbyname(server);
  if (!hp)
  { 
    fprintf(stderr,"unable to locate server named '%s'\n",server);
    close(socket_fd); socket_fd=-1; return ; 
  }
  

  sockaddr_in host;
  memset( (char*) &host,0, sizeof(host));
  host.sin_family = AF_INET;
  host.sin_port = htons(port);
  host.sin_addr.s_addr = htonl (INADDR_ANY);
  memcpy(&host.sin_addr,hp->h_addr,hp->h_length);
    
  if (connect(socket_fd, (struct sockaddr *) &host, sizeof(host))==-1)
  { 
    fprintf(stderr,"unable to connect\n");
    close(socket_fd);
    socket_fd=-1;
    return ;
  }

  uchar sizes[3]={CLIENT_NFS,strlen(filename)+1,strlen(mode)+1};
  if (write(socket_fd,sizes,3)!=3) { r_close("could not send open info"); return ; }
  if (write(socket_fd,filename,sizes[1])!=sizes[1]) { r_close("could not send filename"); return ; }
  if (write(socket_fd,mode,sizes[2])!=sizes[2]) { r_close("could not send mode"); return ; }

  long remote_file_fd;
  if (read(socket_fd,&remote_file_fd,sizeof(remote_file_fd))!=sizeof(remote_file_fd)) 
  { r_close("could not read remote fd"); return ; }   
  remote_file_fd=swap_long(remote_file_fd);
  if (remote_file_fd<0) { r_close("remote fd is bad"); return ; }

  if (read(socket_fd,&size,sizeof(size))!=sizeof(size)) { r_close("could not read remote filesize"); return ; } 
  size=swap_long(size);
}

int remote_file::unbuffered_read(int out_fd, size_t count)
{
  if (socket_fd>=0 && count)
  {
    uchar cmd=NFCMD_READ;
    if (write(socket_fd,&cmd,sizeof(cmd))!=sizeof(cmd)) { r_close("read : could not send command"); return 0; }

    long rsize=swap_long(count);
    if (write(socket_fd,&rsize,sizeof(rsize))!=sizeof(rsize)) { r_close("read : could not send size"); return 0; }

    long total_read=0,total;
    char buf[READ_PACKET_SIZE];
    ushort size;

    ushort packet_size;    
    do
    {
      if (read(socket_fd,&packet_size,sizeof(packet_size))!=sizeof(packet_size)) 
      {
	fprintf(stderr,"could not read packet size\n");
	return 0;
      }
      packet_size=swap_short(packet_size);

      ushort size_read=read(socket_fd,buf+2,packet_size);   

      if (size_read!=packet_size) 
      { 
	if (read(socket_fd,buf+2+size_read,packet_size-size_read)!=packet_size-size_read)
	{
	  fprintf(stderr,"incomplete packet\n"); 
	  return 0; 
	}
      }

      *((short *)buf)=packet_size;
      if (write(out_fd,buf,packet_size+2)!=packet_size+2) comm_failed();

      total_read+=packet_size;
      count-=packet_size;
    } while (packet_size==READ_PACKET_SIZE-2 && count);     
    return total_read;
  }
  return 0;
}

int remote_file::unbuffered_tell()   // ask server where the offset of the file pointer is
{
  if (socket_fd>=0)
  {
    uchar cmd=NFCMD_TELL;
    if (write(socket_fd,&cmd,sizeof(cmd))!=sizeof(cmd)) { r_close("tell : could not send command"); return 0; }

    long offset;
    if (read(socket_fd,&offset,sizeof(offset))!=sizeof(offset)) { r_close("tell : could not read offset"); return 0; }    
    return swap_long(offset);
  }    
  return 0;
}

int remote_file::unbuffered_seek(long offset)  // tell server to seek to a spot in a file
{
  if (socket_fd>=0)
  {
    uchar cmd=NFCMD_SEEK;
    if (write(socket_fd,&cmd,sizeof(cmd))!=sizeof(cmd)) { r_close("seek : could not send command"); return 0; }

    long off=swap_long(offset);
    if (write(socket_fd,&off,sizeof(off))!=sizeof(off)) { r_close("seek : could not send offset"); return 0; }

    if (read(socket_fd,&offset,sizeof(offset))!=sizeof(offset)) { r_close("seek : could not read offset"); return 0; }    
    return swap_long(offset);
  }    
  return 0;
}

int open_file(char *&filename, char *mode)
{
  if (filename[0]!='/' && filename[1]!='/' && default_fs_name[0])   // default file server?
  {
    char tmp_fn[500];  
    sprintf(tmp_fn,"//%s/%s",default_fs_name,filename);
    strcpy(filename,tmp_fn);
  }

  if (filename[0]=='/' && filename[1]=='/')   // passive server file refrence?
  {
    int use_port=DEFAULT_COMM_PORT;

    char server_name[100],*sn;
    filename+=2;
    sn=server_name;
    while (*filename && *filename!='/' && *filename!=':')
      *(sn++)=*(filename++);    
    *sn=0;

    if (*filename==':')    // a port is included in the filename
    {
      filename++;
      char port[100],*p; p=port;
      while (*filename && *filename!='/')
      { *(p++)=*(filename++); }
      *p=0;
      if (!sscanf(port,"%d",&use_port) || use_port<0 || use_port>0x7fff) return -1;
    }

    filename++;


    

    if (!local_address(server_name))
    {
      remote_file *rf=new remote_file(server_name,use_port,filename,mode,remote_file_list);
      if (rf->open_failure())
      {
        delete rf;
	return -1;
      }
      else 
      {
	remote_file_list=rf;
	return rf->socket_fd;
      }      
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

  int f=open(filename,flags,S_IRWXU | S_IRWXG | S_IRWXO);
  if (f>=0)
  { close(f);
    return -2;
  }
  
  return -1;
}

remote_file *find_rfile(int fd)
{
  remote_file *r=remote_file_list;
  for (;r && r->socket_fd!=fd;r=r->next)
  {
    if (r->socket_fd==-1)
    {
      fprintf(stderr,"bad sock\n");
    }
  }
  return r;
}

void unlink_remote_file(remote_file *rf)
{
  if (rf==remote_file_list)
    remote_file_list=rf->next;
  else
  {
    remote_file *last=remote_file_list;
    while (last->next && last->next!=rf) last=last->next;
    last->next=rf->next;
  }
}

void process_engine_command()
{
  uchar cmd;
  if (read(driver_in_fd,&cmd,1)!=1) { mdie("could not read command from engine"); }
  switch (cmd)
  {
    case EGCMD_DIE :
    {
      mdie("received die command");
    } break;

    case NFCMD_SET_FS :
    {
      uchar size;
      char sn[256];
      if (read(driver_in_fd,&size,1)!=1) mdie("could not read filename length");
      if (read(driver_in_fd,sn,size)!=size) mdie("could not read server name");
      strcpy(default_fs_name,sn);
      size=1;  // return success
      if (write(driver_out_fd,&size,1)!=1) mdie("could not send ok to engine");
    } break;    

    case NFCMD_OPEN :
    {
      uchar size[2];
      char filename[300],mode[20],*fn;
      fn=filename;
      if (read(driver_in_fd,size,2)!=2) mdie("could not read fd on open");
      if (read(driver_in_fd,filename,size[0])!=size[0]) mdie("incomplete filename");
      if (read(driver_in_fd,mode,size[1])!=size[1]) mdie("incomplete mode string");
      
      int fd=open_file(fn,mode);
      if (fd==-2)
      {
	uchar st[2];
	st[0]=NF_OPEN_LOCAL_FILE;
	st[1]=strlen(fn)+1;
	if (write(driver_out_fd,st,2)!=2) comm_failed();
	if (write(driver_out_fd,fn,st[1])!=st[1]) comm_failed();
      } else if (fd==-1)
      {
	uchar st=NF_OPEN_FAILED;
	if (write(driver_out_fd,&st,1)!=1) comm_failed(); 
      } else
      {
	uchar st=NF_OPEN_REMOTE_FILE;
	if (write(driver_out_fd,&st,1)!=1) comm_failed(); 	
	if (write(driver_out_fd,&fd,sizeof(fd))!=sizeof(fd)) comm_failed(); 	
      }

    } break;
    case NFCMD_CLOSE :
    case NFCMD_SIZE :
    case NFCMD_TELL :
    case NFCMD_SEEK :
    case NFCMD_READ :
    {
      int fd;
      if (read(driver_in_fd,&fd,sizeof(fd))!=sizeof(fd)) comm_failed();
      remote_file *rf=find_rfile(fd);
      if (!rf) 
	mdie("bad fd for engine command");

      switch (cmd)
      {
	case NFCMD_CLOSE : 
	{ 
	  unlink_remote_file(rf);
	  delete rf; 
	  uchar st=1;
	  if (write(driver_out_fd,&st,1)!=1) comm_failed(); 	
	} break;
	case NFCMD_SIZE  :
	{
	  if (write(driver_out_fd,&rf->size,sizeof(rf->size))!=sizeof(rf->size)) comm_failed(); 		  
	} break;
	case NFCMD_TELL :
	{
	  long offset=rf->unbuffered_tell();
	  if (write(driver_out_fd,&offset,sizeof(offset))!=sizeof(offset)) comm_failed();  
	} break;
	case NFCMD_SEEK :
	{
	  long offset;
	  if (read(driver_in_fd,&offset,sizeof(offset))!=sizeof(offset)) comm_failed();
	  offset=rf->unbuffered_seek(offset);
	  if (write(driver_out_fd,&offset,sizeof(offset))!=sizeof(offset)) comm_failed();  
	} break;
	case NFCMD_READ :
	{
	  long size;
	  if (read(driver_in_fd,&size,sizeof(size))!=sizeof(size)) comm_failed();
	  rf->unbuffered_read(driver_out_fd,size);
	} break;
      }
    } break;    
    default :
    { fprintf(stderr,"net driver : unknown net command %d\n",cmd); die(0); }
  } 
  
}

int process_nfs_command(nfs_client *c)
{
  char cmd;
  if (read(c->socket_fd,&cmd,1)!=1) return 0;
  switch (cmd)
  {
    case NFCMD_READ :
    {
      long size;
      if (read(c->socket_fd,&size,sizeof(size))!=sizeof(size)) return 0;
      size=swap_long(size);

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
      if (read(c->socket_fd,&offset,sizeof(offset))!=sizeof(offset)) return 0;
      offset=swap_long(offset);
      offset=lseek(c->file_fd,offset,0);
      offset=swap_long(offset);
      if (write(c->socket_fd,&offset,sizeof(offset))!=sizeof(offset)) return 0;
      return 1;
    } break;
    case NFCMD_TELL :
    {
      long offset=lseek(c->file_fd,0,SEEK_CUR);
      offset=swap_long(offset);
      if (write(c->socket_fd,&offset,sizeof(offset))!=sizeof(offset)) return 0;
      return 1;
    } break;
    
    default :
    { fprintf(stderr,"net driver : bad command from nfs client\n");
      return 0;
    }
  } 
}

void get_game_data(client *c)
{
  fprintf(stderr,"Get game data from client id %d\n",c->client_id);
}


void add_nfs_client(int fd)
{
  uchar size[2];
  char filename[300],mode[20],*mp;
  if (read(fd,size,2)!=2) { close(fd); return ; }
  if (read(fd,filename,size[0])!=size[0]) { close(fd); return ; }
  if (read(fd,mode,size[1])!=size[1]) { close(fd); return ; }
 
  fprintf(stderr,"remote request for %s ",filename);

  secure_filename(filename,mode);  // make sure this filename isn't a security risk
  if (filename[0]==0) { fprintf(stderr,"(denied)\n"); close(fd); return ; }

  mp=mode;
  int flags=0;

  while (*mp)
  {
    if (*mp=='w') flags|=O_CREAT|O_RDWR;
    else if (*mp=='r') flags|=O_RDONLY;
    mp++;
  }
      
  int f=open(filename,flags,S_IRWXU | S_IRWXG | S_IRWXO);
  
  if (f<0) 
  {
    fprintf(stderr,"(not found)\n");
    f=-1;  // make sure this is -1
  } else fprintf(stderr,"(granted, fd=%d)\n",f);

  long ret=swap_long(f);
  if (write(fd,&ret,sizeof(ret))!=sizeof(ret)) { close(fd); return ; }

  if (f<0)    // no file, sorry
    close(fd);
  else
  {
    long cur_pos=lseek(f,0,SEEK_CUR);
    long size=lseek(f,0,SEEK_END);
    lseek(f,cur_pos,SEEK_SET);
    size=swap_long(size);
    if (write(fd,&size,sizeof(size))!=sizeof(size)) {  close(f); close(fd); return ; }

    first_nfs_client=new nfs_client(fd,f,first_nfs_client);
    first_nfs_client->size=size;
  }
}

int isa_client(int client_id)    // sreach the list of active clients for this id and return 1 if found
{
  int i;
  client *c=first_client;
  for (;c;c=c->next)
    if (c->client_id==client_id) return 1;
  return 0;  
}

void insert_client(int client_id)  // add the client_id into the list of active clients
{
//  client_array[client_id]
//  if (!base->active_clients)
    
}

int add_game_client(int fd)     // returns false if could not join client
{  
  fprintf(stderr,"add game client\n");

  int i,first_free=-1;
  for (i=0;first_free==-1 && i<MAX_JOINERS;i++)
    if (join_array[i].client_id==-1)
      first_free=i;

  if (first_free==-1) return 0;

  int first_free_client=-1;
  for (i=0;first_free_client==-1 && i<MAX_CLIENTS;i++)
    if (!isa_client(i)) 
      first_free_client=i;

  if (first_free_client==-1) return 0;  


  join_array[first_free].next=base->join_list;
  base->join_list=&join_array[first_free];      
  join_array[first_free].client_id=first_free_client; 
  
  first_client=new client(fd,first_free_client,first_client);

}


void add_client()
{
  struct sockaddr from;
  int addr_len=sizeof(from);
  int new_fd=accept(comm_fd,&from,&addr_len);
  if (new_fd>=0)
  {
    char client_type;
    if (read(new_fd,&client_type,1)!=1 ||
	!(client_type==CLIENT_NFS ||
	client_type==CLIENT_ABUSE)) { close(new_fd); return ; }

    if (client_type==CLIENT_NFS)
      add_nfs_client(new_fd);
    else add_game_client(new_fd);    
  }
}

void net_watch()
{
  int i;
  client_array=(client_struct *)(base+1);
  join_array=(join_struct *) (client_array+MAX_CLIENTS);

  for (i=0;i<MAX_JOINERS;i++)
    join_array[i].client_id=-1;

  for (i=0;i<MAX_CLIENTS;i++)
    client_array[i].client_id=i;

  fd_set read_set,exception_set,write_set;
  
  FD_ZERO(&master_set);  
  FD_ZERO(&master_write_set);  
  FD_SET(comm_fd,&master_set);     // new incoming connections & nfs data
  FD_SET(game_fd,&master_set);     // game data

  if (!stand_alone)
  {
    FD_SET(driver_in_fd,&master_set);  // request from engine
    FD_SET(driver_out_fd,&master_set); // check for error on messages to engine
  }

  while (1)
  {
    // first lets set up a select that will cover all of our fd's so if nothing is happening
    // we can block and take no CPU time.

    memcpy(&read_set,&master_set,sizeof(master_set));
    memcpy(&exception_set,&master_set,sizeof(master_set));
    memcpy(&write_set,&master_write_set,sizeof(master_set));
    

    select(FD_SETSIZE,&read_set,&write_set,&exception_set,NULL);

    if (!stand_alone)
    {
      // see if we had any errors talking to the engine
      if (FD_ISSET(driver_in_fd,&exception_set) ||
	  FD_ISSET(driver_out_fd,&exception_set))
      comm_failed();
      
      // see if the engine has anything to say before getting to anyone else
      if (FD_ISSET(driver_in_fd,&read_set))
        process_engine_command();
    }

    if (aquire_mem_lock())  // we need to change shared memory, make sure server is not using it.
    {
      client *c;
      for (c=first_client;c;c=c->next)
      {
        if (FD_ISSET(c->socket_fd,&exception_set))  // error?
	  delete_client(c);
	else if (FD_ISSET(c->socket_fd,&read_set))  // in comming game data from client?
	  get_game_data(c);
      }

      if (FD_ISSET(comm_fd,&read_set))
        add_client();

      nfs_client *nc,*last=NULL;
      for (nc=first_nfs_client;nc;)      // check for nfs request
      {

	int ok=1;

	if (FD_ISSET(nc->socket_fd,&exception_set))
	{
	  ok=0;
	  fprintf(stderr,"Killing nfs client, socket went bad\n");
	} 
	else if (nc->size_to_read)
	{
	  if (FD_ISSET(nc->socket_fd,&write_set))
	    ok=nc->send_read();
	}	    
	else if (FD_ISSET(nc->socket_fd,&read_set))
	  ok=process_nfs_command(nc);    // if we couldn't process the packeted, delete the connection
	    
	if (ok)
	{
	  last=nc;
	  nc=nc->next;
	} else
	{
	  if (last) last->next=nc->next;
	  else first_nfs_client=nc->next;
	  nfs_client *c=nc;
	  nc=nc->next;
	  delete c;
	}
      }

      base->mem_lock=0;
    }       


  }
}



