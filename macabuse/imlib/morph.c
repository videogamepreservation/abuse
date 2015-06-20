#include "morph.hpp"

long trans(long x1, long x2, long frame)
{
  return (((x2-x1)<<5)*frame+(x1<<8))>>8;  
}


void jmorph::show_step_frame(image *screen, int x, int y, int frame_on,
	               color_filter *fli, palette *pal)
{
  short x1,y1,x2,y2;
  unsigned char r1,g1,b1,r2,g2,b2;
  screen->get_clip(x1,y1,x2,y2);
  
  int i;
  long xx,yy;
  
  if (small)
  {
    morph_point8 *m8=(morph_point8 *)p;
    for (i=0;i<total;i++,m8++)
    {
      xx=x+trans(m8->x1,m8->x2,frame_on);
      yy=y+trans(m8->y1,m8->y2,frame_on);
      
      if (xx>=x1 && xx<=x2 && yy>=y1 && yy<=y2)
      {	
	pal->get(m8->start_color,r1,g1,b1);
	pal->get(m8->end_color,r2,g2,b2);
  
	long r=trans(r1,r2,frame_on)>>3,
             g=trans(g1,g2,frame_on)>>3,
	     b=trans(b1,b2,frame_on)>>3;
	
        *(screen->scan_line(yy)+xx)=fli->lookup_color(r,g,b);
      }            
    }
  }    
}


void patched_morph::show_frame(image *screen, int x, int y, 
			       int frame_on, color_filter *fli, palette *pal)
{
  jmorph::show_step_frame(screen,x,y,frame_on,fli,pal);
  int tot=pats[frame_on].patches,xx,yy;
  unsigned char *p=pats[frame_on].patch_data;
  short cx1,cy1,cx2,cy2;
  screen->get_clip(cx1,cy1,cx2,cy2);    
  while (tot--)
  {
    xx=*(p++)+x;
    if (xx<cx1 || xx>cx2)
      p+=2;
    else
    {
      yy=*(p++)+y;
      if (yy<cy1 || yy>cy2)
        p++;
      else
	*(screen->scan_line(yy)+xx)=*(p++);
    }
  }      
}

void patched_morph::show_8(image *screen, int x, int y, int frame_on, color_filter *fli, palette *pal)
{
  jmorph::show_8(screen,x,y,frame_on,fli,pal);
  int tot=pats[frame_on].patches,xx,yy; 
  unsigned char *p=pats[frame_on].patch_data;
  short cx1,cy1,cx2,cy2;
  screen->get_clip(cx1,cy1,cx2,cy2);    
  while (tot--)
  {
    xx=*(p++)+x;
    if (xx<cx1 || xx>cx2)
      p+=2;
    else
    {
      yy=*(p++)+y;
      if (yy<cy1 || yy>cy2)
        p++;
      else
	*(screen->scan_line(yy)+xx)=*(p++);
    }
  }      
}




