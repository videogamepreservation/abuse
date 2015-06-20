

#ifndef __RAM_FILE_HPP_
#define __RAM_FILE_HPP_

#include "specs.hpp"

class ram_file : public bFILE 
{
  uchar *buf;
  int buf_size;
  int buf_end;

  virtual int allow_read_buffering();
  virtual int allow_write_buffering();
  public :
  ram_file();
  virtual int open_failure();
  virtual int unbuffered_read(void *buf, size_t count);       // returns number of bytes read
  virtual int unbuffered_write(void *buf, size_t count);      // returns number of bytes written
  virtual int unbuffered_seek(long offset, int whence);  // whence=SEEK_SET, SEEK_CUR, SEEK_END, ret=0=success
  virtual int unbuffered_tell();
  virtual int file_size();
  virtual ~ram_file();
} ;

#endif
