#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sock.hpp"
//#include "tcpip.hpp"
#include "atalk.hpp"

atalk_protocol atalk;

char delim[]=" \n";
char spinchar[] = "\\|/-";

net_protocol *prot = net_protocol::first;
net_socket::socket_type socktype;
net_address *addr;
int port = 0x9091;
int notify_port = 0x9099;

void spin()
//{{{
{
	return ;
	
  static int pos = 0;
  
  printf("\r%c",spinchar[pos]);
  pos = (pos+1)&3;
}
//}}}

#define MAX_CLIENTS 2

void udp_server()
//{{{
{
  net_socket *srv;
  int cont;

  // Create listen socket to accept client connections
  printf("Starting server listen socket on port %d.\n",port);
  srv = prot->create_listen_socket(port, socktype);
  srv->read_selectable();
  prot->start_notify(notify_port,"Test server",11);
  printf("Waiting for connections\n");

  cont = 1;
  while (cont) {
    spin();
    // wait for next network event
    if (prot->select(0)) {
      if (srv->ready_to_read()) {
        // server socket has data
				char buf[513];
				int len;
        net_address *addr;
        char name[80];

				len = srv->read(buf,512,&addr);
				if (addr)
					addr->store_string(name,80);
				else
					name[0] = 0;

				if (len>0) {
					buf[len] = 0;
					printf("Server: Got [%s] from [%s]\n",buf,name);
				}

				if (addr && len>0) {
					if (strcmp(buf,"die")==0)
						cont = 0;

					srv->write(buf,len,addr);
					printf("Server: Echoing [%s] to [%s]\n",buf,name);
				}
      }
    }
  }
  delete srv;
}
//}}}

void tcp_server()
//{{{
{
  net_socket *srv;
  net_socket *clt[MAX_CLIENTS] = {0,0};
  int state[MAX_CLIENTS] = {0,0};
  char buf[MAX_CLIENTS][256];
  int len[MAX_CLIENTS];
  int cont;

  // Create listen socket to accept client connections
  printf("Starting server listen socket.\n");
  srv = prot->create_listen_socket(port, socktype);
  srv->read_selectable();
  prot->start_notify(notify_port,"Test server",11);
  printf("Waiting for connections\n");

  cont = 1;
  while (cont) {
    spin();
    // wait for next network event
    if (prot->select(0)) {
      //{{{ Check server socket
      if (srv->ready_to_read()) {
        // server socket has data, i.e. connection requested
        int i;
        net_address *addr;
        net_socket *new_clt;
        char name[80];
      
        new_clt = srv->accept(addr);
        addr->store_string(name,80);
        printf("Got connect from [%s]...",name);
      
        // search for available client storage
        for (i=0; i<MAX_CLIENTS; i++)
          if (clt[i] == 0)
            break;
      
        if (i<MAX_CLIENTS) {
          // save socket to client
          clt[i] = new_clt;
      
          // make reads "selectable" to alert of read events
          new_clt->read_selectable();
          printf("accepted\n");
        }
        else
          printf("rejected!\n");
      }
      //}}}
      //{{{ Check client statuses
      for (int i=0; i<MAX_CLIENTS; i++)
	      if (clt[i]) {
	        if (clt[i]->error()) {
	          printf("Aiieeee!  Error on %d.\n",i);
	          delete clt[i];
	          clt[i] = 0;
	        }
	        if (clt[i]->ready_to_read()) {
	          // Client sent us data
	          if (state[i] == 0) {
	            len[i] = clt[i]->read(buf[i], 256);
	            if (len[i] <= 0) {
	              // 0 length read means EOF
	              printf("Closing connection %i\n",i);
	              delete clt[i];
	              clt[i] = 0;
	              break;
	            }
	            else {
	              // normal read
	              clt[i]->write_selectable();
	              clt[i]->read_unselectable();
	              state[i] = 1;
	              buf[i][len[i]] = 0;
	              printf("Server: From %d Got [%s] len %d\n",i,buf[i],len[i]);
	              if (!strcmp(buf[i],"die"))
	                cont = 0;
	            }
	          }
	        }
	        if (clt[i]->ready_to_write()) {
	          if (state[i] == 1) {
	            // Echo data back to client
	            clt[i]->write(buf[i], len[i]);
	            clt[i]->read_selectable();
	            clt[i]->write_unselectable();
	            state[i] = 0;
	            printf("Server: Echoed to %d: [%s] len %d\n",i,buf[i],len[i]);
	          }
	        }
	      }
      //}}}
    }
  }
  for (int i=0; i<MAX_CLIENTS; i++)
    if (clt[i])
      delete clt[i];
  delete srv;
}
//}}}

