#include <string.h>
#include <stdio.h>
#include "dprint.hpp"
#include "keys.hpp"
#include "id.hpp"
#include "macgame.hpp"
#include "mackeys.hpp"
#include "clisp.hpp"
#include "lisp.hpp"

#define APPLEMENU_ID			128
#define ABOUT_ITEM				1
#define ABOUTBOX_ID				128

#define GAMEMENU_ID				129
#define	PLAY_ITEM					1
#define PREFERENCES_ITEM	2
#define   PREFERENCES_ID		128
#define     EDITMODE_ITEM			5
#define     DOUBLEPIXEL_ITEM	7
#define     SCANLINE_ITEM			8
#define     SINGLEPIXEL_ITEM	9
#define     PDEFAULT_BUT			3
#define     PREVERT_BUT				2
#define     POK_BUT						1
#define KEYCONFIG_ITEM		3
#define 	KEYCONFIG_ID			129
#define     UPKEY_ITEM				13
#define     LEFTKEY_ITEM			14
#define     RIGHTKEY_ITEM			15
#define     DOWNKEY_ITEM			16
#define     SPECIALKEY_ITEM		17
#define     SWILEFTKEY_ITEM		18
#define     SWIRIGHTKEY_ITEM	19
#define     KDEFAULT_BUT			3
#define     KREVERT_BUT				2
#define     KOK_BUT						1
#define QUIT_ITEM					5

int MacCont,MacPlay;
extern Rect *gRect;

FILE *open_FILE(char *filename, char *mode);

void HIG_PositionDialog( ResType dialogType, short id)
{
  Handle theTemplate; // Handle to resource template
  Rect        *theRect;   // Bounding box of dialog
  short       left;       // Left side of centered rect   
  short       top;        // Top side of centered rect        
  

  theTemplate = GetResource(dialogType, id);
  theRect = (Rect*) *theTemplate;
  
  left = (gRect->right - 
          (theRect->right - theRect->left)) / 2;

  top = (gRect->bottom - 
          (theRect->bottom - theRect->top)) / 3;

  top = (top > GetMBarHeight() + 1) ? top : GetMBarHeight() + 1;

  theRect->right += left - theRect->left;
  theRect->left = left;
  theRect->bottom += top - theRect->top;
  theRect->top = top;
}

extern int PixMode;
extern int dev;
extern int start_edit;
extern int start_running;
extern int disable_autolight;

void MacPreferences()
{
	DialogPtr dlg;
	short item;
	int cont;
	int emode, dmode;
	short t;
	ControlHandle h;
	Rect r;
	
	HIG_PositionDialog( 'DLOG', PREFERENCES_ID);
	dlg = GetNewDialog(PREFERENCES_ID,0,(WindowPtr)-1);
	SetDialogDefaultItem(dlg,POK_BUT);
	
	cont = 1;
	emode = 0;
	dmode = 0;
	
	// Draw current dialog settings	
	GetDialogItem(dlg,EDITMODE_ITEM,&t,(Handle*)&h,&r);
	SetControlValue(h,emode);
	for (int i=0; i<4; i++)
	{
		GetDialogItem(dlg,DOUBLEPIXEL_ITEM+i,&t,(Handle*)&h,&r);
		SetControlValue(h,i==dmode);
	}

	while (cont)
	{
		ModalDialog(0,&item);
		switch (item)
		{
			case EDITMODE_ITEM:
				emode = !emode;
				GetDialogItem(dlg,EDITMODE_ITEM,&t,(Handle*)&h,&r);
				SetControlValue(h,emode);
				break;
			case DOUBLEPIXEL_ITEM:
			case SCANLINE_ITEM:
			case SINGLEPIXEL_ITEM:
				GetDialogItem(dlg,DOUBLEPIXEL_ITEM+dmode,&t,(Handle*)&h,&r);
				SetControlValue(h,0);
				dmode = item - DOUBLEPIXEL_ITEM;
				GetDialogItem(dlg,DOUBLEPIXEL_ITEM+dmode,&t,(Handle*)&h,&r);
				SetControlValue(h,1);
				break;
			case PDEFAULT_BUT:
			case PREVERT_BUT:
				if (item == PDEFAULT_BUT)
				{
					emode = 0;
					dmode = 0;
				}
				else
				{
				}
				GetDialogItem(dlg,EDITMODE_ITEM,&t,(Handle*)&h,&r);
				SetControlValue(h,emode);
				for (int i=0; i<4; i++)
				{
					GetDialogItem(dlg,DOUBLEPIXEL_ITEM+i,&t,(Handle*)&h,&r);
					SetControlValue(h,i==dmode);
				}
				break;
			case POK_BUT:
				cont = 0;
				break;
		}
	}
	DisposeDialog(dlg);

	if (emode)
	{
		dev|=EDIT_MODE;    
		start_edit=1;
		start_running=1;
		disable_autolight=1;
	}

	PixMode = 3 - dmode;
}

