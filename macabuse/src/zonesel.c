#include "scroller.hpp"
#include "cache.hpp"
#include "sock.hpp"
#include "jwindow.hpp"
#ifdef __MAC__
#include "atalk.hpp"
#endif


extern window_manager *eh;


// used to sort zones into alphabetical order
int str_p_compare(const void *a, const void *b)
{
  return strcmp(  *((char **)a), 
                  *((char **)b));
}


extern net_protocol *prot;

void get_zone()
{
  char *fakes[]={"PC Zone","Apple Zone2","AIX zone"};
  char **strs=fakes;
  int t_zones=3;
  
  //  if (!prot || strcmp(prot->name(),"AppleTalk"))
  // return ;
   

  

  window_manager *w=eh;
  JCFont *f=w->font();

  // notify the user that the computer is not locked (yet)
  char *wait_str="Gathering Zone information, please wait....";
  jwindow *wait_window=w->new_window(xres/2-strlen(wait_str)*f->width()/2-10,
               yres/2-f->height()/2-10,
               -1,-1,
               new info_field(WINDOW_FRAME_LEFT,WINDOW_FRAME_TOP,0,
               wait_str,0));
   w->flush_screen();
   

#ifdef __MAC__
  atalk_protocol *ap=(atalk_protocol *)prot;    
  strs=ap->GetZones(t_zones);
#endif  
 
   w->close_window(wait_window);
   
   if (t_zones==0)  // if there are no zones found return back to caller
     return;

  qsort(strs,t_zones,sizeof(char *),str_p_compare);           

  image *ok_image=cash.img(cash.reg("art/frame.spe","dev_ok",SPEC_IMAGE,1))->copy(),
    *cancel_image=cash.img(cash.reg("art/frame.spe","cancel",SPEC_IMAGE,1))->copy();


  int show_y=20;
  
   button *cancel_button=new button(WINDOW_FRAME_LEFT,WINDOW_FRAME_TOP+show_y*f->height()+25,
     1,cancel_image,0);

  // this is the scroll list which will be inside the window
   //  pick_list(int X, int Y, int ID, int height,
   //	    char **List, int num_entries, int start_yoffset, ifield *Next, image *texture=NULL);

  pick_list *pick=new pick_list(WINDOW_FRAME_LEFT,
      WINDOW_FRAME_TOP,
      1,
      show_y,
      strs,
      t_zones,
      0,
      cancel_button);
     
  jwindow *zwin=w->new_window(xres/2-f->width()*18-10,yres/2-f->height()*10-18,-1,-1,pick);

  event ev;
  int done=0;
  do
  {
    w->flush_screen();
    w->get_event(ev);
    if (ev.type==EV_MESSAGE && ev.message.id==1)
    {
      int pk=pick->get_selection();
#ifdef __MAC__
      if (pk>=0 && pk<t_zones)
        ap->SetZone(strs[pk]);
#endif
      done=1;
    } else if (ev.type==EV_MESSAGE && ev.message.id==2)  // canceled
      done=1;
  } while (!done);
  
  w->close_window(zwin);
  delete cancel_image;
  delete ok_image;
}