patched_morph::patched_morph(image *i1, image *hint1, image *i2, image *hint2, int aneal_steps,
	        color_filter *fli, palette *pal, int frames) : jmorph(i1,hint1,i2,hint2,aneal_steps)
{
  int i,j,w,h,x,y,tot,r,g,b,mark_color,dist;
  unsigned char *cur_patch,*sl,or,og,ob;
  
  frames=9;
  
  patches=frames;  
  pats=(morph_patch *)jmalloc(sizeof(morph_patch)*patches,"morph::patch array");
  w=max(bound_x2(0),bound_x2(1));
  h=max(bound_y2(0),bound_y2(1));    
  image *im=new image(w,h);


  for (i=0;i<patches;i++)
  {
    pats[i].patch_data=NULL;
    pats[i].patches=0;    
    if (i!=0 && i!=patches-1)
    {      
      im->clear();
      jmorph::show_step_frame(im,0,0,i,fli,pal);
      for (j=0;j<4;j++)
      {      
	for (y=0;y<h;y++)
          for (sl=im->scan_line(y),x=0;x<w;x++,sl++)
	  {	
	    mark_color=-1;
	    
	    tot=r=g=b=0;	  	  
	    if (x!=0 && *(sl-1)) 
	    { tot++; 
	      r+=pal->red(*(sl-1)); 
	      g+=pal->green(*(sl-1)); 
	      b+=pal->blue(*(sl-1));		   
	    }	  
	    if (x!=w-1 && *(sl+1)) 
	    { tot++; 
	      r+=pal->red(*(sl+1)); 
	      g+=pal->green(*(sl+1)); 
	      b+=pal->blue(*(sl+1));	
	    }	  
	    if (y!=0 && im->pixel(x,y-1)) 
	    { tot++; 
	      r+=pal->red(im->pixel(x,y-1)); 
	      g+=pal->green(im->pixel(x,y-1)); 
	      b+=pal->blue(im->pixel(x,y-1));
	    }
	    if (y!=h-1 && im->pixel(x,y+1)) 
	    { tot++; 
	      r+=pal->red(im->pixel(x,y+1)); 
	      g+=pal->green(im->pixel(x,y+1)); 
	      b+=pal->blue(im->pixel(x,y+1));
	    }

	    if (*sl && tot==0)  // kill any seperate pixels	    
	      mark_color=0;
	    else if (*sl)
	    {
	      pal->get(*sl,or,og,ob);       // see if we need to blur this on in
	      r/=tot;
	      g/=tot;
	      b/=tot;
	      
	      dist=((int)or-r)*((int)or-r)+((int)og-g)*((int)og-g)+((int)ob-b)*((int)ob-b);
	      if (i>0 && i<patches-1 && dist>3000)
	      {
//		printf("adding blur at %d %d to frame %d, dist=%d\n",x,y,i,dist);
		mark_color=fli->lookup_color(r>>3,g>>3,b>>3);	    
	      }
	    }
	    else if (*sl==0 && tot>=3)	    
	      mark_color=fli->lookup_color((r/tot)>>3,(g/tot)>>3,(b/tot)>>3);	    
	    if (mark_color>=0)
	    {	      
	      pats[i].patches++;
	      pats[i].patch_data=(unsigned char *)realloc(pats[i].patch_data,3*pats[i].patches);
	      cur_patch=pats[i].patch_data+  (pats[i].patches-1)*3;	    
	      *(cur_patch++)=x;
	      *(cur_patch++)=y;
	      *sl=mark_color;	      
	      *(cur_patch++)=*sl;	      
	    }	  
	  }
      }      
    }
  }  
  delete im;
  
}


void jmorph::show_8(image *screen, int x, int y, int frame_on, color_filter *fli, palette *pal)
{
  int pixelx,pixely,i;
  short cx1,cy1,cx2,cy2;
  unsigned char r,g,b;
  unsigned char *scolor,*ecolor,*addr=(unsigned char *)pal->addr();
  screen->get_clip(cx1,cy1,cx2,cy2);

  if (small)
  {
    morph_point8 *p_on=(morph_point8 *)p;
    for (i=0;i<total;i++,p_on++)
    {
      pixelx=(long)((int)p_on->x2-(int)p_on->x1)*(long)frame_on/8+p_on->x1+x;
      pixely=(long)((int)p_on->y2-(int)p_on->y1)*(long)frame_on/8+p_on->y1+y;
      if (pixelx>=cx1 && pixely>=cy1 && pixelx<=cx2 && pixely<=cy2)
      {
        scolor=addr+((int)p_on->start_color)*3;
        ecolor=addr+((int)p_on->end_color)*3;
        r=(((int)*(ecolor++))-((int)*scolor))*frame_on/8+*scolor; scolor++;
        g=(((int)*(ecolor++))-((int)*scolor))*frame_on/8+*scolor; scolor++;
        b=((int)(*ecolor)-(int)(*scolor))*frame_on/8+*scolor;
        *(screen->scan_line(pixely)+pixelx)=fli->lookup_color(r>>3,g>>3,b>>3);
      }
    }
  }
  else
  {
    morph_point16 *p_on=(morph_point16 *)p;
    for (i=0;i<total;i++,p_on++)
    {
      pixelx=(long)((int)p_on->x2-(int)p_on->x1)*(long)frame_on/8+p_on->x1+x;
      pixely=(long)((int)p_on->y2-(int)p_on->y1)*(long)frame_on/8+p_on->y1+y;
      if (pixelx>=cx1 && pixely>=cy1 && pixelx<=cx2 && pixely<=cy2)
      {
        scolor=addr+((int)p_on->start_color)*3;
        ecolor=addr+((int)p_on->end_color)*3;
        r=(((int)*(ecolor++))-((int)*scolor))*frame_on/8+*scolor; scolor++;
        g=(((int)*(ecolor++))-((int)*scolor))*frame_on/8+*scolor; scolor++;
        b=((int)(*ecolor)-(int)(*scolor))*frame_on/8+*scolor;
        *(screen->scan_line(pixely)+pixelx)=fli->lookup_color(r>>3,g>>3,b>>3);
      }
    }
  }
}  


