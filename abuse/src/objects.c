#include "timage.hpp"
#include "objects.hpp"
#include "chars.hpp"

#include "game.hpp"
#include "intsect.hpp"
#include "ability.hpp"
#include "lisp.hpp"
#include "jrand.hpp"
#include "light.hpp"
#include "dprint.hpp"
#include "clisp.hpp"
#include "lisp_gc.hpp"
#include "profile.hpp"

#ifdef SCADALISP
#define LCAR(x)		CAR(x)
#define LCDR(x)		CDR(x)
#else
#define LCAR(x)		(x)->car
#define LCDR(x)		(x)->cdr
#endif /* SCADALISP */

char **object_names;
int total_objects;
game_object *current_object;
view *current_view;

game_object *game_object::copy()
{
  game_object *o=create(otype,x,y);
  o->state=state;
  int i=0;
  for (;i<TOTAL_OBJECT_VARS;i++)
    o->set_var(i,get_var(i));
  memcpy(o->lvars,lvars,4*figures[otype]->tv);
  for (i=0;i<total_objects();i++)
    o->add_object(get_object(i));
  for (i=0;i<total_lights();i++)
    o->add_light(get_light(i));
  return o;
}

int simple_object::total_vars() { return TOTAL_OBJECT_VARS; }


obj_desc object_descriptions[TOTAL_OBJECT_VARS]={
				{"fade_dir",      RC_C },
				{"frame_dir",     RC_C },
				{"direction",     RC_C },
				{"gravity_on",    RC_C },
				{"fade_count",    RC_C },

				{"fade_max",      RC_C },
				{"active",        RC_C },
				{"flags",         RC_C },
				{"aitype",        RC_C },
				{"xvel",          RC_L },

				{"fxvel",         RC_C },
				{"yvel",          RC_L },
				{"fyvel",         RC_C },
				{"xacel",         RC_L },
				{"fxacel",        RC_C },

				{"yacel",         RC_L },
				{"fyacel",        RC_C },
				{"x",             RC_L },
				{"fx",            RC_C },
				{"y",             RC_L },

				{"fy",            RC_C },
				{"hp",            RC_S },
				{"mp",            RC_S },
				{"fmp",           RC_S },
				{"cur_frame",     RC_S },

				{"aistate",       RC_S },
				{"aistate_time",  RC_S },
				{"targetable",    RC_C }

			      };
  
long game_object::get_var_by_name(char *name, int &error)
{
  error=0;
  int i=0;
  for (;i<TOTAL_OBJECT_VARS;i++)
  {
    if (!strcmp(object_descriptions[i].name,name))
      return get_var(i);
  }

  for (i=0;i<figures[otype]->tiv;i++)
  {
    if (!strcmp(lstring_value(symbol_name(figures[otype]->vars[i])),name))
    {
      return lvars[figures[otype]->var_index[i]];
/*      lisp_object_var *cobj=(lisp_object_var *)symbol_value(figures[otype]->vars[i]);
      character_type *t=figures[otype];
      int number=cobj->number;
      if (t->tiv<=number || !t->vars[number])
      {
	lbreak("access : variable does not exsist for this class\n");
	return 0;
      }
      return lvars[t->var_index[number]];*/
    }
  }
  error=1;
  return 0;
}

int game_object::set_var_by_name(char *name, long value)
{
  int i=0;
  for (;i<TOTAL_OBJECT_VARS;i++)
  {
    if (!strcmp(object_descriptions[i].name,name))
    {
      set_var(i,value);
      return 1;
    }
  }
  for (i=0;i<figures[otype]->tiv;i++)
    if (!strcmp(lstring_value(symbol_name(figures[otype]->vars[i])),name))
    {
      lvars[figures[otype]->var_index[i]]=value;
      return 1;
    }
  return 0;
}


char *simple_object::var_name(int x)
{
  return object_descriptions[x].name;
}

int simple_object::var_type(int x)
{
  return object_descriptions[x].type;
}


void simple_object::set_var(int xx, ulong v)
{
  switch (xx)
  {
    case 0 : set_fade_dir(v); break;
    case 1 : set_frame_dir(v); break;
    case 2 : direction=v; break;
    case 3 : set_gravity(v); break;
    case 4 : set_fade_count(v); break;
    case 5 : set_fade_max(v); break;
    case 6 : active=v;break;
    case 7 : set_flags(v); break;
    case 8 : set_aitype(v); break;
    case 9 : set_xvel(v); break;

    case 10 : set_fxvel(v); break;
    case 11 : set_yvel(v); break;
    case 12 : set_fyvel(v); break;
    case 13 : set_xacel(v); break;
    case 14 : set_fxacel(v); break;

    case 15 : set_yacel(v); break;
    case 16 : set_fyacel(v); break;
    case 17 : x=v; break;
    case 18 : set_fx(v); break;
    case 19 : y=v; break;

    case 20 : set_fy(v); break;
    case 21 : set_hp(v); break;
    case 22 : set_mp(v); break;
    case 23 : set_fmp(v); break;

    case 24 : current_frame=v;  break;
    case 25 : set_aistate(v); break;

    case 26 : set_aistate_time(v); break;
    case 27 : set_targetable(v); break;
  }
}

