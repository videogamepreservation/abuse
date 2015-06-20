/* readxwd.c */
/* This program is limited to X11 format XWD files */
#include "filter.hpp"
#include "system.h"
#include "image.hpp"
#include "palette.hpp"
#include "event.hpp"
#include "video.hpp"
#include "dos.h"
#include "main.hpp"
#include <stdio.h>
#include "macs.hpp"
#include "image24.hpp"

#define MAXCOLORS 256  
//#define MC(x) (((x)<3)==0)
#define MC(x) (x!=0)

// the following table converts rgb to cym.
int ca[8]={1,1,1,1,0,0,0,0},
    ma[8]={1,0,1,0,1,0,1,0},
    ya[8]={1,1,0,0,1,1,0,0};
// the following defines make a color threshold
#define RGB2C(r,g,b) (char)(ca[MC(b)|MC(g)<<1|MC(r)<<2])
#define RGB2Y(r,g,b) (char)(ya[MC(b)|MC(g)<<1|MC(r)<<2])
#define RGB2M(r,g,b) (char)(ma[MC(b)|MC(g)<<1|MC(r)<<2])

#define LSBFirst	0
#define MSBFirst	1

#define XYBitmap	0
#define XYPixmap	1
#define ZPixmap		2

#define StaticGray	0
#define GrayScale	1
#define StaticColor	2
#define PseudoColor	3
#define TrueColor	4
#define DirectColor	5

typedef unsigned long xwdval;
#define X11WD_FILE_VERSION 7
typedef struct {
    xwdval header_size;		/* Size of the entire file header (bytes). */
    xwdval file_version;	/* X11WD_FILE_VERSION */
    xwdval pixmap_format;	/* Pixmap format */
    xwdval pixmap_depth;	/* Pixmap depth */
    xwdval pixmap_width;	/* Pixmap width */
    xwdval pixmap_height;	/* Pixmap height */
    xwdval xoffset;		/* Bitmap x offset */
    xwdval byte_order;		/* MSBFirst, LSBFirst */
    xwdval bitmap_unit;		/* Bitmap unit */
    xwdval bitmap_bit_order;	/* MSBFirst, LSBFirst */
    xwdval bitmap_pad;		/* Bitmap scanline pad */
    xwdval bits_per_pixel;	/* Bits per pixel */
    xwdval bytes_per_line;	/* Bytes per scanline */
    xwdval visual_class;	/* Class of colormap */
    xwdval red_mask;		/* Z red mask */
    xwdval green_mask;		/* Z green mask */
    xwdval blue_mask;		/* Z blue mask */
    xwdval bits_per_rgb;	/* Log base 2 of distinct color values */
    xwdval colormap_entries;	/* Number of entries in colormap */
    xwdval ncolors;		/* Number of Color structures */
    xwdval window_width;	/* Window width */
    xwdval window_height;	/* Window height */
    long window_x;		/* Window upper left X coordinate */
    long window_y;		/* Window upper left Y coordinate */
    xwdval window_bdrwidth;	/* Window border width */
    } X11WDFileHeader;

typedef struct {
    unsigned long pixel;
    unsigned short red, green, blue;
    char flags;			/* do_red, do_green, do_blue */
    char pad;
    } X11XColor;

image *getinit(FILE *file, palette *&pal, int *padrightP);
void getimage(FILE *file,image *image, int pad);
int getpixnum(FILE *file);


image *readxwd(char *input_file,palette *&pal)
//char *hdfpal,int *palsize,int *cols,int *rows)
{
  FILE *ifd;
  int padright;
  image *im;
#ifdef __WINDOWS
   HCURSOR OldCur;
   OldCur=SetCursor(LoadCursor(0,IDC_WAIT)); // set the cursor to the hour glass
#endif
  /* open input file */
  if (strcmp(input_file,"-") == 0)
    ifd = stdin;
  else
    ifd = fopen(input_file,"rb");
  /* get X info from file */
  if (!ifd)
  {
#ifndef __WINDOWS    
    printf("Unable to open %s\n"
	   "Get your story strait and try again.\n",input_file);
    exit(0);
#endif
  }
  im=getinit( ifd, pal, &padright);
//cols, rows, &padright, palsize, hdfpal);

  /* alloc image array */

  /* fill image array */
  getimage(ifd, im, padright);
#ifdef __WINDOWS
   SetCursor(OldCur); // set the cursor to the hour glass
#endif

  return(im);
}

int bits_per_item, bits_used, bit_shift, bits_per_pixel, pixel_mask;
int bit_order, byte_swap;
char buf[4];
unsigned char *byteP;
unsigned short *shortP;
unsigned long *longP;


int bs_int(int s);
long bs_long(long s);
short bs_short(short s );


