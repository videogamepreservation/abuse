#include "system.h"
#include "view.hpp"
#include "lisp.hpp"
#include "jwindow.hpp"
#include "config.hpp"
#include "scroller.hpp"
#include "id.hpp"
#include "dev.hpp"
#include "jrand.hpp" 
#include "dprint.hpp"
#include "transp.hpp"
#include "clisp.hpp"
#include "demo.hpp"
#include "sbar.hpp"
#include "nfserver.hpp"
#include "chat.hpp"

#define SHIFT_DOWN_DEFAULT 15
#define SHIFT_RIGHT_DEFAULT 0
 
extern window_manager *eh;
view *player_list=NULL;
int morph_sel_frame_color;

view::~view()
{
  if (local_player()) 
    sbar.associate(NULL);

  if (total_weapons)
  {
    jfree(weapons);
    jfree(last_weapons);
  }
}


extern uchar bright_tint[256];

void view::add_ammo(int weapon_type, int total)
{  
  if (weapon_type>=total_weapons || weapon_type<0)
  {
    printf("weapon out of range\n");
    return ;
  }
  if (weapons[weapon_type]==-1) return ;   // don't have weapon yet, can't give ammo
  int rf=0;

  weapons[weapon_type]+=total;
  if (weapons[weapon_type]<0)  
    weapons[weapon_type]=0;  

  if (weapons[weapon_type]>999)
    weapons[weapon_type]=999;

  if (weapon_total(current_weapon)==0 && current_weapon)
  {
    suggest.send_weapon_change=1;
    if (DEFINEDP(symbol_value(l_switch_to_powerful)) && symbol_value(l_switch_to_powerful))
    {
      int x=total_weapons-1;
      while (x>0 && (x==3 || weapons[x]<=0)) x--;         
      suggest.new_weapon=x;
    } else
      suggest.new_weapon=0;
  }

}

void view::give_weapon(int type) 
{ 
  if (type>=total_weapons || type<0)
  {
    printf("weapon out of range\n");
    return ;
  }
  if (weapons[type]==-1) 
  { 
    weapons[type]=0; 
    sbar.need_refresh();
  }
}

int view::weapon_total(int type)
{
  if (type>=total_weapons || type<0)
  {
    printf("weapon out of range\n");
    return 0;
  }
  if (god) return 100;
  else if (weapons[type]==-1) return 0;
  else return weapons[type];
}


long view::xoff()
{
  if (focus)
  {
    int x=last_x-(cx2-cx1+1)/2+shift_right+pan_x;
    if (x<0) return 0;
    else return x;
  } else return pan_x;
}

long view::interpolated_xoff()
{
  if (focus)
  {
    int x=(last_last_x+last_x)/2-(cx2-cx1+1)/2+shift_right+pan_x;
    if (x<0) return 0;
    else return x;
  } else return pan_x;
}


long view::yoff()
{
  if (focus)
  {
    int y=last_y-(cy2-cy1+1)/2-shift_down+pan_y;
    if (y<0) return 0;
    else return y;
  } else return pan_y;
}


long view::interpolated_yoff()
{
  if (focus)
  {
    int y=(last_y+last_last_y)/2-(cy2-cy1+1)/2-shift_down+pan_y;
    if (y<0) return 0;
    else return y;
  } else return pan_y;
}


void view::update_scroll()
{
  if (focus)
  {
    last_last_x=last_x;
    last_last_y=last_y;
    if (focus->x>last_x)
    {
      if (focus->x-last_x>=no_xright)
        last_x=focus->x-no_xright;
    } else if (focus->x<last_x)
    {
      if (last_x-focus->x>=no_xleft)
        last_x=focus->x+no_xleft;
    }
    if (focus->y>last_y)
    {
      if (focus->y-last_y>=no_ybottom)
        last_y=focus->y-no_ybottom;
    } else if (focus->y<last_y)
    {
      if (last_y-focus->y>=no_ytop)
        last_y=focus->y+no_ytop;
    }
  }
}