long simple_object::get_var(int xx)
{
  switch (xx)
  {
    case 0 : return fade_dir(); break;
    case 1 : return frame_dir(); break;
    case 2 : return direction; break;
    case 3 : return gravity(); break;
    case 4 : return fade_count(); break;
    case 5 : return fade_max(); break;
    case 6 : return active; break;
    case 7 : return flags(); break;
    case 8 : return aitype(); break;
    case 9 : return xvel(); break;
    case 10 : return fxvel(); break;

    case 11 : return yvel(); break;
    case 12 : return fyvel(); break;

    case 13 : return xacel(); break;
    case 14 : return fxacel(); break;

    case 15 : return yacel(); break;
    case 16 : return fyacel(); break;

    case 17 : return x; break;
    case 18 : return fx(); break;

    case 19 : return y; break;
    case 20 : return fy(); break;

    case 21 : return hp(); break;
    case 22 : return mp(); break;
    case 23 : return fmp(); break;

    case 24 : return current_frame;  break;
    case 25 : return aistate(); break;
    case 26 : return aistate_time(); break;
    case 27 : return targetable();
  }
  return 0;
}




int RC_type_size(int type)
{
  switch (type)
  {
    case RC_C : 
    { return 1; } break;
    case RC_S : 
    { return 2; } break;
    case RC_L : 
    { return 4; } break;
  }		
  CHECK(0);
  return 1;
} 

void game_object::reload_notify()
{
  void *ns=figures[otype]->get_fun(OFUN_RELOAD);  
  if (ns)
  {
    game_object *o=current_object;
    current_object=this;

    void *m=mark_heap(TMP_SPACE);
    eval_function((lisp_symbol *)ns,NULL);
    restore_heap(m,TMP_SPACE);

    current_object=o;
  }
}

void game_object::next_sequence()
{
  void *ns=figures[otype]->get_fun(OFUN_NEXT_STATE);
  if (ns)
  {  
    current_object=this;
    void *m=mark_heap(TMP_SPACE);
    void *ret=eval_function((lisp_symbol *)ns,NULL);
    restore_heap(m,TMP_SPACE);
  } else
  {
    switch (state)
    {
      case dieing : 
      { set_state(dead); } break;

      case end_run_jump :
      {
	set_state(running);
      } break;
      case dead :
      case run_jump :
      case run_jump_fall :
      case running :
      { set_state(state); } break;
      case start_run_jump :
      { 
	set_yvel(get_ability(type(),jump_yvel));
	if (xvel()>0)
        set_xvel(get_ability(type(),jump_xvel));
	else if (xvel()<0)
        set_xvel(-get_ability(type(),jump_xvel));
	set_xacel(0);
	set_fxacel(0);
	set_gravity(1);      
	set_state(run_jump);
      } break;

      case flinch_up :
      case flinch_down :
      {
	if (gravity())
	{
	  if (has_sequence(end_run_jump))
	    set_state(end_run_jump);
	  else set_state(stopped);
	} else set_state(stopped);
      } break;

      
      default :
      { set_state(stopped); } break;
    }
  }

}



game_object::~game_object()
{
  if (lvars) jfree(lvars);
  clean_up();
}

void game_object::add_power(int amount)  
{ 
  int max_power=lnumber_value(symbol_value(l_max_power));  
  int n=mp()+amount;
  if (n<0) n=0;
  if (n>max_power) n=max_power;
  set_mp(n);
}

void game_object::add_hp(int amount)
{
  if (controller() && controller()->god) return ;
  int max_hp=lnumber_value(symbol_value(l_max_hp));  
  int n=hp()+amount;
  if (n<0) n=0;
  if (n>max_hp)
    n=max_hp;
  set_hp(n);
}

int game_object::can_morph_into(int type)
{
  if (type!=otype && mp()>=figures[type]->morph_power)
    return 1;
  else return 0;
}

void game_object::morph_into(int type, void (*stat_fun)(int), int anneal, int frames)
{
  set_morph_status(new morph_char(this,type,stat_fun,anneal,frames));
  otype=type;
  set_state(stopped);
}

void game_object::draw_above(view *v)
{
  long x1,y1,x2,y2,sy1,sy2,sx,i;
  picture_space(x1,y1,x2,y2);    

  the_game->game_to_mouse(x1,y1,v,sx,sy2);
  if (sy2>=v->cy1)
  {
    long draw_to=y1-(sy2-v->cy1),tmp=x;
    current_level->foreground_intersect(x,y1,tmp,draw_to);     
    the_game->game_to_mouse(x1,draw_to,v,i,sy1);     // calculate sy1

    sy1=max(v->cy1,sy1);
    sy2=min(v->cy2,sy2);
    trans_image *p=picture();
    
    for (i=sy1;i<=sy2;i++)
      p->put_scan_line(screen,sx,i,0);  
  }  
}

int game_object::push_range()
{
  return get_ability(otype,push_xrange);
}

int game_object::decide()
{
  if (figures[otype]->get_fun(OFUN_AI))
  {
    int old_aistate;
    old_aistate=aistate();

    current_object=this;
    void *m=mark_heap(TMP_SPACE);

    time_marker *prof1;
    if (profiling())
      prof1=new time_marker;

    Cell *ret=(Cell *)eval_function((lisp_symbol *)figures[otype]->get_fun(OFUN_AI),NULL);
    if (profiling())
    {
      time_marker now;
      profile_add_time(this->otype,now.diff_time(prof1));
      delete prof1;
    }

    restore_heap(m,TMP_SPACE);

    if (keep_ai_info())
    {
      if (aistate()!=old_aistate)
      set_aistate_time(0);
      else set_aistate_time(aistate_time()+1);    
    }
    if (!NILP(ret)) return 1;
    else return 0;
  }
  else move(0,0,0);
  return 1;
}

