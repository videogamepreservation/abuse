#include "ant.hpp"
#include "lisp.hpp"
#include "game.hpp"
#include "jrand.hpp"
#include "dev.hpp"
#include "pcxread.hpp"
#include "menu.hpp"
#include "dprint.hpp"
#include "clisp.hpp"
#include "chars.hpp"
#include "lisp_gc.hpp"
#include "cop.hpp"
#include "loadgame.hpp"
#include "nfserver.hpp"
#include "joy.hpp"
#include "demo.hpp"
#include "chat.hpp"
#include "text_gui.hpp"
#include "jdir.hpp"
#include "netcfg.hpp"

#ifdef __WATCOMC__
#include <direct.h>
#endif

extern uchar major_version;
extern uchar minor_version;
extern int has_joystick;

void *l_change_on_pickup,*l_MBULLET_ICON5,*l_MBULLET_ICON20,*l_GRENADE_ICON2,*l_GRENADE_ICON10,*l_ROCKET_ICON2,*l_ROCKET_ICON5,
  *l_FBOMB_ICON1,*l_FBOMB_ICON5,*l_PLASMA_ICON20,*l_PLASMA_ICON50,*l_LSABER_ICON100,*l_DFRIS_ICON4,*l_DFRIS_ICON10,*l_LSABER_ICON50,
  *l_TELEPORTER_SND,*l_gun_tints,*l_ant_tints;

// the following are references to lisp symbols
void *l_difficulty,*l_easy,*l_hard,*l_medium,*l_main_menu,*l_extreme,
     *l_logo,*l_state_art,*l_abilities,*l_state_sfx,
     *l_song_list,*l_filename,*l_sfx_directory,*l_max_hp,*l_default_font,
     *l_morph,*l_max_power,*l_default_abilities,*l_default_ai_function,
     *l_tile_files,*l_empty_cache,*l_range,*l_hurt_all,*l_death_handler,
     *l_title_screen,*l_console_font,*l_fields,*l_dist,*l_pushx,*l_pushy,
     *l_object,*l_tile,*l_fire_object,*l_FIRE,*l_cop_dead_parts,*l_restart_player,
     *l_help_screens,*l_player_draw,*l_sneaky_draw,*l_health_image,*l_fly_image,
     *l_sneaky_image,*l_draw_fast,*l_player_tints,*l_save_order,*l_next_song,
     *l_level_load_start,
     *l_level_load_end,	    *l_cdc_logo,
     *l_keep_backup,
     *l_switch_to_powerful,
     *l_mouse_can_switch,
     *l_ask_save_slot,
     *l_get_local_input,
     *l_post_render,
     *l_chat_input,
     *l_player_text_color,
     *l_level_loaded,        // called when a new level is loaded
     *l_ammo_snd,

		 *l_up_key,
		 *l_down_key,
		 *l_left_key,
		 *l_right_key,
     *l_weapon_left_key,
	   *l_weapon_right_key,
	   *l_special_key;

char game_name[50];
void *sensor_ai();

// variables for the status bar
void        *l_statbar_ammo_x,*l_statbar_ammo_y,
            *l_statbar_ammo_w,*l_statbar_ammo_h,
	    *l_statbar_ammo_bg_color,

            *l_statbar_health_x,*l_statbar_health_y,
            *l_statbar_health_w,*l_statbar_health_h,
	    *l_statbar_health_bg_color,

	    *l_statbar_logo_x,*l_statbar_logo_y;
uchar chatting_enabled=0; 

extern void scatter_line(int x1, int y1, int x2, int y2, int c, int s);
extern void ascatter_line(int x1, int y1, int x2, int y2, int c1, int c2, int s);
extern void show_end();

extern int registered;

view *lget_view(void *arg, char *msg)
{
  game_object *o=(game_object *)lpointer_value(arg);
  view *c=o->controller();
  if (!c)
  {
    dprintf("%s : object does not have a view\n",msg);
    lbreak("");
    exit(0);
  }
  return c;
}

extern int get_option(char *name);
extern void set_login(char *name);



long cli_1(void *args) { return abs(current_object->x-current_level->attacker(current_object)->x); }
long cli_2(void *args) { return abs(current_object->y-current_level->attacker(current_object)->y); }
long cli_3(void *args)
{
  if (!current_object->controller())
    lbreak("object is not a player, cannot determine keypresses");
  else
    return current_object->controller()->key_down(lnumber_value(CAR(args))); 
  return 0;
}

long cli_4(void *args)
{ return the_game->key_down(lnumber_value(CAR(args))); }

long cli_5(void *args)
{ return current_level->attacker(current_object)->state; }

long cli_6(void *args)
{ return current_object->aitype(); }

long cli_7(void *args)
{
  if (!current_object->keep_ai_info())
    current_object->set_aistate(0);
  return current_object->aistate();
}

long cli_8(void *args)
{
  int ns=lnumber_value(CAR(args));
  current_object->set_aistate_time(0);      
  current_object->set_aistate(ns); return 1; 
}

long cli_9(void *args)
{ return jrandom(lnumber_value(CAR(args))); }

long cli_10(void *args)
{ return current_object->aistate_time(); }

long cli_11(void *args)
{ return current_object->state; }

long cli_12(void *args)
{ if (current_level->attacker(current_object)->x>current_object->x) return 1;  else return -1; }

long cli_13(void *args)
{ return current_object->move(lnumber_value(CAR(args)),lnumber_value(CAR(CDR(args))),
				   lnumber_value(CAR(CDR(CDR(args)))));
}

long cli_14(void *args)
{ if (current_object->direction>0) return 1; else return -1; }

long cli_15(void *args)
{ return current_object->otype; }

long cli_16(void *args)
{ return current_object->next_picture(); }

long cli_17(void *args)
{ current_object->set_fade_dir(lnumber_value(CAR(args))); return 1; }

long cli_18(void *args)
{ 
  int cx=lnumber_value(CAR(args));
  args=CDR(args);
  int cy=lnumber_value(CAR(args));
  args=CDR(args);
  int but=lnumber_value(CAR(args));
  return current_object->mover(cx,cy,but);
}

long cli_19(void *args)
{ current_object->set_fade_count(lnumber_value(CAR(args))); return 1; }

long cli_20(void *args)
{ return current_object->fade_count(); }

long cli_21(void *args)
{ return current_object->fade_dir(); }

long cli_22(void *args)
{
  long x1,y1,x2,y2,xp1,yp1,xp2,yp2;
  current_level->attacker(current_object)->picture_space(x1,y1,x2,y2);
  current_object->picture_space(xp1,yp1,xp2,yp2);
  if (xp1>x2 || xp2<x1 || yp1>y2 || yp2<y1) return 0;
  else return 1;
}

long cli_23(void *args)
{ 
  current_object->add_power(lnumber_value(CAR(args))); 
  return 0;
}

long cli_24(void *args)
{ current_object->add_hp(lnumber_value(CAR(args))); return 0; }

long cli_27(void *args)
{ current_object->drawer(); return 1; }

long cli_28(void *args)
{ return (dev & EDIT_MODE); }

long cli_29(void *args)
{ current_object->draw_above(current_view); return 1; }

long cli_30(void *args)
{ return current_object->x; } 

long cli_31(void *args)
{ return current_object->y; }

long cli_32(void *args)
{ 
  long v=lnumber_value(CAR(args));
  current_object->x=v;
  return 1; 
} 

long cli_33(void *args)
{ 
  long v=lnumber_value(CAR(args));
  current_object->y=v;
  return 1; 
}

long cli_34(void *args)
{ 
  return current_level->push_characters(current_object,lnumber_value(CAR(args)),
					    lnumber_value(CAR(CDR(args))));
}

long cli_37(void *args)
{
  long s=lnumber_value(CAR(args));
  current_object->set_state((character_state)s); 
  return (s==current_object->state);
}

long cli_38(void *args)
{ return current_level->attacker(current_object)->x;}

long cli_39(void *args)
{  return current_level->attacker(current_object)->y; }

long cli_40(void *args)
{ current_object->change_aitype(lnumber_value(CAR(args))); return 1; }


long cli_42(void *args)
{ return current_object->xvel(); }

long cli_43(void *args)
{ return current_object->yvel(); }

long cli_44(void *args)
{ current_object->set_xvel(lnumber_value(CAR(args))); return 1; }

long cli_45(void *args)
{ current_object->set_yvel(lnumber_value(CAR(args))); return 1; }

long cli_46(void *args)
{ if (current_level->attacker(current_object)->x>current_object->x) return -1; 
else return 1;
}

long cli_47(void *args)
{ return lnumber_value(CAR(args))&BLOCKED_LEFT; }

long cli_48(void *args)
{ return lnumber_value(CAR(args))&BLOCKED_RIGHT; }

long cli_50(void *args)
{ dev_cont->add_palette(args); return 1;}

long cli_51(void *args)
{ write_PCX(screen,pal,lstring_value(CAR(args))); return 1;}

long cli_52(void *args)
{ the_game->zoom=lnumber_value(CAR(args)); the_game->draw(); return 1;}

long cli_55(void *args)
{ the_game->show_help(lstring_value(CAR(args)));  return 1; }


long cli_56(void *args)
{ return current_object->direction; }

long cli_57(void *args)
{ current_object->direction=lnumber_value(CAR(args)); return 1; }

long cli_58(void *args)
{
  int x1=lnumber_value(CAR(args));
  if (!current_object->controller())      
  { lbreak("set_freeze_time : object is not a focus\n"); }
  else current_object->controller()->freeze_time=x1; 
  return 1;
}

long cli_59(void *args)
{ return menu(args,big_font); }

long cli_60(void *args)
{ event ev; dev_cont->do_command(lstring_value(CAR(args)),ev); return 1; }

long cli_61(void *args)
{ the_game->set_state(lnumber_value(CAR(args))); return 0; }

long cli_62(void *args)
{
  int x1=lnumber_value(CAR(args)); args=CDR(args);
  int y1=lnumber_value(CAR(args)); args=CDR(args);
  int x2=lnumber_value(CAR(args)); args=CDR(args);
  int y2=lnumber_value(CAR(args));
  scene_director.set_text_region(x1,y1,x2,y2);  return 0;
}

long cli_63(void *args)
{ scene_director.set_frame_speed(lnumber_value(CAR(args)));  return 1; }

long cli_64(void *args)
{ scene_director.set_scroll_speed(lnumber_value(CAR(args))); return 1; }

long cli_65(void *args)
{ scene_director.set_pan_speed(lnumber_value(CAR(args))); return 1; }

long cli_66(void *args)
{ scene_director.scroll_text(lstring_value(CAR(args))); return 1; }

long cli_67(void *args)
{
  scene_director.set_pan(lnumber_value(CAR(args)),
      lnumber_value(CAR(CDR(args))),
      lnumber_value(CAR(CDR(CDR(args)))));
  return 1;
} 

long cli_68(void *args)
{ scene_director.wait(CAR(args)); return 1; }


long cli_73(void *args)
{
  the_game->set_level(new level(lnumber_value(CAR(args)),
      lnumber_value(CAR(CDR(args))),
      lstring_value(CAR(CDR(CDR(args))))));
  return 1;
}

long cli_74(void *args)
{
  if (current_level) delete current_level; 
  current_level=new level(100,100,"new_level");   return 0;
} 

long cli_75(void *args)
{
  int amount=lnumber_value(CAR(args)); args=CDR(args);
  game_object *o=((game_object *)lpointer_value(CAR(args))); args=CDR(args);
  int xv=0,yv=0;
  if (args)
  {
    xv=lnumber_value(CAR(args)); args=CDR(args);
    yv=lnumber_value(CAR(args));
  }
  o->do_damage(amount,current_object,current_object->x,current_object->y,xv,yv);
  return 0;
}

long cli_76(void *args)
{  return current_object->hp(); }

