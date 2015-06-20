#include "image.hpp"
#include "video.hpp"
#include "macs.hpp"
#include "event.hpp"
#include <math.h>

#define SHB 7

class vect
{
public :
  long screenx, screeny, screendx, screendy,
       imagex,  imagey,  imagedx,  imagedy;
  short steps;
  void init(short imagex1,  short imagey1,  short imagex2, short imagey2,
       short screenx1, short screeny1, short screenx2, short screeny2);
  short step();
} ;

inline short vect::step()
{
  long oy;
  do
  {
    oy=screeny>>SHB;
    screenx+=screendx;
    screeny+=screendy;
    imagex+=imagedx;
    imagey+=imagedy;
    steps--;
  } while (steps && (screeny>>SHB)==oy);
  return steps>=0;
}

void vect::init(short imagex1,  short imagey1,  short imagex2, short imagey2,
	   short screenx1, short screeny1, short screenx2, short screeny2)
{
  steps=abs(screeny1-screeny2)+1;
  screenx=(long)screenx1<<SHB;
  screeny=(long)screeny1<<SHB;
  if (screenx2>=screenx1)
    screendx=((long)(screenx2-screenx1+1)<<(SHB*2))/((long)(steps<<SHB));
  else screendx=((long)(screenx2-screenx1-1)<<(SHB*2))/((long)(steps<<SHB));
  if (screeny2>=screeny1)
    screendy=((long)(screeny2-screeny1+1)<<(SHB*2))/((long)(steps<<SHB));
  else
    screendy=((long)(screeny2-screeny1-1)<<(SHB*2))/((long)(steps<<SHB));
  imagex =(long)imagex1<<SHB;
  imagey =(long)imagey1<<SHB;
  if (imagex2>=imagex1)
    imagedx=((long)(imagex2-imagex1)<<(SHB*2))/((long)(steps<<SHB));
  else
    imagedx=((long)(imagex2-imagex1)<<(SHB*2))/((long)(steps<<SHB));
  if (imagey2>=imagey1)
    imagedy=((long)(imagey2-imagey1)<<(SHB*2))/((long)(steps<<SHB));
  else
    imagedy=((long)(imagey2-imagey1)<<(SHB*2))/((long)(steps<<SHB));
}


