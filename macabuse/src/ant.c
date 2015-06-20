#include "ant.hpp"
#include "lisp.hpp"
#include "lisp_gc.hpp"
#include "compiled.hpp"
#include "objects.hpp"
#include "level.hpp"
#include "game.hpp"
#include "jrand.hpp"
#include "clisp.hpp"
#include "dev.hpp"
#include "key_cfg.hpp"
#include "status.hpp"
#include <ctype.h>


enum {  ANT_need_to_dodge,     // ant vars
	ANT_no_see_time,
	ANT_hide_flag };

void *ant_ai();

int can_see(game_object *o, long x1, long y1, long x2, long y2)
{
  long nx2=x2,ny2=y2;
  current_level->vforeground_intersect(x1,y1,y2);
  if (y2!=ny2) return 0;

  current_level->boundary_setback(o,x1,y1,x2,y2);
  return (x2==nx2 && y2==ny2);
}


// if we first saw the player or it's been a while since we've seen the player then do a scream
static void scream_check(game_object *o, game_object *b)
{
  if (can_see(o,o->x,o->y,b->x,b->y))
  {
    if (o->lvars[ANT_no_see_time]==0 || o->lvars[ANT_no_see_time]>20)
      the_game->play_sound(S_ASCREAM_SND,127,o->x,o->y);      
    o->lvars[ANT_no_see_time]=1;
  } else o->lvars[ANT_no_see_time]++;
}

int ant_congestion(game_object *o)
{
  for (game_object *d=current_level->first_active_object();d;d=d->next_active)
  {
    if (d->otype==o->otype && abs(o->x-d->x)<30 && abs(o->x-d->y)<20) return 1;
  }
  return 0;
}

int ant_dodge(game_object *o)
{
  if (o->lvars[ANT_need_to_dodge]==1)
  {
    o->lvars[ANT_need_to_dodge]=0;
    if ((jrand()%2)==0)
    {
      o->set_state(stopped);
      o->set_aistate(ANT_JUMP);
      if (!can_see(o,o->x,o->y,o->x,o->y-120))   // is there a roof above?
      {
	o->set_yvel(-17);
	o->set_xvel(0);
	o->set_aistate(ANT_JUMP_ROOF);
	ant_ai();
      } else
      {
	o->set_yvel(-12);
	if (o->direction>0)
	  o->set_xvel(22);
	else o->set_xvel(-22);
	o->set_aistate(ANT_JUMP);
      }
    }
    return 1;
  } else return 0;
}

static int alien_wait_time()
{
  void *v=symbol_value(l_difficulty);
  if (v==l_easy)
    return 6;
  else if (v==l_medium)
   return 4;
  else if (v==l_hard)
    return 2;
  else return 1;
}

static int can_hit_player(game_object *o, game_object *b)
{  
  return can_see(o,o->x+(o->direction>0?15:-15),o->y-15,b->x,b->y-15);
}

static void fire_at_player(game_object *o, game_object *b)
{
  long firex=o->x+(o->direction>0?15:-15),firey=o->y-15,
       playerx=b->x+b->xvel()*8,playery=b->y-15+b->yvel()*2;
  if (can_see(o,o->x,o->y,firex,firey) && can_see(o,firex,firey,playerx,playery))
  {
    int angle=lisp_atan2(firey-playery,playerx-firex);
    void *call_list=NULL;
    p_ref r1(call_list);
    push_onto_list(new_lisp_pointer(b),call_list);
    push_onto_list(new_lisp_number(angle),call_list);
    push_onto_list(new_lisp_number(firey),call_list);
    push_onto_list(new_lisp_number(firex),call_list);
    push_onto_list(new_lisp_number(o->aitype()),call_list);
    push_onto_list(new_lisp_pointer(o),call_list);
    eval_user_fun((lisp_symbol *)l_fire_object,call_list);
    o->set_state((character_state)S_weapon_fire);
  }
}

