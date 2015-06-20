#ifndef __PROPERTY_HPP_
#define __PROPERTY_HPP_

class property;
class property_manager
{
  property *first;
  property *find(char *name);
  public :
  property_manager() { first=0; } 
  void load(char *filename);
  void save(char *filename);
  
  int getd(char *name, int def) { return (int)get(name,def); }
  double get(char *name, double def);
  char  *get(char *name, char *def);

  void setd(char *name, int def) { set(name,def); }
  void set(char *name, double def);
  void set(char *name, char *def);
  ~property_manager();
} ;


#endif
