#include "pcxread.hpp"
#include "palette.hpp"
#include "image.hpp"
#include <stdlib.h>

main(int argc, char **argv)
{
  image_init();
  
  if (argc<3)
  {    
    printf("Usage : %s xsize ysize filename [filename]\n",argv[0]);
    exit(0);    
  }
  
  while (argc>3)
  {
    argc--;
    
     palette *pal;
     image *im=read_PCX(argv[argc],pal);
     
     im->resize(atoi(argv[1]),atoi(argv[2])); 
     write_PCX(im,pal,argv[argc]);
      delete pal;    
     printf("  %s\n",argv[argc]);
          
     clear_errors();
  }  
  image_uninit();
  
}

