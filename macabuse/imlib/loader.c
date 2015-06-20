#include "loader.hpp"
#include "image.hpp"
#include "palette.hpp"
#include "mdlread.hpp"
#include "ppmread.hpp"
#include "xwdread.hpp"
#include "glread.hpp"
#include "pcxread.hpp"
#include "lbmread.hpp"


int tell_color_size(char *filename)
{
  if (bmp_bits(filename)==24) return 24;  
  if (PCX_file_type(filename)==PCX_24) return 24;
  clear_errors();  
  return 8;  
}

image24 *load_any24(char *filename)
{
  if (bmp_bits(filename)==24)
    return read_bmp24(filename);
  else if (PCX_file_type(filename)==PCX_24)
    return read_PCX24(filename); 
  else return NULL;
  
}


// if total_read returned<0 then, this function returns a error message number
short load_any(char *filename, image **&images, palette *&pal, short &total_read)
{
  pal=NULL; images=NULL; total_read=0;
  switch (tell_file_type(filename))
  {
    case LOADER_not_supported : break;
    case LOADER_mdl :  
      total_read=mdl_total_images(filename);
      if (!current_error())
        images=read_mdl(filename,pal,1,total_read,total_read);
      break;
    case LOADER_xwd : 
      images=(image **)jmalloc(sizeof(image *),"loader::xwd image * array");
      total_read=1; 
      images[0]=readxwd(filename,pal);
      break;
    case LOADER_ppm :  
      images=(image **)jmalloc(sizeof(image *),"loader::ppm image * array");
      total_read=1;
      pal=new palette;
      images[0]=read_ppm(filename,pal,PPM_REG);
      break;
    case LOADER_pic :  
      images=(image **)jmalloc(sizeof(image *),"loader::pic image * array");
      total_read=1;
      pal=NULL;
      images[0]=read_pic(filename,pal);
      break;
    case LOADER_bmp8 :  
      images=(image **)jmalloc(sizeof(image *),"loader::bmp image * array");
      total_read=1;
      pal=NULL;
      images[0]=read_bmp(pal,filename);
      break;
    case LOADER_pcx8 :  
      images=(image **)jmalloc(sizeof(image *),"loader::pcx image * array");
      total_read=1;
      pal=NULL;
      images[0]=read_PCX(filename,pal);    
      break;   
    case LOADER_lbm :
      images=(image **)jmalloc(sizeof(image *),"loader::pcx image * array");
      total_read=1;
      pal=NULL;
      images[0]=read_lbm(filename,pal);    
      break;
    default :
      set_error(imNOT_SUPPORTED);
  }    
  if (current_error())
  {
    short i;    
    i=current_error();
    set_error(0);
    return i;
  }
  return 0;
}


graphics_type tell_file_type(char *filename)
{
  FILE *fp;
  unsigned char header[10];
  fp=fopen(filename,"rb");
  if (!fp)
    return LOADER_not_supported;
  else if (fread(header,1,12,fp)!=12)
  { 
    fclose(fp);
    return LOADER_not_supported;    
  }
  fclose(fp);


  if (header[0]=='J' && header[1]=='C'
      && header[2]=='2' && header[3]=='0')
    return LOADER_mdl;
  else if (header[4]==0 && header[5]==0 && header[6]==0 &&
           header[7]==7)
    return LOADER_xwd;
  else if (header[0]=='P' && header[1]=='6')
    return LOADER_ppm;
  else if (header[10]==8 && header[11]==0xff)
    return LOADER_pic;
  else if (header[0]=='F' && header[1]=='O' && header[2]=='R' && header[3]=='M')
    return LOADER_lbm;
  else if (header[0]=='B' && header[1]=='M') 
  {
    switch (tell_color_size(filename))
    {
      case 24 : return LOADER_bmp24; break;      
      case 8 : return LOADER_bmp8; break;	
      default : return LOADER_not_supported; break;
    }
  } else if (header[0]==10)
  {  
    switch (tell_color_size(filename))
    {
      case 24 : return LOADER_pcx24; break;      
      case 8 : return LOADER_pcx8; break;	
      default : return LOADER_not_supported; break;
    }
  }
  return LOADER_not_supported;
}












