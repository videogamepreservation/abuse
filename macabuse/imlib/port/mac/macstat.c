#include "macstat.hpp"
#include "dprint.hpp"

extern Rect *gRect;

#define ABUSE_STAT_ID 		134
#define ABUSE_BARPAT1_ID		128
#define ABUSE_BARPAT2_ID		129
#define ABUSE_BARPAT3_ID		130
#define ABUSE_BARPATBG_ID		131

class mac_status_node
{
  public :  
  char *name;
  mac_status_node *next;
  visual_object *show;
  int last_update;
  mac_status_node(char *Name, visual_object *Show, mac_status_node *Next) 
  { 
  	name=strcpy((char *)jmalloc(strlen(Name)+1,"status name"),Name); 
    show=Show;
    next=Next; 
    last_update=0;
  }
  ~mac_status_node() { jfree(name); if (show) delete show; }
} ; 

mac_status_manager::mac_status_manager()
{ 
  first=NULL; 
  level=0; 
}

void mac_status_manager::push(char *name, visual_object *show)
{
//	return;

	Rect r;
	
  level++;
  first=new mac_status_node(name,show,first);  
	if (level==1)
	{
		PicHandle thePicture;
		Rect picFrame,r;
		int picWX,picWY;
		PixPatHandle patbg;
	
		thePicture = GetPicture(ABUSE_STAT_ID);
		picFrame = (**thePicture).picFrame;
		picWX = (picFrame.right - picFrame.left);
		picWY = (picFrame.bottom - picFrame.top);

#if 1		
		r.left = (gRect->left+gRect->right)/2 - picWX/2;
		r.right = (gRect->left+gRect->right)/2 + picWX/2;
		r.top = (gRect->top+gRect->bottom)/2 - picWY/2;
		r.bottom = (gRect->top+gRect->bottom)/2 + picWY/2;
#else
		r = *gRect;
#endif

		win = (CWindowPtr)NewCWindow(nil, &r, "\p", TRUE, 2, (WindowPtr)-1L, FALSE, 0);
		
		GetGWorld(&saveWorld, &saveDevice);
		SetGWorld((GWorldPtr)win,saveDevice);
		
		pat1 = GetPixPat(ABUSE_BARPAT1_ID);
		pat2 = GetPixPat(ABUSE_BARPAT2_ID);
		pat3 = GetPixPat(ABUSE_BARPAT3_ID);
		patbg = GetPixPat(ABUSE_BARPATBG_ID);

		FillCRect(&r,patbg);
#if 1
		r = picFrame;
#else
		r.left = (gRect->left+gRect->right)/2 - picWX/2;
		r.right = (gRect->left+gRect->right)/2 + picWX/2;
		r.top = (gRect->top+gRect->bottom)/2 - picWY/2;
		r.bottom = (gRect->top+gRect->bottom)/2 + picWY/2;
#endif
		DrawPicture(thePicture, &r);
	
		ReleaseResource((Handle)thePicture);
		ReleaseResource((Handle)patbg);
	}

	if (level == 1 || level == 2)
	{
#if 1
		r.left = 		296 + level*5 - 6 + level;
		r.right = 	296 + level*5;
		r.top =    	205;
		r.bottom = 	205 + 162;
#else
		r.left = 		((gRect->right - gRect->left)/2) + 127 + level*5 - 6 + level;
		r.right = 	((gRect->right - gRect->left)/2) + 127 + level*5;
		r.top =    	((gRect->bottom - gRect->top)/2) + 5;
		r.bottom = 	((gRect->bottom - gRect->top)/2) + 5 + 162;
#endif
		FillCRect(&r,pat1);
	}
}

void mac_status_manager::update(int percentage)
{
//	return;

	Rect r;
	
#if 1
	r.left = 		296 + level*5 - 6 + level;
	r.right = 	296 + level*5;
	r.top =    	205 + 162 - percentage*162/100;
	r.bottom = 	205 + 162;
#else
	r.left = 		((gRect->right - gRect->left)/2) + 127 + level*5 - 6 + level;
	r.right = 	((gRect->right - gRect->left)/2) + 127 + level*5;
	r.top =    	((gRect->bottom - gRect->top)/2) + 5 + 162 - percentage*162/100;
	r.bottom = 	((gRect->bottom - gRect->top)/2) + 5 + 162;
#endif
	if (level == 1)
		FillCRect(&r,pat2);
	else if (level == 2)
		FillCRect(&r,pat3);
}

void mac_status_manager::pop()
{
//	return;

  level--;
  mac_status_node *p=first; 
  first=first->next;
  delete p;

  if (level==0)
  {
		CloseWindow((WindowPtr)win);
		SetGWorld(saveWorld, saveDevice);
		ReleaseResource((Handle)pat1);
		ReleaseResource((Handle)pat2);
		ReleaseResource((Handle)pat3);
  }
}