int game_object::can_hurt(game_object *who)     // collision checking will ask first to see if you
{
  int is_attacker=current_level->is_attacker(this);  
  // it's you against them!  Damage only if it you are attacking or they are attacking you
  // I.E. don't let them hurt themselves, this can change if you over-ride this virtual function

  if (who->hurtable() && (is_attacker || current_level->is_attacker(who) || hurt_all()))
    return 1;
  else return 0;  
}

void game_object::note_attack(game_object *whom)
{
  return ;   // nevermind
}

void game_object::do_flinch(game_object *from)
{
  if (jrandom(2) && has_sequence(flinch_down))
    set_state(flinch_down);
  else if (has_sequence(flinch_up))
    set_state(flinch_up);
}

   
void game_object::do_damage(int amount, game_object *from, long hitx, long hity, 
			    long push_xvel, long push_yvel) 
{

  void *d=figures[otype]->get_fun(OFUN_DAMAGE);  
  if (d)
  {
    void	*am, *frm, *hx, *hy, *px, *py;  
    game_object *o=current_object;
    current_object=this;

    void *m=mark_heap(TMP_SPACE);

    am=new_cons_cell();
    l_ptr_stack.push(&am);

    ((cons_cell *)am)->car=new_lisp_number(amount);   

    frm=new_cons_cell();
    l_ptr_stack.push(&frm);

    ((cons_cell *)frm)->car=new_lisp_pointer(from);

    hx=new_cons_cell();
    l_ptr_stack.push(&hx);

    ((cons_cell *)hx)->car=new_lisp_number(hitx);

    hy=new_cons_cell();
    l_ptr_stack.push(&hy);
    ((cons_cell *)hy)->car=new_lisp_number(hity);

    px=new_cons_cell();
    l_ptr_stack.push(&px);
    ((cons_cell *)px)->car=new_lisp_number(push_xvel);

    py=new_cons_cell();
    l_ptr_stack.push(&py);
    ((cons_cell *)py)->car=new_lisp_number(push_yvel);


    ((cons_cell *)am)->cdr=frm;
    ((cons_cell *)frm)->cdr=hx;
    ((cons_cell *)hx)->cdr=hy;
    ((cons_cell *)hy)->cdr=px;
    ((cons_cell *)px)->cdr=py;

    time_marker *prof1;
    if (profiling())
      prof1=new time_marker;

    eval_user_fun((lisp_symbol *)d,am);
    if (profiling())
    {
      time_marker now;
      profile_add_time(this->otype,now.diff_time(prof1));
      delete prof1;
    }


    l_ptr_stack.pop(6);
   
    restore_heap(m,TMP_SPACE);

    current_object=o;
  } else damage_fun(amount,from,hitx,hity,push_xvel,push_yvel);
#ifdef SCADALISP
  ENDLOCAL();
#endif
}

void game_object::damage_fun(int amount, game_object *from, long hitx, long hity, 
			    long push_xvel, long push_yvel) 
{ 
  if (!hurtable() || !alive()) return ;

  add_hp(-amount);
  set_flags(flags()|FLAG_JUST_HIT);
  do_flinch(from);

  
  set_xvel(xvel()+push_xvel);
  if (push_yvel<0 && !gravity())
    set_gravity(1);
  set_yvel(yvel()+push_yvel);

  view *c=controller();
  if (c && hp()<=0)
  {
    view *v=from->controller();
    if (v) v->kills++;                       // attack from another player?
    else if (from->total_objects()>0)
    {
      v=from->get_object(0)->controller();   // weapon from another player?
      if (v && v!=c) v->kills++;
      else
      {
	v=c;                                 // sucide
	if (v) v->kills--;
      }
    }
  }
}
     


int game_object::facing_attacker(int attackerx)
{ 
  return ((attackerx<x && direction<0) || (attackerx>=x && direction>0)); 

}


void game_object::picture_space(long &x1, long &y1,long &x2, long &y2)
{
  int xc=x_center(),w=picture()->width(),h=picture()->height();  
  if (direction>0)
    x1=x-xc;  
  else x1=x-(w-xc-1);  
  x2=x1+w-1;
  y1=y-h+1;
  y2=y;  
}


int game_object::next_picture()
{
  int ret=1;
  if (frame_dir()>0)
  {
    if (!current_sequence()->next_frame(current_frame))
    {
      next_sequence();
      ret=0;
    }
  }
  else 
  {
    if (!current_sequence()->last_frame(current_frame))
    {
      next_sequence();
      ret=0;
    }
  }
  frame_advance();
  return ret;
}


long game_object::x_center()
{
  return current_sequence()->x_center(current_frame);   
}


void game_object::draw()
{
  if (figures[otype]->get_fun(OFUN_DRAW))
  {
    current_object=this;

    void *m=mark_heap(TMP_SPACE);
    time_marker *prof1;
    if (profiling())
      prof1=new time_marker;

    eval_function((lisp_symbol *)figures[otype]->get_fun(OFUN_DRAW),NULL);
    if (profiling())
    {
      time_marker now;
      profile_add_time(this->otype,now.diff_time(prof1));
      delete prof1;
    }



    restore_heap(m,TMP_SPACE);

  } else drawer();
}


void game_object::map_draw()
{
  if (figures[otype]->get_fun(OFUN_MAP_DRAW))
  {
    current_object=this;

    void *m=mark_heap(TMP_SPACE);
    time_marker *prof1;
    if (profiling())
      prof1=new time_marker;

    eval_function((lisp_symbol *)figures[otype]->get_fun(OFUN_MAP_DRAW),NULL);
    if (profiling())
    {
      time_marker now;
      profile_add_time(this->otype,now.diff_time(prof1));
      delete prof1;
    }

    restore_heap(m,TMP_SPACE);

  }
}