void texture_map(image *screen, image *tx, short *points)
{
  vect right[2],left[2];
  unsigned char *sl;
  short l=0,r=0,y,sa,min,max,i,x1,x2,screeny;
  short cx1,cy1,cx2,cy2;
  long ix,iy,idx,idy,steps,scx;
  screen->get_clip(cx1,cy1,cx2,cy2);
  x1=0;
  x2=0;
  for (i=1;i<4;i++)
    if (points[i*2]<points[x1*2])
      x1=i;
    else if (points[i*2]>points[x2*2])
      x2=i;
  min=0;
  max=0;
  for (i=1;i<4;i++)
    if (points[i*2+1]<points[min*2+1])
      min=i;
    else if (points[i*2+1]>points[max*2+1])
      max=i;

  if (min==0)
  {
    right[0].init(0,0,tx->width()-1,0,                         points[0],points[1],points[2],points[3]);
    right[1].init(tx->width()-1,0,tx->width()-1,tx->height()-1,points[2],points[3],points[4],points[5]);
    left[0].init(0,0,0,tx->height()-1,                         points[0],points[1],points[6],points[7]);
    left[1].init(0,tx->height()-1,tx->width()-1,tx->height()-1,points[6],points[7],points[4],points[5]);
  }
  else if (min==1)
  {
    right[0].init(tx->width()-1,0,tx->width()-1,tx->height()-1, points[2],points[3],points[4],points[5]);
    right[1].init(tx->width()-1,tx->height()-1,0,tx->height()-1,points[4],points[5],points[6],points[7]);
    left[0].init(tx->width()-1,0,0,0,                           points[2],points[3],points[0],points[1]);
    left[1].init(0,0,0,tx->height()-1,                          points[0],points[1],points[6],points[7]);
  } else if (min==2)
  {
    right[0].init(tx->width()-1,tx->height()-1,0,tx->height()-1,points[4],points[5],points[6],points[7]);
    right[1].init(0,tx->height()-1,0,0,                         points[6],points[7],points[0],points[1]);

    left[0].init(tx->width()-1,tx->height()-1, tx->width()-1,0, points[4],points[5],points[2],points[3]);
    left[1].init(tx->width()-1,0,0,0,                           points[2],points[3],points[0],points[1]);
  } else
  {
    right[0].init(0,tx->height()-1,0,0,                         points[6],points[7],points[0],points[1]);
    right[1].init(0,0,tx->width()-1,0,				points[0],points[1],points[2],points[3]);

    left[0].init(0,tx->height()-1,tx->width()-1,tx->height()-1, points[6],points[7],points[4],points[5]);
    left[1].init(tx->width()-1,tx->height()-1, tx->width()-1,0, points[4],points[5],points[2],points[3]);
  }


  do
  {
    screeny=right[r].screeny>>SHB;
    if (screeny>=cy1 && screeny<=cy2)
    {
      sl=screen->scan_line(screeny);
      ix=left[l].imagex;
      iy=left[l].imagey;

      if (left[l].screenx<right[r].screenx)
      { sa=1; steps=right[r].screenx-left[l].screenx; }
      else { sa=-1; steps=left[l].screenx-right[r].screenx; }
      steps+=1<<SHB;
      if (right[r].imagex>ix)
	idx=((right[r].imagex-ix)<<SHB)/steps;
      else idx=((right[r].imagex-ix)<<SHB)/steps;

      if (right[r].imagey>iy)
	idy=((right[r].imagey-iy)<<SHB)/steps;
      else idy=((right[r].imagey-iy)<<SHB)/steps;
      scx=left[l].screenx>>SHB;

      steps=steps>>SHB;
      if (left[l].screenx<right[r].screenx)
	sa=1; else sa=-1;
      while (steps--)
      {
	if (scx>=cx1 && scx<=cx2)
	  sl[scx]=tx->scan_line(iy>>SHB)[ix>>SHB];
	ix+=idx; iy+=idy;
	scx+=sa;
      }
    }

    do
    { y=right[r].screeny>>SHB;
      if (!right[r].step())
	r++;
    } while (r!=2 && y==right[r].screeny>>SHB);

    do
    { y=left[l].screeny>>SHB;
      if (!left[l].step())
	l++;
    } while (l!=2 && y==left[l].screeny>>SHB);

  } while (r!=2 && l!=2);

}


