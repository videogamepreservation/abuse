#ifndef __SPECACHE_HPP_
#define __SPECACHE_HPP_

#include "specs.hpp"


class spec_directory_cache
{
  class filename_node
  {
    public :
    filename_node *left,*right,*next;
    char *fn;
    spec_directory *sd;
    char *filename() { return fn; }
    filename_node(char *filename, spec_directory *dir)
    {
      fn=(char *)memcpy(jmalloc(strlen(filename)+1,"spec_dir cache"),filename,strlen(filename)+1);
      sd=dir;
      next=left=right=0;      
    }
    long size;
  } *fn_root,*fn_list;
  void clear(filename_node *f); // private recursive member  
  long size;
  public :
  spec_directory *get_spec_directory(char *filename, bFILE *fp=NULL);
  spec_directory_cache() { fn_root=0; size=0; }
  void clear();                             // frees up all allocated memory
  void load(bFILE *fp);
  void save(bFILE *fp);
  ~spec_directory_cache() { clear(); }
} ;

extern spec_directory_cache sd_cache;

#endif
