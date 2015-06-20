#include "include.hpp"
#include "ctype.h"

void write_include(image *im, palette *pal, char *filename, char *name)
{
  char tmp_name[200];
  strcpy(tmp_name,name);
  int j,append=0,i;
  for (j=0;j<strlen(name);j++)
    if (toupper(tmp_name[j])<'A' || toupper(tmp_name[j])>'Z')
      tmp_name[j]='_';

  FILE *fp=fopen(filename,"rb");  // see if the file already exsist
  if (fp)
  {
    fclose(fp);
    fp=fopen(filename,"ab");  // if so, append to the end and don't write the palette
    append=1;
  }
  else fp=fopen(filename,"wb");

  if (!fp)
    set_error(imWRITE_ERROR);
  else
  {
    fprintf(fp,"/* File produced by Satan Paint (c) 1994 Jonathan Clark */\n\n");
    if (!append)
    {
      fprintf(fp,"unsigned char %s_palette[256*3] = {\n    ",tmp_name);
      unsigned char *p=(unsigned char *)pal->addr();
      for (i=0;i<768;i++,p++)
      {
				fprintf(fp,"%d",(int)*p);
				if (i==767) 
	        fprintf(fp,"};\n\n");
				else if (i%15==14)
					fprintf(fp,",\n    ");
        else
        	fprintf(fp,", ");
      }
    }
    fprintf(fp,"unsigned char %s[%d*%d]={\n    ",tmp_name,
            im->width(),im->height()); 
    int x,y,max=im->width()*im->height()-1;
    for (y=0,i=0;y<im->height();y++)
      for (x=0;x<im->width();x++,i++)
      {
        fprintf(fp,"%d",(int)im->pixel(x,y));
        if (i==max)
          fprintf(fp,"};\n\n");
        else
          if (i%15==14)
            fprintf(fp,",\n    ");
          else fprintf(fp,", ");
      }
  }
  fclose(fp);
}