image *getinit(FILE *file, palette *&pal, int *padrightP)
{
  /* Assume X11 headers are larger than X10 ones. */
  unsigned char header[sizeof(X11WDFileHeader)];
  image *im;
  X11WDFileHeader *h11P;
  char junk[800];
  int i, dummy1, dummy2, dummy3;
  unsigned  short minred, maxred;
  X11XColor x11col;

  h11P = (X11WDFileHeader *) header;

  if (fread(header,sizeof(*h11P),1,file) != 1 )
    {
      fprintf(stderr,"couldn't read X11 XWD file header");
      exit(-1);
    }
  if ( h11P->file_version != X11WD_FILE_VERSION )
    {
      byte_swap = 1;
      h11P->file_version = bltl(h11P->file_version);
      if (h11P->file_version != X11WD_FILE_VERSION)
      {
        set_error(imINCORRECT_FILETYPE);
        return NULL;
      }

      h11P->header_size = bltl(h11P->header_size);
      h11P->file_version = bltl(h11P->file_version);
      h11P->pixmap_format = bltl(h11P->pixmap_format);
      h11P->pixmap_depth = bltl(h11P->pixmap_depth);
      h11P->pixmap_width = bltl(h11P->pixmap_width);
      h11P->pixmap_height = bltl(h11P->pixmap_height);
      h11P->xoffset =  bltl(h11P->xoffset);
      h11P->byte_order =  bltl(h11P->byte_order);
      h11P->bitmap_unit = bltl(h11P->bitmap_unit);
      h11P->bitmap_bit_order = bltl(h11P->bitmap_bit_order);
      h11P->bitmap_pad = bltl(h11P->bitmap_pad);
      h11P->bits_per_pixel = bltl(h11P->bits_per_pixel);
      h11P->bytes_per_line = bltl(h11P->bytes_per_line);
      h11P->visual_class = bltl(h11P->visual_class);
      h11P->red_mask = bltl(h11P->red_mask);
      h11P->green_mask = bltl(h11P->green_mask);
      h11P->blue_mask = bltl(h11P->blue_mask);
      h11P->bits_per_rgb = bltl(h11P->bits_per_rgb);
      h11P->colormap_entries = bltl(h11P->colormap_entries);
      h11P->ncolors = bltl(h11P->ncolors);
      h11P->window_width = bltl(h11P->window_width);
      h11P->window_height = bltl(h11P->window_height);
      h11P->window_x = bltl(h11P->window_x);
      h11P->window_y = bltl(h11P->window_y);
      h11P->window_bdrwidth = bltl(h11P->window_bdrwidth);
    }

  if ( fread( junk, 1, h11P->header_size - sizeof(*h11P), file ) !=
      h11P->header_size - sizeof(*h11P) )
    {
      fclose(file);
      set_error(imFILE_CORRUPTED);
      return NULL;
    }

  /* Check whether we can handle this dump. */
  if ( h11P->pixmap_depth > 8 )
    {
      fclose(file);
      set_error(imNOT_SUPPORTED);
      return NULL;
    }
  if ( h11P->bits_per_rgb > 8 )
    {
      fclose(file);
      set_error(imNOT_SUPPORTED);
      return NULL;
    }
  if ( h11P->ncolors > MAXCOLORS )
    {
      fclose(file);
      set_error(imNOT_SUPPORTED);
      return NULL;
    }
  if ( h11P->pixmap_format != ZPixmap )
    {
      fclose(file);
      set_error(imNOT_SUPPORTED);
      return NULL;
    }
  if ( h11P->bitmap_unit != 8 && h11P->bitmap_unit != 16 &&
      h11P->bitmap_unit != 32 )
    {
      fclose(file);
      set_error(imNOT_SUPPORTED);
      return NULL;
    }
  im=new image(h11P->pixmap_width,h11P->pixmap_height);
  pal=new palette(h11P->colormap_entries);
  *padrightP = h11P->bytes_per_line * 8 / h11P->bits_per_pixel - im->width();

/*****************************************************************************/
  /* Read X11 colormap. */
minred = 65535;
maxred = 0;
  for ( i = 0; i < h11P->colormap_entries; i++ )
    {
      if ( fread( &x11col, sizeof(X11XColor), 1, file ) != 1 )
	{
          fclose(file);
          set_error(imFILE_CORRUPTED);
          delete pal;
          return NULL;
	}
	else
	{
	  x11col.pixel=bltl(x11col.pixel);
	  x11col.red=bstl(x11col.red);
	  x11col.green=bstl(x11col.green);
	  x11col.blue=bstl(x11col.blue);
	}
      if (x11col.pixel < 256)
	{
	  if (minred > x11col.red) minred = x11col.red;
	  if (maxred < x11col.red) maxred = x11col.red;
	  dummy1 = (unsigned) x11col.red>>8;
	  dummy2 = (unsigned) x11col.green>>8;
	  dummy3 = (unsigned) x11col.blue>>8;
	  pal->set(i,dummy1,dummy2,dummy3);
	}
      else
	{
	  fprintf(stderr,"pixel value outside of valid HDF palette\n");
	  exit(-1);
	}
    }
  /* rest of stuff for getpixnum */
  bits_per_item = h11P->bitmap_unit;
  bits_used = bits_per_item;
  bits_per_pixel = h11P->bits_per_pixel;
  bit_order = h11P->bitmap_bit_order;
  pixel_mask = ( 1 << bits_per_pixel ) - 1;

  byteP = (unsigned char *) buf;
  shortP = (unsigned short *) buf;
  longP = (unsigned long *) buf;
  return im;
}