long cli_77(void *args)
{
  game_object *o=(game_object *)lpointer_value(CAR(args));
  if (!o->controller())      
    dprintf("set shift : object is not a focus\n");
  else o->controller()->shift_down=lnumber_value(CAR(CDR(args))); return 1;
}

long cli_78(void *args)
{
  game_object *o=(game_object *)lpointer_value(CAR(args));
  if (!o->controller())      
    dprintf("set shift : object is not a focus\n");
  else o->controller()->shift_right=lnumber_value(CAR(CDR(args))); return 1;
}

long cli_79(void *args)
{ current_object->set_gravity(lnumber_value(CAR(args))); return 1; }

long cli_80(void *args)
{ return current_object->tick(); }

long cli_81(void *args)
{ current_object->set_xacel((lnumber_value(CAR(args)))); return 1; }

long cli_82(void *args)
{ current_object->set_yacel((lnumber_value(CAR(args)))); return 1; }


long cli_84(void *args)
{ set_local_players(lnumber_value(CAR(args))); return 1; }

long cli_85(void *args)
{ return total_local_players(); }

long cli_86(void *args)
{ light_detail=lnumber_value(CAR(args)); return 1; }

long cli_87(void *args)
{ return light_detail; }

long cli_88(void *args)
{ morph_detail=lnumber_value(CAR(args)); return 1; }

long cli_89(void *args)
{ return morph_detail; }

long cli_90(void *args)
{
  current_object->morph_into(lnumber_value(CAR(args)),NULL,
      lnumber_value(CAR(CDR(args))),
      lnumber_value(CAR(CDR(CDR(args))))); return 1; 
}

long cli_91(void *args)
{ current_object->add_object((game_object *)lpointer_value(CAR(args))); return 1; }

long cli_92(void *args)
{
  long cx1,x1=lnumber_value(CAR(args)); args=lcdr(args);
  long cy1,y1=lnumber_value(CAR(args)); args=lcdr(args);
  long cx2,x2=lnumber_value(CAR(args)); args=lcdr(args);
  long cy2,y2=lnumber_value(CAR(args)); args=lcdr(args);
  long c=lnumber_value(CAR(args));
  the_game->game_to_mouse(x1,y1,current_view,cx1,cy1);
  the_game->game_to_mouse(x2,y2,current_view,cx2,cy2);
  screen->line(cx1,cy1,cx2,cy2,c);
  return 1;
}

long cli_93(void *args)
{ return eh->dark_color(); }

long cli_94(void *args)
{ return eh->medium_color(); }

long cli_95(void *args)
{ return eh->bright_color(); }

long cli_99(void *args)
{ current_object->remove_object((game_object *)lpointer_value(CAR(args))); return 1; }

long cli_100(void *args)
{ current_object->add_light((light_source *)lpointer_value(CAR(args))); return 1; }

long cli_101(void *args)
{ current_object->remove_light((light_source *)lpointer_value(CAR(args))); return 1; }

long cli_102(void *args)
{ return current_object->total_objects(); }

long cli_103(void *args)
{ return current_object->total_lights(); }

long cli_104(void *args)
{ 
  light_source *l=(light_source *)lpointer_value(CAR(args));
  long x=lnumber_value(CAR(CDR(args)));
  if (x>=1)
    l->inner_radius=x;
  l->calc_range();
  return 1;
}

long cli_105(void *args)
{ 
  light_source *l=(light_source *)lpointer_value(CAR(args));
  long x=lnumber_value(CAR(CDR(args)));
  if (x>l->inner_radius)
    l->outer_radius=x;
  l->calc_range();
  return 1;
} 

long cli_106(void *args)
{ 
  light_source *l=(light_source *)lpointer_value(CAR(args));
  l->x=lnumber_value(CAR(CDR(args)));
  l->calc_range();
  return 1;
}

long cli_107(void *args)
{ 
  light_source *l=(light_source *)lpointer_value(CAR(args));
  l->y=lnumber_value(CAR(CDR(args)));
  l->calc_range();
  return 1;
} 

long cli_108(void *args) 
{ 
  light_source *l=(light_source *)lpointer_value(CAR(args));
  l->xshift=lnumber_value(CAR(CDR(args)));
  l->calc_range();
  return 1;
}
 
long cli_109(void *args)
{ 
  light_source *l=(light_source *)lpointer_value(CAR(args));
  l->yshift=lnumber_value(CAR(CDR(args)));
  l->calc_range();
  return 1;
}

long cli_110(void *args)
{ return ((light_source *)lpointer_value(CAR(args)))->inner_radius; }

long cli_111(void *args)
{ return ((light_source *)lpointer_value(CAR(args)))->outer_radius; }

long cli_112(void *args)
{ return ((light_source *)lpointer_value(CAR(args)))->x; }

long cli_113(void *args)
{ return ((light_source *)lpointer_value(CAR(args)))->y; }

long cli_114(void *args)
{ return ((light_source *)lpointer_value(CAR(args)))->xshift; }

long cli_115(void *args)
{ return ((light_source *)lpointer_value(CAR(args)))->yshift; }

long cli_116(void *args)
{ return current_object->xacel(); }

long cli_117(void *args)
{ return current_object->yacel(); }

long cli_118(void *args)
{ current_level->remove_light((light_source *)lpointer_value(CAR(args))); return 1; }

long cli_119(void *args)
{ current_object->set_fx(lnumber_value(CAR(args))); return 1; }

long cli_120(void *args)
{ current_object->set_fy(lnumber_value(CAR(args))); return 1; }

long cli_121(void *args)
{ current_object->set_fxvel(lnumber_value(CAR(args))); return 1; }

long cli_122(void *args)
{ current_object->set_fyvel(lnumber_value(CAR(args))); return 1; }

long cli_123(void *args)
{ current_object->set_fxacel(lnumber_value(CAR(args))); return 1; }

long cli_124(void *args)
{ current_object->set_fyacel(lnumber_value(CAR(args))); return 1; }

long cli_125(void *args)
{ return current_object->picture()->width(); }

long cli_126(void *args)
{ return current_object->picture()->height(); }

long cli_127(void *args)
 { dprintf("trap\n"); return 0; }   // I use this to set gdb break points

long cli_128(void *args)
{ return current_level->platform_push(current_object,lnumber_value(CAR(args)),
					    lnumber_value(CAR(CDR(args))));
} 

long cli_133(void *args) // def_sound
{
  lisp_symbol *sym=NULL;
  if (CDR(args))
  {
    sym=(lisp_symbol *)lcar(args);
    if (item_type(sym)!=L_SYMBOL)
    {
      lbreak("expecting first arg to def-character to be a symbol!\n");
      exit(0);
    }
    args=CDR(args);
  }

  int sp=current_space;
  current_space=PERM_SPACE;
  int id=cash.reg(lstring_value(lcar(args)),NULL,SPEC_EXTERN_SFX,1);
  if (sym)
    set_symbol_number(sym,id);    // set the symbol value to sfx id				     
  current_space=sp;
  return id;
}

long cli_134(void *args)  // play_sound
{
  void *a=args;
  p_ref r1(a);
  int id=lnumber_value(lcar(a));
  if (id<0) return 0;
  a=CDR(a);
  if (!a)
    cash.sfx(id)->play(127);
  else
  {
    int vol=lnumber_value(lcar(a)); a=CDR(a);
    if (a)
    {
      long x=lnumber_value(lcar(a)); a=CDR(a);
      if (!a)
      {
        lprint(args);
        lbreak("expecting y after x in play_sound\n");
        exit(1);
      }
      long y=lnumber_value(lcar(a));
      the_game->play_sound(id,vol,x,y);
    } else cash.sfx(id)->play(vol);        
  } return 0;
}


long cli_142(void *args)   
{
  long x=lnumber_value(CAR(args)); args=CDR(args);
  if (x<0 || x>=total_weapons)
  { 
    lbreak("weapon out of range (%d)\n",x);
    exit(0);
  }
  return weapon_types[x];
}

long cli_143(void *args)  
{
  long x=lnumber_value(CAR(args)); args=CDR(args);
  long y=lnumber_value(CAR(args)); args=CDR(args);
  long r=lnumber_value(CAR(args)); args=CDR(args);
  long m=lnumber_value(CAR(args)); args=CDR(args);
  game_object *o=(game_object *)lpointer_value(CAR(args)); args=CDR(args);
  long mp=lnumber_value(CAR(args));
  current_level->hurt_radius(x,y,r,m,current_object,o,mp);  return 0;
} 

long cli_144(void *args)
{
  view *v=current_object->controller();
  if (!v) dprintf("Can't add weapons for non-players\n");
  else
  {
    long x=lnumber_value(CAR(args)); args=CDR(args);
    long y=lnumber_value(CAR(args)); args=CDR(args);
    if (x<0 || x>=total_weapons)
    { lbreak("weapon out of range (%d)\n",x); exit(0); }
    v->add_ammo(x,y);
  }  return 0;
}

long cli_145(void *args)  
{
  view *v=current_object->controller();
  if (!v) return 0;
  else return v->weapon_total(lnumber_value(CAR(args)));
}

long cli_146(void *args)    
{
  view *v=current_object->controller();
  if (!v) return 0; else return v->current_weapon;
}

long cli_147(void *args)   
{
  view *v=current_object->controller();
  if (!v) { lbreak("current_weapon_type : object cannot hold weapons\n");
  return 0; } 
  else return v->current_weapon;
}

long cli_148(void *args)
{ return lnumber_value(CAR(args))&BLOCKED_UP; }

long cli_149(void *args)
{ return lnumber_value(CAR(args))&BLOCKED_DOWN; }

long cli_150(void *args)   
{
  view *v=current_object->controller();
  int x=lnumber_value(CAR(args));
  if (x<0 || x>=total_weapons)
  { lbreak("weapon out of range (%d)\n",x); exit(0); }
  if (v) v->give_weapon(x);  return 0;
} 

long cli_151(void *args)   
{
  int a=lnumber_value(CAR(args));
  if (a<0 || a>=TOTAL_ABILITIES)
  {
    lprint(args);
    lbreak("bad ability number for get_ability, should be 0..%d, not %d\n",
        TOTAL_ABILITIES,a);
    exit(0);
  }
  return get_ability(current_object->otype,(ability)a);
}

long cli_152(void *args)  
{
  view *v=current_object->controller();
  if (!v) dprintf("Can't use reset_player on non-players\n");
  else
    v->reset_player();	      
  return 0;
}

long cli_153(void *args)   
{
  game_object *o=(game_object *)lpointer_value(CAR(args));
  long x=o->x-current_object->x,
    y=-(o->y-o->picture()->height()/2-(current_object->y-(current_object->picture()->height()/2)));
  return lisp_atan2(y,x);
}

long cli_154(void *args)  
{
  long ang=lnumber_value(CAR(args)); args=CDR(args);
  long mag=lfixed_point_value(CAR(args));
  long xvel=(lisp_cos(ang)>>8)*(mag>>8);
  current_object->set_xvel(xvel>>16);
  current_object->set_fxvel((xvel&0xffff)>>8);
  long yvel=-(lisp_sin(ang)>>8)*(mag>>8);
  current_object->set_yvel(yvel>>16);
  current_object->set_fyvel((yvel&0xffff)>>8);    return 0;
}

long cli_155(void *args) 
{
  int tframes=current_object->total_frames(),f;

  long ang1=lnumber_value(CAR(args)); args=CDR(args);      
  if (ang1<0) ang1=(ang1%360)+360; 
  else if (ang1>=360) ang1=ang1%360;
  long ang2=lnumber_value(CAR(args)); args=CDR(args);      
  if (ang2<0) ang2=(ang2%360)+360; 
  else if (ang2>=360) ang2=ang2%360;

  long ang=(lnumber_value(CAR(args))+90/tframes)%360;
  if (ang1>ang2)
  {
    if (ang<ang1 && ang>ang2) 	
      return 0;
    else if (ang>=ang1)	
      f=(ang-ang1)*tframes/(359-ang1+ang2+1);
    else 
      f=(359-ang1+ang)*tframes/(359-ang1+ang2+1);
  } else if (ang<ang1 || ang>ang2)
    return 0;
  else f=(ang-ang1)*tframes/(ang2-ang1+1);
  if (current_object->direction>0)
    current_object->current_frame=f;
  else
    current_object->current_frame=tframes-f-1;
  return 1;
} 