class CMacKeyConfig
{
protected:
	DialogPtr dlg;
	short tsel;
	int revert_key[7];
	int key[7];

	void LoadDefaults()
	{
  	key[0] = JK_LEFT;
  	key[1] = JK_RIGHT;
  	key[2] = JK_UP;
  	key[3] = JK_DOWN;
  	key[4] = '0';
  	key[5] = JK_CTRL_L;
  	key[6] = '1';
	}

	void ReenterKeys()
	{
		Str255 name;
		short t;
		ControlHandle h;
		Rect r;

		for (int i=0; i<7; i++)
		{
			GetDialogItem(dlg,i + UPKEY_ITEM,&t,(Handle*)&h,&r);
			key_name(key[i],(char*)&name[1]);
			name[0] = strlen((char*)&name[1]);
			SetDialogItemText((Handle)h, name);
		}
		SelectDialogItemText(dlg,tsel + UPKEY_ITEM,0,32767);
	}
	
	void SetKey(int newkey)
	{
		Str255 name;
		short t;
		ControlHandle h;
		Rect r;

		key[tsel] = newkey;
		GetDialogItem(dlg,tsel + UPKEY_ITEM,&t,(Handle*)&h,&r);
		key_name(newkey,(char*)&name[1]);
		name[0] = strlen((char*)&name[1]);
		SetDialogItemText((Handle)h, name);
		if (++tsel>=7)
			tsel = 0;
		SelectDialogItemText(dlg,tsel + UPKEY_ITEM,0,32767);
	}
	
	void LoadKeys();
	
	void WriteKeys();
	
	void ProcessKey(int key)
	{
	  switch ((key & keyCodeMask)>>8)
	  {
	  	case 0x7d : 	SetKey(JK_DOWN); break;
	    case 0x7e :  	SetKey(JK_UP);  break;
	    case 0x7b : 	SetKey(JK_LEFT);  break;
	    case 0x7c : 	SetKey(JK_RIGHT);  break;
	    /*
	    case XK_Control_L : 	SetKey(JK_CTRL_L);  break;
	    case XK_Control_R : 	SetKey(JK_CTRL_R);  break;
	    case XK_Alt_L : 		SetKey(JK_ALT_L);  break;
	    case XK_Alt_R : 		SetKey(JK_ALT_R);  break;
	    case XK_Shift_L : 	SetKey(JK_SHIFT_L);  break;
	    case XK_Shift_R : 	SetKey(JK_SHIFT_R);  break;
	    */
	    case 0x47 : 	SetKey(JK_NUM_LOCK);  break;
	    case 0x73 : 	SetKey(JK_HOME);  break;
	    case 0x77 : 	SetKey(JK_END);  break;
	    case 0x33 :		SetKey(JK_BACKSPACE);  break;
	    case 0x30 :		SetKey(JK_TAB);  break;
	    case 0x4c :		SetKey(JK_ENTER);  break;
	    case 0x39 :		SetKey(JK_CAPS);  break;
	  	case 0x35 :		SetKey(JK_ESC);  break;
	   	case 0x71 :   SetKey(JK_F1); break;
	    case 0x78 :   SetKey(JK_F2);break;
	    case 0x63 :   SetKey(JK_F3); break;
	    case 0x76 :   SetKey(JK_F4); break;
	    case 0x60 :   SetKey(JK_F5); break;
	    case 0x61 :   SetKey(JK_F6); break;
	    case 0x62 :   SetKey(JK_F7); break;
	    case 0x64 :   SetKey(JK_F8); break;
	    case 0x65 :   SetKey(JK_F9); break;
	    case 0x6d :   SetKey(JK_F10); break;
			case 0x67 :   SetKey(JK_INSERT); break;
			case 0x74 :   SetKey(JK_PAGEUP); break;
			case 0x79 :   SetKey(JK_PAGEDOWN); break;
			
	    default :
				SetKey(key & charCodeMask);
	  }
	}
	