void client()
//{{{
{
  net_socket *clt;
  char s[256];
  int len;

  // finding server
	printf("Trying to find server\n");
	net_address *addr;
  while (!(addr = prot->find_address(notify_port,s)))
		spin();

  // Create new socket to server
//	addr->set_port(port);
	addr->store_string(s,256);
  printf("Trying to connect to server [%s].\n",s);
	clt = prot->connect_to_server(addr,socktype);
  //  ((unix_fd*)clt)->broadcastable();
  
  while (1) {
    printf("Enter client data.\n");
    gets(s);

    // Quit on Q
    if (!strcmp(s,"q"))
      break;

    clt->read_unselectable();
    clt->write_selectable();
    while (!clt->ready_to_write() && !clt->error()) {
      spin();
      prot->select(0);
    }

    if (clt->error())
      break;

    len = clt->write(s,strlen(s));
    
    printf("Wrote [%s] length %d\n",s,len);
    
    clt->read_selectable();
    clt->write_unselectable();
    while (!clt->ready_to_read() && !clt->error()) {
      spin();
      prot->select(0);
    }
    
    if (clt->error())
      break;
    
    len = clt->read(s,256);

    if (len <= 0) {
      printf("Server died\n");
      break;
    }
    
    printf("Got [%s] len %d\n",s,len);
  }
  delete clt;
}
//}}}

void ShowZones()
{
	int num_zones;
	char **name;
	
	name = atalk.GetZones(num_zones);
	
	printf("Got %d zones.\n",num_zones);
	for (int i=0; i<num_zones; i++)
		printf("  %s\n",name[i]);
		
	char buff[40];
	
	buff[0] = 0;
	atalk.GetMyZone(buff);

	printf("In zone: [%s]\n",buff);
}

void main()
//{{{
{
  char str[256], *s;

  if (prot->installed()) {
    printf("Testing %s...\n",prot->name());
  }
  else {
    printf("No protocol!\n");
    exit(1);
  }

  while (1) {
    printf("\n> ");
    gets(str);
    s = strtok(str,delim);
    if (!s)
      break;

    switch (s[1]) {
    case 'u':
    case 'U':
      socktype = net_socket::SOCKET_FAST;
      break;
    case 's':
    case 'S':
      socktype = net_socket::SOCKET_SECURE;
      break;
    }

    switch (s[0]) {
    case 'c':
    case 'C':
      s = strtok(0,delim);
#if 0
      addr = prot->get_node_address(s,port,0);
#else

#if 0
      sockaddr_in host;
      char tmp[4];
      
      for (int i=0; i<4; i++)
      {
        int num = 0;
        while (*s)
        {
          if (*s=='.')
          {
            s++;
            break;
          }
          num = num*10 + *s - '0';
          s++;
        }
        tmp[i] = num;
      }
      
      memset( (char*) &host,0, sizeof(host));
      host.sin_family = AF_INET;
      host.sin_port = htons(port);
      host.sin_addr.s_addr = htonl(INADDR_ANY);
      memcpy(&host.sin_addr,tmp,sizeof(in_addr));
      
      addr = new ip_address(&host);
#else
      AddrBlock host;
      short tmp[3];
      
      for (int i=0; i<4; i++)
      {
        int num = 0;
        while (*s)
        {
          if (*s==':')
          {
            s++;
            break;
          }
          num = num*10 + *s - '0';
          s++;
        }
        tmp[i] = num;
      }
      
      addr = new atalk_address(tmp[0],tmp[1]);
      addr->set_port(tmp[2]);
#endif

#endif
      client();
      break;
    case 's':
    case 'S':
			if (socktype == net_socket::SOCKET_FAST)
				udp_server();
			else
				tcp_server();
      break;
    case 'z':
    	ShowZones();
    	break;
    case 'q':
    case 'Q':
      exit(1);
      break;
    case 'h':
    case '?':
    	printf(	"Usage:\n"
    					"  su  - start unsecure server\n"
    					"  ss  - start secure server\n"
    					"  cu  - connect to unsecure server\n"
    					"  cs  - connect to secure server\n"
    					"  z   - show zones\n"
    					"  q   - quit\n"
    					"\n"
    					"Note: sending a die message to a server ends the server\n"
    					);
    	break;
    }
  }
}
//}}}

//{{{ Revision Log
/*//////////////////////////////////////////////////////////////////////
$Log$
//////////////////////////////////////////////////////////////////////*/
//}}}

//{{{ Emacs Locals
// Local Variables:
// folded-file: t
// End:
//}}}