void jmorph::add_filler(int frames)
{
  int w=max(bound_x2(0),bound_x2(1)),h=max(bound_y2(0),bound_y2(1)),
      i,pixelx,pixely,k,l,frame_on;  
  morph_point8 **middle_map,*other;
  unsigned char *end_map;
  CONDITION(small,"add filler not defined for 16bit morphs\n");
  
  if (frames<3) return ;
  

  middle_map=(morph_point8 **)jmalloc(w*h*sizeof(morph_point8 *),
				      "morph::middle_map");  // create an image of pointers
  end_map=(unsigned char *)jmalloc(w*h,
				  "morph::end_map");      // maps all the ending pixels

  for (frame_on=2;frame_on<frames-1;frame_on++)
  {    
    memset(middle_map,0,w*h*sizeof(morph_point8 *));            // initialize them middle pointers NULL
    memset(end_map,0,w*h);                                      // clear all end pixels
  
    morph_point8 *p_on=(morph_point8 *)p;    // p is the array of morph points
    for (i=0;i<total;i++,p_on++)            
    {    
      pixelx=(long)((int)p_on->x2-(int)p_on->x1)*(long) frame_on  /(long) frames  +p_on->x1;
      pixely=(long)((int)p_on->y2-(int)p_on->y1)*(long) frame_on  /(long) frames  +p_on->y1;

      middle_map[pixelx+pixely*w]=p_on;      // map this poisiton back to the morph point
      end_map[p_on->x2+p_on->y2*w]=p_on->end_color;        // note an ending map here
    }

  
    int skipped=0;
    
    for (pixely=0;pixely<h;pixely++)
    {
      for (pixelx=0;pixelx<w;pixelx++)
      { 
	if (middle_map[pixelx+pixely*w]==NULL)  // we are checking for 'duds'  (misplaced black pixels)
	{	
	  int tot;	
	  if (pixelx>0) if (middle_map[pixelx-1+pixely*w]) tot=1; else tot=0;      
	  if (pixelx<w-1) if (middle_map[pixelx+1+pixely*w]) tot++;	
	  if (pixely>0) if (middle_map[pixelx+(pixely-1)*w]) tot++;	
	  if (pixely<h-1) if (middle_map[pixelx+(pixely+1)*w]) tot++;	

	  if (tot>=3)                   // it is surronded by 3 non black squares, this is a dud	
	  {
	    int distance,shortest_distance,shortest_end_x,shortest_end_y;	  
	    morph_point8 *shortest=NULL;
	    
	    for (k=0;k<w;k++)
   	      for (l=0;l<h;l++)
	      { 	     
                other=middle_map[k+(l)*w];
		if (other)
		{
		  int end_x=frames*(pixelx-other->x1)/frame_on+other->x1,
		      end_y=frames*(pixely-other->y1)/frame_on+other->y1;		  
		  if (end_x>=0 && end_y>=0 && end_x<w && end_y<h && end_map[end_x+end_y*w])
		  { 
                    distance=(other->x1-end_x)*(other->x1-end_x)+
		             (other->y1-end_y)*(other->y1-end_y);
		    if (!shortest || shortest_distance>distance)
		    {
		      shortest_distance=distance;
		      shortest=other;
		      shortest_end_x=end_x;
		      shortest_end_y=end_y;		      
		    }
		  }
		}
	      }	    
	    if (shortest)
	    {
	      total++;
	      p=(void *)realloc(p,sizeof(morph_point8)*total);		  
	      morph_point8 *mod=((morph_point8 *)p)+total-1;
	      mod->x1=shortest->x1;
	      mod->y1=shortest->y1;
	      mod->start_color=shortest->start_color;		  

	      mod->x2=shortest_end_x;
	      mod->y2=shortest_end_y;		  
	      mod->end_color=end_map[shortest_end_x+shortest_end_y*w];		  
	    }		
	    else
	    {	      
  	      skipped++;
	      printf("skiped so far : %d (frame %d)\n",skipped,frame_on);
	    }
	    

	  }	
	}
      }
    }  
  }
  
  jfree(middle_map);
  jfree(end_map);
  
}



