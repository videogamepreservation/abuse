#include "image.hpp"
#include "specs.hpp"

image *load_targa(char *filename, palette *pal)
{
  bFILE *fp=open_file(filename,"rb");
  if (!fp)
    return 0;

  if (fp->open_failure())
  {
    delete fp;
    return 0;
  }
  
  unsigned char id,color_map,im_type;

  id=fp->read_byte();
  color_map=fp->read_byte();
  im_type=fp->read_byte();

  if (color_map!=0)
  {
    delete fp;
    return 0;
  }

  if (!(im_type==2 || im_type==10))
  {
    delete fp;
    return 0;
  }

  fp->read_short();
  fp->read_short();
  fp->read_byte();

  fp->read_short();
  fp->read_short();


  int w=fp->read_short();
  int h=fp->read_short();
  unsigned char bpp=fp->read_byte();
  unsigned char im_des=fp->read_byte();

  if (bpp!=32)
  {
    delete fp;
    return 0;
  }

  image *im=new image(w,h);

  int x,y;
  unsigned char ctrl;
  unsigned char bgra[4],*sl,c,lr,lg,lb,ll=0,lc;
  
  
  

  if (im_type==10)
  {    
    for (y=0;y<h;y++)
    {
      sl=im->scan_line(h-y-1);

      for (x=0;x<w;)
      {
        ctrl=fp->read_byte();
        if (ctrl&0x80)
        {
          fp->read(bgra,4);
          ctrl&=(~0x80);
          ctrl++;
          if (bgra[3])
          {
            if (ll && lr==bgra[2] && lg==bgra[1] && lb==bgra[0])
              c=lc;
            else
            {
              c=pal->find_closest_non0(bgra[2],bgra[1],bgra[0]);
              lr=bgra[2];
              lg=bgra[1];
              lb=bgra[0];
              lc=c;
              ll=1;
            }

          }
          else c=0;

          while (ctrl--)
          {
            *sl=c;
            sl++;
            x++;
          }
        } else
        {
          ctrl++;          
          while (ctrl--)
          {
            fp->read(bgra,4);
            if (bgra[3])
            {
              c=pal->find_closest_non0(bgra[2],bgra[1],bgra[0]);
            }
            else c=0;
            *sl=c;
            sl++;
            x++;
          }
        }
      }
    }    
  }



  delete fp;
  return im;
}
