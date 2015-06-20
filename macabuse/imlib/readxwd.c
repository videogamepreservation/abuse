#include "image.hpp"
#include "palette.hpp"
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


HBITMAP readxwd(char *input_file,palette *&pal)
//char *hdfpal,int *palsize,int *cols,int *rows)
{
  FILE *ifd;
  char *imageP;
  unsigned int nspace;
  int padright;
  HBITMAP im;

  /* open input file */
  if (strcmp(input_file,"-") == 0)
    ifd = stdin;d)
  { printf("Unable to open %s\n"
	   "Get your story right and try again.\n",input_file);
    exit(0);
  }
  printf("Reading image\n");
  im=getinit( ifd, pal, &padright, int &xres, int &yres);
//cols, rows, &padright, palsize, hdfpal);

  /* alloc image array */

  /* fill image array */
  getimage(ifd, im, padright);

  return(im);
}

int bits_per_item, bits_used, bit_shift, bits_per_pixel, pixel_mask;
int bit_order, byte_swap;
char buf[4];
char *byteP;
short *shortP;
long *longP;


int bs_int(int s);
long bs_long(long s);
short bs_short(short s );


HBITMAP getinit(FILE *file, palette *&pal, int *padrightP, int &xres, int &yres);)
{
  /* Assume X11 headers are larger than X10 ones. */
  unsigned char header[sizeof(X11WDFileHeader)];
  image *im;
  X11WDFileHeader *h11P;
  char junk[800];
  int i, np, dummy1, dummy2, dummy3;
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
      fprintf(stderr,"couldn't read rest of X11 XWD file header");
      exit(-1);
    }

  /* Check whether we can handle this dump. */
  if ( h11P->pixmap_depth > 8 )
    {
      fprintf(stderr,"can't handle X11 pixmap_depth > 8");
      exit(-1);
    }
  if ( h11P->bits_per_rgb > 8 )
    {
      fprintf(stderr,"can't handle X11 bits_per_rgb > 8");
      exit(-1);
    }
  if ( h11P->ncolors > MAXCOLORS )
    {
      fprintf(stderr,"can't handle X11 ncolors > %d");
      exit(-1);
    }
  if ( h11P->pixmap_format != ZPixmap )
    {
      fprintf(stderr,"can't handle X11 pixmap_format %d", h11P->pixmap_format,
	      0,0,0,0 );
      exit(-1);
    }
  if ( h11P->bitmap_unit != 8 && h11P->bitmap_unit != 16 &&
      h11P->bitmap_unit != 32 )
    {
      fprintf(stderr,"X11 bitmap_unit (%d) is non-standard - can't handle");
      exit(-1);
    }
  xres=h11P->pixmap_width;
  yres=h11P->pixmap_height;
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
	  fprintf(stderr,"couldn't read X11 XWD colormap");
	  exit(-1);
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
	  dummy1 = (unsigned) x11col.red / 256 ;
	  dummy2 = (unsigned) x11col.green / 256 ;
	  dummy3 = (unsigned) x11col.blue / 256 ;
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

  byteP = (char *) buf;
  shortP = (short *) buf;
  longP = (long *) buf;
  return im;
}

