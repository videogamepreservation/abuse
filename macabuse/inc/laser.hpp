#ifndef __LASER_HPP_
#define __LASER_HPP_
#include "objects.hpp"

class laser : public game_object
{
  short h,l_x,
	signal,				       // signal generated when laser hits a person
	on_signal,			       // recieved signal that turns on the laser
	off_signal;
  unsigned char color,hit_color,damage,on,max_speed; 
  signed char l_xvel,l_xaccel;
public :  
  laser(long X, long Y);
  laser(FILE *fp, unsigned char *state_remap);
  virtual int size() { return game_object::size()+5*2+7*1; }
  virtual void recieve_signal(long signal);
  virtual game_objects type() { return O_laser; }
  virtual ifield *make_fields(int ystart, ifield *Next);
  virtual void gather_input(input_manager *inm);
  virtual void save(FILE *fp);
  virtual int decide();
  virtual void draw();  
} ;


#endif