void *ant_ai()
{
  game_object *o=current_object,*b;

  if (o->hp()==0)    // if we are dead return NULL and get deleted
  {
    if (o->state==dead)
    {
      current_level->attacker(current_object)->controller()->kills++;
      return NULL;
    }
    else o->set_state(dead);
    return true_symbol;
  }


  if (o->state==flinch_up || o->state==flinch_down)
  {
    o->next_picture();
    return true_symbol;
  }


  switch (o->aistate())
  {
    case ANT_START :
    {
      o->set_state((character_state)S_hanging);
      if (o->lvars[ANT_hide_flag])
        o->set_aistate(ANT_HIDING);

      else o->set_aistate(ANT_HANGING);
    } break;
    case ANT_HIDING :
    {
      if ((jrand()%128)==0) the_game->play_sound(S_SCARE_SND,127,o->x,o->y);
      if (o->otype!=S_HIDDEN_ANT)
      {
	o->change_type(S_HIDDEN_ANT);      // switch types so noone hurts us.
	o->set_state(stopped);
	o->set_aistate(ANT_HIDING);
      }

      int fall=0;
      if (o->total_objects()==0)
      {
	if (player_list->next)
	  b=current_level->attacker(current_object);
	else b=player_list->focus;
	if (abs(b->x-o->x)<130 && (o->y<b->y))
	  fall=1;
      }      
      else if (o->get_object(0)->aistate()!=0)
        fall=1;
      
      if (fall)
      {
	o->change_type(S_ANT_ROOF);
	o->set_state((character_state)S_falling);
	o->set_aistate(ANT_FALL_DOWN);
	o->set_targetable(1);
      } else o->set_targetable(0);
    } break;
    case ANT_HANGING :
    {
      int fall=0;
      if ((jrand()%128)==0) the_game->play_sound(S_SCARE_SND,127,o->x,o->y);
      if (o->lvars[ANT_hide_flag])
        o->set_aistate(ANT_HIDING);
      else
      {
	o->set_state((character_state)S_hanging);
	if (o->total_objects())
	{
	  if (o->get_object(0)->aistate()!=0)
	  fall=1;
	} else
	{
	  if (player_list->next)
	    b=current_level->attacker(current_object);
	  else b=player_list->focus;
	  if (abs(b->x-o->x)<130 && (o->y<b->y))
	  fall=1;
	}
	if (fall)
	{
	  o->set_state((character_state)S_fall_start);
	  o->set_aistate(ANT_FALL_DOWN);
	  o->set_targetable(1);
	} else o->set_targetable(0);
      }
      
    } break;
    case ANT_FALL_DOWN :
    {
      o->set_state((character_state)S_falling);
       
      if (player_list->next)
      b=current_level->attacker(current_object);
      else b=player_list->focus;
      
      scream_check(o,b);
      int ret=o->mover(0,0,0);
      if ((ret&BLOCKED_DOWN) || !can_see(o,o->x,o->y,o->x,o->y+1))
      {
	o->set_state((character_state)S_landing);
	the_game->play_sound(S_ALAND_SND,127,o->x,o->y);
	o->set_aistate(ANT_LANDING);
      }
    } break;
    case ANT_LANDING :
    {
      if (!o->next_picture())
      {
	long xv=0,yv=2;
	o->try_move(o->x,o->y,xv,yv,1);
	if (yv!=0)
	{
	  o->set_gravity(1);
	  o->set_aistate(ANT_FALL_DOWN);
	}
	else
	{ 
	  o->set_state(stopped);
	  o->set_aistate(ANT_RUNNING);
	  return ant_ai;
	}
      }
    } break;
    case ANT_RUNNING :
    {
      if (player_list->next)
      b=current_level->attacker(current_object);
      else b=player_list->focus;
      scream_check(o,b);
      
      
      if ((jrand()%16)==0)  
      o->lvars[ANT_need_to_dodge]=1;
      if (!ant_dodge(o))
      {
	if ((o->x>b->x && o->direction==-1) || (o->x<b->x && o->direction==1))
	{
	  o->next_picture();
	  if ((jrand()%4)==0 && abs(o->x-b->x)<180 && abs(o->y-b->y)<100 && can_hit_player(o,b))
	  {
	    o->set_state((character_state)S_weapon_fire);
	    o->set_aistate(ANT_FIRE);
	  } else if (abs(o->x-b->x)<100 && abs(o->y-b->y)<10 && (jrand()%4)==0)
	    o->set_aistate(ANT_POUNCE_WAIT);
	  else if (abs(o->x-b->x)>140 && !ant_congestion(o))
	    o->set_aistate(ANT_JUMP);
	  else 
	  {
	    long xm=o->direction>0 ? get_ability(o->otype,run_top_speed) : -get_ability(o->otype,run_top_speed);
	    long ym=0,new_xm=xm;
	    if (o->state!=running) o->set_state(running);

	    o->try_move(o->x,o->y,new_xm,ym,3);
	    if (new_xm!=xm)    // blocked, see if we can climb ramp
	    {
	      new_xm=xm;
	      ym=-abs(xm);
	      o->try_move(o->x,o->y,new_xm,ym,3);
	      if (new_xm==xm)
	      {
		o->x+=new_xm;
		o->y+=ym;
		new_xm=0;
		ym=abs(xm);      // now get back on the ground
		o->try_move(o->x,o->y,new_xm,ym,3);
		o->x+=new_xm;
		o->y+=ym;
	      } else
	      {
		o->direction=0-o->direction;
		o->set_aistate(ANT_JUMP);
	      }
	    } else
	      o->x+=new_xm;
	    new_xm=0;
	    ym=10;       // see if we should fall
	    o->try_move(o->x,o->y,new_xm,ym,3);
	    if (ym==10)
	      o->set_aistate(ANT_FALL_DOWN);
	    else o->y+=ym;
	  }
	} else
	{
	  o->direction=o->x>b->x ? -1 : 1;
	  o->set_aistate(ANT_LANDING);
	}
      }
    } break;
    
    case ANT_POUNCE_WAIT :
    {
      if (!ant_dodge(o))
      {
	o->set_state((character_state)S_pounce_wait);
	if (o->aistate_time()>alien_wait_time())
	{
	  the_game->play_sound(S_ASLASH_SND,127,o->x,o->y);
	  o->set_state(stopped);
	  o->set_aistate(ANT_JUMP);
	}
      }
    } break;

    case ANT_JUMP :
    {
      o->lvars[ANT_need_to_dodge]=0;
      if (o->move(o->direction,-1,0)&BLOCKED_DOWN)
      o->set_aistate(ANT_RUNNING);     
    } break;
    case ANT_FIRE :
    {
      if (!ant_dodge(o))
      {
	if (o->state==S_fire_wait)
	{
	  if (!o->next_picture() || symbol_value(l_difficulty)==l_extreme)
	  {
	    if (player_list->next)
	    b=current_level->attacker(current_object);
	    else b=player_list->focus;
	    fire_at_player(o,b);
	    o->set_state(stopped);
	    o->set_aistate(ANT_RUNNING);
	  }
	} else o->set_state((character_state)S_fire_wait);
      }
    } break;
    case ANT_JUMP_ROOF :
    {
      o->lvars[ANT_need_to_dodge]=0;
      o->set_state((character_state)S_jump_up);
//      o->set_yvel(o->yvel()+1);
      o->set_xacel(0);
      long xv=0,yv=o->yvel();
      o->y-=31;
      o->try_move(o->x,o->y,xv,yv,1);
      o->y+=31+yv;
      if (yv!=o->yvel())
      {
	if (o->yvel()>0)
	{
	  o->set_state(stopped);
	  o->set_aistate(ANT_RUNNING);
	} else
	{
	  o->set_state((character_state)S_top_walk);
	  o->set_aistate(ANT_ROOF_WALK);
	}
	o->set_yvel(0);
      }
    } break;
    case ANT_ROOF_WALK :
    {
      if (player_list->next)
      b=current_level->attacker(current_object);
      else b=player_list->focus;
      scream_check(o,b);     
      if (((jrand()%8)==0 && abs(o->x-b->x)<10 && o->y<b->y) ||
	  o->lvars[ANT_need_to_dodge]==1)
      {
	o->set_gravity(1);
	o->set_state(run_jump);
	o->set_aistate(ANT_JUMP);
	ant_ai();
      }
      else
      {
	if ((o->x>b->x && o->direction>0) || (o->x<b->x && o->direction<0))
	o->direction=-o->direction;
	else if (abs(o->x-b->x)<120 && (jrand()%4)==0)
	{
	  o->set_state((character_state)S_ceil_fire);
	  o->set_aistate(ANT_CEIL_SHOOT);
	  ant_ai();
	} else
	{
	  int speed=o->direction>0 ? get_ability(o->otype,run_top_speed) : 
	            -get_ability(o->otype,run_top_speed);
	  if (can_see(o,o->x,o->y-31,o->x+speed,o->y-31) &&
	      !can_see(o,o->x+speed,o->y-31,o->x+speed,o->y-32))
	  {
	    o->x+=speed;
	    if (!o->next_picture()) o->set_state((character_state)S_top_walk);
	    
	  } else o->set_aistate(ANT_FALL_DOWN);
	}
      }
    } break;
    case ANT_CEIL_SHOOT :
    {
      if (!o->next_picture())
      {
	if (player_list->next)
	  b=current_level->attacker(current_object);
	else b=player_list->focus;
	fire_at_player(o,b);
	o->set_state((character_state)S_top_walk);
	o->set_aistate(ANT_ROOF_WALK);
      }
    } break;
  }
  
 
  
  return true_symbol;
}