long cli_156(void *args)
{
  int x=current_object->current_frame;
  current_object->set_state((character_state)lnumber_value(CAR(args)));
  current_object->current_frame=x; return 0;
}


long cli_168(void *args)
{ if (current_object->morph_status()) return 1; else return 0; }

long cli_169(void *args)
{
  long am=lnumber_value(CAR(args)); args=CDR(args);
  game_object *from=(game_object *)lpointer_value(CAR(args)); args=CDR(args);
  long hitx=lnumber_value(CAR(args)); args=CDR(args);      
  long hity=lnumber_value(CAR(args)); args=CDR(args);      
  long px=lnumber_value(CAR(args)); args=CDR(args);      
  long py=lnumber_value(CAR(args)); args=CDR(args);
  current_object->damage_fun(am,from,hitx,hity,px,py);
  return 1;
}

long cli_170(void *args)
{ return current_object->gravity(); }

long cli_171(void *args)   
{
  view *v=current_object->controller();
  if (!v) dprintf("make_view_solid : object has no view\n");
  else
    v->draw_solid=lnumber_value(CAR(args));      
  return 1;
} 

long cli_172(void *args)
{
  void *a=args;
  int r=lnumber_value(CAR(a)); a=CDR(a);
  int g=lnumber_value(CAR(a)); a=CDR(a);
  int b=lnumber_value(CAR(a));
  if (r<0 || b<0 || g<0 || r>255 || g>255 || b>255)
  {
    lprint(args);
    lbreak("color out of range (0..255) in color lookup\n");
    exit(0);
  }
  return color_table->lookup_color(r>>3,g>>3,b>>3);
}

long cli_173(void *args)  
{
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else return v->x_suggestion; return 0;
}

long cli_174(void *args)  
{
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else return v->y_suggestion; return 0;
}

long cli_175(void *args)   
{
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else return v->b1_suggestion; return 0;
}

long cli_176(void *args)  
{
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else return v->b2_suggestion; return 0;
}

long cli_177(void *args)  
{
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else return v->b3_suggestion; return 0;
}

long cli_178(void *args)  
{
  bg_xmul=lnumber_value(CAR(args)); args=CDR(args);
  bg_xdiv=lnumber_value(CAR(args)); args=CDR(args);
  bg_ymul=lnumber_value(CAR(args)); args=CDR(args);
  bg_ydiv=lnumber_value(CAR(args)); 
  if (bg_xdiv==0) { bg_xdiv=1; lprint(args); dprintf("bg_set_scroll : cannot set xdiv to 0\n"); }
  if (bg_ydiv==0) { bg_ydiv=1; lprint(args); dprintf("bg_set_scroll : cannot set ydiv to 0\n"); }
  return 1;
}

long cli_179(void *args)  
{
  view *v=lget_view(CAR(args),"set_ambient_light");       args=CDR(args);
  long x=lnumber_value(CAR(args));
  if (x>=0 && x<64) v->ambient=x;
  return 1;
}

long cli_180(void *args)
{ return lget_view(CAR(args),"ambient_light")->ambient; }

long cli_181(void *args)
{
  int x=current_object->total_objects();
  game_object *who=(game_object *)lpointer_value(CAR(args));
  for (int i=0;i<x;i++)
    if (current_object->get_object(i)==who) return 1;
  return 0;
}

long cli_182(void *args)
{
  current_object->change_type(lnumber_value(CAR(args)));
  return 1;
}

long cli_184(void *args)
{ return current_object->current_frame; }

long cli_185(void *args)
{ return current_object->fx(); }

long cli_186(void *args)
{ return current_object->fy(); }

long cli_187(void *args)
{ return current_object->fxvel(); }

long cli_188(void *args)
{ return current_object->fyvel(); }

long cli_189(void *args)
{ return current_object->fxacel(); }

long cli_190(void *args)
{ return current_object->fyacel(); }


long cli_191(void *args) 
{
  char *fn=lstring_value(CAR(args)); args=CDR(args);      
//      stat_bar=cash.reg_object(fn,CAR(args),SPEC_IMAGE,1);
  return 0;
} 

long cli_192(void *args)  
{     
  long x=lnumber_value(CAR(args)); args=CDR(args);
  long y=lnumber_value(CAR(args)); args=CDR(args);
  long type=lnumber_value(CAR(args));
  if (x<0 || y<0 || x>=current_level->foreground_width() || y>=current_level->foreground_width())
    lbreak("%d %d is out of range of fg map",x,y);
  else	
    current_level->put_fg(x,y,type);
  return 1;
}

long cli_193(void *args) 
{ 
  long x=lnumber_value(CAR(args)); args=CDR(args);
  long y=lnumber_value(CAR(args));
  if (x<0 || y<0 || x>=current_level->foreground_width() || y>=current_level->foreground_width())
    lbreak("%d %d is out of range of fg map",x,y);
  else return current_level->get_fg(x,y);
  return 0;
}

long cli_194(void *args)  
{     
  long x=lnumber_value(CAR(args)); args=CDR(args);
  long y=lnumber_value(CAR(args)); args=CDR(args);
  long type=lnumber_value(CAR(args));
  if (x<0 || y<0 || x>=current_level->background_width() || y>=current_level->background_width())
    lbreak("%d %d is out of range of fg map",x,y);
  else	
    current_level->put_bg(x,y,type);
  return 1;
}

long cli_195(void *args)  
{ 
  long x=lnumber_value(CAR(args)); args=CDR(args);
  long y=lnumber_value(CAR(args));
  if (x<0 || y<0 || x>=current_level->background_width() || y>=current_level->background_width())
    lbreak("%d %d is out of range of fg map",x,y);
  else return current_level->get_bg(x,y);
  return 0;
}

long cli_196(void *args)
{  load_tiles(args);  return 1 ; }

long cli_197(void *args)  
{
  bFILE *fp=open_file(lstring_value(CAR(args)),"rb");
  if (fp->open_failure())
  {
    delete fp;
    lbreak("load_palette : could not open file %s for reading",lstring_value(CAR(args)));
    exit(1);
  } else
  {
    spec_directory sd(fp);
    spec_entry *se=sd.find(SPEC_PALETTE);
    if (!se) lbreak("File %s has no palette!\n",lstring_value(CAR(args)));
    else
    {
      if (pal) delete pal;
      pal=new palette(se,fp);
    }
    delete fp;
  }
  return 0;
}

long cli_198(void *args)   
{
  bFILE *fp=open_file(lstring_value(CAR(args)),"rb");
  if (fp->open_failure())
  {
    delete fp;
    lbreak("load_color_filter : could not open file %s for reading",lstring_value(CAR(args)));
    exit(1);
  } else
  {
    spec_directory sd(fp);
    spec_entry *se=sd.find(SPEC_COLOR_TABLE);
    if (!se) lbreak("File %s has no color filter!",lstring_value(CAR(args)));
    else
    {
      if (color_table) delete color_table;
      color_table=new color_filter(se,fp);
    }
    delete fp;
  }
  return 0;
}

long cli_199(void *args) 
{
  current_start_type=lnumber_value(CAR(args));
  set_local_players(1);
  return 1;
}

long cli_200(void *args)
{
  long xv=lnumber_value(CAR(args));  args=CDR(args);
  long yv=lnumber_value(CAR(args));  args=CDR(args);
  int top=2;
  if (args)
    if (!CAR(args)) top=0;
        
  long oxv=xv,oyv=yv;
  current_object->try_move(current_object->x,current_object->y,xv,yv,1|top);
  current_object->x+=xv;
  current_object->y+=yv;
  return (oxv==xv && oyv==yv);
}

long cli_201(void *args)
{
  long x=lnumber_value(CAR(args));
  return figures[current_object->otype]->get_sequence((character_state)x)->length();
}

long cli_202(void *args)
{ 
  long x1=lnumber_value(CAR(args)); args=CDR(args);
  long y1=lnumber_value(CAR(args)); args=CDR(args);
  long x2=lnumber_value(CAR(args)); args=CDR(args);
  long y2=lnumber_value(CAR(args)); args=CDR(args);
  void *block_all=CAR(args);
  long nx2=x2,ny2=y2;
  if (x2==x1)
    current_level->vforeground_intersect(x1,y1,y2);
  else
    current_level->foreground_intersect(x1,y1,x2,y2);
  if (x2!=nx2 || y2!=ny2) return 0;
      
  if (block_all)
    current_level->all_boundary_setback(current_object,x1,y1,x2,y2);
  else
    current_level->boundary_setback(current_object,x1,y1,x2,y2);
  return (x2==nx2 && y2==ny2);

}

long cli_203(void *args)
{
  char *fn=lstring_value(CAR(args)); args=CDR(args);
  char *name=lstring_value(CAR(args));
  big_font_pict=cash.reg(fn,name,SPEC_IMAGE,1);     
  return 1;
}

long cli_204(void *args)
{
  char *fn=lstring_value(CAR(args)); args=CDR(args);
  char *name=lstring_value(CAR(args));
  small_font_pict=cash.reg(fn,name,SPEC_IMAGE,1);     
  return 1;
}

long cli_205(void *args)
{
  char *fn=lstring_value(CAR(args)); args=CDR(args);
  char *name=lstring_value(CAR(args));
  console_font_pict=cash.reg(fn,name,SPEC_IMAGE,1);     
  return 1;
}

long cli_206(void *args)  
{
  long x=lnumber_value(CAR(args));
  if (x<current_object->total_frames())
    current_object->current_frame=x;
  else      
    lbreak("%d out of range for set_current_frame",x);
  return 1;
}
 
long cli_208(void *args)     
{
  current_object->draw_trans(lnumber_value(CAR(args)),lnumber_value(CAR(CDR(args))));
  return 1;
}

long cli_209(void *args)   
{
  current_object->draw_tint(lnumber_value(CAR(args)));
  return 1;
}

long cli_210(void *args)
{
  current_object->draw_predator();
  return 1;
}

long cli_211(void *args) 
{ return lget_view(CAR(args),"shift_down")->shift_right; return 1; }

long cli_212(void *args) 
{ return lget_view(CAR(args),"shift_right")->shift_down; return 1; }

long cli_213(void *args)
{ 
  view *v=lget_view(CAR(args),"set_no_scroll_range"); args=CDR(args);
  v->no_xleft=lnumber_value(CAR(args)); args=CDR(args);
  v->no_ytop=lnumber_value(CAR(args));  args=CDR(args);
  v->no_xright=lnumber_value(CAR(args)); args=CDR(args);
  v->no_ybottom=lnumber_value(CAR(args));
  return 0;
}

long cli_215(void *args)   
{
  char *fn=lstring_value(CAR(args)); args=CDR(args);
  char *name=lstring_value(CAR(args)); args=CDR(args);
  return cash.reg(fn,name,SPEC_IMAGE,1);
}

long cli_216(void *args)  
{
  long x1=lnumber_value(CAR(args)); args=lcdr(args);
  long y1=lnumber_value(CAR(args)); args=lcdr(args);
  long id=lnumber_value(CAR(args));
  cash.img(id)->put_image(screen,x1,y1,1);
  return 1;
}

long cli_217(void *args)  
{
  view *v=current_object->controller();
  if (!v) lbreak("object has no view : view_x1");
  else return v->cx1;
  return 0;
}

long cli_218(void *args)  
{
  view *v=current_object->controller();
  if (!v) lbreak("object has no view : view_x1");
  else return v->cy1;
  return 0;
}