void game_object::draw_trans(int count, int max)
{
  trans_image *cpict=picture();
  cpict->put_fade(screen,
		  (direction<0 ? x-(cpict->width()-x_center()-1) : x-x_center())-current_vxadd,
		  y-cpict->height()+1-current_vyadd,
		  count,max,
		  color_table,the_game->current_palette());
}


void game_object::draw_tint(int tint_id)
{
  trans_image *cpict=picture();
  if (fade_count())      
    cpict->put_fade_tint(screen,
		       (direction<0 ? x-(cpict->width()-x_center()-1) : x-x_center())-current_vxadd,
		       y-cpict->height()+1-current_vyadd,
		       fade_count(),fade_max(),
		       cash.ctint(tint_id)->data,
		       color_table,the_game->current_palette());


  else
    cpict->put_remaped(screen,
		       (direction<0 ? x-(cpict->width()-x_center()-1) : x-x_center())-current_vxadd,
		       y-cpict->height()+1-current_vyadd,
		       cash.ctint(tint_id)->data);
}


void game_object::draw_double_tint(int tint_id, int tint2)
{
  trans_image *cpict=picture();
  if (fade_count())      
    cpict->put_fade_tint(screen,
		       (direction<0 ? x-(cpict->width()-x_center()-1) : x-x_center())-current_vxadd,
		       y-cpict->height()+1-current_vyadd,
		       fade_count(),fade_max(),
		       cash.ctint(tint_id)->data,
		       color_table,the_game->current_palette());


  else
    cpict->put_double_remaped(screen,
		       (direction<0 ? x-(cpict->width()-x_center()-1) : x-x_center())-current_vxadd,
		       y-cpict->height()+1-current_vyadd,
		       cash.ctint(tint_id)->data,
		       cash.ctint(tint2)->data);
}



void game_object::draw_predator()
{
  trans_image *cpict=picture();
  cpict->put_predator(screen,
		     (direction<0 ? x-(cpict->width()-x_center()-1) : x-x_center())-current_vxadd,
		     y-cpict->height()+1-current_vyadd);
		    
}

void game_object::drawer()
{
  trans_image *cpict;

  if (morph_status())
  {
    morph_status()->draw(this,current_view);
    if (morph_status()->frames_left()<1)
      set_morph_status(NULL);
  }
  else
  {
    view *v=controller();

    if (fade_count())      
      draw_trans(fade_count(),fade_max());
    else
    {
      trans_image *cpict=picture();
      cpict->put_image(screen,
		       (direction<0 ? x-(cpict->width()-x_center()-1) : x-x_center())-current_vxadd,
		       y-cpict->height()+1-current_vyadd);
    }
  }
}

game_object *game_object::try_move(long x, long y, long &xv, long &yv, int checks)
{
  if (xv || yv)  // make sure they are suggesting movement
  {    
    game_object *who1=NULL,*who2=NULL;      // who did we intersect?  
    long x2,y2,h;  

    if (checks&1)
    {      
      x2=x+xv;
      y2=y+yv;    
      current_level->foreground_intersect(x,y,x2,y2);
      if (!stoppable())
        who1=current_level->boundary_setback(this,x,y,x2,y2);
      else
        who1=current_level->all_boundary_setback(this,x,y,x2,y2);  
      xv=x2-x;
      yv=y2-y;
    }
    

    if (checks&2)
    {      
      h=picture()->height();    
      x2=x+xv;    
      y2=y-h+1+yv; 
      current_level->foreground_intersect(x,y-h+1,x2,y2);
      if (!stoppable())
        who2=current_level->all_boundary_setback(this,x,y-h+1,x2,y2);    
      else
        who2=current_level->boundary_setback(this,x,y-h+1,x2,y2);
      xv=x2-x;
      yv=y2-y+h-1;  
    }
        
    if (who2) return who2;
    else return who1;
  }
  else return NULL;  
}