jmorph::jmorph(spec_entry *e, bFILE *fp)
{
  int i;
  fp->seek(e->offset,0);
  fp->read(&total,4);  
  total=long_to_local(total);
  if (e->type==SPEC_MORPH_POINTS_8 || e->type==SPEC_PATCHED_MORPH)
  {
    p=(void *)jmalloc(sizeof(morph_point8)*total,"morph8::point array");
    fp->read(p,sizeof(morph_point8)*total);
    small=1;
  }
  else
  {
    p=(void *)jmalloc(sizeof(morph_point16)*total,"morph16::point array");

    for (i=0;i<total;i++)
    { 
      ((morph_point16 *)p+i)->x1=fp->read_short();
      ((morph_point16 *)p+i)->y1=fp->read_short();
      ((morph_point16 *)p+i)->x2=fp->read_short();
      ((morph_point16 *)p+i)->y2=fp->read_short();
      fp->read( &((morph_point16 *)p+i)->start_color,1);
      fp->read( &((morph_point16 *)p+i)->end_color,1);
    }

    small=0;
  }
  w[0]=fp->read_short();
  h[0]=fp->read_short();  
  w[1]=fp->read_short();
  h[1]=fp->read_short();
}

void jmorph::show_frame(image *screen, int x, int y, 
                      int frames, int frame_on, color_filter *fli, palette *pal)
{
  int pixelx,pixely,i;
  short cx1,cy1,cx2,cy2;
  unsigned char r,g,b;
  unsigned char *scolor,*ecolor,*addr=(unsigned char *)pal->addr();
  screen->get_clip(cx1,cy1,cx2,cy2);

  if (small)
  {
    morph_point8 *p_on=(morph_point8 *)p;
    for (i=0;i<total;i++,p_on++)
    {
      pixelx=(long)((int)p_on->x2-(int)p_on->x1)*(long)frame_on/(long)frames+p_on->x1+x;
      pixely=(long)((int)p_on->y2-(int)p_on->y1)*(long)frame_on/(long)frames+p_on->y1+y;
      if (pixelx>=cx1 && pixely>=cy1 && pixelx<=cx2 && pixely<=cy2)
      {
        scolor=addr+((int)p_on->start_color)*3;
        ecolor=addr+((int)p_on->end_color)*3;
        r=(((int)*(ecolor++))-((int)*scolor))*frame_on/frames+*scolor; scolor++;
        g=(((int)*(ecolor++))-((int)*scolor))*frame_on/frames+*scolor; scolor++;
        b=((int)(*ecolor)-(int)(*scolor))*frame_on/frames+*scolor;
        *(screen->scan_line(pixely)+pixelx)=fli->lookup_color(r>>3,g>>3,b>>3);
      }
    }
  }
  else
  {
    morph_point16 *p_on=(morph_point16 *)p;
    for (i=0;i<total;i++,p_on++)
    {
      pixelx=(long)((int)p_on->x2-(int)p_on->x1)*(long)frame_on/(long)frames+p_on->x1+x;
      pixely=(long)((int)p_on->y2-(int)p_on->y1)*(long)frame_on/(long)frames+p_on->y1+y;
      if (pixelx>=cx1 && pixely>=cy1 && pixelx<=cx2 && pixely<=cy2)
      {
        scolor=addr+((int)p_on->start_color)*3;
        ecolor=addr+((int)p_on->end_color)*3;
        r=(((int)*(ecolor++))-((int)*scolor))*frame_on/frames+*scolor; scolor++;
        g=(((int)*(ecolor++))-((int)*scolor))*frame_on/frames+*scolor; scolor++;
        b=((int)(*ecolor)-(int)(*scolor))*frame_on/frames+*scolor;
        *(screen->scan_line(pixely)+pixelx)=fli->lookup_color(r>>3,g>>3,b>>3);
      }
    }
  }
}  

