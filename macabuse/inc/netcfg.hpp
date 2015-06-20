#ifndef __NETCFG_HPP_
#define __NETCFG_HPP_
#include "jwindow.hpp"
#include "sock.hpp"

class net_configuration
{
  public :
  enum { SINGLE_PLAYER, SERVER, CLIENT, RESTART_SERVER, RESTART_CLIENT, RESTART_SINGLE } state;

  int restart_state();
  int notify_reset();

  unsigned short port,
  							 game_port,
                 server_port;  // if we are a server, use our_port
  char name[100];
  char server_name[100];
  net_address *addr;

  char min_players,
       max_players;
  short kills;

  net_configuration();
  int input();   // pulls up dialog box and input fileds
  void cfg_error(char *msg);
  int confirm_inputs(jwindow *j, int server);
  void error(char *message);
  int confirm_inputs(input_manager *i, int server);
  ifield *center_ifield(ifield *i,int x1, int x2, ifield *place_below);
  int get_options(int server);
} ;

extern net_configuration *main_net_cfg;

#endif