void *game_object::float_tick()  // returns 1 if you hit something, 0 otherwise
{
  long ret=0;
  if (hp()<=0)
  {
    if (state!=dead)
    {
      set_xacel(0);
      set_fxacel(0);

      if (has_sequence(dieing))
      {      
	if (state!=dieing)
	{
	  set_state(dieing);
	  set_xvel(0);
	}
      } else 
      { set_xvel(0); 
	set_fxvel(0);
	if (has_sequence(dead))
          set_state(dead); 
	else return 0;
      }
    } 
  }

  long fxv=sfxvel()+sfxacel(),fyv=sfyvel()+sfyacel();
  long xv=xvel()+xacel()+(fxv>>8),yv=yvel()+yacel()+(fyv>>8);

  if (xv!=xvel() || yv!=yvel())   // only store vel's if changed so we don't increase object size
  {
    set_xvel(xv);
    set_yvel(yv);
  }

  if (fxv!=sfxvel() || fyv!=sfyvel())
  {
    set_fxvel(fxv&0xff);
    set_fyvel(fyv&0xff);
  }

 
  if (fxv || fyv || xv || yv)   // don't even try if there is no velocity
  {
    long nx=x,ny=y;
    long ffx=fx()+sfxvel(),ffy=fy()+sfyvel();
    long nxv=xvel()+(ffx>>8);
    long nyv=yvel()+(ffy>>8);
    set_fx(ffx&0xff);
    set_fy(ffy&0xff);
    
    long old_nxv=nxv,old_nyv=nyv;
    game_object *hit_object=try_move(x,y,nxv,nyv,3);   // now find out what velocity is safe to use
    
/*    if (get_cflag(CFLAG_STOPPABLE))
    {
      game_object *r=current_level->boundary_setback(exclude,x,y,nxv,nyv,1);
      if (r) hit_object=r;
    }*/

    x+=nxv;
    y+=nyv;
    if (old_nxv!=nxv || old_nyv!=nyv)
    {
      long lx=last_tile_hit_x,ly=last_tile_hit_y;
      stop();
      if (old_nxv==0)
      {
	if (old_nyv>0) ret|=BLOCKED_DOWN;
	else if (old_nyv<0) ret|=BLOCKED_UP;
      } else if (old_nyv==0)
      {
	if (old_nxv>0) ret|=BLOCKED_RIGHT;
	else if (old_nxv<0) ret|=BLOCKED_LEFT;
      } else
      {
	long tx=(old_nxv>0 ? 1 : -1),ty=0;
	try_move(x,y,tx,ty,3);
	if (!tx) 	
	  ret|=(old_nxv>0 ? BLOCKED_RIGHT : BLOCKED_LEFT);	
	else tx=0;

	ty=(old_nyv>0 ? 1 : -1);
	try_move(x,y,tx,ty,3);
	if (!ty) 	
	  ret|=(old_nyv>0 ? BLOCKED_DOWN : BLOCKED_UP);
	
	if (!ret)
	  ret|=(old_nyv>0 ? BLOCKED_DOWN : BLOCKED_UP) | (old_nxv>0 ? BLOCKED_RIGHT : BLOCKED_LEFT);

      }

      void *rlist=NULL;   // return list
      p_ref r1(rlist);

      if (hit_object)
      {
	push_onto_list(new_lisp_pointer(hit_object),rlist);
	push_onto_list(l_object,rlist);
      } else
      {
	push_onto_list(new_lisp_number(ly),rlist);
	push_onto_list(new_lisp_number(lx),rlist);
	push_onto_list(l_tile,rlist);
      }
      push_onto_list(new_lisp_number(ret),rlist);

      return rlist;
    } else return true_symbol;
  }
  return true_symbol;
}

int game_object::tick()      // returns blocked status
{
  int blocked=0;

  long xt=0,yt=2;
  try_move(x,y-2,xt,yt,1);    // make sure we are not falling through the floor
  y=y-2+yt;
  
  if (flags()&FLAG_JUST_BLOCKED)
    set_flags(flags()-FLAG_JUST_BLOCKED);

  if (gravity() && !floating())
  {
    int fya;
    if (yacel()>=0)
      fya=sfyacel()+200;
    else
      fya=sfyacel()-200;

    set_yacel(yacel()+(fya>>8));
    set_fyacel(fya&255);
  }
  
  // first let's move the guy acording to his physics
  long xa=xacel(),ya=yacel(),fxa=sfxacel(),fya=sfyacel();
  if (xa || ya || fxa || fya)
  {
    int fxv=sfxvel(),fyv=sfyvel();
    fxv+=fxa;  fyv+=fya;    
    long xv=xvel()+xa+(fxv>>8); 
    set_xvel(xvel()+xa+(fxv>>8));
    set_yvel(yvel()+ya+(fyv>>8));
    set_fxvel(fxv&0xff);
    set_fyvel(fyv&0xff);
  }
  
  // check to see if this advancement causes him to collide with objects
  long old_vy=yvel(),old_vx=xvel();  // save the correct veloicties

  if (old_vx || old_vy)
  {
    int up=0;
    if (yvel()<=0)  // if we are going up or a strait across check up and down
    up=2;
    long xv=xvel(),yv=yvel();
    game_object *h=try_move(x,y,xv,yv,1|up);       // now find out what velocity is safe to use
    set_xvel(xv);
    set_yvel(yv);
    x+=xv;
    y+=yv;

    if (h && stoppable()) return BLOCKED_LEFT|BLOCKED_RIGHT;

    if (xv!=old_vx || yv!=old_vy)     // he collided with something
    {          
      if (gravity())                         // was he going up or down?
      {	                       
	long fall_xv=0,old_fall_vy,fall_vy;
	old_fall_vy=fall_vy=old_vy-yvel();             // make sure he gets all of his yvel
	try_move(x,y,fall_xv,fall_vy,1|up);
	if (old_vy>0 && fall_vy<old_fall_vy)       // he was trying to fall, but he hit the ground
	{
	  if (old_vy>0)	
	  {
	    blocked|=BLOCKED_DOWN;
	    
	    if (!xvel() && has_sequence(end_run_jump))	  
	    {
	      set_xvel(old_vx);
	      set_state(end_run_jump);
	    }
	    else set_state(stopped);	  
	  }
	  else blocked|=BLOCKED_UP;


	  if (state==run_jump_fall)
	  {
	    if (has_sequence(running))
	    set_state(running);
	    else
	    {
	      stop_x();
	      set_state(stopped);
	    }
	  }
	  else
	  {
	    set_yacel(0);
	    set_fyacel(0);
	    set_yvel(0);
	    set_fyvel(0);
	    set_gravity(0);
	  }

	} else 
	{
	  if (old_vy!=0)
	  {
	    long testx=old_vx<0 ? -1 : 1,testy=0;    // see if we were stopped left/right
                                                     // or just up down
	    try_move(x,y,testx,testy,1|up);
	    if (testx==0)                           // blocked left/right, set flag
	    {
	      if (old_vx<0)
	        blocked|=BLOCKED_LEFT;
	      else
	        blocked|=BLOCKED_RIGHT;
	    }
	    else if (old_vy<0)
	      blocked|=BLOCKED_UP;
	    else if (old_vy>0)
	      blocked|=BLOCKED_DOWN;

	  } else if (old_vx<0)   // we can skip left/right test because player wasn't moving up/down
            blocked|=BLOCKED_LEFT;
	  else
            blocked|=BLOCKED_RIGHT;

	  set_xvel(0);
	  set_fxvel(0);
	  if (old_vy<0 && fall_vy>0)        
	  {
	    set_yvel(yvel()+fall_vy);
	  } else set_yvel(yvel()+fall_vy);
	}      
	y+=fall_vy;            
      }    
      else                  // see if we can make him 'climb' the hill
      {
	long ox=x,oy=y;       // rember orginal position in case climb doesn't work

	long climb_xvel=0,climb_yvel=-5;	    // try to move up one pixel to step over the 
	try_move(x,y,climb_xvel,climb_yvel,3);  // jutting polygon line
	y+=climb_yvel;

	climb_xvel=old_vx-xvel();
	climb_yvel=-(abs(climb_xvel));	    // now try 45 degree slope
	try_move(x,y,climb_xvel,climb_yvel,3);

	if (abs(climb_xvel)>0)  // see if he got further by climbing
	{
	  blocked=blocked&(~(BLOCKED_LEFT|BLOCKED_RIGHT));
	  x+=climb_xvel;
	  y+=climb_yvel;

	  set_xvel(xvel()+climb_xvel);
	  set_yvel(0);          
	  set_fyvel(0);
	  
	  // now put him back on the ground          
	  climb_yvel=abs(climb_xvel)+5;               // plus one to put him back on the ground
	  climb_xvel=0;
	  try_move(x,y,climb_xvel,climb_yvel,1);
	  if (climb_yvel)
          y+=climb_yvel;
	}
	else	         
	{	
	  if (old_vx<0)
          blocked|=BLOCKED_LEFT;
	  else
          blocked|=BLOCKED_RIGHT;
	  set_state(stopped);      // nope, musta hit a wall, stop the poor fella
	  x=ox;
	  y=oy;	
	}
	
      }
      
      if (xvel()!=old_vx && state!=run_jump_fall && state!=end_run_jump)
      {
	set_xacel(0); 
	set_fxacel(0);
      }
    }
  }
     
  if (yacel()==0 && !gravity())       // he is not falling, make sure he can't
  {
    long nvx=0,nvy=yvel()+12;  // check three pixels below for ground
    try_move(x,y,nvx,nvy,1); 
    if (nvy>11)                    // if he falls more than 2 pixels, then he falls
    {
      if (state!=run_jump_fall)      // make him fall
      {       
        if (has_sequence(run_jump_fall))
          set_state(run_jump_fall);
        set_gravity(1);
      }
    } else if (nvy)   // if he fells less than 3 pixels then let him descend 'hills'
    {      
      y+=nvy;
      blocked|=BLOCKED_DOWN;      
    }    
  }
  return blocked;
}