long cli_219(void *args)
{
  view *v=current_object->controller();
  if (!v) lbreak("object has no view : view_x1");
  else return v->cx2;
  return 0;
}

long cli_220(void *args)   
{
  view *v=current_object->controller();
  if (!v) lbreak("object has no view : view_x1");
  else return v->cy2;
  return 0;
}

long cli_221(void *args)
{
  view *v=current_object->controller();
  if (!v) lbreak("object has no view : view_push_down");
  else v->last_y-=lnumber_value(CAR(args));
  return 1;
}

long cli_222(void *args)
{
  view *v=current_object->controller();
  if (!v) lbreak("object has no view : local_player");
  else return v->local_player();
  return 0;
}

long cli_223(void *args)
{
  char *fn=lstring_value(CAR(args));
  current_level->save(fn,1);
  return 1;
}

long cli_224(void *args)  
{
  current_object->set_hp(lnumber_value(CAR(args)));   
  return 1;
}

long cli_225(void *args)  
{
  char *fn=lstring_value(CAR(args));
  the_game->request_level_load(fn);
  return 1;
}

long cli_226(void *args)
{
  strcpy(level_file,lstring_value(CAR(args)));
  return 1;
}

long cli_227(void *args)
{
  return cash.reg(lstring_value(CAR(args)),"palette",SPEC_PALETTE,1);  
}

long cli_228(void *args)  
{
  palette *p=pal->copy();
  uchar *addr=(uchar *)p->addr();
  int r,g,b;
  int ra=lnumber_value(CAR(args)); args=CDR(args);
  int ga=lnumber_value(CAR(args)); args=CDR(args);
  int ba=lnumber_value(CAR(args));
  for (int i=0;i<256;i++)
  {
    r=(int)*addr+ra; if (r>255) r=255; else if (r<0) r=0; *addr=(uchar)r; addr++;
    g=(int)*addr+ga; if (g>255) g=255; else if (g<0) g=0; *addr=(uchar)g; addr++;
    b=(int)*addr+ba; if (b>255) b=255; else if (b<0) b=0; *addr=(uchar)b; addr++;
  }
  p->load();
  delete p;
  return 0;
}

long cli_229(void *args)  
{
  view *v=current_object->controller();
  if (!v) lbreak("object has no view : local_player");
  else return v->player_number;
  return 0;
}

long cli_230(void *args) 
{
  view *v=current_object->controller();
  if (!v) lbreak("object has no view : local_player");
  else 
  {
    long x=lnumber_value(CAR(args));
    if (x<0 || x>=total_weapons)
    { lbreak("weapon out of range (%d)\n",x); exit(0); }
    v->current_weapon=x;	
  }
  return 1;
}

long cli_231(void *args)  
{
  view *v=current_object->controller();
  if (!v) lbreak("object has no view : local_player");
  else return v->has_weapon(lnumber_value(CAR(args)));
  return 0;
}

long cli_232(void *args)
{
  ambient_ramp+=lnumber_value(CAR(args));
  return 1;
}

long cli_233(void *args)
{ int x=0; view *v=player_list; for (;v;v=v->next,x++); return x; } 


long cli_234(void *args)  
{
  long cx1,x1=lnumber_value(CAR(args)); args=lcdr(args);
  long cy1,y1=lnumber_value(CAR(args)); args=lcdr(args);
  long cx2,x2=lnumber_value(CAR(args)); args=lcdr(args);
  long cy2,y2=lnumber_value(CAR(args)); args=lcdr(args);
  long c=lnumber_value(CAR(args)); args=lcdr(args);
  long s=lnumber_value(CAR(args));
  the_game->game_to_mouse(x1,y1,current_view,cx1,cy1);
  the_game->game_to_mouse(x2,y2,current_view,cx2,cy2);
  scatter_line(cx1,cy1,cx2,cy2,c,s);
  return 1;

}

long cli_235(void *args)   
{ 
  if (current_level) return current_level->tick_counter(); 
  else return 0;
}

long cli_236(void *args) 
{
  return current_object->controller()!=NULL;
}

long cli_237(void *args)  
{
  rand_on+=lnumber_value(CAR(args)); return 1;
}

long cli_238(void *args)
{
  return current_object->total_frames();
}

long cli_239(void *args)
{ 
  current_level->to_front(current_object); 
  return 1;
}

long cli_240(void *args) 
{ 
  current_level->to_back(current_object); 
  return 1;
}

long cli_241(void *args)  
{ 
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else return v->pointer_x;
  return 0;
} 

long cli_242(void *args) 
{ 
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else return v->pointer_y;
  return 0;
}

long cli_243(void *args)
{
  if (player_list->next || demo_man.current_state()!=demo_manager::NORMAL)
    return 0;
  else
    return (frame_panic>10);
}

long cli_244(void *args)   
{
  long cx1,x1=lnumber_value(CAR(args)); args=lcdr(args);
  long cy1,y1=lnumber_value(CAR(args)); args=lcdr(args);
  long cx2,x2=lnumber_value(CAR(args)); args=lcdr(args);
  long cy2,y2=lnumber_value(CAR(args)); args=lcdr(args);
  long c1=lnumber_value(CAR(args)); args=lcdr(args);
  long c2=lnumber_value(CAR(args)); args=lcdr(args);
  long s=lnumber_value(CAR(args));
  the_game->game_to_mouse(x1,y1,current_view,cx1,cy1);
  the_game->game_to_mouse(x2,y2,current_view,cx2,cy2);
  ascatter_line(cx1,cy1,cx2,cy2,c1,c2,s);
  return 1;

}

long cli_245(void *args)
{
  return rand_on;
}

long cli_246(void *args)  
{
  rand_on=lnumber_value(CAR(args));
  return 1;
}

long cli_247(void *args)  
{
  long cx1=lnumber_value(CAR(args)); args=lcdr(args);
  long cy1=lnumber_value(CAR(args)); args=lcdr(args);
  long cx2=lnumber_value(CAR(args)); args=lcdr(args);
  long cy2=lnumber_value(CAR(args)); args=lcdr(args);
  long c1=lnumber_value(CAR(args)); args=lcdr(args);      
  screen->bar(cx1,cy1,cx2,cy2,c1); 
  return 1;
}

long cli_248(void *args)   
{
  return start_argc;
}

long cli_249(void *args)  
{
  if ((sound_avail&MUSIC_INITIALIZED))
  {
    char *fn=lstring_value(CAR(args));
    if (current_song)
    {
      if (current_song->playing())
        current_song->stop();
      delete current_song;
    }
    current_song=new song(fn);
    current_song->play(music_volume);
    dprintf("Playing %s at volume %d\n",fn,music_volume);
  }
  return 1;
} 

long cli_250(void *args)  
{
  if (current_song && current_song->playing())
    current_song->stop();
  delete current_song;
  current_song=NULL;
  return 1;
}

long cli_251(void *args)
{ return current_object->targetable(); }

long cli_252(void *args)
{ current_object->set_targetable( CAR(args)==NULL ? 0 : 1); return 1; }

long cli_253(void *args)
{ show_stats(); return 1; }

long cli_254(void *args) 
{
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else return v->kills;
  return 0;
}

long cli_255(void *args) 
{
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else return v->tkills;
  return 0;
}

long cli_256(void *args) 
{
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else return v->secrets;
  return 0;
}

long cli_257(void *args)  
{
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else return v->tsecrets;
  return 0;
}

long cli_258(void *args) 
{
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else v->kills=lnumber_value(CAR(args)); 
  return 1;
}

long cli_259(void *args) 
{
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else v->tkills=lnumber_value(CAR(args)); 
  return 1;
}

long cli_260(void *args) 
{
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else v->secrets=lnumber_value(CAR(args)); 
  return 1;
}

long cli_261(void *args)  
{
  view *v=current_object->controller();
  if (!v) { lprint(args); dprintf("get_player_inputs : object has no view!\n"); }
  else v->tsecrets=lnumber_value(CAR(args)); 
  return 1;
} 

long cli_262(void *args)   
{
  the_game->request_end();
  return 1;
}

long cli_263(void *args)    
{
  the_game->reset_keymap();
  return load_game(1,symbol_str("SAVE")); //get_save_spot(); shit
}

long cli_264(void *args)  
{
  mem_report("mem.rep");
  return 1;
}

long cli_265(void *args)   
{
  return major_version;
}

long cli_266(void *args)    
{
  return minor_version;
}

long cli_267(void *args)    
{
  current_object->draw_double_tint(lnumber_value(CAR(args)),lnumber_value(CAR(CDR(args))));
  return 1;
}

long cli_268(void *args)   
{
  return cash.img(lnumber_value(CAR(args)))->width();
}

long cli_269(void *args)   
{
  return cash.img(lnumber_value(CAR(args)))->height();
}

long cli_270(void *args) 
{
  return current_level->foreground_width();
}

long cli_271(void *args) 
{
  return current_level->foreground_height();
}

long cli_272(void *args)  
{
  return current_level->background_width();
}

long cli_273(void *args) 
{
  return current_level->background_height();
}

long cli_274(void *args)   
{
  return get_keycode(lstring_value(CAR(args)));
}

long cli_275(void *args)    
{
  int id=lnumber_value(CAR(args));  args=CDR(args);
  int x=lnumber_value(CAR(args));  args=CDR(args);
  int y=lnumber_value(CAR(args));
  c_target=id;
  if (screen)
    eh->set_mouse_shape(cash.img(c_target)->copy(),x,y);
  return 0;
}

long cli_276(void *args)   
{      
  if (!main_net_cfg) return 0;
  return become_server(game_name);
}

long cli_277(void *args) 
{
  JCFont *fnt=(JCFont *)lpointer_value(CAR(args)); args=CDR(args);
  long x=lnumber_value(CAR(args));       args=CDR(args);
  long y=lnumber_value(CAR(args));       args=CDR(args);
  char *st=lstring_value(CAR(args));     args=CDR(args);
  int color=-1;
  if (args)
    color=lnumber_value(CAR(args));
  fnt->put_string(screen,x,y,st,color);
  return 0;
}

long cli_278(void *args) 
{ return ((JCFont *)lpointer_value(CAR(args)))->width(); }

long cli_279(void *args) 
{ return ((JCFont *)lpointer_value(CAR(args)))->height(); }

long cli_280(void *args) 
{ if (chat) chat->put_all(lstring_value(CAR(args))); return 1; }

long cli_281(void *args)    
{
  view *v=current_object->controller();
  if (!v) { lbreak("get_player_name : object has no view!\n"); }
  else strcpy(v->name,lstring_value(CAR(args))); 
  return 1;
}

long cli_282(void *args)   
{
  long x1=lnumber_value(CAR(args));   args=CDR(args);
  long y1=lnumber_value(CAR(args));   args=CDR(args);
  long x2=lnumber_value(CAR(args));   args=CDR(args);
  long y2=lnumber_value(CAR(args));   args=CDR(args);
  long c=lnumber_value(CAR(args));
  screen->bar(x1,y1,x2,y2,c);
  return 0;
}

long cli_283(void *args)   
{
  long x1=lnumber_value(CAR(args));   args=CDR(args);
  long y1=lnumber_value(CAR(args));   args=CDR(args);
  long x2=lnumber_value(CAR(args));   args=CDR(args);
  long y2=lnumber_value(CAR(args));   args=CDR(args);
  long c=lnumber_value(CAR(args));
  screen->rectangle(x1,y1,x2,y2,c);
  return 0;
}

long cli_284(void *args)   
{
  if (get_option(lstring_value(CAR(args))))
    return 1;
  else return 0;
}

long cli_285(void *args)  
{
  char cd[100];
  getcwd(cd,100);
  int t=change_dir(lstring_value(CAR(args)));
  change_dir(cd);
  return t;
}