void getimage(FILE *file,image *im,int pad)
{
  int i,j;
  unsigned char *sl;
#if BYTE_ORDER!=BIG_ENDIAN
  printf("little guys\n");
#endif
  printf("getting image, bits_per_item = %d %d %d\n",bits_per_item,bits_used,bit_order==MSBFirst);
  for (i=0; i<im->height(); i++)
    {
      sl=im->scan_line(i);
      for (j=0; j<im->width(); j++)
	sl[j]= getpixnum(file);

      for ( j = 0; j < pad; j++ )
	getpixnum( file );
    }
}


int getpixnum(FILE *file)
{
  int p;
  if ( bits_used == bits_per_item )
    {
      if ( fread( buf, bits_per_item / 8, 1, file ) != 1 )
	fprintf(stderr, "couldn't read bits" );
      if ( byte_swap )
	switch ( bits_per_item )
	  {
	  case 8:
	    break;

	  case 16: *shortP=short_swap(*shortP); break;

	  case 32: *longP=long_swap(*longP); break;

	  default:
	    fprintf(stderr, "can't happen" );
	  }
      bits_used = 0;

//      if ( bit_order == MSBFirst )
	bit_shift = bits_per_item - bits_per_pixel;
//      else
//	bit_shift = 0;
    }

  switch ( bits_per_item )
    {
    case 8:
      p = ( *byteP >> bit_shift) & pixel_mask;
      break;

    case 16:
      p = ( *shortP >> bit_shift) & pixel_mask;
      break;

    case 32:
      p = ( *longP >> bit_shift) & pixel_mask;
      break;

    default:
      fprintf(stderr, "can't happen" );
    }

//  if ( bit_order == MSBFirst )
    bit_shift -= bits_per_pixel;
//  else
//    bit_shift += bits_per_pixel;
  bits_used += bits_per_pixel;

  return p;
}


short bs_short(short s )
{
  short ss;
  unsigned char *bp, t;

  ss = s;
  bp = (unsigned char *) &ss;
  t = bp[0];
  bp[0] = bp[1];
  bp[1] = t;
  return ss;
}

int bs_int(int i )
{
  int ii;
  unsigned char *bp, t;

  ii = i;
  bp = (unsigned char *) &ii;
  t = bp[0];
  bp[0] = bp[3];
  bp[3] = t;
  t = bp[1];
  bp[1] = bp[2];
  bp[2] = t;
  return ii;
}

long bs_long(long l )
{
  return bs_int( l );
}

struct BMP_header
{
  char id[2];
  long filesize;
  short reserved[2];
  long headersize,infosize,width,height;
  short biplanes,bits;
  long bicompression, bisizeimage, bixpelspermeter, biypelspermeter,
       biclrused,biclrimportant;
} bmp;

int read_BMP_header(FILE *fp)
{
  if (!fread(&bmp.id,1,2,fp)) return 0;         // 2 0
  bmp.filesize=read_long(fp);                   // 4 4
  if (!fread(bmp.reserved,1,4,fp)) return 0;    // 4 8
  bmp.headersize=read_long(fp);                 // 4 12
  bmp.infosize=read_long(fp);                   // 4 16
  bmp.width=read_long(fp);                      // 4 20
  bmp.height=read_long(fp);                     // 4 24
  bmp.biplanes=read_short(fp);                  // 2 26
  bmp.bits=read_short(fp);                      // 2 28
  bmp.bicompression=read_long(fp);              // 4 32
  bmp.bisizeimage=read_long(fp);                // 4 36
  bmp.bixpelspermeter=read_long(fp);            // 4 40
  bmp.biypelspermeter=read_long(fp);            // 4 44
  bmp.biclrused=read_long(fp);                  // 4 48
  bmp.biclrimportant=read_long(fp);             // 4 52
  return 1;
  
}