void fade_in(image *im, int steps);
void fade_out(int steps);

time_marker stat_show_start;
static status_manager *old_stat;

class next_level_status_manager : public status_manager
{
  int level;
  int x1,y1,x2,y2,color,last;
  public :
  next_level_status_manager(int x1, int y1, int x2, int y2, int color) : x1(x1),y1(y1),x2(x2),y2(y2),color(color)
  {
    level=0;
    last=-1;
  }

  virtual void push(char *name, visual_object *show)
  {
    level++;
  }

  virtual void update(int percentage)
  {
    if (percentage!=last)
    {
      last=percentage;

      if (level==1)
      {
        int w=x1+(x2-x1)*percentage/100;
        screen->bar(x1,y1,w,y2,color);
        if (w<x2)
          screen->bar(w,y1,x2,y2,0);
      }
      eh->flush_screen();
    }

  }

  virtual void pop() { level--; }
  virtual void force_display() { ; }
} ;

void show_stats()
{
  if (current_level)
  {
    int xp,yp;

    if (!player_list->next)
    {
      fade_out(8);
      eh->set_mouse_position(0,0);
      screen->clear();

      switch_mode(VMODE_640x480);

      load_image_into_screen("art/frame.spe","end_level_screen",xp,yp);
      fade_in(NULL,16);
    } else
    {
      xp=screen->width()/2-640/2;
      yp=screen->height()/2-480/2;
    }



    char name[50];
    strcpy(name,current_level->original_name());
    char dig1=name[strlen(name)-strlen(".spe")-2];
    char dig2=name[strlen(name)-strlen(".spe")-1];

   
    char msg[50];
    int lev_num;


    if (isdigit(dig1) && isdigit(dig2))
      lev_num=(dig1-'0')*10+dig2-'0';
    else
      lev_num=dig2-'0';

    sprintf(msg,symbol_str("lev_complete"),lev_num);   

    int w=eh->font()->width()*strlen(msg),h=eh->font()->height();    
    int x1=0,y1=0,x2=xres,y2=yres;
    //    int x=(x1+x2)/2-w/2,y=(y1+y2)/2-h/2;
    int x=444+xp,y=293+yp;   
   
    eh->font()->put_string(screen,x+1,y+1,msg,eh->dark_color());    
    eh->font()->put_string(screen,x,y,msg,eh->bright_color());
    
    y+=eh->font()->height();
    sprintf(msg,"Time taken.. %0.1f minutes",current_level->time_taken()/60.0);
    eh->font()->put_string(screen,x+1,y+1,msg,eh->dark_color());    
    eh->font()->put_string(screen,x,y,msg,eh->bright_color());

    y+=eh->font()->height();
    sprintf(msg,"Kills....... %d",player_list->kills);
    eh->font()->put_string(screen,x+1,y+1,msg,eh->dark_color());    
    eh->font()->put_string(screen,x,y,msg,eh->bright_color());



    eh->flush_screen();
    stat_show_start.get_time();

    old_stat=stat_man;
    stat_man=new next_level_status_manager(xp+443,yp+424,xp+589,yp+443,93);  
  }
}



void end_stats()
{
  time_marker now;
  while (now.diff_time(&stat_show_start)<2)
    now.get_time();

  view *f=player_list;
  for (;f;f=f->next)
    f->kills=0;

  fade_out(8);
  switch_mode(VMODE_320x200);
  pal->load();
  stat_man=old_stat;
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