long cli_286(void *args) 
{
  if (change_dir(lstring_value(CAR(args))))
    return 1;
  else return 0;      
}

long cli_287(void *args) 
{
  void *title=CAR(args); args=CDR(args);
  void *source=CAR(args); args=CDR(args);
  void *dest=CAR(args); args=CDR(args); 

  return nice_copy(lstring_value(title),lstring_value(source),lstring_value(dest));      
}

long cli_288(void *args) 
{
  if (CAR(args)) the_game->set_delay(1); else the_game->set_delay(0);
  return 1;
}

long cli_289(void *args) 
{
  set_login(lstring_value(CAR(args)));    
  return 1;
} 

long cli_290(void *args) 
{
  chatting_enabled=1;
  return 1;
}

long cli_291(void *args) 
{
  demo_start=1;
  return 1;
}

long cli_292(void *args)  
{
  if (main_net_cfg && main_net_cfg->state==net_configuration::CLIENT)
    return 1;
  else return 0;
}

long cli_293(void *args)  
{
  if (main_net_cfg && (main_net_cfg->state==net_configuration::CLIENT || main_net_cfg->state==net_configuration::SERVER))
  {
    view *v=player_list;
    for (;v;v=v->next)
      if (v->kills>=main_net_cfg->kills)
        return 1;
    
    
  }
  return 0;
}

long cli_294(void *args) 
{
  view *v=player_list;
  for (;v;v=v->next)
  {
    v->tkills+=v->kills;

    v->kills=0;
    game_object *o=current_object;
    current_object=v->focus;

    eval_function((lisp_symbol *)l_restart_player,NULL);
    v->reset_player();
    v->focus->set_aistate(0);
    current_object=o;	
  }
  return 1;
} 

long cli_295(void *args) 
{
  strncpy(game_name,lstring_value(CAR(args)),sizeof(game_name));
  game_name[sizeof(game_name)-1]=0;
  return 1;
}

long cli_296(void *args)     
{
  if (main_net_cfg)
    main_net_cfg->min_players=lnumber_value(CAR(args));
  return 0;
}

long cli_297(void *args)  // expire cache item
{
  cash.expire(lnumber_value(CAR(args)));
  return 0;
}

#define add_c_function(st,mn,mx,num) add_c_function(st,mn,mx,cli_##num)
#define add_c_bool_fun(st,mn,mx,num) add_c_bool_fun(st,mn,mx,cli_##num)


