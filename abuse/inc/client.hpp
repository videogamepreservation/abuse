#ifndef __CLIENT_HPP_
#define __CLIENT_HPP_
/*

  Client duties :

       - Get local inputs
       - Send current inputs

       - read server commands until 
       - process commands
       - ability to detach from server
       - ability to "talk" with a local server
         through global local_server
	   and functions local_server->insert_received_packet(pk);
	                 local_server->get_output_packet(pk);


*/
#include "macs.hpp"
#include "packet.hpp"
class view;

class game_client
{
  int process_command(uchar cmd, view *player, packet &pk);
  int need_to_read_views;
public :
  int cnum;
  game_client(int client_number);
  void request_entry();                   // asks server for entry into the game
  void wait_entry();                      // wait for signal from server so we can download level
  void entry_continue();                  // server needs ack before continue from above
  void get_local_inputs(packet &pk);      // writes inputs to packet as commands
  void send_local_request(packet &pk);    // sends commands to server

  void read_server_commands(packet &pk);  // reads everybodies commands
  void process_packet(packet &pk);
  void quit_server();
  void read_views();
} ;

extern game_client *local_client;

#endif



