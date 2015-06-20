#include "macs.hpp"
#include "particle.hpp"
#include "view.hpp"
#include "lisp.hpp"
#include "cache.hpp"
#include "jrand.hpp"
#include "dprint.hpp"

static int total_pseqs=0;
static part_sequence **pseqs=NULL;
static part_animation *first_anim=NULL,*last_anim=NULL;

void free_pframes()
{
  for (int i=0;i<total_pseqs;i++)
    delete pseqs[i];
  if (total_pseqs)
    jfree(pseqs);
}

part_frame::~part_frame()
{
  jfree(data);
}

void add_panim(int id, long x, long y, int dir)
{
  CONDITION(id>=0 && id<total_pseqs,"bad id for particle animation");
  part_animation *pan=new part_animation(pseqs[id],x,y,dir,NULL);
  if (!first_anim)
    first_anim=last_anim=pan;
  else
  {
    last_anim->next=pan;
    last_anim=pan;
  }  
}

void delete_panims()
{
  while (first_anim)
  {
    last_anim=first_anim;
    first_anim=first_anim->next;
    delete last_anim;
  }
  last_anim=NULL;
}

int defun_pseq(void *args)
{
  lisp_symbol *sym=(lisp_symbol *)lcar(args);
  if (item_type(sym)!=L_SYMBOL)
  {
    lprint(args);
    dprintf("expecting first arg to def-particle to be a symbol!\n");
    exit(0);
  }
  int sp=current_space;
  current_space=PERM_SPACE;
  set_symbol_number(sym,total_pseqs);   // set the symbol value to the object number
  current_space=sp;
  pseqs=(part_sequence **)jrealloc(pseqs,sizeof(part_sequence *)*(total_pseqs+1),"particle seq array");

  args=lcdr(args);
  pseqs[total_pseqs]=new part_sequence(args);
  total_pseqs++;  
  return total_pseqs;
}

extern int total_files_open;

part_sequence::part_sequence(void *args)
{
  char *fn=lstring_value(lcar(args)); 
  bFILE *fp=open_file(fn,"rb");
  if (fp->open_failure())
  {
    delete fp;
    lprint(args);
    dprintf("\nparticle sequence : Unable to open %s for reading\n",fn);
    dprintf("total files open=%d\n",total_files_open);

    FILE *fp=fopen(fn,"rb");
    dprintf("convet = %d\n",fp!=NULL);
    exit(1);
  }

  // count how many frames are in the file
  spec_directory sd(fp);
  delete fp;
  tframes=0;
  int i=0;
  for (;i<sd.total;i++)
    if (sd.entries[i]->type==SPEC_PARTICLE) tframes++;
  frames=(int *)jmalloc(sizeof(int)*tframes,"part_frame id list\n");
 
  int on=0;
  for (i=0;i<sd.total;i++)
    if (sd.entries[i]->type==SPEC_PARTICLE)     
      frames[on++]=cash.reg(fn,sd.entries[i]->name,SPEC_PARTICLE,1);

}

part_frame::part_frame(bFILE *fp)
{
  t=fp->read_long();
  data=(part *)jmalloc(sizeof(part)*t,"particle frame");
  x1=y1=100000; x2=y2=-100000;
  for (int i=0;i<t;i++)
  {
    short x=fp->read_short();
    short y=fp->read_short();
    if (x<x1) x1=x;
    if (y<y1) y1=y;
    if (x>x2) x2=x;
    if (y>y2) y2=x;
    data[i].x=x;
    data[i].y=y;   
    data[i].color=fp->read_byte(); 
  }
}

void tick_panims()
{
  part_animation *last=NULL;
  for (part_animation *p=first_anim;p;)
  {
    p->frame++;
    if (p->frame>=p->seq->tframes)
    {
      if (last)
        last->next=p->next;
      else first_anim=first_anim->next;
      if (last_anim==p) last_anim=last;
      part_animation *d=p;
      p=p->next;
      delete d;
    } else 
    {
      last=p;
      p=p->next;
    }

  }
}

void draw_panims(view *v)
{
  for (part_animation *p=first_anim;p;p=p->next)      
  {
    cash.part(p->seq->frames[p->frame])->draw(screen,p->x-v->xoff()+v->cx1,p->y-v->yoff()+v->cy1,p->dir);
  }
}

void part_frame::draw(image *screen, int x, int y, int dir)
{
  short cx1,cy1,cx2,cy2;
  screen->get_clip(cx1,cy1,cx2,cy2);
  if (x+x1>cx2 || x+x2<cx1 || y+y1>cy2 || y+y2<cy1) return ;

  part *pon=data;
  cy1-=y;
  cy2-=y;

  int i=t;
  while (i && pon->y<cy1) { pon++; i--; }
  if (!i) return ;
  if (dir>0)
  {
    while (i && pon->y<=cy2)
    {
      long dx=x-pon->x;
      if (dx>=cx1 && dx<=cx2)
      *(screen->scan_line(pon->y+y)+dx)=pon->color;
      i--;
      pon++;
    }   
  } else
  {
    while (i && pon->y<=cy2)
    {
      long dx=pon->x+x;
      if (dx>=cx1 && dx<=cx2)
        *(screen->scan_line(pon->y+y)+dx)=pon->color;
      i--;
      pon++;
    }   
  }

}




void scatter_line(int x1, int y1, int x2, int y2, int c, int s)
{
  short cx1,cy1,cx2,cy2;
  screen->get_clip(cx1,cy1,cx2,cy2);
 
  int t=abs(x2-x1)>abs(y2-y1) ? abs(x2-x1)+1 : abs(y2-y1)+1;
  long xo=x1<<16,yo=y1<<16,dx=((x2-x1)<<16)/t,dy=((y2-y1)<<16)/t,count=0,x,y;
  uchar *sl;

  int xm=(1<<s);
  int ym=(1<<s);
  s=(15-s);


  while (t--)
  {
    x=(xo>>16)+(jrand()>>s)-xm;
    y=(yo>>16)+(jrand()>>s)-ym;
    if (!(x<cx1 || y<cy1 || x>cx2 || y>cy2))
      *(screen->scan_line(y)+x)=c;
    xo+=dx; yo+=dy;
  }

}



void ascatter_line(int x1, int y1, int x2, int y2, int c1, int c2, int s)
{
  short cx1,cy1,cx2,cy2;
  screen->get_clip(cx1,cy1,cx2,cy2);


  int t=abs(x2-x1)>abs(y2-y1) ? abs(x2-x1)+1 : abs(y2-y1)+1;
  long xo=x1<<16,yo=y1<<16,dx=((x2-x1)<<16)/t,dy=((y2-y1)<<16)/t,count=0,x,y;
  uchar *sl;

  int xm=(1<<s);
  int ym=(1<<s);
  s=(15-s);

  int w=screen->width();
  uchar *addr;


  while (t--)
  {
    x=(xo>>16)+(jrand()>>s)-xm;
    y=(yo>>16)+(jrand()>>s)-ym;
    if (!(x<=cx1 || y<=cy1 || x>=cx2 || y>=cy2))
    {
      addr=screen->scan_line(y)+x;
      *addr=c1;
      *(addr+w)=c2;
      *(addr-w)=c2;
      *(addr-1)=c2;
      *(addr+1)=c2;
    }
    xo+=dx; yo+=dy;
  }

}