void clisp_init()                            // call by lisp_init, defines symbols and functions
                                             // to irnterface with c
{
  l_easy=make_find_symbol("easy");
  l_medium=make_find_symbol("medium");
  l_hard=make_find_symbol("hard");
  l_extreme=make_find_symbol("extreme");

  l_logo=make_find_symbol("logo");
  l_morph=make_find_symbol("morph");

  l_pushx=make_find_symbol("pushx");
  l_pushy=make_find_symbol("pushy");

  l_dist=make_find_symbol("dist");
  l_state_art=make_find_symbol("state_art");
  l_abilities=make_find_symbol("abilities");
  l_default_abilities=make_find_symbol("default_abilities");
  l_state_sfx=make_find_symbol("state_sfx");
  l_filename=make_find_symbol("filename");
  l_sfx_directory=make_find_symbol("sfx_directory");
  l_default_font=make_find_symbol("default_font");
  l_console_font=make_find_symbol("console_font");
  l_default_ai_function=make_find_symbol("default_ai");
  l_tile_files=make_find_symbol("tile_files");
  l_empty_cache=make_find_symbol("empty_cache");
  l_range=make_find_symbol("range");
  l_difficulty=make_find_symbol("difficulty");
  l_death_handler=make_find_symbol("death_handler");
  l_title_screen=make_find_symbol("title_screen");
  l_fields=make_find_symbol("fields");
  l_FIRE=make_find_symbol("FIRE");
  l_fire_object=make_find_symbol("fire_object");
  l_fire_object=make_find_symbol("fire_object");
  l_cop_dead_parts=make_find_symbol("cop_dead_parts");
  l_restart_player=make_find_symbol("restart_player");
  l_help_screens=make_find_symbol("help_screens");
  l_save_order=make_find_symbol("save_order");
  l_next_song=make_find_symbol("next_song");
  l_player_draw=make_find_symbol("player_draw");
  l_sneaky_draw=make_find_symbol("sneaky_draw");
  l_keep_backup=make_find_symbol("keep_backup");
  l_level_loaded=make_find_symbol("level_loaded");

  l_draw_fast=make_find_symbol("draw_fast");
  l_player_tints=make_find_symbol("player_tints");
  l_ant_tints=make_find_symbol("ant_tints");

  l_max_hp=make_find_symbol("max_hp");
  set_symbol_number(l_max_hp,200);
  l_max_power=make_find_symbol("max_power");
  l_main_menu=make_find_symbol("main_menu");
  set_symbol_number(l_max_power,999);

  set_symbol_number(make_find_symbol("run_state"),RUN_STATE);
  set_symbol_number(make_find_symbol("pause_state"),PAUSE_STATE);
  set_symbol_number(make_find_symbol("menu_state"),MENU_STATE);
  set_symbol_number(make_find_symbol("scene_state"),SCENE_STATE);

  l_statbar_ammo_x=make_find_symbol("statbar_ammo_x");
  l_statbar_ammo_y=make_find_symbol("statbar_ammo_y");
  l_statbar_ammo_w=make_find_symbol("statbar_ammo_w");
  l_statbar_ammo_h=make_find_symbol("statbar_ammo_h");
  l_statbar_ammo_bg_color=make_find_symbol("statbar_ammo_bg_color");

  l_statbar_health_x=make_find_symbol("statbar_health_x");
  l_statbar_health_y=make_find_symbol("statbar_health_y");
  l_statbar_health_w=make_find_symbol("statbar_health_w");
  l_statbar_health_h=make_find_symbol("statbar_health_h");
  l_statbar_health_bg_color=make_find_symbol("statbar_health_bg_color");

  l_statbar_logo_x=make_find_symbol("statbar_logo_x");
  l_statbar_logo_y=make_find_symbol("statbar_logo_y");
  l_object=make_find_symbol("object");
  l_tile=make_find_symbol("tile");
  l_cdc_logo=make_find_symbol("logo");

  l_switch_to_powerful=make_find_symbol("switch_to_powerful");
  l_mouse_can_switch=make_find_symbol("mouse_can_switch");
  l_ask_save_slot=make_find_symbol("ask_save_slot");

  l_level_load_start=make_find_symbol("level_load_start");
  l_level_load_end=make_find_symbol("level_load_end");
  l_get_local_input=make_find_symbol("get_local_input");
  l_chat_input=make_find_symbol("chat_input");
  l_player_text_color=make_find_symbol("player_text_color");


  l_change_on_pickup=make_find_symbol("change_on_pickup");
  l_MBULLET_ICON5=make_find_symbol("MBULLET_ICON5");
  l_MBULLET_ICON20=make_find_symbol("MBULLET_ICON20");
  l_GRENADE_ICON2=make_find_symbol("GRENADE_ICON2");
  l_GRENADE_ICON10=make_find_symbol("GRENADE_ICON10");
  

  l_ROCKET_ICON2=make_find_symbol("ROCKET_ICON2");
  l_ROCKET_ICON5=make_find_symbol("ROCKET_ICON5");


  l_FBOMB_ICON1=make_find_symbol("FBOMB_ICON1");
  l_FBOMB_ICON5=make_find_symbol("FBOMB_ICON5");

  l_PLASMA_ICON20=make_find_symbol("PLASMA_ICON20");
  l_PLASMA_ICON50=make_find_symbol("PLASMA_ICON50");

  l_LSABER_ICON50=make_find_symbol("LSABER_ICON50");
  l_LSABER_ICON100=make_find_symbol("LSABER_ICON100");

  l_DFRIS_ICON4=make_find_symbol("DFRIS_ICON4");
  l_DFRIS_ICON10=make_find_symbol("DFRIS_ICON10");

  l_TELEPORTER_SND=make_find_symbol("TELEPORTER_SND");
  l_gun_tints=make_find_symbol("gun_tints");


  l_ammo_snd=make_find_symbol("AMMO_SND");
  
	l_up_key						=	make_find_symbol("up_key");
        set_symbol_number(l_up_key,JK_UP);

	l_down_key					=	make_find_symbol("down_key");        
        set_symbol_number(l_down_key,JK_DOWN);

	l_left_key        	=	make_find_symbol("left_key");
        set_symbol_number(l_left_key,JK_LEFT);

	l_right_key       	=	make_find_symbol("right_key");
        set_symbol_number(l_right_key,JK_RIGHT);

	l_weapon_left_key 	=	make_find_symbol("weapon_left_key");
        set_symbol_number(l_weapon_left_key,JK_CTRL_R);

	l_weapon_right_key	=	make_find_symbol("weapon_right_key");	
        set_symbol_number(l_weapon_right_key,JK_INSERT);

	l_special_key     	=	make_find_symbol("special_key");
        set_symbol_number(l_special_key,JK_END);


  int i;
  for (i=0;i<MAX_STATE;i++)
    set_symbol_number(make_find_symbol(state_names[i]),i);
  for (i=0;i<TOTAL_ABILITIES;i++)
    set_symbol_number(make_find_symbol(ability_names[i]),i);
  for (i=0;i<TOTAL_CFLAGS;i++)
    set_symbol_number(make_find_symbol(cflag_names[i]),i);

  l_song_list=make_find_symbol("song_list");  
  l_post_render=make_find_symbol("post_render");

  add_c_function("distx",0,0,                   1);
  add_c_function("disty",0,0,                   2);
  add_c_bool_fun("key_pressed",1,1,             3);
  add_c_bool_fun("local_key_pressed",1,1,       4);

  add_c_function("bg_state",0,0,                5);
  add_c_function("aitype",0,0,                  6);
  add_c_function("aistate",0,0,                 7);
  add_c_function("set_aistate",1,1,             8);
  add_c_function("random",1,1,                  9);
  add_c_function("state_time",0,0,             10);
  add_c_function("state",0,0,                  11);
  add_c_function("toward",0,0,                 12);
  add_c_function("move",3,3,                   13);
  add_c_function("facing",0,0,                 14);
  add_c_function("otype",0,0,                  15);
  add_c_bool_fun("next_picture",0,0,           16);
  add_c_bool_fun("set_fade_dir",1,1,           17);
  add_c_function("mover",3,3,                  18);
  add_c_bool_fun("set_fade_count",1,1,         19);
  add_c_function("fade_count",0,0,             20);
  add_c_function("fade_dir",0,0,               21);
  add_c_bool_fun("touching_bg",0,0,            22);
  add_c_function("add_power",1,1,              23);
  add_c_function("add_hp",1,1,                 24);

  add_c_bool_fun("draw",0,0,                   27);
  add_c_bool_fun("edit_mode",0,0,              28);
  add_c_bool_fun("draw_above",0,0,             29);
  add_c_function("x",0,0,                      30);
  add_c_function("y",0,0,                      31);
  add_c_bool_fun("set_x",1,1,                  32);
  add_c_bool_fun("set_y",1,1,                  33);  
  add_c_bool_fun("push_characters",2,2,        34);



  add_c_bool_fun("set_state",1,1,              37);
  add_c_function("bg_x",0,0,                   38);
  add_c_function("bg_y",0,0,                   39);
  add_c_bool_fun("set_aitype",1,1,             40);

  add_c_function("xvel",0,0,                   42);
  add_c_function("yvel",0,0,                   43);
  add_c_bool_fun("set_xvel",1,1,               44);
  add_c_bool_fun("set_yvel",1,1,               45);
  add_c_function("away",0,0,                   46);
  add_c_bool_fun("blocked_left",1,1,           47);
  add_c_bool_fun("blocked_right",1,1,          48);

  add_c_function("add_palette",1,-1,           50);    // name, w,h,x,y,scale, tiles
  add_c_bool_fun("screen_shot",1,1,            51);    // filename

  add_c_bool_fun("set_zoom",1,1,               52);
  add_c_function("show_help",1,1,              55);    // type, x,y
  add_c_function("direction",0,0,              56);
  add_c_function("set_direction",1,1,          57);

  add_c_bool_fun("freeze_player",1,1,          58);   // freeze time

  add_c_function("menu",1,-1,                  59); 
  add_c_bool_fun("do_command",1,1,             60);   // command string
  add_c_bool_fun("set_game_state",1,1,         61);
  

// scene control functions, game must first be set to scene mode.
  add_c_bool_fun("scene:set_text_region",4,4,  62);
  add_c_bool_fun("scene:set_frame_speed",1,1,  63);
  add_c_bool_fun("scene:set_scroll_speed",1,1, 64);
  add_c_bool_fun("scene:set_pan_speed",1,1,    65);
  add_c_bool_fun("scene:scroll_text",1,1,      66);
  add_c_bool_fun("scene:pan",3,3,              67);
  add_c_bool_fun("scene:wait",1,1,             68);

  add_c_bool_fun("level:new",3,3,              74);    // width, height, name

  add_c_bool_fun("do_damage",2,4,              75);    // amount, to_object, [pushx pushy]
  add_c_function("hp",0,0,                     76);
  add_c_bool_fun("set_shift_down",2,2,         77);
  add_c_bool_fun("set_shift_right",2,2,        78);
  add_c_bool_fun("set_gravity",1,1,            79);
  add_c_function("tick",0,0,                   80);

  add_c_bool_fun("set_xacel",1,1,              81);
  add_c_bool_fun("set_yacel",1,1,              82);
  add_c_bool_fun("set_local_players",1,1,      84);   // set # of players on this machine, unsupported?
  add_c_function("local_players",0,0,          85);

  add_c_bool_fun("set_light_detail",1,1,       86);
  add_c_function("light_detail",0,0,           87);
  add_c_bool_fun("set_morph_detail",1,1,       88);
  add_c_function("morph_detail",0,0,           89);
  add_c_bool_fun("morph_into",3,3,             90);       // type aneal frames
  add_c_bool_fun("link_object",1,1,            91); 

  add_c_bool_fun("draw_line",5,5,              92); 
  add_c_function("dark_color",0,0,             93); 
  add_c_function("medium_color",0,0,           94); 
  add_c_function("bright_color",0,0,           95); 

  add_c_bool_fun("remove_object",1,1,          99); 
  add_c_bool_fun("link_light",1,1,            100);  
  add_c_bool_fun("remove_light",1,1,          101); 
  add_c_function("total_objects",0,0,         102); 
  add_c_function("total_lights",0,0,          103); 

  add_c_bool_fun("set_light_r1",2,2,          104);
  add_c_bool_fun("set_light_r2",2,2,          105);
  add_c_bool_fun("set_light_x",2,2,           106);
  add_c_bool_fun("set_light_y",2,2,           107);
  add_c_bool_fun("set_light_xshift",2,2,      108);
  add_c_bool_fun("set_light_yshift",2,2,      109);

  add_c_function("light_r1",1,1,              110);
  add_c_function("light_r2",1,1,              111);
  add_c_function("light_x",1,1,               112);
  add_c_function("light_y",1,1,               113);
  add_c_function("light_xshift",1,1,          114);
  add_c_function("light_yshift",1,1,          115);

  add_c_function("xacel",0,0,                 116);
  add_c_function("yacel",0,0,                 117);
  add_c_bool_fun("delete_light",1,1,          118); 

  add_c_bool_fun("set_fx",1,1,                119);
  add_c_bool_fun("set_fy",1,1,                120);
  add_c_bool_fun("set_fxvel",1,1,             121);
  add_c_bool_fun("set_fyvel",1,1,             122);
  add_c_bool_fun("set_fxacel",1,1,            123);
  add_c_bool_fun("set_fyacel",1,1,            124);
  add_c_function("picture_width",0,0,         125);
  add_c_function("picture_height",0,0,        126);
  add_c_bool_fun("trap",0,0,                  127);
  add_c_bool_fun("platform_push",2,2,         128);

  add_c_function("def_sound",1,2,             133);  // symbol, filename [ or just filenmae]
  add_c_bool_fun("play_sound",1,4,            134);

  add_c_function("weapon_to_type",1,1,        142);  // returns total for type weapon
  add_c_bool_fun("hurt_radius",6,6,           143);  // x y radius max_damage exclude_object max_push

  add_c_bool_fun("add_ammo",2,2,              144);  // weapon_type, amount 
  add_c_function("ammo_total",1,1,            145);  // returns total for type weapon
  add_c_function("current_weapon",0,0,        146);  // weapon_type, amount
  add_c_function("current_weapon_type",0,0,   147);  // returns total for type weapon

  add_c_bool_fun("blocked_up",1,1,            148);
  add_c_bool_fun("blocked_down",1,1,          149);
  add_c_bool_fun("give_weapon",1,1,           150);  // type
  add_c_function("get_ability",1,1,           151);
  add_c_bool_fun("reset_player",0,0,          152);
  add_c_function("site_angle",1,1,            153);
  add_c_bool_fun("set_course",2,2,            154);  // angle, magnitude
  add_c_bool_fun("set_frame_angle",3,3,       155);  // ang1,ang2, ang
  add_c_bool_fun("jump_state",1,1,            156);  // don't reset current_frame  

  add_c_bool_fun("morphing",0,0,              168);
  add_c_bool_fun("damage_fun",6,6,            169);
  add_c_bool_fun("gravity",0,0,               170);
  add_c_bool_fun("make_view_solid",1,1,       171);
  add_c_function("find_rgb",3,3,              172);

  add_c_function("player_x_suggest",0,0,      173);  // return player "joystick" x
  add_c_function("player_y_suggest",0,0,      174);
  add_c_function("player_b1_suggest",0,0,     175);
  add_c_function("player_b2_suggest",0,0,     176);
  add_c_function("player_b3_suggest",0,0,     177);

  add_c_bool_fun("set_bg_scroll",4,4,         178);  // xmul xdiv ymul ydiv
  add_c_bool_fun("set_ambient_light",2,2,     179);  // player, 0..63 (out of bounds ignored)
  add_c_function("ambient_light",1,1,         180);  // player
  add_c_bool_fun("has_object",1,1,            181);  // true if linked with object x
  add_c_bool_fun("set_otype",1,1,             182);  // otype

  add_c_function("current_frame",0,0,         184);  
  add_c_function("fx",0,0,                    185);
  add_c_function("fy",0,0,                    186);
  add_c_function("fxvel",0,0,                 187);
  add_c_function("fyvel",0,0,                 188);
  add_c_function("fxacel",0,0,                189);
  add_c_function("fyacel",0,0,                190);
  add_c_bool_fun("set_stat_bar",2,2,          191);  // filename, object
  add_c_bool_fun("set_fg_tile",3,3,           192);  // x,y, tile #
  add_c_function("fg_tile",2,2,               193);  // x,y
  add_c_bool_fun("set_bg_tile",3,3,           194);  // x,y, tile #
  add_c_function("bg_tile",2,2,               195);  // x,y
  add_c_bool_fun("load_tiles",1,-1,           196);  // filename1, filename2...
  add_c_bool_fun("load_palette",1,1,          197);  // filename
  add_c_bool_fun("load_color_filter",1,1,     198);  // filename
  add_c_bool_fun("create_players",1,1,        199);  // player type, returns true if game is networked
  add_c_bool_fun("try_move",2,3,              200);  // xv yv (check_top=t) -> returns T if not blocked
  add_c_function("sequence_length",1,1,       201);  // sequence number
  add_c_bool_fun("can_see",5,5,               202);  // x1,y1, x2,y2, chars_block
  add_c_function("load_big_font",2,2,         203);  // filename, name
  add_c_function("load_small_font",2,2,       204);  // filename, name
  add_c_function("load_console_font",2,2,     205);  // filename, name
  add_c_function("set_current_frame",1,1,     206);  

  add_c_bool_fun("draw_transparent",2,2,      208);  // count, max
  add_c_bool_fun("draw_tint",1,1,             209);  // tint id number
  add_c_bool_fun("draw_predator",0,0,         210);  // tint_number

  add_c_function("shift_down",1,1,            211);  // player
  add_c_function("shift_right",1,1,           212);  // player
  add_c_bool_fun("set_no_scroll_range",5,5,   213);  // player, x1,y1,x2,y2

  add_c_function("def_image",2,2,             215);  // filename,name
  add_c_bool_fun("put_image",3,3,             216);  // x,y,id
  add_c_function("view_x1",0,0,               217);
  add_c_function("view_y1",0,0,               218);
  add_c_function("view_x2",0,0,               219);
  add_c_function("view_y2",0,0,               220);
  add_c_function("view_push_down",1,1,        221);
  add_c_bool_fun("local_player",0,0,          222);
  add_c_bool_fun("save_game",1,1,             223);  // filename
  add_c_bool_fun("set_hp",1,1,                224);
  add_c_bool_fun("request_level_load",1,1,    225);  // filename
  add_c_bool_fun("set_first_level",1,1,       226);  // filename
  add_c_function("def_tint",1,1,              227);  // filename
  add_c_function("tint_palette",3,3,          228);  // radd,gadd,badd
  add_c_function("player_number",0,0,         229);  
  add_c_bool_fun("set_current_weapon",1,1,    230);  // type
  add_c_bool_fun("has_weapon",1,1,            231);  // type
  add_c_bool_fun("ambient_ramp",1,1,          232);
  add_c_function("total_players",0,0,         233);
  add_c_bool_fun("scatter_line",6,6,          234);  // x1,y1,x2,y2, color, scatter value
  add_c_function("game_tick",0,0,             235);
  add_c_bool_fun("isa_player",0,0,            236);
  add_c_bool_fun("shift_rand_table",1,1,      237);  // amount
  add_c_function("total_frames",0,0,          238);
  add_c_function("raise",0,0,                 239);  // call only from reload constructor!
  add_c_function("lower",0,0,                 240);  // call only from reload constructor!

  add_c_function("player_pointer_x",0,0,      241);
  add_c_function("player_pointer_y",0,0,      242);
  add_c_bool_fun("frame_panic",0,0,           243);
  add_c_bool_fun("ascatter_line",7,7,         244);  // x1,y1,x2,y2, color1, color2, scatter value
  add_c_function("rand_on",0,0,               245);
  add_c_function("set_rand_on",1,1,           246);
  add_c_function("bar",5,5,                   247);
  add_c_function("argc",0,0,                  248);
  add_c_bool_fun("play_song",1,1,             249);  // filename
  add_c_bool_fun("stop_song",0,0,             250);
  add_c_bool_fun("targetable",0,0,            251);
  add_c_bool_fun("set_targetable",1,1,        252);  // T or nil
  add_c_bool_fun("show_stats",0,0,            253);

  add_c_function("kills",0,0,                 254);
  add_c_function("tkills",0,0,                255);
  add_c_function("secrets",0,0,               256);
  add_c_function("tsecrets",0,0,              257);

  add_c_bool_fun("set_kills",1,1,             258);
  add_c_bool_fun("set_tkills",1,1,            259);
  add_c_bool_fun("set_secrets",1,1,           260);
  add_c_bool_fun("set_tsecrets",1,1,          261);
  add_c_bool_fun("request_end_game",0,0,      262);
  add_c_function("get_save_slot",0,0,         263);
  add_c_bool_fun("mem_report",0,0,            264);
  add_c_function("major_version",0,0,         265);
  add_c_function("minor_version",0,0,         266);
  add_c_bool_fun("draw_double_tint",2,2,      267);  // tint1 id number, tint 2 id number
  add_c_function("image_width",1,1,           268);  // image number
  add_c_function("image_height",1,1,          269);  // image number
  add_c_function("foreground_width",0,0,      270);
  add_c_function("foreground_height",0,0,     271);
  add_c_function("background_width",0,0,      272);
  add_c_function("background_height",0,0,     273);
  add_c_function("get_key_code",1,1,          274);  // name of key, returns code that can be used with keypressed
  add_c_bool_fun("set_cursor_shape",3,3,      275);  // image id, x hot, y hot
  add_c_bool_fun("start_server",0,0,          276);
  add_c_bool_fun("put_string",4,5,            277);  // font,x,y,string, [color]
  add_c_function("font_width",1,1,            278);  // font
  add_c_function("font_height",1,1,           279);  // font
  add_c_bool_fun("chat_print",1,1,            280);  // chat string
  add_c_bool_fun("set_player_name",1,1,       281);  // name
  add_c_bool_fun("draw_bar",5,5,              282);  // x1,y1,x2,y2,color
  add_c_bool_fun("draw_rect",5,5,             283);  // x1,y1,x2,y2,color
  add_c_bool_fun("get_option",1,1,            284);
  add_c_bool_fun("dir_exsist",1,1,            285);
  add_c_bool_fun("chdir",1,1,                 286);
  add_c_bool_fun("nice_copy",3,3,             287);  // source file, dest file
  add_c_bool_fun("set_delay_on",1,1,          288);  // T or nil
  add_c_bool_fun("set_login",1,1,             289);  // name
  add_c_bool_fun("enable_chatting",0,0,       290);
  add_c_bool_fun("demo_break_enable",0,0,     291);
  add_c_bool_fun("am_a_client",0,0,           292);
  add_c_bool_fun("time_for_next_level",0,0,   293); 
  add_c_bool_fun("reset_kills",0,0,           294);
  add_c_bool_fun("set_game_name",1,1,         295);  // name
  add_c_bool_fun("set_net_min_players",1,1,   296); 
  add_c_bool_fun("expire_cache_item",1,1,     297);

  add_lisp_function("go_state",1,1,              0);
  add_lisp_function("with_object",2,-1,          1);
  add_lisp_function("bmove",0,1,                 2);   // returns true=unblocked, nil=block, or object
  add_lisp_function("me",0,0,                    3);
  add_lisp_function("bg",0,0,                    4);
  add_lisp_function("find_closest",1,1,          5);
  add_lisp_function("find_xclosest",1,1,         6); 
  add_lisp_function("find_xrange",2,2,           7); 
  add_lisp_function("add_object",3,4,            8);    // type, x,y (type)
  add_lisp_function("first_focus",0,0,           9);
  add_lisp_function("next_focus",1,1,           10);
  add_lisp_function("get_object",1,1,           11);
  add_lisp_function("get_light",1,1,            12);
  add_lisp_function("with_objects",1,1,         13);
  add_lisp_function("add_light",7,7,            14);   // type, x, y, r1, r2, xshift, yshift
  add_lisp_function("find_enemy",1,1,           15);   // exclude
  add_lisp_function("user_fun",0,-1,            16);   // calls anobject's user function
  add_lisp_function("time",2,2,                 17);   // returns a fixed point (times and operation)
  add_lisp_function("name",0,0,                 18);   // returns current object's name (for debugin)
  add_lisp_function("float_tick",0,0,           19);
  add_lisp_function("find_object_in_area",5,5,  20);  // x1, y1, x2, y2  type_list
  add_lisp_function("find_object_in_angle",3,3, 21);  // start_angle end_angle type_list
  add_lisp_function("add_object_after",3,4,     22);  // type, x,y (type)
  add_lisp_function("def_char",2,-1,            23);  // needs at least 2 parms
  add_lisp_function("see_dist",4,4,             24);  // returns longest unblocked path from x1,y1,x2,y2
  add_lisp_function("platform",0,0,             25);
  add_lisp_function("level_name",0,0,           26);
  add_lisp_function("ant_ai",0,0,               27);
  add_lisp_function("sensor_ai",0,0,            28);
  add_lisp_function("dev_draw",0,0,             29);
  add_lisp_function("top_ai",0,0,               30);
  add_lisp_function("laser_ufun",2,2,           31);
  add_lisp_function("top_ufun",2,2,             32);

  add_lisp_function("player_rocket_ufun",2,2,   34);

  if (registered)
  {
    add_lisp_function("plaser_ufun",2,2,          33);
    add_lisp_function("lsaber_ufun",2,2,          35);
  }


  add_lisp_function("cop_mover",3,3,            36);
  add_lisp_function("latter_ai",0,0,            37);
  add_lisp_function("with_obj0",-1,-1,          38);
  add_lisp_function("activated",0,0,            39);
  add_lisp_function("top_draw",0,0,             40);
  add_lisp_function("bottom_draw",0,0,          41);
  add_lisp_function("mover_ai",0,0,             42);
  add_lisp_function("sgun_ai",0,0,              43);
  add_lisp_function("last_savegame_name",0,0,   44);
  add_lisp_function("next_savegame_name",0,0,   45);
  add_lisp_function("argv",1,1,                 46);
  add_lisp_function("joy_stat",0,0,             47);  // xm ym b1 b2 b3
  add_lisp_function("mouse_stat",0,0,           48);  // mx my b1 b2 b3
  add_lisp_function("mouse_to_game",2,2,        49);  // pass in x,y -> x,y
  add_lisp_function("game_to_mouse",2,2,        50);  // pass in x,y -> x,y
  add_lisp_function("get_main_font",0,0,        51);
  add_lisp_function("player_name",0,0,          52);
  add_lisp_function("nice_input",3,3,           53);  // title, prompt, default -> returns input
  add_lisp_function("get_cwd",0,0,              54);
  add_lisp_function("system",1,1,               55);
  add_lisp_function("convert_slashes",2,2,      56);
  add_lisp_function("show_yes_no",4,4,          57);
  add_lisp_function("get_directory",1,1,        58);  // path
  add_lisp_function("nice_menu",3,3,            59);  // title, menu_title, list -> return selection number
  add_lisp_function("respawn_ai",0,0,           60);

  add_lisp_function("score_draw",0,0,           61);
  add_lisp_function("show_kills",0,0,           62); 
  add_lisp_function("mkptr",1,1,                63);  
  add_lisp_function("seq",3,3,                  64);
  add_lisp_function("weapon_icon_ai",0,0,       65); 
  add_lisp_function("on_draw",0,0,              66);
  add_lisp_function("tp2_ai",0,0,               67);
  add_lisp_function("push_char",2,2,            68);
  add_lisp_function("gun_draw",0,0,             69);
  add_lisp_function("ant_draw",0,0,             70);
  add_lisp_function("middle_draw",0,0,          71);
  add_lisp_function("exp_draw",0,0,             72);
  add_lisp_function("exp_ai",0,0,               73);
}


