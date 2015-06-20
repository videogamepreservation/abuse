#ifndef __SERVER2_HPP_
#define __SERVER2_HPP_
#error hi

class client_descriptor;


class game_server
{
  client_descriptor *client_list;
  int sync_check;
  public :
  game_server(int argc, char **argv, int port);
//  int init_failed() { return in==NULL; }
  void check_for_clients();
  void receive_inputs();         // reads inputs from all non-local clients
  void send_inputs();            // pass collected inputs to all non-local clients
  void join_new_players();
  ~game_server();
} ;

extern int start_running;
extern game_server *local_server;       // created on server machine, NULL on all others

void game_net_init(int argc, char **argv);
#endif


