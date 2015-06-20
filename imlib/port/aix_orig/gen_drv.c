
#define NUM_CHANNELS 8
#define STDOUT_FD driver_out_fd
#define STDIN_FD  driver_in_fd



#define DOUT_NAME  "/tmp/sfxdrv.signal"
#define DIN_NAME "/tmp/sfxdrv.command"


enum { SFXCMD_QUIT,
       SFXCMD_REGISTER,
       SFXCMD_UNREGISTER,
       SFXCMD_PLAY
     };

typedef struct sfx_handle_s sfx_handle;

struct sfx_handle_s
{
  int          shm_id; 
  void         *shm_data_pointer;
  sfx_handle   *next;  
  long         size;
  int          use_count;
};

int driver_out_fd,driver_in_fd;
sfx_handle *sfx_list=NULL;



#define TOTAL_SIGS 29

int sigs[TOTAL_SIGS]={SIGHUP,SIGINT,SIGQUIT,SIGILL,SIGTRAP,
		      SIGABRT,SIGIOT,SIGBUS,SIGFPE,SIGKILL,
		      SIGUSR1,SIGSEGV,SIGUSR2,SIGPIPE,SIGALRM,
		      SIGTERM,SIGCHLD,SIGCONT,SIGSTOP,
		      SIGTSTP,SIGTTIN,SIGTTOU,SIGIO,
		      SIGURG,SIGXCPU,SIGXFSZ,SIGVTALRM,SIGPROF,
		      SIGWINCH};
 
struct channel
{
  unsigned char *data;         // next data to be qued
  long left;                   // how much is left to play?  
  unsigned char volume;        // indexed into volume table
  unsigned long add_time;      // time at which the channel started playing
  sfx_handle *snd;             // pointer to actual sound, so delete can stop us if need be
} channels[NUM_CHANNELS];

#define uchar unsigned char
short buf[BUF_SIZE];   // mixing buffer

// add a new sound effect into the channel list, if no free channel
// oldest gets replaced with new sound
void play(sfx_handle *snd, unsigned char volume)
{
  int i,free_channel=-1;
  unsigned long oldest_channel=0,
                oldest_channel_time=10000,
                newest_channel_time=0;

  for (i=0;i<NUM_CHANNELS;i++)
  {
    if (channels[i].data)
    { 
      int my_time=channels[i].add_time;
      if (my_time<oldest_channel_time)
      {
	oldest_channel=i;
	oldest_channel_time=my_time;
      }
      if (my_time>newest_channel_time)
	newest_channel_time=my_time;
    } else free_channel=i;
  }
  
  if (free_channel==-1)
    free_channel=oldest_channel;

  channels[free_channel].snd=snd;
  channels[free_channel].add_time=newest_channel_time+1;
  channels[free_channel].data=(uchar *)snd->shm_data_pointer;
  channels[free_channel].left=snd->size;
  channels[free_channel].volume=volume*32/128;
//  fprintf(stderr, "vol=%d\n", channels[free_channel].volume);
}


int output_sounds()  // return 0 if no sounds to ouput
{
  unsigned char *s;
  int i,j;
  unsigned char *data;
  
  int run_size;
  memset(buf,0,sizeof buf);
  for (j=0;j<NUM_CHANNELS;j++)
  {
    data = channels[j].data;
    if (data)
    {
      if (channels[j].left<=BUF_SIZE)    // handle imminent channel death
	  {
        run_size=channels[j].left;
		channels[j].data = NULL;
	  }
      else
	  {
		run_size=BUF_SIZE;
		channels[j].data += BUF_SIZE;
	  }
	  channels[j].left -= BUF_SIZE;
      
      // add the chanels together into an int
	  // reserve the clip and scale to proper 16-bit for the output step
      for (i=0;i<run_size;i++)
		buf[i] += ((short)data[i] - 128) * channels[j].volume;

    }
  }

  output_samples(buf);

  return 1;    // always say we have something, we will do blanks if nothing else

}



#ifdef __sgi
void clean_up(...)
#else
void clean_up(int why)      // on exit unattach all shared memory links
#endif
{  
  sfx_handle *last;
  while (sfx_list)
  {
    shmdt((char *)sfx_list->shm_data_pointer);
    if (shmctl(sfx_list->shm_id,IPC_RMID,NULL)!=0)
      printf("shmctl failed, why?\n");
    last=sfx_list;
    sfx_list=sfx_list->next;
    free(last);
  }
  sound_uninit();
  unlink(DIN_NAME);
  unlink(DOUT_NAME);
}

void die()
{ clean_up(0);
  exit(0);
}


static int sound_fd_ready_to_read(int fd)
{
  struct timeval tv={0,0};
  fd_set kbd_set,ex_set;
  FD_ZERO(&kbd_set);
  FD_SET(fd,&kbd_set);
  memcpy((void *)&ex_set,(void *)&kbd_set,sizeof(ex_set));
  select(FD_SETSIZE,&kbd_set,NULL,&ex_set,&tv);                // check for exception
  if (FD_ISSET(fd,&ex_set))
    die();
  return (FD_ISSET(fd,&kbd_set));
}