int bmp_bits(char *filename)
{
  FILE *fp;
  fp=fopen(filename,"rb");
  if (!fp) return 0;
  if (!read_BMP_header(fp)) return 0; 
  fclose(fp);

  if (bmp.id[0]!='B' || bmp.id[1]!='M')  
    return 0;
  else return bmp.bits;
}

image24 *read_bmp24(char *filename)
{
  image24 *im;
  FILE *fp;
  int i,j;
  fp=fopen(filename,"rb");
  if (!fp)
    return NULL;
  if (!read_BMP_header(fp)) return NULL;
  
  if (bmp.id[0]!='B' || bmp.id[1]!='M')  
    return NULL;

  if (bmp.bits!=24)
    return NULL;
  
  im=new image24((bmp.width+3)&(0xffffffff-3),
		 (bmp.height+3)&0xffffffff-3);
  if (!im)
    return NULL;
  
  unsigned char *sl;
  int trailer=im->width()%4;
  if (trailer==1) trailer=3;
  else if (trailer==3) trailer=1;
  uchar buf[9];
  for (i=im->height();i;i--)
  {
    sl=im->scan_line(i-1);
    for (j=0;j<im->width();j++)    
    { 
      fread(sl+2,1,1,fp);
      fread(sl+1,1,1,fp);      
      fread(sl,1,1,fp); 
      sl+=3;  
    }    
    if (trailer)
      fread(buf,trailer*3,1,fp);
  }
  fclose(fp);

  return im; 
}

image *read_bmp(palette *&pal, char *filename)
{
  image *im;
  FILE *fp;
  unsigned char pal_quad[4];
  char *scrap;
  int bytes,i;
  fp=fopen(filename,"rb");
  if (!fp)
    return NULL;

  if (!read_BMP_header(fp)) return NULL;
  
  if (bmp.id[0]!='B' || bmp.id[1]!='M')  
    return NULL;

  if (bmp.bits!=8)
    return NULL;
  
  im=new image((bmp.width+3)&(0xffffffff-3),
		 (bmp.height+3)&0xffffffff-3);

  if (!im)
    return NULL;

  pal=new palette(256);
  for (i=0;i<256;i++)
  {
    fread(pal_quad,1,4,fp);
    pal->set(i,pal_quad[2],pal_quad[1],pal_quad[0]);
  }
  bytes=(im->width()+3)/4;
  bytes*=4;
  scrap=(char *)jmalloc(bytes,"xwd_read scrap");
  for (i=im->height();i;i--)
  {
    fread(scrap,1,bytes,fp);
    memcpy(im->scan_line(i-1),scrap,im->width());
  }
  jfree(scrap);
  fclose(fp);
  return im;
}

int write_BMP_header(FILE *fp)
{
  if (!fwrite(&bmp.id,1,2,fp)) return 0;  
  write_long(fp,bmp.filesize);  
  if (!fwrite(bmp.reserved,1,4,fp)) return 0;   
  write_long(fp,bmp.headersize);
  write_long(fp,bmp.infosize);  
  write_long(fp,bmp.width);
  write_long(fp,bmp.height);
  write_short(fp,bmp.biplanes);
  write_short(fp,bmp.bits);
  write_long(fp,bmp.bicompression);
  write_long(fp,bmp.bisizeimage);
  write_long(fp,bmp.bixpelspermeter);
  write_long(fp,bmp.biypelspermeter);
  write_long(fp,bmp.biclrused);
  write_long(fp,bmp.biclrimportant);   
  return 1;
  
}

void write_bmp(image *im, palette *pal, char *filename)
{
  FILE *fp;
  int i,bytes;
  unsigned char pal_quad[4];
  fp=fopen(filename,"wb");
  if (!fp)
  { printf("Error : unable to open %s for writing!\n");
    exit(0);
  }
  bytes=(im->width()+3)/4;
  memset((char *)&bmp,0,sizeof(bmp));
  memcpy(bmp.id,"BM",2);
  bmp.width=im->width();
  bmp.height=im->height();
  bmp.bits=8;
  bmp.infosize=0x28L;
  bmp.biplanes=1;
  bmp.headersize=1078;
  bmp.filesize=bmp.headersize+bytes*im->height()*4;
  bmp.bisizeimage=im->width()*im->height();
  bmp.biclrused=256;
  bmp.biclrimportant=256;
  write_BMP_header(fp);
  
  pal_quad[3]=0;
  for (i=0;i<256;i++)
  {
    pal->get(i,pal_quad[2],pal_quad[1],pal_quad[0]);
    fwrite(pal_quad,1,4,fp);
  }
  for (i=im->height();i;i--)
    fwrite(im->scan_line(i-1),1,bytes*4,fp);
  fclose(fp);
}


