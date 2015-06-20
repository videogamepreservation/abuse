#include "jmalloc.hpp"
#define PARTICLE_SIZE 2            // 2x2 block of pixels

class particle
{
public :
  unsigned char *pict;
  int x,y;
  particle *left,*right,*up,*down;
  particle(int X, int Y, int size);
  ~particle() { jfree(pict); }
} ;

particle::particle(int X, int Y, image *im, int size);
{ 
  x=X; y=Y; 
  left=right=up=down=NULL; 
  pict=(unsigned char *)jmalloc(size*size,"Particle");
}

class particle_image
{
  int particles;
  particle *top;
  int size;
public :
  particle_image(image *im, int particle_size);       // im will be erased!
} ;


particle_image::particle_image(image *im, int particle_size)     
{
  int x,y,py,p;
  unsigned char *sl,*sl2;
  top=NULL;
  size=particle_size;

  int matw=(im->width()+size-1)/size,math=(im->height()+size-1)/size;
  particle *matrix[matw*math];


  // break the image up into particles and store in a big matrix
  for (y=0;y<im->height();y++)
  {
    sl=im->scan_line(y);
    py=y/size;
    for (x=0;x<im->width();x++,sl+=px)
    {
      px=x/size;
      sl2=matrix[py*matw+px]->pict+(y%size)*size;      // address of particle image memory

      int copy_size;

      memset(sl2,0,size);
      
      if (*sl)
      {
	px=x/size;
	py=y/size;
      }

  }
  if (px<0)       // couldn't find any non-blank pixels
    particles=0;
  else
    add_particle_line(px,py,im);
}
