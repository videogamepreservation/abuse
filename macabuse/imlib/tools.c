#include "tools.hpp"



tool_picker::~tool_picker() 
{ delete old_pal; 
  delete map; 
  for (int i=0;i<total_icons;i++)
    delete icons[i];                   // delete visual object, which should be a "shell"  
}

void tool_picker::remap(palette *pal, window_manager *wm, image *screen)
{
  delete map;
  map=new filter(old_pal,pal);
  draw_first(screen,wm);
}

tool_picker::tool_picker(int X, int Y, int ID, 
	      int show_h, visual_object **Icons, int *Ids, int total_ic, 
			 palette *icon_palette, palette *pal, window_manager *wm, ifield *Next) :
  spicker(X,Y,ID,show_h,1,1,0,Next)
{
  iw=ih=0;
  icons=Icons;
  ids=Ids;
  total_icons=total_ic;
  for (int i=0;i<total_ic;i++)
  {
    if (icons[i]->width(wm)>iw) iw=icons[i]->width(wm);
    if (icons[i]->height(wm)>ih) ih=icons[i]->height(wm);
  }
  map=new filter(icon_palette,pal);
  old_pal=icon_palette->copy();
  reconfigure();
}

void tool_picker::draw_item(window_manager *wm, image *screen, int x, int y, int num, int active)
{
  if (!active)
    screen->bar(x,y,x+iw-1,y+ih-1,wm->black());
  else
    screen->bar(x,y,x+iw-1,y+ih-1,wm->bright_color());
  icons[num]->draw(screen,x,y,wm,map);
}