void game_object::defaults()
{
  set_state(state);
  if (otype!=0xffff)
  {
    int s=get_ability(otype,start_hp);
    if (s!=default_simple.hp())
      set_hp(s);
  }
}

void game_object::frame_advance()
{
  int ad=current_sequence()->get_advance(current_frame);
  if (ad && current_level)
  {
    long xv;
    if (direction>0) xv=ad; else xv=-ad;
    long yv=0;
    try_move(x,y,xv,yv,3);
    x+=xv;
  }   
}

void game_object::set_state(character_state s, int frame_direction)
{
  if (has_sequence(s))
    state=s;
  else state=stopped;

  current_frame=0;  
  if (frame_direction!=1)
    set_frame_dir(frame_direction); 

  frame_advance();
}


game_object *create(int type, long x, long y, int skip_constructor, int aitype)
{
  game_object *g=new game_object(type,skip_constructor);
  g->x=x; g->y=y; g->last_x=x; g->last_y=y;
  if (aitype)
    g->set_aitype(aitype);
  if (figures[type]->get_fun(OFUN_CONSTRUCTOR) && !skip_constructor)
  {
    game_object *o=current_object;
    current_object=g;    

    void *m=mark_heap(TMP_SPACE);

    time_marker *prof1;
    if (profiling())
      prof1=new time_marker;

    eval_function((lisp_symbol *)figures[type]->get_fun(OFUN_CONSTRUCTOR),NULL);
    if (profiling())
    {
      time_marker now;
      profile_add_time(type,now.diff_time(prof1));
      delete prof1;
    }



    restore_heap(m,TMP_SPACE);

    current_object=o;
  }
  return g;
}

int base_size()
{
  return 1+  
         1*8+
	 2*5+
	 4*7;
}

int game_object::size()
{
  return base_size();
}



