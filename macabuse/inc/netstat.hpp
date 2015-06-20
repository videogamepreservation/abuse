#ifndef NETSTAT
#define NETSTAT


#include "status.hpp"
#include "palette.hpp"
class net_status_node;

class net_status_manager : public status_manager
{
  int last_update,xp,yp;
  palette *old_pal;
  net_status_node *first;
  int level,last_level,last_percent,first_percent;
  char *g_file;

  int first_x1,first_y1,
    first_x2,first_y2,
    second_x1,second_y1,
    second_x2,second_y2,
    color1,
    color2;

public :

  net_status_manager(char *graphic_file, 
      int x1, int y1,
      int x2, int y2,
      int color1, int color2);

  virtual void push(char *name, visual_object *show);
  virtual void update(int percentage);
  virtual void pop();
} ;

#endif