void jmorph::show_24frame(unsigned char *screen, int width, int height,
                  int x, int y, int frames, int frame_on, palette *pal)
{
    int pixelx,pixely,i;
  unsigned char *scolor,*ecolor,*addr=(unsigned char *)pal->addr(),*pix;
 
  if (small)
  {
    morph_point8 *p_on=(morph_point8 *)p;
    for (i=0;i<total;i++,p_on++)
    {
      pixelx=(long)((int)p_on->x2-(int)p_on->x1)*(long)frame_on/(long)frames+p_on->x1+x;
      pixely=(long)((int)p_on->y2-(int)p_on->y1)*(long)frame_on/(long)frames+p_on->y1+y;

      if (pixelx>=0 && pixely>=0 && pixelx<width && pixely<height)
      {	
        scolor=addr+((int)p_on->start_color)*3;
        ecolor=addr+((int)p_on->end_color)*3;
        pix=screen+pixelx*3+pixely*3*width;               
        *pix=(((int)*(ecolor++))-((int)*scolor))*frame_on/frames+*scolor; scolor++; pix++;	
        *pix=(((int)*(ecolor++))-((int)*scolor))*frame_on/frames+*scolor; scolor++; pix++;	
        *pix=((int)(*ecolor)-(int)(*scolor))*frame_on/frames+*scolor;
      }
    }
  }
  else
  {
    morph_point16 *p_on=(morph_point16 *)p;
    for (i=0;i<total;i++,p_on++)
    {
      pixelx=(long)((int)p_on->x2-(int)p_on->x1)*(long)frame_on/(long)frames+p_on->x1+x;
      pixely=(long)((int)p_on->y2-(int)p_on->y1)*(long)frame_on/(long)frames+p_on->y1+y;

      if (pixelx>=0 && pixely>=0 && pixelx<width && pixely<height)
      {	
        scolor=addr+((int)p_on->start_color)*3;
        ecolor=addr+((int)p_on->end_color)*3;
        pix=screen+pixelx*3+pixely*3*width;               
        *pix=(((int)*(ecolor++))-((int)*scolor))*frame_on/frames+*scolor; scolor++; pix++;	
        *pix=(((int)*(ecolor++))-((int)*scolor))*frame_on/frames+*scolor; scolor++; pix++;	
        *pix=((int)(*ecolor)-(int)(*scolor))*frame_on/frames+*scolor;
      }
    }
  }
}


