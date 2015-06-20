#include "macs.hpp"
#include "image.hpp"
#include "palette.hpp"
#include "video.hpp"
#include "system.h"
#include <stdio.h>

image *read_glfont(char *fn)
{
  image *im,*sub;
  unsigned short length,y;
  unsigned char size,first,width,height,gsize,last;
  FILE *fp;
  fp=fopen(fn,"rb");
  if (!fp) return NULL;
  fread(&length,1,2,fp);  length=int_to_local(length);
  fread(&size,1,1,fp);
  fread(&first,1,1,fp);
  if (size+first>255) { set_error(imFILE_CORRUPTED); fclose(fp); return NULL; }
  fread(&width,1,1,fp);
  fread(&height,1,1,fp);
  fread(&gsize,1,1,fp);
  make_block(sizeof(image));
  im=new image(width*32,height*8);
  make_block(sizeof(image));
  sub=new image(width,height);
  im->clear();  // in case all the fonts aren't in the file, clear extra area
  last=first+size-1;
  while (first<=last)
  {
    for (y=0;(int)y<(int)height;y++)
    {
      fread(sub->scan_line(y),1,gsize/height,fp);
      sub->unpack_scanline(y);
    }
    sub->put_image(im,(first%32)*width,(first/32)*height);
    first++;
  }
  delete sub;
  return im;
}

image *read_pic(char *fn, palette *&pal)
{
  image *im;
  char x[4],bpp;
  unsigned char *sl,esc,c,n,marker,vmode;
  unsigned short w,h,len,bufsize,blocks,sn,esize,edesc;
  int xx,yy;
  FILE *fp;
  im=NULL;
  fp=fopen(fn,"rb");

  fread(&x[0],1,2,fp);
  fread(&w,1,2,fp);
  fread(&h,1,2,fp);
  w=int_to_local(w);  h=int_to_local(h);
  fread(x,1,4,fp);
  fread(&bpp,1,1,fp);
  fread(&marker,1,1,fp);
  if (marker!=0xff)
  { fclose(fp); set_error(imFILE_CORRUPTED); return NULL; }

  im=new image(w,h);

  fread(&vmode,1,1,fp);
  fread(&edesc,1,2,fp);
  edesc=int_to_local(edesc);
  fread(&esize,1,2,fp);
  esize=int_to_local(esize);
  if (esize==768 && !pal)
  { pal=new palette(1<<bpp);
    fread(pal->addr(),1,(1<<bpp)*3,fp);
    pal->shift(2);
  }
  else if (esize)
    fseek(fp,esize,SEEK_CUR);
  fread(&blocks,1,2,fp);
  blocks=int_to_local(blocks);

  yy=h; xx=w;

  while (blocks-- && w>=1 && yy>=0)
  {
    fread(&bufsize,1,2,fp);
    bufsize=int_to_local(bufsize);
    fread(&len,1,2,fp);
    len=int_to_local(len);
    fread(&esc,1,1,fp);
    while (yy>=0 && len)
    {
      fread(&c,1,1,fp);
      if (c!=esc)
      {
	if (xx>=w) { yy--; xx=0; sl=im->scan_line(yy);
	if (yy==h) printf("bufsize=%d\n",bufsize); CHECK(yy<h); }
	sl[xx++]=c;     len--;
      }
      else
      {
	fread(&n,1,1,fp);
	if (n!=0)
	{
	  fread(&c,1,1,fp);
	  while (n-- && yy>=0 && len)
	  {
	    if (xx>=w) { yy--; xx=0; sl=im->scan_line(yy);
	      if (yy==h) printf("bufsize=%d\n",bufsize); CHECK(yy<h); }
	    sl[xx++]=c; len--;
	  }
	}
	else
	{
	  fread(&sn,1,2,fp);
	  sn=int_to_local(sn);
	  fread(&c,1,1,fp);
	  while (sn-- && yy>=0 && len)
	  {
	    if (xx>=w) { yy--; xx=0; sl=im->scan_line(yy); CHECK(yy<h); }
	    sl[xx++]=c; len--;
	  }
	}

      }
    }
  }
  fclose(fp);
  return im;
}

image *read_clp(char *fn)
{
  palette *pal=NULL;
  image *im=read_pic(fn,pal);
  if (pal) delete pal;
  return im;
}

