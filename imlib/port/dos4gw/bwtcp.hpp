#include "jnet.hpp"
#define PK_BUFFER_SIZE 4096

class bwt_out_socket : public out_socket
{
  int fd;
  public :
  uchar pk_buffer[PK_BUFFER_SIZE];  
  long pk_buffer_ro,pk_buffer_last;
  int try_connect(char *rhost, int port);
  bwt_out_socket(int FD) { fd=FD; }
  bwt_out_socket();
  virtual int ready_to_read();
  virtual int ready_to_write();
  virtual int send(packet &pk);
  virtual int get(packet &pk);
  virtual ~bwt_out_socket();
  void bwt_fill_buffer();
  int get_fd() { return fd; }
} ;


class bwt_in_socket : public in_socket
{
  int port;
  public :
  bwt_in_socket(int Port);
  bwt_out_socket *listeners[5];
  virtual out_socket *check_for_connect();
  virtual ~bwt_in_socket();
} ;


int bwt_init();
void bwt_uninit();
int bwt_get_my_ip();








