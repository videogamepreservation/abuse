#include "level.hpp"
#include "intsect.hpp"

class collide_patch
{
  public :
  long total,x1,y1,x2,y2;
  game_object **touch;  
  collide_patch *next;
  collide_patch(long X1, long Y1, long X2, long Y2, collide_patch *Next)
  { 
    x1=X1; y1=Y1; x2=X2; y2=Y2;
    next=Next;
    total=0;
    touch=NULL;
  }
  void add_collide(long X1, long Y1, long X2, long Y2, game_object *who);
  collide_patch *copy(collide_patch *Next);
  ~collide_patch() { if (total) jfree(touch); }
} ;


collide_patch *collide_patch::copy(collide_patch *Next)
{ 
  collide_patch *p=new collide_patch(x1,y1,x2,y2,Next); 
  p->total=total;
  if (total)    
  {
    p->touch=(game_object **)jmalloc(total*sizeof(game_object *),"collide patches");
    memcpy(p->touch,touch,total*(sizeof(game_object *)));
  }
  else 
    p->touch=NULL; 
  return p;
}


void add_collide(collide_patch *&first, long x1, long y1, long x2, long y2, 
			    game_object *who)
{  
  collide_patch *last=NULL,*next;
  for (collide_patch *p=first;p;p=next)
  {
    next=p->next;
    // first see if light patch we are adding is enclosed entirely by another patch
    if (x1>=p->x1 && y1>=p->y1 && x2<=p->x2 && y2<=p->y2)
    {
      if (x1>p->x1)
      {
	first=p->copy(first);
	first->x2=x1-1;
      }
      if (x2<p->x2)
      {
	first=p->copy(first);
	first->x1=x2+1;
      }
      if (y1>p->y1)
      {
	first=p->copy(first);
	first->x1=x1;
	first->x2=x2;
	first->y2=y1-1;
      }
      if (y2<p->y2)
      {
	first=p->copy(first);
	first->x1=x1;
	first->x2=x2;
	first->y1=y2+1;
      }
      p->x1=x1; p->y1=y1; p->x2=x2; p->y2=y2;

      p->total++;     
      p->touch=(game_object **)jrealloc(p->touch,sizeof(game_object *)*p->total,"object_patch_list");
      p->touch[p->total-1]=who;
      return ;
    }

    // see if the patch completly covers another patch.
    if (x1<=p->x1 && y1<=p->y1 && x2>=p->x2 && y2>=p->y2)
    {
      if (x1<p->x1)
        add_collide(first,x1,y1,p->x1-1,y2,who);
      if (x2>p->x2)
        add_collide(first,p->x2+1,y1,x2,y2,who);
      if (y1<p->y1)
        add_collide(first,p->x1,y1,p->x2,p->y1-1,who);
      if (y2>p->y2)
        add_collide(first,p->x1,p->y2+1,p->x2,y2,who);
      p->total++;     
      p->touch=(game_object **)jrealloc(p->touch,sizeof(game_object *)*p->total,"object_patch_list");
      p->touch[p->total-1]=who;
      return ;
    }

    // see if we intersect another rect
    if (!(x2<p->x1 || y2<p->y1 || x1>p->x2 || y1>p->y2))  
    {
      int ax1,ay1,ax2,ay2;
      if (x1<p->x1)
      {
        add_collide(first,x1,max(y1,p->y1),p->x1-1,min(y2,p->y2),who);
	ax1=p->x1;
      } else
	ax1=x1;

      if (x2>p->x2)
      {
        add_collide(first,p->x2+1,max(y1,p->y1),x2,min(y2,p->y2),who);
	ax2=p->x2;
      } 
      else
	ax2=x2;

      if (y1<p->y1)
      {       
        add_collide(first,x1,y1,x2,p->y1-1,who);
	ay1=p->y1;
      } else 
	ay1=y1;

      if (y2>p->y2)
      {
        add_collide(first,x1,p->y2+1,x2,y2,who);
	ay2=p->y2;
      } else 
	ay2=y2;

       
      add_collide(first,ax1,ay1,ax2,ay2,who);      

      return ;    
    }

  }
  
  first=new collide_patch(x1,y1,x2,y2,first);
  first->total=1;
  first->touch=(game_object **)jmalloc(sizeof(game_object *)*1,"object_patch_list");
  first->touch[0]=who;
}




void level::check_collisions()
{
  game_object *target,*rec,*subject;
  collide_patch *f=NULL;
  long sx1,sy1,sx2,sy2,tx1,ty1,tx2,ty2,hitx,hity,
      s_centerx,t_centerx;

  for (int l=0;l<attack_total;l++)
  {
    subject=attack_list[l];
    subject->picture_space(sx1,sy1,sx2,sy2);
    rec=NULL;
    

    for (int j=0;j<target_total && !rec;j++)
    {
      target=target_list[j];	
      target->picture_space(tx1,ty1,tx2,ty2);
      if (!(sx2<tx1 || sy2<ty1 || sx1>tx2 || sy1>ty2))  // check to see if picture spaces collide
      {

	try_pushback(subject,target);

	if (subject->can_hurt(target))    // see if we can hurt him before calculating
	{
	  t_centerx=target->x_center();
	  point_list *s_hit,*t_damage;
	  
	  s_hit=subject->current_figure()->hit;

	  if (target->direction>0)
	    t_damage=target->current_figure()->f_damage;
	  else
	    t_damage=target->current_figure()->b_damage;

	  unsigned char *s_dat=s_hit->data,*t_dat;
	  int i,j;
	  for (i=(int)s_hit->tot-1;i>0 && !rec;i--)
	  {
	    for (t_dat=t_damage->data,j=(int)t_damage->tot-1;j>0 && !rec;j--)
	    {
	      long x1,y1,x2,y2,          // define the two line segments to check
	      xp1,yp1,xp2,yp2;

	      xp1=target->x+target->tx(*t_dat);  t_dat++;	      
	      yp1=target->y+target->ty(*t_dat);  t_dat++;
	      xp2=target->x+target->tx(*t_dat); 
	      yp2=target->y+target->ty(t_dat[1]); 

	      x1=subject->x+subject->tx(s_dat[0]);  
	      y1=subject->y+subject->ty(s_dat[1]); 
	      x2=subject->x+subject->tx(s_dat[2]); 
	      y2=subject->y+subject->ty(s_dat[3]); 
	      

	      // ok, now we know which line segemnts to check for intersection
	      // now check to see if (x1,y1-x2,y2) intercest with (xp1,yp1-xp2,yp2)
	      int _x2=x2,_y2=y2;	      
	      setback_intersect(x1, y1, x2, y2, xp1, yp1, xp2, yp2,0);


	      if (x2!=_x2 || _y2!=y2)
	      {
		rec=target;
		hitx=((x1+x2)/2+(xp1+xp2)/2)/2;
		hity=((y1+y1)/2+(yp1+yp2)/2)/2;
	      }
	    }
	    s_dat+=2;
	  }    	
	}
      }
    }
    if (rec)
    {
      rec->do_damage((int)subject->current_figure()->hit_damage,subject,hitx,hity,0,0);
      subject->note_attack(rec);
    }    
  }
}

