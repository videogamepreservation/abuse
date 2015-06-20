#include "items.hpp"

backtile::backtile(spec_entry *e, FILE *fp)
{
  im=new image(e,fp);
  next=read_short(fp);
}

foretile::foretile(spec_entry *e, FILE *fp)
{
  unsigned char d[2],i; 
  im=new image(e,fp);
  next=read_short(fp);
  fread(&damage,1,1,fp);
  points=new point_list(fp);
}

figure::figure(spec_entry *e, FILE *fp)
{
  im=new image(e,fp);
  fread(&hit_damage,1,1,fp);
  fread(&xcfg,1,1,fp);
  
  touch=new point_list(fp); 
  damage=new point_list(fp); 
  hit=new point_list(fp); 
}