	void ProcessModifiers()
	{
		unsigned char km[16];
		char modstat[8];
		char stat;
		int keyed;

		// Get key status of modifiers
		GetKeys((unsigned long*)&km);
		keyed = -1;
		for (int i=0; i<MAC_LAST-MAC_COMMAND; i++) {
			stat = (km[ (i+MAC_COMMAND) >> 3 ] >> ((i+MAC_COMMAND) & 7)) & 1;
			if (stat && !modstat[i])
				keyed = i;
			modstat[i] = stat;
		}
		
		if (keyed >= 0)
			SetKey(mac_map[keyed]);
	}
	
public:
	void RunDialog();
};
static CMacKeyConfig MacKeyConfig;

void CMacKeyConfig::LoadKeys()
{
  FILE *fp=open_FILE("mackeys.lsp","rb");
	if (fp)
	{
		char st[256];
		fgets(st,255,fp);
		sscanf(st+17,"%d %d %d %d %d %d %d",
					 &key[1],&key[2],&key[0],&key[3],&key[5],&key[6],&key[4] );
		fclose(fp);
	}	 
  else 
  {
  	dprintf("Unable to read to file gamma.lsp\n");
  	LoadDefaults();
	}
	for (int i=0; i<7; i++)
		revert_key[i] = key[i];
}

void CMacKeyConfig::WriteKeys()
{
  FILE *fp=open_FILE("mackeys.lsp","wb");
  if (fp)
  {
		fprintf(fp,"(setq key_list '( %d %d %d %d %d %d %d ))\n",
								key[1], key[2], key[0], key[3], key[5], key[6], key[4] );
								
	  set_symbol_number(l_left_key, key[1]);
	  set_symbol_number(l_right_key, key[2]);
	  set_symbol_number(l_up_key, key[0]);
	  set_symbol_number(l_down_key, key[3]);
	  set_symbol_number(l_weapon_left_key, key[5]);
	  set_symbol_number(l_weapon_right_key, key[6]);
	  set_symbol_number(l_special_key, key[4]);
								
		fclose(fp);
  } 
  else
  	dprintf("Unable to write to file mackeys.lsp\n");
}
	