int game_object::move(int cx, int cy, int button)
{  
  int ret=0;

  if (figures[otype]->get_fun(OFUN_MOVER))      // is a lisp move function defined?
  {
    void *lcx,*lcy,*lb;

    game_object *o=current_object;
    current_object=this;


    // make a list of the parameters, and call the lisp function
    lcx=new_cons_cell();
    l_ptr_stack.push(&lcx);
    ((cons_cell *)lcx)->car=new_lisp_number(cx);

    lcy=new_cons_cell();
    l_ptr_stack.push(&lcy);
    ((cons_cell *)lcy)->car=new_lisp_number(cy);

    lb=new_cons_cell();
    l_ptr_stack.push(&lb);
    ((cons_cell *)lb)->car=new_lisp_number(button);


    ((cons_cell *)lcx)->cdr=lcy;
    ((cons_cell *)lcy)->cdr=lb;

    void *m=mark_heap(TMP_SPACE);

    time_marker *prof1;
    if (profiling())
      prof1=new time_marker;

    void *r=eval_function((lisp_symbol *)figures[otype]->get_fun(OFUN_MOVER),
			  (void *)lcx);
    if (profiling())
    {
      time_marker now;
      profile_add_time(this->otype,now.diff_time(prof1));
      delete prof1;
    }

    restore_heap(m,TMP_SPACE);

    l_ptr_stack.pop(3);
    if (item_type(r)!=L_NUMBER)
    {
      lprint(r);
      lbreak("Object %s did not return a number from it's mover function!\n"
	     "It should return a number to indicate it's blocked status to the\n"
	     "ai function.",object_names[otype]);	
    }
    ret|=lnumber_value(r);
    current_object=o;
  }
  else ret|=mover(cx,cy,button);    

  return ret;
}

int game_object::mover(int cx, int cy, int button)  // return false if the route is blocked
{
  if (hp()<=0) 
    return tick();

  if (flinch_state(state))                    // flinching? don't move
    cx=cy=button=0;
    
 
  if (cx)          // x movement suggested?
  {      
    if (state==stopped)   // see if started moving
    {   
      if (has_sequence(running))
      {
	if (cx>0)
	{
	  direction=1;
          set_xvel(get_ability(type(),run_top_speed));
	}
	else
	{
	  direction=-1;
	  set_xacel(-get_ability(type(),run_top_speed));
	}
	set_state(running);
      }
    } else if (state==run_jump || state==run_jump_fall)
    {
      if (cx>0)
      {
	direction=1;
	set_xacel(get_ability(type(),start_accel));           // set the appropriate accel
      } else
      {
	direction=-1;
	set_xacel(-get_ability(type(),start_accel));           // set the appropriate accel
      }	  
    }
    else 
    {
      // turn him around if he pressed the other way, he is not walking so a fast turn
      // is needed, don't go through the turn sequence
      if ((cx>0  && direction<0) || (cx<0 && direction>0))
        direction=-direction;      
    } 
  }         // not pressing left or right, so slow down or stop
  else if (!gravity() && state!=start_run_jump)
  {    
    long stop_acel;
    if (xvel()<0)                                    // he was going left
    {
      stop_acel=get_ability(type(),stop_accel);    // find out how fast he can slow down
      if (xvel()+stop_acel>=0)                       // if this acceleration is enough to stop him
      {
	stop_x();
	if (!gravity())	
	  set_state(stopped);      
      } else { set_xacel(stop_acel); }
    } else if (xvel()>0)
    {
      stop_acel=-get_ability(type(),stop_accel);
      if (xvel()+stop_acel<=0)
      {
	stop_x();
	if (!gravity())
	  set_state(stopped);
      } else set_xacel(stop_acel);
    } else if (!gravity())                // don't stop in the air
    { 
      set_xacel(0);
      set_fxacel(0);
      // Check to see if we should go to stop state 
      if (state==running)
        set_state(stopped);
    }   
  }    
  

/*  if (state==still_jump || state==still_jump_fall || state==end_still_jump)
  {
    set_xacel(0);
    set_fxacel(0);
    if (xvel()>0) set_xvel(get_ability(type(),jump_top_speed));
    else if (xvel()<0) set_xvel(-get_ability(type(),jump_top_speed));
  } else if (state==run_jump || state==run_jump_fall || state==end_run_jump)
  {
    set_xacel(0);
    set_fxacel(0);
    if (xvel()>0) set_xvel(get_ability(type(),jump_top_speed));
    else if (xvel()<0) set_xvel(-get_ability(type(),jump_top_speed));
  } */
  
  // see if the user said to jump
  if (cy<0 && !floating() && !gravity())
  {
    set_gravity(1);      
    set_yvel(get_ability(type(),jump_yvel));
//    if (cx && has_sequence(run_jump))
      set_state(run_jump);
    if (xvel()!=0)
      if (direction>0)
        set_xvel(get_ability(type(),jump_top_speed));
      else
        set_xvel(-get_ability(type(),jump_top_speed));
    set_xacel(0);


/*    if (state==stopped)  
    {
      if (cx && has_sequence(start_run_jump))
        set_state(start_run_jump);
      else if (has_sequence(start_still_jump))
        set_state(start_still_jump);                
      else 
      {

      }
    }
    else if (state==running && has_sequence(run_jump))
    {
      set_state(run_jump);
      set_yvel(get_ability(type(),jump_yvel));
      set_gravity(1);      
    }    
    else if (state==walking || state==turn_around)  // if walking check to see if has a
    {
      if (has_sequence(start_still_jump))
        set_state(start_still_jump);    
      else
      {
        set_yvel(get_ability(type(),jump_yvel));
        set_gravity(1);      
        if (has_sequence(run_jump))
          set_state(run_jump);
      }
    }    */
  }



  if (state==run_jump && yvel()>0)
    set_state(run_jump_fall);
    


//  if (state!=end_still_jump && state!=end_run_jump)
  {    
    if (cx>0)
      set_xacel(get_ability(type(),start_accel));
    else if (cx<0)
      set_xacel(-get_ability(type(),start_accel));
  }  
  
  // make sure they are not going faster than their maximum speed
  int top_speed;
  if (state==stopped || state==end_run_jump)
    top_speed=get_ability(type(),walk_top_speed);
  else if (state==running)
    top_speed=get_ability(type(),run_top_speed);    
  else if (state==run_jump || state==run_jump_fall || state==start_run_jump)
  {
    top_speed=get_ability(type(),jump_top_speed);      
    if (!cx) top_speed=0;
  }
  else top_speed=1000;
    
      
  if (abs(xvel()+xacel())>top_speed)
  {    
    if (xacel()<0) set_xacel(-top_speed-xvel());
    else set_xacel(top_speed-xvel());
  }  
    
  character_state old_state=state;
  int old_frame=current_frame;    
  int tick_stat=tick();

  // if he started to jump and slammed into a wall then make sure he stays in this state
  // so he can finish the jump
  if (!tick_stat && (old_state==start_run_jump))
  {
    set_state(old_state);
    current_frame=old_frame;
    next_picture();    
  }
    
  return tick_stat;
    
}




