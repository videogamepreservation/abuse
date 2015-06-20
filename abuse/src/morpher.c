#include "morpher.hpp"
#include "game.hpp"
#include "objects.hpp"
#include "view.hpp"

void morph_char::draw(game_object *who, view *v)
{
  if (fleft)
  {
    long rx,ry;
    the_game->game_to_mouse(who->x-(cx>>16),who->y-(cy>>16),v,rx,ry);
    mor->show(screen,rx,ry,color_table,pal,1000);
    cx+=dcx;
    cy+=dcy;  
    fleft--;
  }
}



morph_char::morph_char(game_object *who, int to_type, void (*stat_fun)(int), int anneal, int frames)
{  
  mor=NULL;
  character_type *t1=figures[who->otype],*t2=figures[to_type];
  if (!t1->has_sequence(morph_pose) || t1->morph_mask<0 || 
      !t2->has_sequence(morph_pose) || t2->morph_mask<0)
    fleft=0;
  else
  {
    if (anneal==-1)
    {
      switch (morph_detail)
      {
	case HIGH_DETAIL : 
	{ anneal=30; } break;
	case MEDIUM_DETAIL : 
	{ anneal=15; } break;
	case LOW_DETAIL : 
	{ anneal=8; } break;
	case POOR_DETAIL : 
	{ anneal=3; } break;
      }
    }

    fleft=frames;
    trans_image *h1=new trans_image(cash.img(t1->morph_mask),"morph tmp"),
                *h2=new trans_image(cash.img(t2->morph_mask),"morph tmp");
    super_morph *sm=new super_morph(h1,h2,anneal,stat_fun);
    if (sm->t)
    {
      delete h1;
      delete h2;
      figure *f1=t1->get_sequence(morph_pose)->get_figure(0),
      *f2=t2->get_sequence(morph_pose)->get_figure(0);
      image *i1=f1->forward->make_image(),
      *i2=f2->forward->make_image();

      mor=new smorph_player(sm,pal,i1,i2,fleft,who->direction);
      delete i2;
      delete i1;
      delete sm;

      if (who->direction>0)
      {
	cx=((int)f1->xcfg)<<16;
	dcx=(((int)f2->xcfg-(int)f1->xcfg)<<16)/(fleft-1);
      } else
      {
	cx=(mor->w-((int)f1->xcfg))<<16;
	dcx=((((int)f1->xcfg-(int)f2->xcfg))<<16)/(fleft-1);
      }
      cy=((int)f1->height()-1)<<16;
      dcy=((f2->height()-f1->height())<<16)/(fleft-1);
    } else 
    {
      delete sm;
      fleft=0;
    }
  }    
}











