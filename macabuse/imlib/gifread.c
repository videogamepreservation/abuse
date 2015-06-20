#include "gifread.hpp"
#include "palette.hpp"
#include "image.hpp"
#include "video.hpp"
#include "linked.hpp"
#include "gifdecod.hpp"
#include "system.h"
#include "dos.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "dir.h"
#include "macs.hpp"


struct {
	unsigned short int	Width;
	unsigned short int	Height;
	unsigned char	ColorMap[3][256];
	unsigned short int	BitPixel;
	unsigned short int	ColorResolution;
	unsigned short int	Background;
	unsigned short int	AspectRatio;
} GifScreen;

struct {
  unsigned short int w,h;
  unsigned char color_info,background,reserved;
} gif_screen;

struct {
  unsigned short int xoff,yoff,w,h;
  unsigned char color_info;
} gif_image;

image *read_gif(char *fn, palette *&pal)
{
  char buf[100],er;
  unsigned char sep;
  int ncolors;
  FILE *fp;
  image *im;
  clear_errors();
  fp=fopen(fn,"rb");
  er=0;
  im=NULL;
  if (fp==NULL) er=imFILE_NOT_FOUND;
  else
  {
    if (fread(buf,1,6,fp)==6)
    {
      buf[6]=0;
      if (!strcmp("GIF87a",buf))
      {
        fread((char *)&gif_screen.w,2,1,fp);
        gif_screen.w=int_to_local(gif_screen.w);
        fread((char *)&gif_screen.h,2,1,fp);
        gif_screen.h=int_to_local(gif_screen.h);
        fread((char *)&gif_screen.color_info,1,1,fp);
        fread((char *)&gif_screen.background,1,1,fp);
        if (fread((char *)&gif_screen.reserved,1,1,fp)==1)
	{
	  if (gif_screen.color_info&128)
	  {
	    ncolors=2<<(gif_screen.color_info&0x0f);
	    make_block(sizeof(palette));
//	    pal=new palette(ncolors);
	    pal=new palette(256);
	    if (pal)
	    {  
              if (fread((char *)pal->addr(),1,ncolors*3,fp)!=ncolors*3) er=imREAD_ERROR;
	    } else er=imMEMORY_ERROR;
	  }
	  if (!er)
	  { do
	    {
	      if (fread((char *)&sep,1,1,fp)!=1)
		er=imREAD_ERROR;
	    } while (!er && sep!=',');
            fread((char *)&gif_image.xoff,2,1,fp);
            gif_image.xoff=int_to_local(gif_image.xoff);
            fread((char *)&gif_image.yoff,2,1,fp);
            gif_image.yoff=int_to_local(gif_image.yoff);
            fread((char *)&gif_image.w,2,1,fp);
            gif_image.w=int_to_local(gif_image.w);
            fread((char *)&gif_image.h,2,1,fp);
            gif_image.h=int_to_local(gif_image.h);
	    if (!er && (fread((char *)&gif_image.color_info,1,1,fp)==1))
	    {
	      if (gif_image.color_info&128)
	      {
		ncolors=2<<(gif_image.color_info&0x0f);
                CHECK(ncolors<=256);
		make_block(sizeof(palette));
		pal = new palette(ncolors);
		if (pal)
		{ if (fread((char *)pal->addr(),1,ncolors*3,fp)!=ncolors*3) er=imREAD_ERROR;
		} else er=imMEMORY_ERROR;
	      }

	      if (!er)
	      {
		make_block(sizeof(image));
		im=new image(gif_image.w+1,gif_image.h);
		decode_gif_data(im,fp);
		fclose(fp);
	      }

	    } else er=imREAD_ERROR;
	  }

	} else er=imREAD_ERROR;
      } else er=imINCORRECT_FILETYPE;
    } else er=imREAD_ERROR;
    fclose(fp);
  }
  set_error(er);
  return im;
}
