#include "pcxread.hpp"
#include "specs.hpp"

struct PCX_header_type
{
  char manufactururer,version,encoding,bits_per_pixel;
  short xmin,ymin,xmax,ymax,hres,vres;
  char palette[48];
  char reserved,color_planes;
  short bytes_per_line,palette_type;
  char filter[58];
} PCX_header;

int read_PCX_header(FILE *fp)
{
  if (!fread(&PCX_header.manufactururer,1,1,fp)) return 0;  
  if (!fread(&PCX_header.version,1,1,fp)) return 0;  
  if (!fread(&PCX_header.encoding,1,1,fp)) return 0;  
  if (!fread(&PCX_header.bits_per_pixel,1,1,fp)) return 0;  
  PCX_header.xmin=read_short(fp);
  PCX_header.ymin=read_short(fp);  
  PCX_header.xmax=read_short(fp);
  PCX_header.ymax=read_short(fp);
  PCX_header.hres=read_short(fp);
  PCX_header.vres=read_short(fp);
  if (!fread(PCX_header.palette,1,48,fp)) return 0;
  if (!fread(&PCX_header.reserved,1,1,fp)) return 0;
  if (!fread(&PCX_header.color_planes,1,1,fp)) return 0;
  PCX_header.bytes_per_line=read_short(fp); 
  PCX_header.palette_type=read_short(fp); 
  if (!fread(PCX_header.filter,1,58,fp)) return 0;
  return 1;
}

int write_PCX_header(FILE *fp)
{
  if (!fwrite(&PCX_header.manufactururer,1,1,fp)) return 0;  
  if (!fwrite(&PCX_header.version,1,1,fp)) return 0;  
  if (!fwrite(&PCX_header.encoding,1,1,fp)) return 0;  
  if (!fwrite(&PCX_header.bits_per_pixel,1,1,fp)) return 0;  
  write_short(fp,PCX_header.xmin);
  write_short(fp,PCX_header.ymin);  
  write_short(fp,PCX_header.xmax);
  write_short(fp,PCX_header.ymax);
  write_short(fp,PCX_header.hres);
  write_short(fp,PCX_header.vres);
  if (!fwrite(PCX_header.palette,1,48,fp)) return 0;
  if (!fwrite(&PCX_header.reserved,1,1,fp)) return 0;
  if (!fwrite(&PCX_header.color_planes,1,1,fp)) return 0;
  write_short(fp,PCX_header.bytes_per_line); 
  write_short(fp,PCX_header.palette_type); 
  if (!fwrite(PCX_header.filter,1,58,fp)) return 0;
  return 1;
}



PCX_type PCX_file_type(char *filename)
{
  FILE *fp=fopen(filename,"rb");  
  if (!fp)
  { set_error(imREAD_ERROR);  
    return not_PCX;
  }
  
  if (!read_PCX_header(fp))     
  {
    fclose(fp);    
    set_error(imREAD_ERROR);  
    return not_PCX;    
  }
  fclose(fp);      
  if (PCX_header.manufactururer!=10) 
    return not_PCX;  
  if (PCX_header.color_planes==3 && PCX_header.bits_per_pixel==8)
    return PCX_24;
  else if (PCX_header.color_planes==1 && PCX_header.bits_per_pixel==8)
    return PCX_8;
  else return not_PCX;    
}

void read_PCX_line(FILE *fp, unsigned char *start, short skip, int width)
{
  int c,n=0,i;
  
  do
  {
    c=fgetc(fp)&0xff;
    if ((c&0xc0)==0xc0)
    {
      i=c&0x3f;
      c=fgetc(fp);
      while (i--) 
      {
	*start=c;
	start+=skip;
        n++;	
      }
    }
    else
    {
      *start=c;
      start+=skip;
      n++;
    }    
  } while (n<width);    
}


image24 *read_PCX24(char *filename)
{
  if (PCX_file_type(filename)!=PCX_24) return NULL;  
  FILE *fp=fopen(filename,"rb");
  read_PCX_header(fp);  
  image24 *im=new image24(PCX_header.xmax-PCX_header.xmin+1,
			  PCX_header.ymax-PCX_header.ymin+1);
  int y;
  for (y=0;y<im->height();y++)
  {
    read_PCX_line(fp,im->scan_line(y),3,PCX_header.bytes_per_line);
    read_PCX_line(fp,im->scan_line(y)+1,3,PCX_header.bytes_per_line);
    read_PCX_line(fp,im->scan_line(y)+2,3,PCX_header.bytes_per_line);    
  }
  fclose(fp);
  return im;  
}

image *read_PCX(char *filename, palette *&pal)
{
  if (PCX_file_type(filename)!=PCX_8) return NULL;  
  FILE *fp=fopen(filename,"rb");
  read_PCX_header(fp);
  
  image *im=new image(PCX_header.xmax-PCX_header.xmin+1,
		      PCX_header.ymax-PCX_header.ymin+1);
  int y;
  for (y=0;y<im->height();y++)
    read_PCX_line(fp,im->scan_line(y),1,PCX_header.bytes_per_line);
  unsigned char palette_confirm;  
  if (!fread(&palette_confirm,1,1,fp) || palette_confirm!=12)
  {    
    pal=new palette;
    pal->defaults();
  }
  else
  {
    pal=new palette;
    fread(pal->addr(),1,256*3,fp);
  } 
  fclose(fp);
  return im;  
}  

void write_PCX(image *im, palette *pal, char *filename)
{
  FILE *fp=fopen(filename,"wb");
  if (!fp) 
  {
    set_error(imWRITE_ERROR);
    return ;
  }
  
  
  PCX_header.manufactururer=10;  
  PCX_header.version=5;  
  PCX_header.encoding=1;
  PCX_header.bits_per_pixel=8;
  PCX_header.xmin=0;
  PCX_header.ymin=0;
  PCX_header.xmax=im->width()-1;
  PCX_header.ymax=im->height()-1;
  PCX_header.hres=320;
  PCX_header.vres=200;
  PCX_header.reserved=0;
  PCX_header.color_planes=1;
  PCX_header.bytes_per_line=im->width();
  PCX_header.palette_type=0;
  memset(PCX_header.filter,0,58);


  if (!write_PCX_header(fp)) 
  {
    set_error( imWRITE_ERROR);
    return ;  
  }
  
  int y,run_length,x;
  unsigned char *sl,code;  
  for (y=0;y<im->height();y++)
  {
    sl=im->scan_line(y);
    for (x=0;x<im->width();)
    {
      run_length=1;      
      while (x+run_length<im->width() && sl[x]==sl[x+run_length])
        run_length++;
      if (run_length==1 && sl[x]<64) 
        fputc(sl[x],fp);      
      else
      {
        if (run_length>=64)
	  run_length=63;
	code=0xc0 | run_length;
	fputc(code,fp);
	fputc(sl[x],fp);
	
      }      
      x+=run_length;
      
    }    
  }
  fputc(12,fp);  // note that there is a palette attached
  fwrite(pal->addr(),1,256*3,fp);
  fclose(fp);    
}



