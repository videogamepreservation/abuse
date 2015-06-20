#ifndef __PACKET_HPP_
#define __PACKET_HPP_
#include "macs.hpp"
class packet
{
  public :
  uchar *buf;
  long buf_size,ro,wo,rend;
  int pre_size;
  void make_bigger(int max);

  int get_read_position() { return ro; }
  void set_read_position(int x) { ro=x; }
  int read(uchar *buffer, int size);
  int write(uchar *buffer, int size);
  int eop() { return ro>=rend; }
  void reset();
  packet(int prefix_size=2);
  void get_string(char *st, int len);
  int advance(long offset);

  void write_long(ulong x);      // writes can't fail...
  void write_short(ushort x);
  void write_byte(uchar x);
  void insert_into(packet &pk);
  int size() { return rend-pre_size; }
  ~packet();
} ;



#endif
