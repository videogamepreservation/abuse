#include "mdlread.hpp"
#include "macs.hpp"
#include "linked.hpp"
#include "system.h"
#include <stdio.h>


//  write_mdl take an array of pointer to images and a palette
// and the generates a Moving DeLight file format containing all the
// images.  Note, only the mode 320x200x256 is sopprted here for saving
// images.  All images should be sized so they will fit on an mdl screen
// but no checking of that is done hhere.
void write_mdl(image **images, short total_images, palette *pal,char *fn,
		short firstpage, short images_per_page)
{
  FILE *fp;
  char buf[18];
  unsigned short xy[2],x;
  char name[13],page;
  unsigned char *c;
  short i;
  palette *np;
  clear_errors();
  CONDITION(images && pal && fn && total_images>0,"bad parms");
  CONDITION(pal->pal_size()==256,"MDL write only support 256 color images");

  fp=fopen(fn,"wb");
  if (!fp) set_error(imWRITE_ERROR);
  else
  { strcpy(buf,"JC20");            // Signature for mdl file
    buf[4]=255;                    // 255 is graph driver 320x200x256
    buf[5]=0;                      // graph mode 0 for this graph driver
    buf[6]=19;                     // the BIOS mode is 19
    fwrite(buf,7,1,fp);
    np=pal->copy();                // make a copy before we change
    np->shift(-COLOR_SHIFT);       // PC palette don't have 8 bit color regs
    fwrite(np->addr(),1,768,fp);
    delete np;                     // destroy the copy me made
    memset(buf,0,11);              // 11 reserved bytes (0) follow
    fwrite(buf,11,1,fp);
    for (i=0;i<total_images;i++)
    {
      memset(buf,0,6);            // each image has 6 bytes of reserved 0
      fwrite(buf,6,1,fp);
      xy[0]=int_to_intel(i%100+20); xy[1]=int_to_intel(30);  // the x and y position on the screen
      fwrite(xy,4,1,fp);
      sprintf(name,"JC%-10d",i);  // set the name of the image
      fwrite(name,12,1,fp);

      page=firstpage+i/images_per_page;

      fwrite(&page,1,1,fp);         // put all of the image on the first page
      xy[0]=int_to_intel(images[i]->width()*images[i]->height()+4);  // calc the size of the image
    
      fwrite(xy,2,1,fp);
      xy[0]=int_to_intel(images[i]->width());
      fwrite(xy,2,1,fp);
      xy[0]=int_to_intel(images[i]->height());
      fwrite(xy,2,1,fp);
      for (x=0;x<(unsigned short)images[i]->height();x++)   // write all the scan_lines for the
      { c=images[i]->scan_line(x);            // image
	fwrite(c,images[i]->width(),1,fp);
      }
    }
    fclose(fp);                // close the file and make sure buffers empty
  }
}

short mdl_total_images(char *fn)
{
  char buf[800];
  unsigned short xy[2],t;
  FILE *fp;
  fp=fopen(fn,"rb");
  if (!fp)
  { set_error(imFILE_NOT_FOUND);
    return 0;
  }
  if (fread(buf,2,1,fp)!=1)
    set_error(imFILE_CORRUPTED);
  else if (buf[0]!='J' || buf[1]!='C')
    set_error(imINCORRECT_FILETYPE);
  else if (fread(buf,5,1,fp)!=1)
    set_error(imFILE_CORRUPTED);
  else if (buf[4]!=0x13)
    set_error(imNOT_SUPPORTED);
  if (current_error()) { fclose(fp); return 0;}
  fread(buf,1,768+11,fp);
  t=0;
  while (!feof(fp))
  { if (fread(buf,1,23,fp)==23)
    {
      fread(xy,2,1,fp);
      xy[0]=int_to_local(xy[0]);
      fseek(fp,xy[0],SEEK_CUR);
      t++;
    }
  }
  fclose(fp);
  return t;
}

// read_mdl returns an array containing pointers to all the desired images
// and a palette that is read form the file
// to load image numbers 4 through 9 let start =4, end=9
image **read_mdl(char *fn, palette *&pal, short startn, short endn, short &total)
{
  FILE *fp;
  image **im;
  char buf[50];
  unsigned short xy[2],i,j;
  clear_errors();
  make_block(sizeof(FILE));
  im=NULL;
  total=0;
  startn--;
  CHECK(fn && (startn<=endn || endn==-1) && startn>=0);
  fp=fopen(fn,"rb");
  if (!fp)
  { set_error(imFILE_NOT_FOUND);
    return NULL;
  }
  if (fread(buf,2,1,fp)!=1)
    set_error(imFILE_CORRUPTED);
  else if (buf[0]!='J' || buf[1]!='C')
    set_error(imINCORRECT_FILETYPE);
  else if (fread(buf,5,1,fp)!=1)
    set_error(imFILE_CORRUPTED);
  else if (buf[4]!=0x13)
    set_error(imNOT_SUPPORTED);
  else
  {
    make_block(sizeof(palette));
    pal=new palette(256);
    if (!pal)
    {  set_error(imMEMORY_ERROR); return NULL; }
    if (fread(pal->addr(),1,768,fp)!=768)
      set_error(imFILE_CORRUPTED);
    else if (fread(buf,1,11,fp)!=11)
      set_error(imFILE_CORRUPTED);
    else
    {
      pal->shift(2);
      pal->set_all_used();
      while (startn && !current_error())
      { if (fread(buf,1,23,fp)!=23)
	  set_error(imFILE_CORRUPTED);
	fread(xy,2,1,fp);
	xy[0]=int_to_local(xy[0]);
	fseek(fp,xy[0],SEEK_CUR);
	startn--; if (endn>0) endn--;
      }
      if (!current_error())
	im=(image **)jmalloc(sizeof(image *)*endn,"mdl_read::image * array");

      while ((startn<endn || endn==-1) && !feof(fp) && !current_error())
      {
	if (fread(buf,1,23,fp)==23) 
        {
  	  if (fread(&j,1,2,fp)!=2) set_error(imFILE_CORRUPTED);
	  else
	  {
	    j=int_to_local(j);
	    j-=4;
            xy[0]=5; xy[1]=5;
	    if (fread(xy,1,4,fp)!=4) set_error(imFILE_CORRUPTED);
	    make_block(sizeof(image));
	    xy[0]=int_to_local(xy[0]);
	    xy[1]=int_to_local(xy[1]);
	    im[startn]=new image(xy[0],xy[1]);
	    total++;
	    for (i=0;i<xy[1];i++)
	      if (fread(im[startn]->scan_line(i),xy[0],1,fp)!=1)
	        set_error(imFILE_CORRUPTED);
	      else j-=xy[0];
	    if (j)
	      fseek(fp,j,SEEK_CUR);
	  }
	  startn++;
        }
      }
    }
  }
  fclose(fp);
  return im;
}