// Note : args for l_caller have not been evaluated yet!
void *l_caller(long number, void *args)
{
  p_ref r1(args);
  switch (number)
  {
    case 0 : 
    {
      current_object->set_aistate(lnumber_value(eval(CAR(args)))); 
      current_object->set_aistate_time(0);
      void *ai=figures[current_object->otype]->get_fun(OFUN_AI);
      if (!ai)
      {
	lbreak("hrump... call to go_state, and no ai function defined?\n"
	       "Are you calling from move function (not mover)?\n");
	exit(0);
      }
      return eval_function((lisp_symbol *)ai,NULL);
    } break;
    case 1 :
    {
      game_object *old_cur=current_object;
      current_object=(game_object *)lpointer_value(eval(CAR(args))); 
      void *ret=eval_block(CDR(args));
      current_object=old_cur;
      return ret;
    }  break;


    case 2 :
    {
      int whit;
      game_object *o;
      if (args)
        o=(game_object *)lpointer_value(eval(CAR(args)));
      else o=current_object;
      game_object *hit=current_object->bmove(whit,o);
      if (hit)
        return new_lisp_pointer(hit);
      else if (whit) return NULL;
      else return true_symbol;
    } break;

    case 3 : return new_lisp_pointer(current_object); break;
    case 4 : 
    { if (player_list->next)
        return new_lisp_pointer(current_level->attacker(current_object));
      else return new_lisp_pointer(player_list->focus); } break;
    case 5 : return new_lisp_pointer(current_level->find_closest(current_object->x,
								 current_object->y,
						       lnumber_value(eval(CAR(args))),
				                       current_object)); break;
    case 6 : return new_lisp_pointer(current_level->find_xclosest(current_object->x,
								  current_object->y,
								  lnumber_value(eval(CAR(args))),
								  current_object
								  )); break;
    case 7 : 
    {
      long n1=lnumber_value(eval(CAR(args)));
      long n2=lnumber_value(eval(CAR(CDR(args))));
      return new_lisp_pointer(current_level->find_xrange(current_object->x,
							 current_object->y,
							 n1,
							 n2
							 ));
    } break;
    case 8 : 
    {
      int type=lnumber_value(eval(CAR(args)));          args=CDR(args);
      long x=lnumber_value(eval(CAR(args)));       args=CDR(args);
      long y=lnumber_value(eval(CAR(args)));  args=CDR(args);
      game_object *o;
      if (args)
        o=create(type,x,y,0,lnumber_value(eval(CAR(args))));
      else
        o=create(type,x,y);
      if (current_level)
        current_level->add_object(o);
      return new_lisp_pointer(o);
    } break;
    case 22 : 
    {
      int type=lnumber_value(eval(CAR(args)));          args=CDR(args);
      long x=lnumber_value(eval(CAR(args)));       args=CDR(args);
      long y=lnumber_value(eval(CAR(args)));  args=CDR(args);
      game_object *o;
      if (args)
        o=create(type,x,y,0,lnumber_value(eval(CAR(args))));
      else
        o=create(type,x,y);
      if (current_level)
        current_level->add_object_after(o,current_object);
      return new_lisp_pointer(o);
    } break;

    case 9 : return new_lisp_pointer(the_game->first_view->focus); break;
    case 10 : 
    {
      view *v=((game_object *)lpointer_value(eval(CAR(args))))->controller()->next;
      if (v)
        return new_lisp_pointer(v->focus);
      else return NULL;
    } break;
    case 11 : 
    { 
      return new_lisp_pointer
      ((void *)current_object->get_object(lnumber_value(eval(CAR(args)))));
    } break;
    case 12 : 
    { 
      return new_lisp_pointer
      ((void *)current_object->get_light(lnumber_value(eval(CAR(args)))));
    } break;
    case 13 :
    {
      game_object *old_cur=current_object;
      void *ret=NULL;
      for (int i=0;i<old_cur->total_objects();i++)
      {	
	current_object=old_cur->get_object(i);
	ret=eval(CAR(args));
      }
      current_object=old_cur;
      return ret;
    } break;
    case 14 :
    {
      int t=lnumber_value(eval(CAR(args))); args=lcdr(args);
      int x=lnumber_value(eval(CAR(args))); args=lcdr(args);
      int y=lnumber_value(eval(CAR(args))); args=lcdr(args);
      int r1=lnumber_value(eval(CAR(args))); args=lcdr(args);
      int r2=lnumber_value(eval(CAR(args))); args=lcdr(args);
      int xs=lnumber_value(eval(CAR(args))); args=lcdr(args);
      int ys=lnumber_value(eval(CAR(args)));
      return new_lisp_pointer(add_light_source(t,x,y,r1,r2,xs,ys));
    } break;
    case 15 :
    {
//      return current_lev shit
    } break;
    case 16 :
    {
      void *f=figures[current_object->otype]->get_fun(OFUN_USER_FUN);
      if (!f) return NULL;
      return eval_function((lisp_symbol *)f,args);      
    } break;
    case 17 :
    {
      long trials=lnumber_value(eval(CAR(args)));
      args=CDR(args);
      time_marker start;
      for (int x=0;x<trials;x++)
      {
	clear_tmp();
	eval(CAR(args));
      }
      time_marker end;
      return new_lisp_fixed_point((long)(end.diff_time(&start)*(1<<16)));
    } break;
    case 18 :
    { return new_lisp_string(object_names[current_object->otype]); } break;
    case 19 :
    { return current_object->float_tick(); } break;
    case 20 :
    {
      long x1=lnumber_value(eval(CAR(args))); args=CDR(args);
      long y1=lnumber_value(eval(CAR(args))); args=CDR(args);
      long x2=lnumber_value(eval(CAR(args))); args=CDR(args);
      long y2=lnumber_value(eval(CAR(args))); args=CDR(args);

      void *list=eval(CAR(args));
      game_object *find=current_level->find_object_in_area(current_object->x,
					      current_object->y,
					      x1,y1,x2,y2,list,current_object);
      if (find) return new_lisp_pointer(find); 
      else return NULL;
    } break;

    case 21 :
    {
      long a1=lnumber_value(eval(CAR(args))); args=CDR(args);
      long a2=lnumber_value(eval(CAR(args))); args=CDR(args);

      void *list=eval(CAR(args));
      p_ref r1(list);
      game_object *find=current_level->find_object_in_angle(current_object->x,
							current_object->y,
							a1,a2,list,current_object);
      if (find) return new_lisp_pointer(find); 
      else return NULL;
    } break;
    case 23 :         // def_character
    {
      lisp_symbol *sym=(lisp_symbol *)lcar(args);
      if (item_type(sym)!=L_SYMBOL)
      {
	lbreak("expecting first arg to def-character to be a symbol!\n");
	exit(0);
      }
      int sp=current_space;
      current_space=PERM_SPACE;
      set_symbol_number(sym,total_objects);   // set the symbol value to the object number
      current_space=sp;
      if (!total_objects)
      {
        object_names=(char **)jmalloc(sizeof(char *)*(total_objects+1),"object name list");
	figures=(character_type **)jmalloc(sizeof(character_type *)*(total_objects+1),"character types");
      }
      else
      {
        object_names=(char **)jrealloc(object_names,sizeof(char *)*(total_objects+1),
				       "object name list");
	figures=(character_type **)jrealloc(figures,sizeof(character_type *)*(total_objects+1),
					    "character types");
      }

      object_names[total_objects]=strcpy(
	  (char *)jmalloc(strlen(lstring_value(symbol_name(sym)))+1,"object name"),
					 lstring_value(symbol_name(sym)));
      figures[total_objects]=new character_type(CDR(args),sym);
      total_objects++;
      return new_lisp_number(total_objects-1);
    } break;
    case 24 :
    {
      long x1=lnumber_value(eval(CAR(args)));  args=CDR(args);
      long y1=lnumber_value(eval(CAR(args)));  args=CDR(args);
      long x2=lnumber_value(eval(CAR(args)));  args=CDR(args);
      long y2=lnumber_value(eval(CAR(args)));
      current_level->foreground_intersect(x1,y1,x2,y2);
      void *ret=NULL;
      push_onto_list(new_lisp_number(y2),ret);
      push_onto_list(new_lisp_number(x2),ret);
      return ret;
    } break;
    case 25 :
    {
#ifdef __WATCOMC__
      return make_find_symbol("WATCOM");
#endif
#ifdef __linux__
      return make_find_symbol("LINUX");
#endif

#ifdef __sgi
      return make_find_symbol("IRIX");
#endif

#ifdef __MAC__
      return make_find_symbol("MAC");
#endif

    } break;
    case 26 :
    {
      return new_lisp_string(current_level->name());
    } break;
    case 27 : return ant_ai(); break;
    case 28 : return sensor_ai(); break;
    case 29 : if (dev&EDIT_MODE) current_object->drawer(); break;
    case 30 : return top_ai(); break;
    case 31 : return laser_ufun(args); break;
    case 32 : return top_ufun(args); break;
    case 33 : return plaser_ufun(args); break;
    case 34 : return player_rocket_ufun(args); break;
    case 35 : return lsaber_ufun(args); break;
    case 36 :
    {
      
      long xm,ym,but;
      xm=lnumber_value(CAR(args)); args=CDR(args);
      ym=lnumber_value(CAR(args)); args=CDR(args);
      but=lnumber_value(CAR(args));
      return cop_mover(xm,ym,but);
    } break;
    case 37 : return ladder_ai(); break;
    case 38 :
    {
      game_object *old_cur=current_object;
      current_object=current_object->get_object(0);
      void *ret=eval_block(args);
      current_object=old_cur;
      return ret;
    }  break;
    case 39 :
    {
      if (current_object->total_objects()==0)
        return true_symbol;
      else if (current_object->get_object(0)->aistate())
        return true_symbol;
      else return NULL;
    } break;
    case 40 : top_draw(); break;
    case 41 : bottom_draw(); break;
    case 42 : return mover_ai(); break;
    case 43 : return sgun_ai();
    case 44 :
    {
      char nm[50];
      last_savegame_name(nm);
      return new_lisp_string(nm);
    } break;
    case 45 :
    {
      char nm[50];
      sprintf(nm,"save%04d.pcx",load_game(1,symbol_str("LOAD")));
//      get_savegame_name(nm);
      the_game->reset_keymap();
      return new_lisp_string(nm);
    } break;
    case 46 :
    {
      return new_lisp_string(start_argv[lnumber_value(eval(CAR(args)))]);
    } break;
    case 47 :
    {
      int xv,yv,b1,b2,b3;
      if (has_joystick)
        joy_status(b1,b2,b3,xv,yv);
      else b1=b2=b3=xv=yv=0;

      void *ret=NULL;
      p_ref r1(ret);
      push_onto_list(new_lisp_number(b3),ret);
      push_onto_list(new_lisp_number(b2),ret);
      push_onto_list(new_lisp_number(b1),ret);
      push_onto_list(new_lisp_number(yv),ret);
      push_onto_list(new_lisp_number(xv),ret);
      return ret;
    } break;
    case 48 :
    {
      void *ret=NULL;
      {
	p_ref r1(ret);
	push_onto_list(new_lisp_number((last_demo_mbut&4)==4),ret);
	push_onto_list(new_lisp_number((last_demo_mbut&2)==2),ret);
	push_onto_list(new_lisp_number((last_demo_mbut&1)==1),ret);
	push_onto_list(new_lisp_number(last_demo_my),ret);
	push_onto_list(new_lisp_number(last_demo_mx),ret);
      }
      return ret;
    } break;
    case 49 :
    {
      long x=lnumber_value(eval(CAR(args))); args=CDR(args);
      long y=lnumber_value(eval(CAR(args))); args=CDR(args);

      long rx,ry;
      the_game->mouse_to_game(x,y,rx,ry);
      void *ret=NULL;
      {
	p_ref r1(ret);
	push_onto_list(new_lisp_number(ry),ret);
	push_onto_list(new_lisp_number(rx),ret);
      }
      return ret;
    } break;
    case 50 :
    {
      long x=lnumber_value(eval(CAR(args))); args=CDR(args);
      long y=lnumber_value(eval(CAR(args))); args=CDR(args);

      long rx,ry;
      the_game->game_to_mouse(x,y,current_view,rx,ry);
      void *ret=NULL;
      {
	p_ref r1(ret);
	push_onto_list(new_lisp_number(ry),ret);
	push_onto_list(new_lisp_number(rx),ret);
      }
      return ret;
    } break;
    case 51 :   return new_lisp_pointer(eh->font()); break;
    case 52 : 
    {
      view *c=current_object->controller();
      if (!c)
        lbreak("object is not a player, cannot return name");
      else
        return new_lisp_string(c->name);
    } break;
    case 53 :
    {
      char tit[100],prompt[100],def[100];
      strcpy(tit,lstring_value(eval(CAR(args))));  args=CDR(args);
      strcpy(prompt,lstring_value(eval(CAR(args))));  args=CDR(args);
      strcpy(def,lstring_value(eval(CAR(args))));  args=CDR(args);
      return nice_input(tit,prompt,def);
    } break;
    case 54 :
    {
      char cd[150];
      getcwd(cd,100);
      return new_lisp_string(cd);
    } break;
    case 55 :
    { system(lstring_value(eval(CAR(args)))); } break;
    case 56 :
    {
      void *fn=eval(CAR(args)); args=CDR(args);
      char tmp[200];
      {
	p_ref r1(fn);
	char *slash=lstring_value(eval(CAR(args)));
	char *filename=lstring_value(fn);

	char *s=filename,*tp;
	
	for (tp=tmp;*s;s++,tp++)
	{
	  if (*s=='/' || *s=='\\') 
	  *tp=*slash;
	  else *tp=*s;
	}
	*tp=0;
      }
      return new_lisp_string(tmp);
    } break;
    case 57 :
    {
      return show_yes_no(CAR(args),CAR(CDR(args)),CAR(CDR(CDR(args))),CAR(CDR(CDR(CDR(args)))));
    } break;
    case 58 :
    {
      char **files,**dirs;
      int tfiles,tdirs,i;

      get_directory(lstring_value(eval(CAR(args))),files,tfiles,dirs,tdirs);
      void *fl=NULL,*dl=NULL,*rl=NULL;
      {
	p_ref r1(fl),r2(dl);
	
	for (i=tfiles-1;i>=0;i--) { push_onto_list(new_lisp_string(files[i]),fl); jfree(files[i]); }
	jfree(files);

	for (i=tdirs-1;i>=0;i--) { push_onto_list(new_lisp_string(dirs[i]),dl); jfree(dirs[i]); }
	jfree(dirs);
	
	push_onto_list(dl,rl);
	push_onto_list(fl,rl);
      }
      
      return rl;
    } break;
    case 59 :
    {
      return nice_menu(CAR(args),CAR(CDR(args)),CAR(CDR(CDR(args))));
    } break;
    case 60 : return respawn_ai(); break;
    case 61 : return score_draw();  break;
    case 62 : return show_kills(); break;
    case 63 : 
    {
    	long x;
    	sscanf(lstring_value(eval(CAR(args))),"%x",&x);
    	return new_lisp_pointer((void *)x);
    } break;
    case 64 :
    {
      char name[256],name2[256];
      strcpy(name,lstring_value(eval(CAR(args))));  args=CDR(args);
      long first=lnumber_value(eval(CAR(args)));  args=CDR(args);
      long last=lnumber_value(eval(CAR(args)));
      long i;
      void *ret=NULL;
      p_ref r1(ret);

      if (last>=first)
      {
        for (i=last;i>=first;i--)
        {
          sprintf(name2,"%s%04d.pcx",name,i);
          push_onto_list(new_lisp_string(name2),ret);
        }
      } else
      {
        for (i=last;i<=first;i++)
        {
          sprintf(name2,"%s%04d.pcx",name,i);
          push_onto_list(new_lisp_string(name2),ret);
        }
      }      
      return ret;
    } break;
    case 65 :
    { return weapon_icon_ai(); } break;
    case 66 :
    { return on_draw(); } break;
    case 67 :
    { return tp2_ai(); } break;
    case 68 :
    { return push_char(args); } break;
    case 69 :
    { return gun_draw(); } break;
    case 70 :
    { return ant_draw(); } break;
    case 71 :
    { return middle_draw(); } break;
    case 72 :
    { return exp_draw(); } break;
    case 73 :
    { return exp_ai(); } break;
  }
  return NULL;
}

//extern bFILE *rcheck,*rcheck_lp;


int get_lprop_number(void *symbol, int def)  // returns def if symbol undefined or not number type
{
  void *v=symbol_value(symbol);
  if (v)
  {
    switch (item_type(v))
    {
      case L_FIXED_POINT :
      case L_NUMBER : 
      { return lnumber_value(v); } break; 
      default : return def;		      
    }
  } else return def;
}