jmorph::jmorph(image *i1, image *hint1, image *i2, image *hint2, 
                          int aneal_steps)
{
  w[0]=i1->width();
  h[0]=i1->height();  
  w[1]=i2->width();
  h[1]=i2->height();  

  struct { int start,end; } crange[256];
  int hint_hist1[256],hint_hist2[256],x,y,total_hints=0,randp,cur_pix,
      last=0,first=0,i,j,k,findx1,findy1,findx2,findy2,total1,total2,points;
  unsigned char *sl,color;
  void *plist;


  CONDITION(i1->width()==hint1->width() && i1->height()==hint1->height() && 
            i2->width()==hint2->width() && i2->height()==hint2->height(),
            "Image sizes do not correspond with hint sizes");
  if (i1->width()>255 || i2->width()>255 || i1->height()>255 || i2->height()>255) 
    small=0;
  else small=1;

  memset(hint_hist1,0,sizeof(hint_hist1));
  memset(hint_hist2,0,sizeof(hint_hist2));
  total=0;

  // first found out how many hints we have to follow
  for (y=0;y<hint1->height();y++)
  { sl=hint1->scan_line(y);
    for (x=hint1->width();x;x--,sl++)
      if (*sl) hint_hist1[*sl]++;
  }
  for (y=0;y<hint2->height();y++)
  { sl=hint2->scan_line(y);
    for (x=hint2->width();x;x--,sl++)
      if (*sl) hint_hist2[*sl]++;
  }

  // check the user and see if the mask match up
  for (x=0;x<256;x++) 
  {
    if ((hint_hist1[x]!=0 && hint_hist2[x]==0) ||
        (hint_hist1[x]==0 && hint_hist2[x]!=0))
    { printf("Color %d used for hinting is prent in one hint mask, but not the other\n",x);
      exit(1);
    } else if (hint_hist1[x])
    { 
      if (!first) first=x;
      total_hints++;
      if (hint_hist1[x]>hint_hist2[x])
        y=hint_hist1[x];
      else
        y=hint_hist2[x];
      total+=y;
      crange[x].start=last;
      crange[x].end=last+y-1;
      last+=y;
    }
  }
  if (small)
    plist=(void *)jmalloc(sizeof(morph_point8)*total,"morph8::point array");
  else
    plist=(void *)jmalloc(sizeof(morph_point16)*total,"morph16::point array");
  CHECK(plist);
  for (i=0,color=0;i<total_hints;i++)
  {
    color++; while (!hint_hist1[color]) color++;
    findx1=findx2=-1; findy1=findy2=0;
    total1=hint_hist1[color];
    total2=hint_hist2[color];
    if (total1>total2) points=total1; else points=total2;
    for (j=0;j<points;j++)
    {
      if (total1)  // are there any more pixels left in this image?
      { total1--;
        do {
          findx1++; 
          if (findx1>=hint1->width()) { findx1=0; findy1++; } 
        } while (hint1->pixel(findx1,findy1)!=color);
      }
      else
      {
        randp=rand()%j+crange[color].start;
        if (small)
        { findx1=((morph_point8 *)plist)[randp].x1; 
          findy1=((morph_point8 *)plist)[randp].y1;
        } else
        { findx1=((morph_point16 *)plist)[randp].x1; 
          findy1=((morph_point16 *)plist)[randp].y1;
        }
      }
      if (total2)  // are there any more pixels left in this image?
      { 
        total2--;
        do {
          findx2++; 
          if (findx2>=hint2->width()) { findx2=0; findy2++; } 
        } while (hint2->pixel(findx2,findy2)!=color);
      }
      else
      {
        randp=rand()%j+crange[color].start;
        if (small)
        { findx2=((morph_point8 *)plist)[randp].x2; 
          findy2=((morph_point8 *)plist)[randp].y2;
        } else
        { findx2=((morph_point16 *)plist)[randp].x2; 
          findy2=((morph_point16 *)plist)[randp].y2;
        }
      }
      cur_pix=j+crange[color].start;
      CHECK(cur_pix<total);

      if (small)
      {
        ((morph_point8 *)plist)[cur_pix].x1=findx1;  
        ((morph_point8 *)plist)[cur_pix].x2=findx2;  
        ((morph_point8 *)plist)[cur_pix].y1=findy1;  
        ((morph_point8 *)plist)[cur_pix].y2=findy2;  
        ((morph_point8 *)plist)[cur_pix].start_color=i1->pixel(findx1,findy1);
        ((morph_point8 *)plist)[cur_pix].end_color=i2->pixel(findx2,findy2);
      }
      else
      {
        ((morph_point16 *)plist)[cur_pix].x1=findx1;  
        ((morph_point16 *)plist)[cur_pix].x2=findx2;  
        ((morph_point16 *)plist)[cur_pix].y1=findy1;  
        ((morph_point16 *)plist)[cur_pix].y2=findy2;  
        ((morph_point16 *)plist)[cur_pix].start_color=i1->pixel(findx1,findy1);
        ((morph_point16 *)plist)[cur_pix].end_color=i2->pixel(findx2,findy2);
      }
    }
  }
  int swap_point,distance,new_distance,hint_color,first_point,range,start;

  int sx2,sy2,sec;
  for (i=0;i<aneal_steps;i++)
  {
    for (j=0,first_point=0;j<total_hints;j++)
    {
      if (small)
        hint_color=hint1->pixel(((morph_point8 *)plist)[first_point].x1,((morph_point8 *)plist)[first_point].y1);
      else
        hint_color=hint1->pixel(((morph_point16 *)plist)[first_point].x1,((morph_point16 *)plist)[first_point].y1);
      start=crange[hint_color].start;
      range=crange[hint_color].end-start+1;
      for(k=crange[hint_color].start;k<=crange[hint_color].end;k++) 
      {
        swap_point=rand()%range+start;
        if (small)
        {
          morph_point8 *pk,*ps;
          pk=(morph_point8 *)plist+k;
          ps=(morph_point8 *)plist+swap_point;

          distance=(pk->x2-pk->x1)*(pk->x2-pk->x1)+(pk->y2-pk->y1)*(pk->y2-pk->y1)+
                   (ps->x2-ps->x1)*(ps->x2-ps->x1)+(ps->y2-ps->y1)*(ps->y2-ps->y1);
          new_distance=(ps->x2-pk->x1)*(ps->x2-pk->x1)+(ps->y2-pk->y1)*(ps->y2-pk->y1)+
                       (pk->x2-ps->x1)*(pk->x2-ps->x1)+(pk->y2-ps->y1)*(pk->y2-ps->y1);
          if (new_distance<distance)
          { 
            sx2=pk->x2;         sy2=pk->y2;
            sec=pk->end_color;
            pk->x2=ps->x2;      pk->y2=ps->y2;
            pk->end_color=ps->end_color;
            ps->x2=sx2;         ps->y2=sy2;
            ps->end_color=sec;
          }
        } else
        {
          morph_point16 *pk,*ps;
          pk=(morph_point16 *)plist+k;
          ps=(morph_point16 *)plist+swap_point;

          distance=(pk->x2-pk->x1)*(pk->x2-pk->x1)+(pk->y2-pk->y1)*(pk->y2-pk->y1)+
                   (ps->x2-ps->x1)*(ps->x2-ps->x1)+(ps->y2-ps->y1)*(ps->y2-ps->y1);
          new_distance=(ps->x2-pk->x1)*(ps->x2-pk->x1)+(ps->y2-pk->y1)*(ps->y2-pk->y1)+
                       (pk->x2-ps->x1)*(pk->x2-ps->x1)+(pk->y2-ps->y1)*(pk->y2-ps->y1);
          if (new_distance<distance)
          { 
            sx2=pk->x2;         sy2=pk->y2;
            sec=pk->end_color;
            pk->x2=ps->x2;      pk->y2=ps->y2;
            pk->end_color=ps->end_color;
            ps->x2=sx2;         ps->y2=sy2;
            ps->end_color=sec;
          }
        }
      }
      first_point=crange[hint_color].end+1;
    }
  }
  p=plist;
} 

