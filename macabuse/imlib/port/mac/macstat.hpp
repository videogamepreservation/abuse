#ifndef MACSTAT_HPP
#define MACSTAT_HPP

#include "status.hpp"      // get visual object declaration
#include <QDOffscreen.h>

class mac_status_node;
class mac_status_manager : public status_manager
{
protected:
  CWindowPtr win;
	GWorldPtr saveWorld;
	GDHandle saveDevice;
	PixPatHandle pat1,pat2,pat3;
	
  int level;
  mac_status_node *first;
public :
  mac_status_manager();
  virtual void push(char *name, visual_object *show);
  virtual void update(int percentage);
  virtual void pop();
} ;

#endif

