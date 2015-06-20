

enum { WAIT_INPUT,      // driver is waiting on input from client
       WAIT_SEND,       // driver waiting on all input collected before sending
       WAIT_PROCESS     // driver waiting for engine to change this to WAIT_INPUT
     };



struct base_memory_struct
{
  client_list *active_clients
  join_struct *join_list;

} ;

struct client_list
{
  short client_id;    // used by engine to refer to specific client
                      // the driver is responsible for allocating/deallocating client id's

  char input_state;   // one of the above wait states

  char command_buffer[1024];  // engine will process this data
   
  client_list *next;

// net driver may have private information here

} ;




struct join_struct
{
  int client_id;

  join_struct *next;

// net driver may have private information here
} ;






/*

  Net driver to engine API


  File API
  -----------------------------------------------------
  int open_file(char *filename, char *mode);
  Note : filename may contain a references to another server.
    If the player starts with -net all files locations default to
    those on the specified server.  But if a filename has a
    "//" leading it's filename, the location of the file prefixes
    the filename.  example "//abuser.location.edu/~joe/file1.lsp"
    specifies a file on the server abuser.location.edu in joe's
    home directory called file1.lsp.

    The net driver should contact that address using the abuse file
    protocol.  If no server is running there the server reports the
    file does not exsist, -1 should be return.


  int close_file(int fd);
  int read(int fd, void *buffer, int size);  // returns bytes read
  long seek(int fd, int offset);
  long tell(int fd);
  long file_size(int fd);





  Game API
  -------------------------------------------------------
  client_side :
    join_game(char *hostname);

  server side :
    kill_client(int client id);
  

*/