void sound_watch()
{

  sfx_handle *sfx;

  while (1)
  {
    while (sound_fd_ready_to_read(STDIN_FD))
    {
      uchar cmd;
			int rc;
      if ((rc=read(STDIN_FD,&cmd,1))!=1)
      { die(); }
		  fprintf(stderr,"recv'd %d bytes\n", rc);
      switch (cmd)
      {
	case SFXCMD_REGISTER :
	{
	  int shm_id;
	  long size;
	  uchar return_code;
	  if (read(STDIN_FD,&shm_id,sizeof(shm_id))!=sizeof(shm_id))
	  { fprintf(stderr,"sndrv er1\n"); die(); }
	  if (read(STDIN_FD,&size,sizeof(size))!=sizeof(size))
	  { fprintf(stderr,"sndrv er2\n"); die(); }

	  sfx=malloc(sizeof(sfx_handle));
	  sfx->shm_id=shm_id;
	  sfx->shm_data_pointer=shmat(shm_id,NULL,0);
	  if (!sfx->shm_data_pointer)
	  { fprintf(stderr,"Sound driver : unable to attach shared memory segment\n"); die(); }
	  sfx->size=size;
	  sfx->use_count=0;
	  sfx->next=sfx_list;
	  sfx_list=sfx;
	  	
		fprintf(stderr,"registered %d %d\n", shm_id, size);

	  cmd=1;
	  if (write(STDOUT_FD,&cmd,sizeof(cmd))!=sizeof(cmd))
	  { fprintf(stderr,"sndrv er3\n"); die(); }

	} break;
	case SFXCMD_UNREGISTER :
	{
	  int shm_id,i;
	  sfx_handle *f,*find=NULL,*last_find=NULL;
	  if (read(STDIN_FD,&shm_id,sizeof(shm_id))!=sizeof(shm_id))
	  { die(); }
	  for (f=sfx_list;f && !find;f=f->next)   // see if we can find the shm id in list
	  {
	    if (f->shm_id==shm_id)
	      find=f;
	    last_find=find;         // svae last link so we can remove from the list
	  }
	  
	  if (find)
	  {
	    // see if there are any sound channels playing this sound, if so stop them because
	    // memory is fixing to be deleted
	    for (i=0;i<NUM_CHANNELS;i++)
	    {
	      if (channels[i].snd==find)
	        channels[i].data=NULL;
	    }
	    // now free the memory
	    shmdt((char *)find->shm_data_pointer);          // detach and remove shm
	    if (last_find) 
	      last_find->next=find->next;           // unlink from sound list
	    else
	      sfx_list=sfx_list->next;
	  } else { fprintf(stderr,"Attempt to remove unknown sound effect\n"); die(); }
	} break;

	case SFXCMD_PLAY :
	{
	  int shm_id,i,volume;
	  sfx_handle *f;
	  if (read(STDIN_FD,&shm_id,sizeof(shm_id))!=sizeof(shm_id))
	    die();
	  if (read(STDIN_FD,&volume,sizeof(volume))!=sizeof(volume))
	    die();
	  for (f=sfx_list;f && f->shm_id!=shm_id;f=f->next);
	  if (f)
	    play(f,volume);
	  else fprintf(stderr,"sound driver : bad id to play\n");
//		fprintf(stderr, "play %d %d \n", shm_id, volume);
	} break;
	default :     // die on unknown or DIE command
	{ die(); }
      } 
    }
    output_sounds();
  }
}


main()
{

	int i;
	uchar success;
/*
	int chd;
  chd=fork();
  if (chd)
  {
    printf("%d\n",chd);         // tell parent the sound driver's process number
    return 0;
  }
*/
	printf("%d\n",getpid());         // tell parent the sound driver's process number
	fclose(stdout);

  success=sound_init();    // initailize sound and send status to who ever ran us.
  if (!success)
  {
    printf("-1");
    return 0;
  }

  unlink(DIN_NAME);
  unlink(DOUT_NAME);

//  int old_mask=umask(S_IRWXU | S_IRWXG | S_IRWXO);
  if (mkfifo(DIN_NAME,S_IRWXU | S_IRWXG | S_IRWXO))
  { perror("Sound driver : unable to make fifo in /tmp");
    return 0;
  }
  chmod(DIN_NAME,S_IRWXU | S_IRWXG | S_IRWXO);

  if (mkfifo(DOUT_NAME,S_IRWXU | S_IRWXG | S_IRWXO))
  { perror("Sound driver : unable to make fifo in /tmp");
    return 0;
  }

  chmod(DOUT_NAME,S_IRWXU | S_IRWXG | S_IRWXO);
  //umask(old_mask);

  driver_out_fd=open(DOUT_NAME,O_RDWR);
  if (driver_out_fd<0)
  { perror(DOUT_NAME); 
    exit(1);
  }

  driver_in_fd=open(DIN_NAME,O_RDWR);
  if (driver_in_fd<0)
  { perror(DIN_NAME); 
    exit(1);
  }

  for (i=0;i<TOTAL_SIGS;i++)
    signal(sigs[i],clean_up);

  sound_watch();

  return 0;
}