int morph_width;

int morph8_compare(const void *p1, const void *p2)
{
  int pos1=((morph_point8 *)p1)->x1+((morph_point8 *)p1)->y1*morph_width;  
  int pos2=((morph_point8 *)p2)->x1+((morph_point8 *)p2)->y1*morph_width;  
  if (pos1<pos2) return -1;
  else if (pos1==pos2) return 0;
  else return 1;
  
}


int jmorph::write(bFILE *fp)
{
  int i;

  
  fp->write_long(total);  
  if (small)
  {

    // if the points are put in order then they can be compressed easier
    morph_width=max(bound_x2(0),bound_x2(1));      // global used by qsort compare routine
    qsort(p,total,sizeof(morph_point8),morph8_compare);  
    
    fp->write(p,sizeof(morph_point8)*total); 
  }  
  else
  {
    for (i=0;i<total;i++)
    { fp->write_short(((morph_point16 *)p+i)->x1);
      fp->write_short(((morph_point16 *)p+i)->y1);
      fp->write_short(((morph_point16 *)p+i)->x2);
      fp->write_short(((morph_point16 *)p+i)->y2);
      fp->write( &((morph_point16 *)p+i)->start_color,1);
      fp->write( &((morph_point16 *)p+i)->end_color,1);
    }
  }
  fp->write_short(w[0]);
  fp->write_short(h[0]);
  fp->write_short(w[1]);
  fp->write_short(h[1]);
  return 1;
}




step_morph::step_morph(patched_morph *mor, palette *pal, int frame_direction, int face_direction)
{
  int i;
  pm=mor;
  
  total=mor->total_points();      
  points=(step_struct *)jmalloc(sizeof(step_struct)*total,"step_morph::points");  
  
  dir=frame_direction;
  face_dir=face_direction;
  
  if (dir>0)
    frame_on=0;
  else
    frame_on=mor->patches-1;
  
  if (mor->small_morph())
  {    
      int x1,y1,x2,y2;

    
      if (frame_direction>0)
      {      
	morph_point8 *m8=mor->small_points();      
	
	for (i=0;i<total;i++,m8++)
	{     	
	  x1=(int)m8->x1;
	  x2=(int)m8->x2;

	  y1=(int)m8->y1;
	  y2=(int)m8->y2;

	  points[i].x=x1<<8;
	  points[i].y=y1<<8;       
	  points[i].dx=(x2-x1)<<5;
	  points[i].dy=(y2-y1)<<5;		
         
	  unsigned char r1,g1,b1,r2,g2,b2;
	  pal->get(m8->start_color,r1,g1,b1);
	  pal->get(m8->end_color,r2,g2,b2);
	  points[i].r=r1<<8;
	  points[i].g=g1<<8;
	  points[i].b=b1<<8;

	  points[i].dr=(long)((int)r2-(int)r1)<<5;
	  points[i].dg=(long)((int)g2-(int)g1)<<5;
	  points[i].db=(long)((int)b2-(int)b1)<<5;			
	}
      }    
      else
      {      
	morph_point8 *m8=mor->small_points();      
	for (i=0;i<total;i++,m8++)
	{     	
	  x1=(int)m8->x1;
	  x2=(int)m8->x2;

	  y1=(int)m8->y1;
	  y2=(int)m8->y2;

	  points[i].x=x2<<8;
	  points[i].y=y2<<8;       
	  points[i].dx=(x1-x2)<<5;
	  points[i].dy=(y1-y2)<<5;		

	  unsigned char r1,g1,b1,r2,g2,b2;
	  pal->get(m8->start_color,r2,g2,b2);
	  pal->get(m8->end_color,r1,g1,b1);
	  points[i].r=r1<<8;
	  points[i].g=g1<<8;
	  points[i].b=b1<<8;

	  points[i].dr=(long)((int)r2-(int)r1)<<5;
	  points[i].dg=(long)((int)g2-(int)g1)<<5;
	  points[i].db=(long)((int)b2-(int)b1)<<5;			
	}
      }    
  }  


}

