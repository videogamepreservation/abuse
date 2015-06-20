#ifndef __GO_HPP_
#define __GO_HPP_
#include "objects.hpp"

class elcontrol : public game_object
{
public:  
  short allow_dir;  
  elcontrol(long X, long Y);  
  elcontrol(FILE *fp, unsigned char *state_remap);
  virtual int size() { return game_object::size()+2; }
  virtual game_objects type() { return O_elcontrol; }  
  virtual ifield *make_fields(int ystart, ifield *Next);
  virtual void gather_input(input_manager *inm);
  virtual void save(FILE *fp) { game_object::save(fp); write_short(fp,allow_dir); }
  virtual int decide() { return 1; }  // not dead
  virtual int move(int cx, int cy, int button)  { return 0; }  // not blocked          
  virtual void draw();  // only show when DEV mode is on    
} ;

 

class elevator : public game_object
{
  short dir,speed;
public :  
  elcontrol *find_stop();
  elevator(long X, long Y);    
  elevator(FILE *fp, unsigned char *state_remap);
  virtual int size();
  virtual void recieve_signal(long signal) { ; }
  virtual game_objects type() { return O_elevator; }
  virtual ifield *make_fields(int ystart, ifield *Next);  
  virtual void gather_input(input_manager *inm);  
  virtual void save(FILE *fp);
  virtual int can_block(game_object *who);
  virtual int decide();
  virtual void draw();  // draw cables above the elevator
} ;

class sensor : public game_object
{
  short xrange,yrange,signal,activate;  
public :
  sensor(long X, long Y) { defaults(); xrange=yrange=signal=0; activate=-1; }
  sensor(FILE *fp, unsigned char *state_remap);
  virtual int size();
  virtual game_objects type() { return O_sensor; }
  virtual ifield *make_fields(int ystart, ifield *Next);
  virtual void gather_input(input_manager *inm);  
  virtual void save(FILE *fp);

  virtual void draw();  // only show when DEV mode is on    
  virtual int decide();
  virtual int move(int cx, int cy, int button)  { return 0; }  // not blocked          

  char *aname();
  void get_activate(char *name);
  
} ;


#endif

