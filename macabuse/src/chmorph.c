#include "chmorph.hpp"

jmorph *normal_to_pacman;
color_filter *color_table;

void load_morphs()
{
  FILE *fp=fopen("n2pac.mph","rb");
  CONDITION(fp,"unable to open morph file n2pac.mph");
  
  spec_directory sd(fp);
  sd.find(" 

}

  