game_object *game_object::bmove(int &whit, game_object *exclude)
{

  // first let's move the guy acording to his physics
  long xa=xacel(),ya=yacel(),fxa=sfxacel(),fya=sfyacel();
  if (xa || ya || fxa || fya)
  {
    int fxv=sfxvel(),fyv=sfyvel();
    fxv+=fxa;  fyv+=fya;    
    long xv=xvel()+xa+(fxv>>8); 
    set_xvel(xvel()+xa+(fxv>>8));
    set_yvel(yvel()+ya+(fyv>>8));
    set_fxvel(fxv&0xff);
    set_fyvel(fyv&0xff);
  }
  
  long ox2,oy2;

  long nx=x+xvel(),nfx=fx()+fxvel(),ny=y+yvel(),nfy=fy()+fyvel();
  nx+=nfx>>8;
  ny+=nfy>>8;
 

  // check to see if this advancement causes him to collide with objects
  ox2=nx;
  oy2=ny;  // save the correct veloicties
  
  current_level->foreground_intersect(x,y,nx,ny);  // first see how far we can travel
  game_object *ret=current_level->all_boundary_setback(exclude,x,y,nx,ny);
  x=nx;
  y=ny;
  set_fx(nfx&0xff);
  set_fy(nfy&0xff);
  if (ret)
  {
    if (!ret->hurtable())   // object is not hurtable, return as if hit wall.
    { whit=1;
      return NULL;
    } else
    return ret;
  }
  else
  {
    whit=(nx!=ox2 || ny!=oy2);
    return NULL;
  }
}



int object_to_number_in_list(game_object *who, object_node *list)
{
  int x=1;
  while (list) 
  { 
    if (who==list->me) return x; 
    else list=list->next;
    x++;
  }
  return 0;
}

game_object *number_to_object_in_list(long x, object_node *list)
{
  if (!x) return NULL; x--;
  while (x && list) { list=list->next; x--; }
  if (list) return list->me;
  else return NULL;
}


void delete_object_list(object_node *first)
{
  while (first)
  {
    object_node *n=first;
    first=first->next;
    delete n;
  }
}


long object_list_length(object_node *list)
{
  long x=0;
  while (list) { list=list->next; x++; }
  return x;
  
}



game_object::game_object(int Type, int load) 
{ 
  if (Type<0xffff)
  {
    int t=figures[Type]->tv;
    if (t)
    {
      lvars=(long *)jmalloc(t*4,"object vars");
      memset(lvars,0,t*4);
    }
    else lvars=NULL;
  } else lvars=NULL;

  otype=Type; 
  if (!load) defaults(); 
}


int game_object::reduced_state()
{
  long x=0;
  for (int i=0;i<figures[otype]->ts;i++)
  {
    if (i==state) return x;
      else
    if (figures[otype]->seq[i]) x++;
  }
  return 0;
}


void game_object::change_aitype(int new_type)
{
  set_aitype(new_type);
  if (otype<0xffff)
  {
    void *f=figures[otype]->get_fun(OFUN_CHANGE_TYPE);  
    if (f) 
    {
      game_object *o=current_object;
      current_object=(game_object *)this;

      time_marker *prof1;
      if (profiling())
        prof1=new time_marker;

      eval_user_fun((lisp_symbol *)f,NULL);

      if (profiling())
      {
	time_marker now;
	profile_add_time(this->otype,now.diff_time(prof1));
	delete prof1;
      }


      current_object=o;
    }
  }
}


void game_object::change_type(int new_type)
{
  if (lvars) jfree(lvars);     // free old variable

  if (otype<0xffff)
  {
    int t=figures[new_type]->tv;
    if (t)
    {
      lvars=(long *)jmalloc(t*4,"object vars");
      memset(lvars,0,t*4);
    }
    else lvars=NULL;
  } else return;
  otype=new_type;

  if (figures[new_type]->get_fun(OFUN_CONSTRUCTOR))
  {
    game_object *o=current_object;
    current_object=this;    

    void *m=mark_heap(TMP_SPACE);

    time_marker *prof1;
    if (profiling())
      prof1=new time_marker;

    eval_function((lisp_symbol *)figures[new_type]->get_fun(OFUN_CONSTRUCTOR),NULL);
    if (profiling())
    {
      time_marker now;
      profile_add_time(otype,now.diff_time(prof1));
      delete prof1;
    }


    restore_heap(m,TMP_SPACE);

    current_object=o;
  }  
}