// slower, but allows transparency textures
void clear_texture_map(image *screen, image *tx, short *points)
{
  vect right[3],left[3];
  unsigned char *sl;
  short l=0,r=0,y,sa,min,max,i,x1,x2,screeny;
  short cx1,cy1,cx2,cy2;
  long ix,iy,idx,idy,steps,scx;
  screen->get_clip(cx1,cy1,cx2,cy2);
  x1=0;
  x2=0;
  for (i=1;i<4;i++)
    if (points[i*2]<points[x1*2])
      x1=i;
    else if (points[i*2]>points[x2*2])
      x2=i;
  min=0;
  max=0;
  for (i=1;i<4;i++)
    if (points[i*2+1]<points[min*2+1])
      min=i;
    else if (points[i*2+1]>points[max*2+1])
      max=i;



  if (min==0)
  {
    right[0].init(0,0,tx->width()-1,0,                         points[0],points[1],points[2],points[3]);
    right[1].init(tx->width()-1,0,tx->width()-1,tx->height()-1,points[2],points[3],points[4],points[5]);
    right[2].init(tx->width()-1,tx->height()-1,0,tx->height()-1,points[4],points[5],points[6],points[7]);

    left[0].init(0,0,0,tx->height()-1,                         points[0],points[1],points[6],points[7]);
    left[1].init(0,tx->height()-1,tx->width()-1,tx->height()-1,points[6],points[7],points[4],points[5]);
    left[2].init(tx->width()-1,tx->height()-1,tx->width()-1,0, points[4],points[5],points[2],points[3]);
  }
  else if (min==1)
  {
    right[0].init(tx->width()-1,0,tx->width()-1,tx->height()-1, points[2],points[3],points[4],points[5]);
    right[1].init(tx->width()-1,tx->height()-1,0,tx->height()-1,points[4],points[5],points[6],points[7]);
    right[2].init(0,tx->height()-1,0,0,                         points[6],points[7],points[0],points[1]);

    left[0].init(tx->width()-1,0,0,0,                           points[2],points[3],points[0],points[1]);
    left[1].init(0,0,0,tx->height()-1,                          points[0],points[1],points[6],points[7]);
    left[2].init(0,tx->height()-1,tx->width()-1,tx->height()-1, points[6],points[7],points[4],points[5]);
  } else if (min==2)
  {
    right[0].init(tx->width()-1,tx->height()-1,0,tx->height()-1,points[4],points[5],points[6],points[7]);
    right[1].init(0,tx->height()-1,0,0,                         points[6],points[7],points[0],points[1]);
    right[2].init(0,0,tx->width()-1,0,                          points[0],points[1],points[2],points[3]);

    left[0].init(tx->width()-1,tx->height()-1, tx->width()-1,0, points[4],points[5],points[2],points[3]);
    left[1].init(tx->width()-1,0,0,0,                           points[2],points[3],points[0],points[1]);
    left[2].init(0,0,0,tx->height()-1,                          points[0],points[1],points[6],points[7]);
  } else
  {
    right[0].init(0,tx->height()-1,0,0,                         points[6],points[7],points[0],points[1]);
    right[1].init(0,0,tx->width()-1,0,				points[0],points[1],points[2],points[3]);
    right[2].init(tx->width()-1,0,tx->width()-1,tx->height(),   points[2],points[3],points[4],points[5]);

    left[0].init(0,tx->height()-1,tx->width()-1,tx->height()-1, points[6],points[7],points[4],points[5]);
    left[1].init(tx->width()-1,tx->height()-1, tx->width()-1,0, points[4],points[5],points[2],points[3]);
    left[2].init(tx->width()-1,0,0,0,                           points[2],points[3],points[0],points[1]);
  }


  do
  {
    screeny=right[r].screeny>>SHB;
    if (screeny>=cy1 && screeny<=cy2)
    {
      sl=screen->scan_line(screeny);
      ix=left[l].imagex;
      iy=left[l].imagey;

      if (left[l].screenx<right[r].screenx)
      { sa=1; steps=right[r].screenx-left[l].screenx; }
      else { sa=-1; steps=left[l].screenx-right[r].screenx; }
      steps+=1<<SHB;
      if (right[r].imagex>ix)
	idx=((right[r].imagex-ix)<<SHB)/steps;
      else idx=((right[r].imagex-ix)<<SHB)/steps;

      if (right[r].imagey>iy)
	idy=((right[r].imagey-iy)<<SHB)/steps;
      else idy=((right[r].imagey-iy)<<SHB)/steps;
      scx=left[l].screenx>>SHB;

      steps=steps>>SHB;
      if (left[l].screenx<right[r].screenx)
	sa=1; else sa=-1;
      while (steps--)
      {
	if (scx>=cx1 && scx<=cx2)
	{
	  unsigned char c=tx->scan_line(iy>>SHB)[ix>>SHB];
	  if (c!=current_background)
	    sl[scx]=c;
	}
	ix+=idx; iy+=idy;
	scx+=sa;
      }
    }

    do
    { y=right[r].screeny>>SHB;
      if (!right[r].step())
	r++;
    } while (r!=2 && y==(right[r].screeny>>SHB));

    do
    { y=left[l].screeny>>SHB;
      if (!left[l].step())
	l++;
    } while (l!=2 && y==(left[l].screeny>>SHB));

  } while ((r!=2 || l==0) && (l!=2 || r==0));

}