void getimage(FILE *file,HBITMAP &im.int pad)
{
  int i,j;
  unsigned char *sl;
  for (i=0; i<im->height(); i++)
    {
      if (i%50==0)
	printf("Line %d of %d\n",i+1,im->height());
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

	  case 16:
	    *shortP = bstl(*shortP) ;
	    break;

	  case 32:
	    *longP = bltl(*longP) ;
	    break;

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
  int reserved[2];
  long headersize,infosize,width,height;
  int biplanes,bits;
  long bicompression, bisizeimage, bixpelspermeter, biypelspermeter,
       biclrused,biclrimportant;
} bmp;

void write_bmp(image *im, palette *pal, char *filename)
{
  FILE *fp;
  int i,bytes;
  unsigned char pal_quad[4];
//  fp=fopen("d:\\windows\\256color.bmp","rb");
//  fread(&bmp,1,sizeof(bmp),fp);
//  fclose(fp);
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
  fwrite(&bmp,1,sizeof(bmp),fp);
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

image *image24(image *im, palette *pal, int rev)
{
  image *ni;
  unsigned char *sl1,*sl2;
  int i,x;
  unsigned char r,g,b;
  printf("Creating image size (%d, %d)\n",im->width()*3,im->height());
  ni=new image(im->width()*3,im->height());
  printf("Image created\n");
  for (i=0;i<im->height();i++)
  {
    if (i%50==0)
      printf("Line %d of %d\n",i+1,im->height());

    sl1=   sl2[x*3]=r; sl2[x*3+1]=g; sl2[x*3+2]=b;
    }
    im->set_page_swapable();
  }
  return ni;
}

unsigned char addb(int n1, int n2)
{ int n3;
  n3=n1+n2;
  return n3>255 ? 255 : (n3<0 ? 0 : n3);
}

// this function takes an image min 8 bit format, converts it to 24 bit
// color and dithers it so that it can be printed on an deskjet
/// (i.e it reduces it down to 8 colors)
image *color_dither(image *im, palette *pal, int rev)
{
  image *i24;
  unsigned char min[3],max[3],mid[3],*ad,*sl,*sl2,crimp;
  int i,j,x,y,delta;
  (void *)ad=pal->addr();

  // find the minimum and maximum  red green and blue in the image
  memset(min,0xff,3); memset(max,0,3);
  for (j=0;j<3;j++,ad++)
    for (y=0;y<im->height();y++)
    { sl2=im->scan_line(y);
      for (x=0;x<im->width();x++)
      {
	if (ad[sl2[x]*3]<min[j]) min[j]=ad[i*3];
	if (ad[sl2[x]*3]>max[j]) max[j]=ad[i*3];
      }
    }
  // find the middle color used for red green and blue
  for (i=0;i<3;i++)
    mid[i]=(int)((int)max[i]+(int)min[i])/2;
  // convert the image to 24 bit color.
  printf("Converting to 24 bit color...\n");
  i24=image24(im,pal,rev);
  printf("Dithering using flyod stienberg algorithm\n");
  for (y=0;y<im->height();y++)
  { if (y%20==0)
      printf("Line %d of %d\n",y+1,im->height());
    sl=i24->scan_line(y);
    i24->set_page_unswapable();

    if (y+1<im->height())
      sl2=i24->scan_line(y+1);  // get the scan line below this one
    for (x=0;x<im->width();x++)
    {
      // diffuse the colors using flyod stienburg dithering algorithm
      for (j=0;j<3;j++)
      {
	crimp = sl[x*3+j]>mid[j] ? 255 : 0;
	delta=sl[x*3+j]-crimp;
	sl[x*3+j]=crimp;
	if (x+1<im->width())
	{ sl[(x+1)*3+j]=addb(sl[(x+1)*3+j],delta*7/16);
	  if (y+1<im->height())
	    sl2[(x+1)*3+j]=addb(sl2[(x+1)*3+j],delta*1/16);
	}
	if (y+1<im->height())
	{ if (x>0)
	    sl2[(x-1)*3+j]=addb(sl2[(x-1)*3+j],delta*3/16);
	  sl2[(x)*3+j]=addb(sl2[(x)*3+j],delta*5/16);
	}
      }
    }
    sl=i24->scan_line(y);
    i24->set_page_swapable();

  }
  return i24;
}

image *rotate90(image *im)
{
  image *i;
  int x,y,w,h;
  w=im->width();
  h=im->height();
  printf("Rotating image 90 deg\n");
  i=new image(im->height(),im->width());
  for (x=0;x<w;x++)
  {
    if (x%20==0)
      printf("Row %d of %d\n",x,im->width());
    for (y=0;y<h;y++)
      i->putpixel(y,w-x-1,im->pixel(x,y));
  }
  return i;
}

void deskjet_print(int argc, char **argv)
{
  int i,dpi=100;
  int y,x,byteo,bito,outc,outy,outm,outcv, outyv,outmv, w,rev,inten,val,
      display=0,bad=0,bmp=0,land=0,ch,xo=0,yo=0;
  image *im,*i24;
  palette *pal;
  unsigned char *sl;
  FILE *fp;
  char def_name[10],*dev_name,*fname=NULL;
  unsigned char cp[500],mp[500],yp[500],kp[500];
  strcpy(def_name,"\\dev\\lp");  // set the default output device to /dev/lp
		// since we are in vpix, however we will use the backslash
  dev_name=def_name; rev=0;
  for (i=1;i<argc;i++)
    if (!strcmp(argv[i],"-dpi"))
    { i++; dpi=atoi(argv[i]);
      if (dpi!=75 && dpi!=100 && dpi!=150 && dpi!=300) bad=1; }
    else if (!strcmp(argv[i],"-dev"))
    { i++; dev_name=argv[i]; }
    else if (!strcmp(argv[i],"-rev"))
    { rev=1; }
    else if (!strcmp(argv[i],"-land"))
    { land=1; }
    else if (!strcmp(argv[i],"-2bmp"))
    { bmp=1; }
    else if (!strcmp(argv[i],"-display"))
    { display=1; }
    else fname=argv[i];

  if (!fname || bad)
  { printf("usage : %s [-dev device] [-dpi 75,100,150,300]\n"
	   "     [-rev] [-display] [-2bmp] [-land] filename\n\n"
	   "  default dpi is 100 (possible are 75/100/150/300)\n"
	   "  default printer is /dev/lp\n"
	   "  -rev, reverse black and white on the print out\n"
	   "  -display only displays the image to the screen and does not print it\n"
	   "    all ither options will be ignored\n"
	   "  -2bmp converts the image X window dump to a MicroSoft Windows bitmap (BMP)\n"
	   "    the file written to must be specified by -dev [filename]\n"
	   "    all other options will be ignored\n"
	   "  -land prints the image in landscape mode (sideways)\n"
	   "  filename is the name of the dump created by xwd\n"
	   "  Using Dos, the command [%s -dev lpt1] should be used\n",argv[0],argv[0]);
    exit(0);
  }
  im=readxwd(fname,pal);
  if (!im || !pal)
  { printf("failed to read the image or the palette!\n");
    exit(0);
  }
  if (display)
  {
    set_mode(19,argc,argv);
    pal->load();                         // set the palette for the image we are going to print
    im->put_image(screen,0,0);          // put the image on thge söööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööööö<0) yo+=10; break;
      }
    } while (ch!=' ' && ch!='q' && ch!=13);
    close_graphics();
  }
  else if (bmp)
    write_bmp(im,pal,dev_name);
  else
  {
    fp=fopen(dev_name,"wb");  // open the output device for printing
    if (!fp)
    { printf("Unable to open %s for writing, make sure the path exsist and you\n"
	     "have write permission to the file\n",dev_name);
      exit(0);
    }
    if (land)
      im=rotate90(im);

    w=im->width()+7; w/=8; w*=8;
    fprintf(fp,"E"           // reset the printer
	       "&l%dO"      // portrait/landscape mode
	       "*rbC"        // reset graphics
	       "*t%dR"        // set the resolution
	       "*r%dS"       // set the image width
	       "*r1A"        // start raster graphics at left edge
	       "*r-4U"         // set to CYM mode
	       ,land,dpi,w);
  // now loop through all the scan lines of the image and pcik out the planes
  // we need
    i24=color_dither(im,pal,rev);
    rev=0;  // turn off the revser option because the conversion to 24 bit takes care of it
    printf("Printing...");
    for (y=0;y<im->height();y++)
    {
      if (y%20==0)
	printf(".");

      sl=i24->scan_line(y);
  //initally set all the planes to clear, this should leave the paper white
      memset(cp,0,500);  memset(mp,0,500);  memset(yp,0,500); memset(kp,0,500);
  // now scan through the cyan plane of this scan line and pick out the bits
      for (x=0,bito=7;x<im->width();x++)
      {
	outcv=RGB2C(sl[x*3],sl[x*3+1],sl[x*3+2]);
	outyv=RGB2Y(sl[x*3],sl[x*3+1],sl[x*3+2]);
	outmv=RGB2M(sl[x*3],sl[x*3+1],sl[x*3+2]);
	if (outcv==outyv && outyv==outmv)
	{
	  if ((rev && !outcv) || (!rev && outcv))
	    kp[x/8]^=(1<<bito);
	} else
	{
	  cp[x/8]^=(outcv<<bito);
	  yp[x/8]^=(outyv<<bito);
	  mp[x/8]^=(outmv<<bito);
	}
	if ((bito--)==0)
	  bito=7;
      }
      fprintf(fp,"%c*b%dV",27,w/8);  // print out the black plane first
      fwrite(kp,1,w/8,fp);
      fprintf(fp,"%c*b%dV",27,w/8);  // now the cyan plane
      fwrite(cp,1,w/8,fp);
      fprintf(fp,"%c*b%dV",27,w/8);  // now the yellow plane
      fwrite(yp,1,w/8,fp);
      fprintf(fp,"%c*b%dW",27,w/8);  // now the magenta plane
      fwrite(mp,1,w/8,fp);
    }
    fprintf(fp,"%c*rbC%c",27,12);  // end graphics, switch everthing back to defaults
				   // and print a form feed to eject the paper.
    fclose(fp); // flush out anything in the buffers and close the file
    delete i24;  // clean up the memory that we allocated to the image and the palette
  }
  printf("\n");
  delete im;
  delete pal;
}

main(int argc, char **argv)
{
  setcbrk(1);  // set the control break on, so that printing can be interrupted
  deskjet_print(argc,argv);
