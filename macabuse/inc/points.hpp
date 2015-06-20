#ifndef __POINTS_HPP_
#define __POINTS_HPP_
#include <stdio.h>
#include <stdlib.h>
#include "jmalloc.hpp"
#include "specs.hpp"

class point_list
{
public :
  unsigned char tot;
  unsigned char *data;
  point_list(unsigned char how_many, unsigned char *Data);
  point_list() { tot=0; data=NULL; }
  point_list(bFILE *fp);
  void save(bFILE *fp);
  long size() { return 1+2*tot; }
  point_list *copy() { return new point_list(tot,data); }
  ~point_list() { if (tot) { jfree(data); } }
} ;

#endif


