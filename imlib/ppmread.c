#include "image.hpp"
#include "palette.hpp"
#include <stdio.h>
#include "macs.hpp"
#include "ppmread.hpp"


int read_ppm_header(FILE *fp, int *parm)
{
  int nr;
  char st[100],sig[50];
  nr=0;
  fscanf(fp,"%s",sig);
  if (!strcmp(sig,"P6"))
  {
    do
    {
      fscanf(fp,"%s",sig);
      if (sig[0]=='#')
        fgets(st,100,fp);
      else
      { 
        if (sscanf(sig,"%d",parm))
        {  nr++; parm++; }
        else return 0;
      } 
    } while (nr<3 && !feof(fp));
  }
  else return 0;
//  fgets(st,100,fp);
  return 1;
} 

void write_ppm(image *im,palette *pal,char *fn)
{
  FILE *fp;
  CHECK(im && pal && fn);
  unsigned char r[3],*c;
  int x,y;
  clear_errors();
  fp=fopen(fn,"wb");
  if (!fp) set_error(imWRITE_ERROR);
  else
  {
    fprintf(fp,"%s %d %d %d\n","P6",im->width(),im->height(),(int)256);
    for (y=0;y<im->height();y++)
    { c=(unsigned char *)im->scan_line(y);
      for (x=0;x<im->width();x++)
      { r[0]=pal->red(c[x]);
	r[1]=pal->green(c[x]);
	r[2]=pal->blue(c[x]);
	fwrite(&r[0],1,1,fp);
	fwrite(&r[1],1,1,fp);
	fwrite(&r[2],1,1,fp);
      }
    }
    fclose(fp);
  }
}

#define TSIZE 1001

image *read_ppm(char *fn,palette *&pal, int pal_type)
{
  FILE *fp;
  image *im;
  unsigned char *c,col[3];

  char buf[30];
  int l,h,maxc,i,j,parm[3],find_color;
  CONDITION(fn,"Null filename");
  clear_errors();
  fp=fopen(fn,"rb");
  im=NULL;
  CONDITION(fp,"Filename not found"); 
  if (!fp) { set_error(imFILE_NOT_FOUND); return NULL; }

  if (read_ppm_header(fp, parm)==0) set_error(imFILE_CORRUPTED);
  else
  {
    l=parm[0]; h=parm[1]; maxc=parm[2];
    
    if (!pal)
      pal=new palette;
    fgets(buf,30,fp);
    im=new image(l,h);
    printf("Created image %d,%d\n",l,h);
    for (i=0;i<h;i++)
    { c=(unsigned char *)im->scan_line(i);
      for (j=0;j<l;j++)
      {
	if (fread(col,1,3,fp)!=3) set_error(imFILE_CORRUPTED);
	if (pal_type==PPM_R3G3B2)
	  c[j]=(col[0]*7/255)|((col[1]*7/255)<<3)|((col[2]*3/255)<<6);
	else if (pal_type==PPM_BW)
	  c[j]=col[0]*255/parm[2];
	else
	{
	  find_color=pal->find_color(col[0],col[1],col[2]);
	  if (find_color>=0) c[j]=find_color;
	  else c[j]=(unsigned char) pal->add_color(col[0],col[1],col[2]);
	}
      }
    }
  }
  fclose(fp);
  return im;
}