void CMacKeyConfig::RunDialog()
{
	GrafPtr savePort;
	short item;
	int cont;
	EventRecord ev;
	short t;
	ControlHandle h;
	Rect r;

	LoadKeys();
	
	HIG_PositionDialog( 'DLOG', KEYCONFIG_ID);
	dlg = GetNewDialog(KEYCONFIG_ID,0,(WindowPtr)-1);
	
	GetPort( &savePort );
	SetPort( dlg );
	
	SetDialogDefaultItem(dlg,KOK_BUT);
	UpdateDialog(dlg,dlg->visRgn);

	tsel = 0;
	ReenterKeys();
	
	cont = 1;
	while (cont)
	{
		if (GetNextEvent(everyEvent,&ev))
		{
			switch (ev.what)
			{
			case mouseDown:
				GlobalToLocal(&ev.where);
				item = FindDialogItem(dlg, ev.where) + 1;
				switch (item)
				{
					case UPKEY_ITEM:
					case LEFTKEY_ITEM:
					case RIGHTKEY_ITEM:
					case DOWNKEY_ITEM:
					case SPECIALKEY_ITEM:
					case SWILEFTKEY_ITEM:
					case SWIRIGHTKEY_ITEM:
						// Key box selection
						tsel = item - UPKEY_ITEM;
						SelectDialogItemText(dlg,item,0,32767);
						break;
					case KDEFAULT_BUT:
						// reset keys to default
						GetDialogItem(dlg,item,&t,(Handle*)&h,&r);
						if (TrackControl(h,ev.where,0))
						{
							LoadDefaults();
							ReenterKeys();
						}
						break;
					case KREVERT_BUT:
						// revert keys to previous
						GetDialogItem(dlg,item,&t,(Handle*)&h,&r);
						if (TrackControl(h,ev.where,0))
						{
							for (int i=0; i<7; i++)
								key[i] = revert_key[i];
							ReenterKeys();
						}
						break;
					case KOK_BUT:
						// OK were done
						GetDialogItem(dlg,item,&t,(Handle*)&h,&r);
						if (TrackControl(h,ev.where,0))
							cont = 0;
						break;
				} // end switch (item)
				break;
			case keyDown:
				ProcessKey(ev.message);
				break;
			case updateEvt:
				BeginUpdate(dlg);
				UpdtDialog(dlg,dlg->visRgn);
				EndUpdate(dlg);
				break;
			} // end switch (ev.what)
		} // end if
		
		ProcessModifiers();
	}
	SetPort( savePort );
	DisposeDialog(dlg);
	
	WriteKeys();
}

void MacAppleMenuCommand(short menuItem)
{
  Str255 deskAccName;

  switch (menuItem)
  {
    case ABOUT_ITEM:
	    HIG_PositionDialog( 'ALRT', ABOUTBOX_ID);
	    Alert(ABOUTBOX_ID, NULL);
	    break;
	
    default:
      GetItem(GetMHandle(APPLEMENU_ID), menuItem,deskAccName);
      OpenDeskAcc(deskAccName);
      break;
  }
}

void MacKeyConfigMenu()
{
  MacKeyConfig.RunDialog();
}

void MacGameMenuCommand(short menuItem)
{
  switch (menuItem)
  {
  	case PLAY_ITEM:
			MacCont = 0;
			MacPlay = 1;
  		break;
  	case PREFERENCES_ITEM:
  		MacPreferences();
  		break;
  	case KEYCONFIG_ITEM:
	  	MacKeyConfigMenu();
  		break;
    case QUIT_ITEM:
			MacCont = 0;
			MacPlay = 0;
      break;
  }
}

void MacMenuCommand(long menuItem_Id)
{
  short menuId = HiWord(menuItem_Id);
  short menuItem = LoWord(menuItem_Id);

  switch (menuId)
  {
    case APPLEMENU_ID:
      MacAppleMenuCommand(menuItem);
      break;

    case GAMEMENU_ID:
      MacGameMenuCommand(menuItem);
      break;
  }

  HiliteMenu(0);
}

void MacKeyEvent(char key,short modifiers)
{
  if ((modifiers & cmdKey) != 0)
    MacMenuCommand(MenuKey(key));
}

void MacMouseEvent(const EventRecord * eventPtr)
{
  WindowPtr whichWindow;
  short partCode;

  // find out where the user clicked
  partCode = FindWindow(eventPtr->where, &whichWindow);

  switch(partCode)
  {
    case inMenuBar:
      MacMenuCommand(MenuSelect(eventPtr->where));
    	break;
        
    case inSysWindow:       
      SystemClick(eventPtr, whichWindow);
    	break;
  }
}

int MacMenu()
{
  EventRecord event;
  
  // make sure app is supposed to be running.
  MacCont = 1;
  MacPlay = 0;
  while(MacCont)
  {
  	SystemTask();
  	if (GetNextEvent(everyEvent,&event))
	    switch(event.what)
	    {
	      case mouseDown:         
	        MacMouseEvent(&event);
	      	break;
	                      
	      case keyDown:
	      case autoKey:
	        MacKeyEvent((char)(event.message &charCodeMask),event.modifiers);
	      	break;
	      
	    }
	}
	
	return MacPlay;
}
