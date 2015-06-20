#define MAX_PLAYERS        50
#define PLAYER_INPUT_SIZE  10

enum { PLAYER_FREE,
       PLAYER_CONNECTED,
       PLAYER_DISCONNECTED } ;  // server shold set to PLAYER_FREE after netdriver sets to disconnect


#define NET_FLAG_RUNNING      1
#define NET_FLAG_LOCK_ENTRIES 2   

struct net_struct
{
  char server_flags;
  char driver_flags;
  unsigned char player_status[MAX_PLAYERS];
 
  

}