char cur_user_name[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

#ifdef __WATCOMC__
char *get_login()
{  if (cur_user_name[0]) return cur_user_name; else return "DOS user"; }

#include <dos.h>
#elif defined( __POWERPC__ )

char *get_login()
{  if (cur_user_name[0]) return cur_user_name; else return "Mac user"; }

#else
char *get_login()
{  if (cur_user_name[0]) return cur_user_name; else return (getlogin() ? getlogin() : "unknown"); }

#endif

void set_login(char *name)
{ strncpy(cur_user_name,name,20); }

view::view(game_object *Focus, view *Next, int number)
{ 
  chat_buf[0]=0; 

  draw_solid=-1;
  no_xleft=0;
  no_xright=0;
  no_ytop=0;
  no_ybottom=0;
  if (Focus)
  {
    last_x=Focus->x;
    last_y=Focus->y;
  } else
  {
    last_x=last_y=0;
  }

  last_last_x=last_x;
  last_last_y=last_y;
  last_hp=last_ammo=-1;
  last_type=-1;
  tsecrets=secrets=0;
  tkills=kills=0;

  reset_keymap();

  ambient=32;
  current_weapon=0;

  strcpy(name,get_login());
  suggest.send_view=0;
  suggest.send_weapon_change=0;


  god=0;

  player_number=number;
  cx1=0;
  cy1=0;
  cx2=100;
  cy2=100;
  focus=Focus;
  next=Next;
  shift_down=SHIFT_DOWN_DEFAULT;
  shift_right=SHIFT_RIGHT_DEFAULT;
  x_suggestion=0;
  y_suggestion=0;
  b1_suggestion=0;
  b2_suggestion=0;
  b3_suggestion=0;
  b4_suggestion=0;
  pointer_x=0;
  pointer_y=0;

  pan_x=0;
  pan_y=0;
  last_type=0;
  freeze_time=0;

  if (total_weapons)
  {
    weapons=(long *)jmalloc(total_weapons*sizeof(long),"weapon array");
    last_weapons=(long *)jmalloc(total_weapons*sizeof(long),"last weapon array");
    memset(weapons,0xff,total_weapons*sizeof(long));   // set all to -1
    memset(last_weapons,0xff,total_weapons*sizeof(long));   // set all to -1  
  }

  if (total_weapons)
    weapons[0]=0;
  if (local_player())
    sbar.associate(this);
  sbar.need_refresh();
}

long view::x_center() 
{
  if (!focus)
    return (cx1+cx2)/2;
  else
    return focus->x; 
}   

long view::y_center() 
{ 
  if (!focus)
    return (cy1+cy2)/2;
  else
    return focus->y; 
}

void view::draw_character_damage()
{   
  if (focus && drawable())
  {
    if (last_hp!=focus->hp()) draw_hp();
    int i;
    for (i=0;i<total_weapons;i++)
      if (weapons[i]!=last_weapons[i])
      {
	last_weapons[i]=weapons[i];
        sbar.draw_ammo(screen,i,weapons[i],current_weapon==i);
      }
  }
}



ushort make_sync()
{
  ushort x=0;
  if (!current_level) return 0;
  if (current_level)
  {
    view *f=player_list;
    for (;f;f=f->next)
    {
      if (f->focus)
      {
	x^=(f->focus->x&0xffff);
	x^=(f->focus->y&0xffff);
      }
    }
  }
  x^=rand_on;

  return x;
}



void view::get_input()
{
  int sug_x,sug_y,sug_b1,sug_b2,sug_b3,sug_b4;
  long sug_px,sug_py;

  if (DEFINEDP(symbol_function(l_get_local_input)))
  {
    void *ret=eval_function((lisp_symbol *)l_get_local_input,NULL);
    sug_x=lnumber_value(CAR(ret));  ret=CDR(ret);
    sug_y=lnumber_value(CAR(ret));  ret=CDR(ret);
    if (CAR(ret)) sug_b1=1; else sug_b1=0; ret=CDR(ret);
    if (CAR(ret)) sug_b2=1; else sug_b2=0;  ret=CDR(ret);
    int x=lnumber_value(CAR(ret)); ret=CDR(ret);
    if (x<0) sug_b3=1; else sug_b3=0;
    if (x>0) sug_b4=1; else sug_b4=0;    
    
    long bx=lnumber_value(CAR(ret));  ret=CDR(ret);
    long by=lnumber_value(CAR(ret));  ret=CDR(ret);
    the_game->mouse_to_game(bx,by,sug_px,sug_py,this);

  } else
  {
    get_movement(0,sug_x,sug_y,sug_b1,sug_b2,sug_b3,sug_b4);
    if (focus)
    {
      the_game->mouse_to_game(last_demo_mx,last_demo_my,sug_px,sug_py,this);
      if (last_demo_mbut&1) sug_b2=1;
      if (last_demo_mbut&2) sug_b1=1;
    } else sug_px=sug_py=0;
  }


  if (view_changed())
  {
    base->packet.write_byte(SCMD_VIEW_RESIZE);
    base->packet.write_byte(player_number);
    base->packet.write_long(suggest.cx1);
    base->packet.write_long(suggest.cy1);
    base->packet.write_long(suggest.cx2);
    base->packet.write_long(suggest.cy2);

    base->packet.write_long(suggest.pan_x);
    base->packet.write_long(suggest.pan_y);
    base->packet.write_long(suggest.shift_down);
    base->packet.write_long(suggest.shift_right);
  }

  if (weapon_changed())
  {
    base->packet.write_byte(SCMD_WEAPON_CHANGE);
    base->packet.write_byte(player_number);
    base->packet.write_long(suggest.new_weapon);
  }

  base->packet.write_byte(SCMD_SET_INPUT);
  base->packet.write_byte(player_number);


  uchar mflags=0;
  if (sug_x>0)
    mflags|=1;
  else if (sug_x<0)
    mflags|=2;

  if (sug_y>0)
    mflags|=4;
  else if (sug_y<0)
    mflags|=8;

  if (sug_b1) mflags|=16;
  if (sug_b2) mflags|=32;
  if (sug_b3) mflags|=64;
  if (sug_b4) mflags|=128;

  base->packet.write_byte(mflags);

  base->packet.write_short((ushort)((short)sug_px));
  base->packet.write_short((ushort)((short)sug_py));

}


void view::add_chat_key(int key)  // return string if buf is complete
{
  int len=strlen(chat_buf);
  if (key==JK_BACKSPACE)
  {
    if (len)
    {
      chat_buf[len-1]=0;
      if (local_player() && chat)
        chat->draw_user(chat_buf);
    }
  } else if (key!=JK_ENTER)
  {
    chat_buf[len]=key;
    chat_buf[len+1]=0;
    if (local_player() && chat)
      chat->draw_user(chat_buf);
  }

  if (len>38 || key==JK_ENTER)
  {
    if (DEFINEDP(symbol_function(l_chat_input)))
    {
      game_object *o=current_object;
      current_object=focus;

      void *m=mark_heap(TMP_SPACE);
      void *list=NULL;
      push_onto_list(new_lisp_string(chat_buf),list);       
      eval_function((lisp_symbol *)l_chat_input,list);
      restore_heap(m,TMP_SPACE);

      current_object=o;

    } else
    {           
      if (chat)
        chat->put_all(chat_buf);
    }
    chat_buf[0]=0;
    if (local_player() && chat)
      chat->draw_user(chat_buf);
  }
}

int view::process_input(char cmd, uchar *&pk)   // return 0 if something went wrong
{
  switch (cmd)
  {
    case SCMD_CHAT_KEYPRESS :
    {
      add_chat_key(*(pk++));
    } break;
    case SCMD_VIEW_RESIZE :
    { 
      long x[8];
      memcpy(x,pk,8*4);  pk+=8*4;
      cx1=lltl(x[0]);
      cy1=lltl(x[1]);
      cx2=lltl(x[2]);
      cy2=lltl(x[3]);
	
      pan_x=lltl(x[4]);
      pan_y=lltl(x[5]);
      shift_down=lltl(x[6]);
      shift_right=lltl(x[7]);
      if (small_render)
      {
	small_render->resize(cx2-cx1+1,cy2-cy1+1);
      }

      suggest.send_view=0;
      if (local_player())
        the_game->draw();
      return 1;
    }
    case SCMD_WEAPON_CHANGE :
    {
      long x;
      memcpy(&x,pk,4);  pk+=4;
      current_weapon=lltl(x);

      if (local_player())
        sbar.need_refresh();
      suggest.send_weapon_change=0;
      return 1;
    } break;

    case SCMD_SET_INPUT :
    {
      uchar x=*(pk++);

      if (x&1) x_suggestion=1;
      else if (x&2) x_suggestion=-1;
      else x_suggestion=0;

      if (x&4) y_suggestion=1;
      else if (x&8) y_suggestion=-1;
      else y_suggestion=0;

      if (x&16) b1_suggestion=1; else b1_suggestion=0;
      if (x&32) b2_suggestion=1; else b2_suggestion=0;
      if (x&64) b3_suggestion=1; else b3_suggestion=0;
      if (x&128) b4_suggestion=1; else b4_suggestion=0;

      ushort p[2];
      memcpy(p,pk,2*2);  pk+=2*2;
      
      pointer_x=(short)(lstl(p[0]));
      pointer_y=(short)(lstl(p[1]));
	
      return 1;
    } break;
    case SCMD_KEYPRESS : set_key_down(*(pk++),1); break;
    case SCMD_EXT_KEYPRESS : set_key_down(*(pk++)+256,1); break;
    case SCMD_KEYRELEASE : set_key_down(*(pk++),0); break;
    case SCMD_EXT_KEYRELEASE : set_key_down(*(pk++)+256,0); break;
  }
  return 1;
}

int view::local_player()
{
  return player_number==client_number();
}

void view::next_weapon()
{
  int c=current_weapon;

  while (c<total_weapons-1)
  {
    c++;
    if (weapon_total(c)>0)
    {
      suggest.send_weapon_change=1;
      suggest.new_weapon=c;
      return ;
    }
  }
 
  c=0;
  while (c!=current_weapon)
  {
    if (weapon_total(c)>0)
    {
      suggest.send_weapon_change=1;
      suggest.new_weapon=c;
      return ;
    }
    c++;
  }
}

void view::last_weapon()
{
 
  int c=current_weapon;

  while (c>=1)
  {
    c--;
    if (weapon_total(c)>0 || c==0)
    {
      suggest.send_weapon_change=1;
      suggest.new_weapon=c;
      return ;
    }
  }

  c=total_weapons-1;
  while (c!=current_weapon)
  {
    if (weapon_total(c)>0 || c==0)
    {
      suggest.send_weapon_change=1;
      suggest.new_weapon=c;
      return ;
    }
    c--;
  }

}

int view::handle_event(event &ev)
{
  if (ev.type==EV_KEY)
  {
   switch (ev.key)
   {
     case '1' : 
     case '2' :
     case '3' :
     case '4' :
     case '5' :
     case '6' :
     case '7' :
     { if (((dev&EDIT_MODE)==0) && (weapon_total(ev.key-'1')>0))
       {
	 suggest.send_weapon_change=1;
	 suggest.new_weapon=ev.key-'1';
       }
     } break;

     case JK_HOME :
     case JK_CTRL_L :
     case JK_CTRL_R :
     { if (total_weapons) last_weapon(); return 1; } break;
     case JK_PAGEUP :
     case JK_INSERT :
     { if (total_weapons) next_weapon(); return 1;} break;
   }
 }
  return 0;
}


void view::draw_hp()
{ 
  if (focus)
  {
    int h=focus->hp();    
    last_hp=h;
    sbar.draw_health(screen,focus->hp());
  } else
    sbar.draw_health(screen,0);
}

int view::drawable()
{  
  return local_player();
}


void recalc_local_view_space()   // calculates view areas for local players, should be called 
                                 // when adding or deleting local players
{
  if (screen)
  {
    int t=total_local_players();
    if (!t) return ;

    int Xres=small_render ? xres/2 : xres;
    int Yres=small_render ? yres/2 : yres;

    int h=Yres/t;
    int w=h*320/200,y=5;
    if (w<300) w=300;

    for (view *f=player_list;f;f=f->next)
    {
      if (f->local_player())
      {
	f->suggest.cx1=Xres/2-w/2;
	f->suggest.cx2=Xres/2+w/2;
	if (f->suggest.cx1<2) f->suggest.cx1=2;
	if (f->suggest.cx2>Xres-2) f->suggest.cx2=Xres-2;    

	f->suggest.cy1=y;
	f->suggest.cy2=h-(total_weapons ? 33 : 0);

	f->suggest.shift_down=f->shift_down;
	f->suggest.shift_right=f->shift_right;
	f->suggest.pan_x=f->pan_x;
	f->suggest.pan_y=f->pan_y;
	f->suggest.send_view=1;

	if (!player_list->next)
	{
	  f->cx1=f->suggest.cx1;
	  f->cy1=f->suggest.cy1;
	  f->cx2=f->suggest.cx2;
	  f->cy2=f->suggest.cy2;
	  f->suggest.send_view=0;
	}
	y+=h;
      }
    }
  }

}


void set_local_players(int total)
{
  int rdw=0;
  if (total<1) return ;

  view *last=NULL;
  for (view *f=player_list;f;f=f->next)
  {
    if (total && f->local_player())
      total--;
    else if (!total && f->local_player())  // too many local players, delete this one
    {
      view *n=last->next;
      while (n && !n->local_player()) n=n->next;  // find next local player

      if (last)
        last->next=n;
      else        
      {
	if (n)    // make sure we have at least one local player
          player_list=n;
      }            
      last=f;
      rdw=1;
    }
  }
  
  while (total)   // see if we need to add new players
  {
    game_object *o=create(current_start_type,50,50);    
    view *v;
    if (!player_list)
    {
      player_list=new view(o,NULL,0);
      v=player_list;
    }
    else
    {
      view *f=player_list;
      for (;f && f->next;f=f->next);   	 
      f->next=new view(o,NULL,f->player_number+1);
      v=f->next;
    }
    v->cx1=320/2-155; v->cy1=200/2-95; v->cx2=320/2+155; v->cy2=200/2+(total_weapons ? 60 : 95);
    v->focus->set_controller(v);
    total--;
    rdw=1;
  }  
  if (rdw)
    recalc_local_view_space();
}


int total_local_players()
{
  int t=0;
  for (view *f=player_list;f;f=f->next) 
    if (f->local_player()) t++;
  return t;
}


void view::resize_view(long Cx1, long Cy1, long Cx2, long Cy2)
{
  if (cx1!=Cx1 || cx2!=Cx2 || cy1!=Cy1 || cy2!=Cy2)
  {
    cx1=Cx1; cy1=Cy1;
    cx2=Cx2; cy2=Cy2;
    if (playing_state(the_game->state) && local_player())
      the_game->draw(0);
  }
}


void view::set_input(int cx, int cy, int b1, int b2, int b3, int b4, int px, int py)
{
  x_suggestion=cx;
  y_suggestion=cy;
  b1_suggestion=b1;
  b2_suggestion=b2;
  b3_suggestion=b3;
  b4_suggestion=b4;
  pointer_x=px;
  pointer_y=py;
}



void view::reset_player()
{
  if (focus)
  {
   
    game_object *start=current_level ? current_level->get_random_start(320,focus->controller()) : 0;
    focus->defaults();
    if (start)
    {
      focus->x=start->x;
      focus->y=start->y;
      dprintf("reset position to %d %d\n",start->x,start->y);
    } 
    focus->set_state(stopped);
    memset(weapons,0xff,total_weapons*sizeof(long));   
    memset(last_weapons,0xff,total_weapons*sizeof(long));   

    shift_down=SHIFT_DOWN_DEFAULT;
    shift_right=SHIFT_RIGHT_DEFAULT;

    if (total_weapons)
      weapons[0]=0;  // give him the first weapon
    current_weapon=0;

    memset(focus->lvars,0,figures[focus->otype]->tv*4);
    focus->set_aistate(0);
    if (figures[focus->otype]->get_fun(OFUN_CONSTRUCTOR))
    {
      game_object *o=current_object;
      current_object=focus;
      eval_user_fun((lisp_symbol *)figures[focus->otype]->get_fun(OFUN_CONSTRUCTOR),NULL);
      current_object=o;
    }
    sbar.redraw(screen);

    int i;
    for (i=0;i<focus->total_objects();i++)   // reset the vars for the attached objects
    {      
      game_object *o=focus->get_object(i);
      memset(o->lvars,0,figures[o->otype]->tv*4);
    }

  }
}





object_node *make_player_onodes(int player_num)
{
  object_node *first=NULL,*last;
  for (view *o=player_list;o;o=o->next)
  {
    if (o->focus && (player_num==-1 || o->player_number==player_num))
    {
      if (!object_to_number_in_list(o->focus,first))
      {
	object_node *q=new object_node(o->focus,NULL);
	if (first)
	  last->next=q;
	else first=q;
	last=q;
      }
      for (int i=0;i<o->focus->total_objects();i++)
      {
	game_object *p=o->focus->get_object(i);

	if (!object_to_number_in_list(p,first))
	{
	  object_node *q=new object_node(p,NULL);
	  if (first)
	    last->next=q;
	  else first=q;
	  last=q;
	}
      }      
    }
  }
  return first;
}




enum { V_CX1, V_CY1, V_CX2, V_CY2,
       V_SHIFT_DOWN, V_SHIFT_RIGHT,
       V_GOD,
       V_PLAYER_NUMBER,
       V_DRAW_SOLID,
       V_LIVES,
       V_CURRENT_WEAPON,
       V_X_SUGGESTION, V_Y_SUGGESTION, V_B1_SUGGESTION, V_B2_SUGGESTION, V_B3_SUGGESTION, V_B4_SUGGESTION,
       V_PAN_X, V_PAN_Y,
       V_NO_XLEFT, V_NO_XRIGHT, V_NO_YTOP, V_NO_YBOTTOM,
       V_LAST_X, V_LAST_Y, V_LAST_LEFT, V_LAST_RIGHT, V_LAST_UP, V_LAST_DOWN,
       V_LAST_B1, V_LAST_B2, V_LAST_B3, V_LAST_B4,
       V_LAST_HP,
       V_SECRETS, V_KILLS, V_TSECRETS, V_TKILLS,
       V_AMBIENT,
       V_POINTER_X, V_POINTER_Y,
       V_LAST_LAST_X, V_LAST_LAST_Y,
       V_FREEZE_TIME };

#define TVV (V_FREEZE_TIME+1)

static char *vv_names[TVV]={
  "view.cx1",  "view.cy1",  "view.cx2",  "view.cy2", 
  "view.shift_down",  "view.shift_right",
  "view.god",
  "view.player_number",
  "view.draw_solid",
  "view.lives",
  "view.current_weapon",
  "view.x_suggestion",  "view.y_suggestion",  
   "view.b1_suggestion",  "view.b2_suggestion",  "view.b3_suggestion",  "view.b4_suggestion",
  "view.pan_x",  "view.pan_y",
  "view.no_xleft",  "view.no_xright",  "view.no_ytop",  "view.no_ybottom",
  "view.last_x",  "view.last_y",  "view.last_left",  "view.last_right",  "view.last_up",  "view.last_down", 
  "view.last_b1",  "view.last_b2",  "view.last_b3",  "view.last_b4",
  "view.last_hp",
  "view.secrets",  "view.kills",  "view.tsecrets",  "view.tkills",
  "view.ambient",
  "view.pointer_x",  "view.pointer_y",
  "view.last_last_x",  "view.last_last_y",
  "view.freeze_time"
};


int total_view_vars() 
{ return TVV;
}

char *get_view_var_name(int num)
{ return vv_names[num]; }

long view::get_view_var_value(int num)
{
  switch (num)
  {
    case V_CX1 : return cx1; break;
    case V_CY1 : return cy1; break;
    case V_CX2 : return cx2; break;
    case V_CY2 : return cy2; break;
    case V_SHIFT_DOWN : return shift_down; break;
    case V_SHIFT_RIGHT : return shift_right; break;
    case V_GOD : return god; break;
    case V_PLAYER_NUMBER : return player_number; break;

    case V_DRAW_SOLID : return draw_solid; break;
    case V_CURRENT_WEAPON : return current_weapon; break;
    case V_X_SUGGESTION : return x_suggestion; break;
    case V_Y_SUGGESTION : return y_suggestion; break;
    case V_B1_SUGGESTION : return b1_suggestion; break;
    case V_B2_SUGGESTION : return b2_suggestion; break;
    case V_B3_SUGGESTION : return b3_suggestion; break;
    case V_B4_SUGGESTION : return b4_suggestion; break;

    case V_PAN_X : return pan_x; break;
    case V_PAN_Y : return pan_y; break;
    case V_NO_XLEFT : return no_xleft; break;
    case V_NO_XRIGHT : return no_xright; break;
    case V_NO_YTOP : return no_ytop; break;
    case V_NO_YBOTTOM : return no_ybottom; break;
    case V_LAST_X : return last_x; break;
    case V_LAST_Y : return last_y; break;
    case V_LAST_LEFT : return last_left; break;
    case V_LAST_RIGHT : return last_right; break;
    case V_LAST_UP : return last_up; break;
    case V_LAST_DOWN : return last_down; break;
    case V_LAST_B1 : return last_b1; break;
    case V_LAST_B2 : return last_b2; break;
    case V_LAST_B3 : return last_b3; break;
    case V_LAST_B4 : return last_b4; break;
    case V_LAST_HP : return last_hp; break;
    case V_SECRETS : return secrets; break;
    case V_KILLS : return kills; break;
    case V_TSECRETS : return tsecrets; break;
    case V_TKILLS : return tkills; break;
    case V_AMBIENT : return ambient; break;
    case V_POINTER_X : return pointer_x; break;
    case V_POINTER_Y : return pointer_y; break;
    case V_LAST_LAST_X : return last_last_x; break;
    case V_LAST_LAST_Y : return last_last_y; break;
    case V_FREEZE_TIME : return freeze_time; break;
  }
  return 0;
}



long view::set_view_var_value(int num, long x)
{
  switch (num)
  {
    case V_CX1 : cx1=x; break;
    case V_CY1 : cy1=x; break;
    case V_CX2 : cx2=x; break;
    case V_CY2 : cy2=x; break;
    case V_SHIFT_DOWN : shift_down=x; break;
    case V_SHIFT_RIGHT : shift_right=x; break;
    case V_GOD : god=x; break;
    case V_PLAYER_NUMBER : { player_number=x; if (local_player()) sbar.associate(this); }  break;

    case V_DRAW_SOLID : draw_solid=x; break;
    case V_CURRENT_WEAPON : { current_weapon=x; sbar.need_refresh(); } break;
    case V_X_SUGGESTION : x_suggestion=x; break;
    case V_Y_SUGGESTION : y_suggestion=x; break;
    case V_B1_SUGGESTION : b1_suggestion=x; break;
    case V_B2_SUGGESTION : b2_suggestion=x; break;
    case V_B3_SUGGESTION : b3_suggestion=x; break;
    case V_B4_SUGGESTION : b4_suggestion=x; break;

    case V_PAN_X : pan_x=x; break;
    case V_PAN_Y : pan_y=x; break;
    case V_NO_XLEFT : no_xleft=x; break;
    case V_NO_XRIGHT : no_xright=x; break;
    case V_NO_YTOP : no_ytop=x; break;
    case V_NO_YBOTTOM : no_ybottom=x; break;
    case V_LAST_X : last_x=x; break;
    case V_LAST_Y : last_y=x; break;
    case V_LAST_LEFT : last_left=x; break;
    case V_LAST_RIGHT : last_right=x; break;
    case V_LAST_UP : last_up=x; break;
    case V_LAST_DOWN : last_down=x; break;
    case V_LAST_B1 : last_b1=x; break;
    case V_LAST_B2 : last_b2=x; break;
    case V_LAST_B3 : last_b3=x; break;
    case V_LAST_B4 : last_b4=x; break;

    case V_LAST_HP : last_hp=x; break;
    case V_SECRETS : secrets=x; break;
    case V_KILLS : kills=x; break;
    case V_TSECRETS : tsecrets=x; break;
    case V_TKILLS : tkills=x; break;
    case V_AMBIENT : ambient=x; break;
    case V_POINTER_X : pointer_x=x; break;
    case V_POINTER_Y : pointer_y=x; break;
    case V_LAST_LAST_X : last_last_x=x; break; 
    case V_LAST_LAST_Y : last_last_y=x; break;
    case V_FREEZE_TIME : freeze_time=x; break;
  }
  return 1;
}


void view::configure_for_area(area_controller *a)
{
  if (a->ambient>=0 && a->ambient!=ambient)
  {
    if (ambient>a->ambient)
    {
      ambient-=a->ambient_speed;
      if (ambient<a->ambient) 
        ambient=a->ambient;
    }
    else 
    {
      ambient+=a->ambient_speed;
      if (ambient>a->ambient) 
        ambient=a->ambient;
    }    
  }

  if (!view_shift_disabled)
  {
    if (a->view_xoff!=pan_x)
    {
      if (pan_x>a->view_xoff)
      {
	pan_x-=a->view_xoff_speed;
	if (pan_x<a->view_xoff) 
        pan_x=a->view_xoff;
      }
      else
      {
	pan_x+=a->view_xoff_speed;
	if (pan_x>a->view_xoff) 
        pan_x=a->view_xoff;
      }
    }

    if (a->view_yoff!=pan_y)
    {
      if (pan_y>a->view_yoff)
      {
	pan_y-=a->view_yoff_speed;
	if (pan_y<a->view_yoff) 
        pan_y=a->view_yoff;
      }
      else
      {
	pan_y+=a->view_yoff_speed;
	if (pan_y>a->view_yoff) 
        pan_y=a->view_yoff;
      }
    }  
  }
}


void process_packet_commands(uchar *pk, int size)
{
  long sync_short=-1;

  if (!size) return ;
  pk[size]=SCMD_END_OF_PACKET;

  uchar cmd;
  int already_reloaded=0;


  do
  {
    cmd=*(pk++);
    switch (cmd)
    {
      case SCMD_WEAPON_CHANGE :
      case SCMD_SET_INPUT :
      case SCMD_VIEW_RESIZE :
      case SCMD_KEYPRESS :
      case SCMD_KEYRELEASE :
      case SCMD_EXT_KEYPRESS :
      case SCMD_EXT_KEYRELEASE :
      case SCMD_CHAT_KEYPRESS :
      {
	uchar player_num=*(pk++);

	view *v=player_list;
	for (;v && v->player_number!=player_num;v=v->next);
	if (v)
	{
	  if (v->player_number==player_num)
	  v->process_input(cmd,pk);
	}      
	else
	{
	  dprintf("Evil error : bad player number in packet\n");
	  return ;
	}
      } break;
      case SCMD_RELOAD :
      {
	if (!already_reloaded)
	{
	  net_reload();
	  already_reloaded=1;
	}
      } break;

      case SCMD_SYNC :
      {
	ushort x;
	memcpy(&x,pk,2);  pk+=2;
	x=lstl(x);
	if (demo_man.current_state()==demo_manager::PLAYING)
	sync_short=make_sync();

	if (sync_short==-1)
	sync_short=x;
	else if (x!=sync_short && !already_reloaded)
	{
	  dprintf("out of sync %d (packet=%d, calced=%d)\n",current_level->tick_counter(),x,sync_short);
	  if (demo_man.current_state()==demo_manager::NORMAL)
	    net_reload();
	  already_reloaded=1;
	}
      } break;
      case SCMD_DELETE_CLIENT :
      {
	uchar player_num=*(pk++);
	view *v=player_list,*last=NULL;
	for (;v && v->player_number!=player_num;v=v->next)
	last=v;
	if (!v)
	dprintf("evil : delete client %d, but no such client\n");
	else
	{

	  // make a list of all objects associated with this player
	  object_node *on=make_player_onodes(player_num);   
	  while (on)
	  {
	    current_level->delete_object(on->me);
	    object_node *last=on;
	    on=on->next;
	    delete last;
	  }

	  v->focus=NULL;
	  if (last)
	  last->next=v->next;
	  else player_list=player_list->next;

	  delete v;
	}
      } break;
      defualt :
      dprintf("Unknown net command %d\n",cmd);

    }
  } while (cmd!=SCMD_END_OF_PACKET);
}