void step_morph::show_frame(image *screen, int x, int y,  color_filter *fli)
{
  short x1,y1,x2,y2;
  screen->get_clip(x1,y1,x2,y2);

  int i,px,py,ww=max(pm->bound_x2(0),pm->bound_x2(1))-1;
  step_struct *ss;

  morph_patch *pat=pm->pats+frame_on;  
  int j,tp=pat->patches;
  unsigned char *sl=pat->patch_data;    

  if (face_dir>0)
  {    
    for (i=0,ss=points;i<total;i++,ss++)
    {
      px=x+(ss->x>>(16-8));
      py=y+(ss->y>>(16-8));
      if (px>=x1 && px<=x2 && py>=y1 && py<=y2)
        *(screen->scan_line(py)+px)=fli->lookup_color(ss->r>>(19-8),ss->g>>(19-8),ss->b>>(19-8));

      ss->x+=ss->dx;
      ss->y+=ss->dy;
      ss->r+=ss->dr;
      ss->g+=ss->dg;
      ss->b+=ss->db;        
    }  

    for (j=0;j<tp;j++,sl++)
    {
      px=x+*(sl++);
      py=y+*(sl++);
      if (px>=x1 && px<=x2 && py>=y1 && py<=y2)
        *(screen->scan_line(py)+px)=*sl;
    }             
  } else
  {
    for (i=0,ss=points;i<total;i++,ss++)
    {
      px=x+ww-(ss->x>>(16-8));
      py=y+(ss->y>>(16-8));
      if (px>=x1 && px<=x2 && py>=y1 && py<=y2)
        *(screen->scan_line(py)+px)=fli->lookup_color(ss->r>>(19-8),ss->g>>(19-8),ss->b>>(19-8));

      ss->x+=ss->dx;
      ss->y+=ss->dy;
      ss->r+=ss->dr;
      ss->g+=ss->dg;
      ss->b+=ss->db;        
    }  

    for (j=0;j<tp;j++,sl++)
    {
      px=x+(ww-(int)(*(sl++)));
      py=y+*(sl++);
      if (px>=x1 && px<=x2 && py>=y1 && py<=y2)
        *(screen->scan_line(py)+px)=*sl;
    }          
  }
  
  frame_on+=dir;
  
}

void step_morph::reverse_direction()
{
  step_struct *s=points;
  int i; 
  for (i=0;i<total;i++,s++)
  {
    s->dx=-s->dx;
    s->dy=-s->dy;
    s->dr=-s->dr;
    s->dg=-s->dg;
    s->db=-s->db;    
  }
  dir=-dir;
  
}


patched_morph::patched_morph(spec_entry *e, bFILE *fp) : jmorph(e,fp)
{
  int i;
  
  patches=fp->read_short();
  pats=(morph_patch *)jmalloc(sizeof(morph_patch)*patches,"patched_morph::points");
  
  for (i=0;i<patches;i++)
  {
    pats[i].patches=fp->read_short();
    if (pats[i].patches)
    {      
      pats[i].patch_data=(unsigned char *)jmalloc(3*pats[i].patches,"patched_morph::patch_data");
      fp->read(pats[i].patch_data,3*pats[i].patches);      
    }
    else
      pats[i].patch_data=NULL;
  }   
}


