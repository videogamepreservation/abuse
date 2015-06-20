#ifndef __STATUS_HPP_
#define __STATUS_HPP_

#include "visobj.hpp"      // get visual object declaration


class status_manager
{
  public :
  virtual void push(char *name, visual_object *show) = 0;
  virtual void update(int percentage) = 0;
  virtual void pop() = 0;
  virtual void force_display() { ; }
} ;


class text_status_node;

class text_status_manager : public status_manager
{
  public :
  int level;
  text_status_node *first;
  text_status_manager();
  virtual void push(char *name, visual_object *show);
  virtual void update(int percentage);
  virtual void pop();
} ;


extern status_manager *stat_man;

class stack_stat  // something you can declare on the stact that is sure to get cleaned up
{
  public :
  stack_stat(char *st, visual_object *show=NULL) { if (stat_man) stat_man->push(st,show); }
  ~stack_stat() { if (stat_man) stat_man->pop(); }
} ;


#endif

