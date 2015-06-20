#include "image24.hpp"
#include "image.hpp"

image24::image24(unsigned short width, unsigned short height,
                 unsigned char *buffer)
{
  w=width;
  h=height;
  
  data=(unsigned char *)jmalloc(width*height*3,"image24");
  CONDITION(data,"unable to alloc enough memory for 24 bit image");    
}

void image24::clear(unsigned char r, unsigned char g, 
                    unsigned char b)
{
  int x,y;
  unsigned char *p=data;  
  for (y=0;y<h;y++)
  {    
    for (x=0;x<w;x++)
    {
      *(p++)=r;
      *(p++)=g;
      *(p++)=b;
    }    
  }  
}


void image24::add_error(int x, int y, int r_error, int g_error, int b_error, int error_mult)
{
  if (x>=w || x<0 || y>=h || y<0)           // check to make sure this pixel is in the image
    return ;
  unsigned char *pix=data+(y*w*3+x*3);    
  int result;
  
  result=(int)(*pix)+r_error*error_mult/32;
  if (result>255)
    *pix=255;
  else if (result<0) *pix=0;
  else *pix=result;  
  pix++;

  result=(int)(*pix)+g_error*error_mult/32;
  if (result>255)
    *pix=255;
  else if (result<0) *pix=0;
  else *pix=result;
  pix++;
  
  result=(int)(*pix)+b_error*error_mult/32;
  if (result>255)
    *pix=255;
  else if (result<0) *pix=0;
  else *pix=result;
}


image *image24::dither(palette *pal)
{  
  int i,j,closest;
  unsigned char r,g,b,*cur_pixel=data,ar,ag,ab;  
  image *dest=new image(w,h);  
  for (j=0;j<h;j++)
  {    
    for (i=0;i<w;i++)
    {
      r=(*cur_pixel);  cur_pixel++;
      g=(*cur_pixel);  cur_pixel++;
      b=(*cur_pixel);  cur_pixel++;      
      closest=pal->find_closest(r,g,b);           // find the closest match in palette
      dest->putpixel(i,j,closest);               // set the pixel to this color

      pal->get(closest,ar,ag,ab);                   // see what the actual color we used was
  
      add_error(i+1,j,ar-r,ag-g,ab-b,8);  
      add_error(i+2,j,ar-r,ag-g,ab-b,4);  

      add_error(i-2,j+1,ar-r,ag-g,ab-b,2);
      add_error(i-1,j+1,ar-r,ag-g,ab-b,4);
      add_error(i,j+1,ar-r,ag-g,ab-b,8);
      add_error(i+1,j+1,ar-r,ag-g,ab-b,4);
      add_error(i+2,j+1,ar-r,ag-g,ab-b,2);
    }
  }   
  return dest;
}





