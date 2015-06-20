/*      Key driver, by Jonathan Clark 
          (portions from showkey.c)
*/

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <fcntl.h>
#include <sys/stat.h>



unsigned char keyboard_buf[16];

#include <linux/keyboard.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>

int fd,my_console=-1;
struct termios old_term;

void clean_up(int close_fd) {
	ioctl(fd, KDSKBMODE, K_XLATE);

	tcsetattr(fd, TCSAFLUSH, &old_term);
        if (close_fd)
	  {
	    close(fd);
	    unlink("/tmp/jckey-driver");
	    exit(0);
	  }
}

void go_raw(int open_fd)
{
	struct termios new_term;
        struct vt_stat v;
        if (open_fd)
        {
  		if ((fd = open("/dev/console", O_RDONLY)) < 0) {
			perror("/dev/console");
			exit(1);
		}
	}
        if (my_console==-1)
        {
          ioctl(fd,VT_GETSTATE,&v);  /* see which console we are using */ 
          my_console=v.v_active;
        }
        ioctl(fd,VT_WAITACTIVE,my_console);  /* wait for our console to come back */

	tcgetattr(fd, &new_term);
	tcgetattr(fd, &old_term);

	new_term.c_lflag = new_term.c_lflag & ~ (ICANON | ECHO | ISIG );
	new_term.c_cc[VMIN] = 1; //sizeof(keyboard_buf);
	new_term.c_cc[VTIME] = 0;	/* 0.1 sec intercharacter timeout */

	tcsetattr(fd, TCSAFLUSH, &new_term);
	if (ioctl(fd, KDSKBMODE,
		K_RAW)) {
		perror("KDSKBMODE");
		exit(1);
	}
        system("stty raw </dev/console >/dev/console");

}

void die(int x) {
        fprintf(stderr,"die %d\n",x);
   
	clean_up(1);
	exit(1);
}

int key_watch(int write_fd) 
{
        unsigned char map[128];

	int i, n,lalt,ralt,switch_to,lctrl,rctrl,up,quit;


	/*
		if we receive a signal, we want to exit nicely, in
		order not to leave the keyboard in an unusable mode
	*/
	signal(SIGHUP, die);
	signal(SIGINT, die);
	signal(SIGQUIT, die);
	signal(SIGILL, die);
	signal(SIGTRAP, die);
	signal(SIGABRT, die);
	signal(SIGIOT, die);
	signal(SIGFPE, die);
	signal(SIGKILL, die);
	signal(SIGUSR1, die);
	signal(SIGSEGV, die);
	signal(SIGUSR2, die);
	signal(SIGPIPE, die);
	signal(SIGTERM, die);
	signal(SIGSTKFLT, die);
	/*signal(SIGCHLD, die);*/
	signal(SIGCONT, die);
	signal(SIGSTOP, die);
	signal(SIGTSTP, die);
	signal(SIGTTIN, die);
	signal(SIGTTOU, die);

        go_raw(1);
        quit=lalt=ralt=lctrl=rctrl=0;
        memset(map,0,128);
	while (!quit) {
		n = read(fd, keyboard_buf, sizeof(keyboard_buf));
		for (i = 0; i < n; i++) 
                { up=!(keyboard_buf[i]&0x80);
                  if ((keyboard_buf[i]&0x7f)==56) lalt=up; 
                  if ((keyboard_buf[i]&0x7f)==96) ralt=up;
                  if ((keyboard_buf[i]&0x7f)==29) lctrl=up;
                  if (up || up!=map[(keyboard_buf[i]&0x7f)])
                  {
                    map[(keyboard_buf[i]&0x7f)]=up;
                    if (!write(write_fd,&keyboard_buf[i],1))
		    {
		      fprintf(stderr,"keydrv : unable to write to parent (cleaning up)\n");
		      clean_up(1);
		      exit(0);
		    }
                  }
                }
		for (i = 0; i < n; i++) 
                {
                  if ((lalt || ralt) && (keyboard_buf[i]&0x7f)>=59 &&
                      (keyboard_buf[i]&0x7f)<=68)
                  { 

                    switch_to=(keyboard_buf[i]&0x7f)-58;
                    clean_up(0);
		    
                    ioctl(fd,VT_ACTIVATE,switch_to);
                    go_raw(0); 
                  }
                  if ((lctrl || rctrl) && (keyboard_buf[i]&0x7f)==46)
                  {
                    fprintf(stderr,"^C\n"); fflush(stderr);
                    quit=1;
                  }
                }

	}
	clean_up(1);
	exit(0);
}


main()
{
  int f,chd;
  unlink("/tmp/jckey-driver");
  if (mkfifo("/tmp/jckey-driver",S_IRWXU))
  { perror("Key driver : unable to make fifo /tmp/jckey-driver");
    exit(1);
  }
  chd=fork();
  if (chd) 
  { printf("%d\n",chd);
    return 0;
  }
  f=open("/tmp/jckey-driver",O_WRONLY);
  if (f<0)
  { perror("/tmp/jckey-driver"); 
    exit(1);
  }
  key_watch(f);
  close(f);
  unlink("/tmp/jckey-driver");
}
