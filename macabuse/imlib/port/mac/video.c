#include <QDOffscreen.h>
#include <Palettes.h>
#include <Displays.h>
#include <Video.h>
#include <Menus.h>

#include "video.hpp"
#include "dev.hpp"

#include "filter.hpp"
#include "globals.hpp"
#include "system.h"
#include "dos.h"
#include "macs.hpp"
#include "bitmap.h"
#include "image.hpp"
#include "jmalloc.hpp"
#include "dprint.hpp"

#include "RequestVideo.h"

#include "hack.hpp"

// Resource id for crack window
#define 	WIND_CRACK 	1000

#define PAL_DIRECT    // direct palette routines - yes!
#define VIDEO_DIRECT  // direct video routines - yes!

unsigned char current_background;

// Some number for jollies
int vmode;
image *screen;

Rect Bounds;
CWindowPtr mainwin,backwin;
CTabHandle MacCT,saved_pal;
#ifndef PAL_DIRECT
PaletteHandle MacPal;
#endif
int PixMult;
int PixMode;

int HackMode = 0;

// Menu Globals
short OldMBarHeight = 0;
RgnHandle OldVisRgn = 0;

// DM stuff
VideoRequestRec requestRec;
VideoRequestRec originalRec;
short		currentDepth;
short		currentHorizontal;
short		currentVertical;

// Direct Video globals
GDHandle gd;
short gVideoRowBytes;
char *gVideoMem;
char *gGame1VideoMem;
char *gGame2VideoMem;
char *gRealVideoMem;
Rect *gRect;

#ifdef VIDEO_DIRECT
void (*SpeedCopyBits)(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy);
void SpeedCopyBits11(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy);
void SpeedCopyBits21Fast(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy);
void SpeedCopyBits21Slow(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy);
void SpeedCopyBits22Fast(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy);
void SpeedCopyBits22Slow(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy);
#endif

class CMacStartup {
public:

	void ToolBoxInit()
	{
		// Initialize mac toolboxes
		InitGraf(&qd.thePort);
    InitFonts();
    FlushEvents(everyEvent - osMask - diskMask, 0);
    InitWindows();
    InitMenus();
//    TEInit();
    InitDialogs(0L);
    InitCursor();

		SetApplLimit(GetApplLimit() - 327680);

//    MaxApplZone();
	}

	void BuildMenuBar();
	void HideMenu();
	void RestoreMenu();
	
	void SetMode();
	void GrabScreen();	
	void MakeWindows();
	void DestroyWindows();
	
	CMacStartup()
	{
		ToolBoxInit();
		
		PixMode = 3;

		xres = 640;
		yres = 480;
	}
	
	void Init();
	void Quit();
	~CMacStartup();
	
} MacStartup;

void CMacStartup::SetMode()
{
	requestRec.screenDevice = nil;					// find any screen
	requestRec.reqBitDepth = 8;							// bit depth request
	requestRec.reqHorizontal = 640;					// H request
	requestRec.reqVertical = 480;						// V request
	requestRec.displayMode = nil;						// must init to nil
	requestRec.depthMode = nil;							// must init to nil
	requestRec.requestFlags = 1<<kAllValidModesBit;						
																					// give me the HxV over bit depth, and only safe video modes

	// make the request and set it if we have one....
	RVRequestVideoSetting(&requestRec);

	if (requestRec.screenDevice == nil)		// make sure we found a device...possible if there are no "safe" video modes
	{
		dprintf("Can't get current video mode\n");
		exit(1);
	}

	// Get current setting
	originalRec.screenDevice = requestRec.screenDevice;		// this screen
	RVGetCurrentVideoSetting(&originalRec);
	
	HackSetRequest (&requestRec);
//		if (noErr != RVConfirmVideoRequest (&requestRec))
//			RVSetVideoRequest (&originalRec);
}

void CMacStartup::BuildMenuBar()
{
  #define MENUBARID 		128
	#define APPLEMENUID  	128
  Handle menuBarH;

  menuBarH = GetNewMBar(MENUBARID);

  if (menuBarH != NULL)
  {
    SetMenuBar(menuBarH);
    AddResMenu(GetMHandle(APPLEMENUID), 'DRVR');
    DrawMenuBar();	        
  }
  else
  {
    dprintf("Can't get Menu!\n");
  	exit(1);
  }
}

void CMacStartup::HideMenu()
{
	RgnHandle DesktopRgn;

	if (OldMBarHeight == 0)
	{
		// Get and copy the current gray region
		DesktopRgn = LMGetGrayRgn();
		OldVisRgn = NewRgn();
		CopyRgn(DesktopRgn, OldVisRgn);
		
		// Fudge the menu bar height
		OldMBarHeight = GetMBarHeight();
		LMSetMBarHeight(0);

		// Turn the gray into the old gray region plus the menu bar region
		Rect MenuRect;
		MenuRect.left = 0;
		MenuRect.top = 0;
		MenuRect.right = qd.screenBits.bounds.right;
		MenuRect.bottom = OldMBarHeight;
		RgnHandle MenuRgn = NewRgn();
		RectRgn(MenuRgn, &MenuRect);

		UnionRgn(OldVisRgn, MenuRgn, DesktopRgn);
		DisposeRgn(MenuRgn);
	}
}

void CMacStartup::RestoreMenu()
{
	if (OldMBarHeight && OldVisRgn)
	{
		// Restore the menu bar height
		LMSetMBarHeight(OldMBarHeight);
		OldMBarHeight = 0;

		// Restore the old desktop region
		CopyRgn(OldVisRgn, LMGetGrayRgn());
		DisposeRgn(OldVisRgn);
		OldVisRgn = 0;
		
		// Redraw the menu bar
		HiliteMenu(0);
		DrawMenuBar();
	}
}

void CMacStartup::GrabScreen()
{
	PixMapHandle PMH;

	gd = requestRec.screenDevice;
	PMH = (*gd)->gdPMap;
	LockPixels(PMH);
	gVideoRowBytes = (*PMH)->rowBytes & 0x3FFF;
	gVideoMem = gRealVideoMem = GetPixBaseAddr(PMH);
	gRect = &(*gd)->gdRect;

	// set color table for 8 bit mode
#ifdef PAL_DIRECT
	MacCT = (**((**gd).gdPMap)).pmTable;
#else
	MacCT = GetCTable(8);
	MacPal = NewPalette(256,nil,pmTolerant,0);
	(**MacCT).ctSeed = GetCTSeed();
#endif

	xres = gRect->right - gRect->left;
	yres = gRect->bottom - gRect->top;
	
	gGame1VideoMem = gRealVideoMem + ((xres/2 - 320/2)&~7) + gVideoRowBytes 
		  						* ( yres/2 - 200/2 );
	gGame2VideoMem = gRealVideoMem + ((xres/2 - 640/2)&~7) + gVideoRowBytes 
		  						* ( yres/2 - 400/2 );
}

void CMacStartup::MakeWindows()
{
	Rect CurBounds;
	GrafPtr savePort;

#ifdef VIDEO_DIRECT

	// use hack to get pixel mode
	SpeedCopyBits = &SpeedCopyBits11; PixMult = 1;

#endif

	Bounds = *gRect;
	
	backwin = (CWindowPtr)NewCWindow(nil, &Bounds, "\p", TRUE, 2, (WindowPtr)-1L, FALSE, 0);
	SetGWorld((GWorldPtr)backwin,gd);

	char *p = gVideoMem;
	for (int y=Bounds.top; y<Bounds.bottom; y++)
	{
		memset(p,0xff,Bounds.right-Bounds.left);
		p += gVideoRowBytes;
	}		

#ifndef VIDEO_DIRECT
	Bounds.left = (gRect->left+gRect->right)/2 - xres/2 * PixMult;
	Bounds.right = (gRect->left+gRect->right)/2 + xres/2 * PixMult;
	Bounds.top = (gRect->top+gRect->bottom)/2 - yres/2 * PixMult;
	Bounds.bottom = (gRect->top+gRect->bottom)/2 + yres/2 * PixMult;
	mainwin = (CWindowPtr)NewCWindow(nil, &Bounds, "\p", TRUE, 2, (WindowPtr)-1L, FALSE, 0);
	SetGWorld((GWorldPtr)mainwin,gd);
	CurBounds = mainwin->portRect;

	xres = (Bounds.right - Bounds.left)/PixMult;
	yres = (Bounds.bottom - Bounds.top)/PixMult;
#endif

	// save old palette
	saved_pal = (**((**gd).gdPMap)).pmTable;
	HandToHand((Handle*)&saved_pal);
}

void CMacStartup::DestroyWindows()
{
#ifdef PAL_DIRECT
	ColorSpec *spec,*spec2;
	VDSetEntryRecord setEntriesRec;
	Ptr				 csPtr;
	QDErr			error;
	RgnHandle junkRgn;

	spec = (**MacCT).ctTable;
	spec2 = (**saved_pal).ctTable;
	for (int i=0; i<(**MacCT).ctSize; i++)
	{
		spec[i].rgb.red = spec2[i].rgb.red;
		spec[i].rgb.green = spec2[i].rgb.green;
		spec[i].rgb.blue = spec2[i].rgb.blue;
		spec[i].value = spec2[i].value;
	}
	setEntriesRec.csTable = (ColorSpec *)&(**MacCT).ctTable;
	setEntriesRec.csStart = 0;
	setEntriesRec.csCount = (**MacCT).ctSize;
	csPtr = (Ptr) &setEntriesRec;
	error = Control ((**gd).gdRefNum, cscSetEntries, (Ptr) &csPtr);
	if (error)
		dprintf("aieee! palette problem!\n");
#else
	// restore palette
	MacPal = NewPalette((**saved_pal).ctSize,saved_pal,pmTolerant,0);
	NSetPalette((WindowPtr)mainwin,MacPal,pmAllUpdates);
	ActivatePalette((WindowPtr)mainwin);
#endif
#ifndef VIDEO_DIRECT	
	CloseWindow((WindowPtr)mainwin);
#endif
	CloseWindow((WindowPtr)backwin);	
}

void CMacStartup::Init()
{
	HideMenu();	

#ifdef VIDEO_DIRECT
	HideCursor();
#else
	// erase cursor
	ShieldCursor(&CurBounds,topLeft((**(mainwin->portPixMap)).bounds));
#endif

	SetMode();
//		BuildMenuBar();
	GrabScreen();
//	fprintf(stderr,"Initting Screen.\n");

	MakeWindows();
}

void CMacStartup::Quit()
{
	DestroyWindows();
}

CMacStartup::~CMacStartup()
{
	HackRestoreRequest (&originalRec);
//	RVSetVideoAsScreenPrefs ();
	RestoreMenu();
	FlushEvents(everyEvent, 0);

	// restore cursor
	ShowCursor();
}
	
void InitMacScreen()
{
	MacStartup.Init();
}

void QuitMacScreen()
{
	MacStartup.Quit();
}

int get_vmode()
{ 
	return vmode; 
}

void image::make_page(short width, short height, unsigned char *page_buffer)
//  creates memory that will be touched externally, for routines to copy to
//  something will copy this memory to video memory
{
  if (special && !special->static_mem)
  {
#ifdef VIDEO_DIRECT
    data=(unsigned char *)jmalloc(width*height,"image::direct_data");
#else
		GWorldPtr gw;
		PixMapHandle pixmaph;
		QDErr err;
		Rect r;

  	r.left = 0;
  	r.top = 0;
  	r.right = width;
  	r.bottom = height;
  	// use mac image, but set
		err = NewGWorld( &gw, 8, &r, MacCT, nil, 0 );
  	special->extended_descriptor = gw;
		pixmaph = GetGWorldPixMap(gw);
		HLockHi((Handle)pixmaph);
		LockPixels(pixmaph);
  	data = (unsigned char *)GetPixBaseAddr(pixmaph);
  	(**pixmaph).pmTable = MacCT;
  	
  	// yikes! hack the row bytes
  	(**pixmaph).rowBytes = 0x8000 | width;
  	w = (**pixmaph).rowBytes & 0x3fffl;
  	h = height;
 #endif
  }
  else
  {
    if (!page_buffer)
    	// no preallocated image, so allocate some
      data=(unsigned char *)jmalloc(width*height,"image::data");
    else
    	// we want to use a preallocated image as this image's memory
    	data=page_buffer;
  }

  if (special)
  	// set clipping area
    special->resize(width,height);
}

void image::delete_page()
//  frees page memory
{
	if (special && !special->static_mem) {
#ifdef VIDEO_DIRECT
		jfree(data);
#else
		GWorldPtr gw;
	
		gw = (GWorldPtr)special->extended_descriptor;
		
		if (gw)
			DisposeGWorld(gw);
#endif
		special->extended_descriptor = 0;
	}
	else if (!special)
		jfree(data);
}

void set_mode(video_mode new_mode, int argc, char **argv)
{
	// create screen memory
  screen=new image(xres,yres,NULL,2);  

	// clear screen
  screen->clear();
  update_dirty(screen);
  
  if (new_mode==VMODE_320x200)
    Pre_Hack_Mode();
}

void RestoreMac()
{
	ColorSpec *spec,*spec2;
	VDSetEntryRecord setEntriesRec;
	Ptr				 csPtr;
	QDErr			error;

	char *p = gVideoMem;
	for (int y=Bounds.top; y<Bounds.bottom; y++)
	{
		memset(p,0xff,Bounds.right-Bounds.left);
		p += gVideoRowBytes;
	}		

	spec = (**MacCT).ctTable;
	spec2 = (**saved_pal).ctTable;
	for (int i=0; i<(**MacCT).ctSize; i++)
	{
		spec[i].rgb.red = spec2[i].rgb.red;
		spec[i].rgb.green = spec2[i].rgb.green;
		spec[i].rgb.blue = spec2[i].rgb.blue;
		spec[i].value = spec2[i].value;
	}
	setEntriesRec.csTable = (ColorSpec *)&(**MacCT).ctTable;
	setEntriesRec.csStart = 0;
	setEntriesRec.csCount = (**MacCT).ctSize;
	csPtr = (Ptr) &setEntriesRec;
	error = Control ((**gd).gdRefNum, cscSetEntries, (Ptr) &csPtr);
	if (error)
		dprintf("aieee! palette problem!\n");
		
	ShowCursor();
}

void UnRestoreMac()
{
	HideCursor();
}

void close_graphics()
{
  delete screen;
}

#ifdef VIDEO_DIRECT

void SpeedCopyBits11(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy)
{
	int x,y;
	unsigned char *p,*q,*pp,*qq;
	unsigned long srclen = im->width();
	
	p = (unsigned char *)im->scan_line(sy1) + sx1;
	q = (unsigned char *)gVideoMem + gVideoRowBytes*(dy+Bounds.top) + (dx+Bounds.left);
	for (y=sy1; y<=sy2; y++)
	{
#if 0
		pp = p;
		qq = q;
		for (x=sx1; x<=sx2; x++)
			*(qq++) = *(pp++);
#else
		memcpy(q,p,sx2-sx1+1);
#endif
		p += srclen;
		q += gVideoRowBytes;
	}
}

#ifdef __POWERPC__
asm void SCB21InnerLoop(unsigned char *p, unsigned char *q, unsigned char *q2,
												 long src_margin_width, long margin_width,
												 int xcount, int ycount )
{
				stmw		r22,-40(SP) 					// save registers used
				addi		r3,r3,-4							// predecrement pointers, so that we can use load w/ update
				addi		r4,r4,-4
				addi		r6,r6,-4							//  "hacked" optimization to reduce a substraction:
																			//  since we preload data, we need to adjust the pointer
lupe0:
				mr			r29,r8								//  reset counter to xcount
				lwzu		r31,4(r3)							//  preload first data
lupe1:
				lwzu		r26,4(r3)							// preload next data

				rlwimi	r30,r31, 0, 0, 7			// cool shift & mask to expand low pixels
				rlwimi	r30,r31,24, 8,15
				rlwimi	r30,r31,24,16,23
				rlwimi	r30,r31,16,24,31

				stwu		r30,4(r4)							// save off 1 "fat" pixels
				
				rlwimi	r28,r31,16, 0, 7			// shift for high pixels
				rlwimi	r28,r31,8 , 8,15
				rlwimi	r28,r31,8 ,16,23
				rlwimi	r28,r31,0 ,24,31

				stwu		r28,4(r4)							// another "fat" pixels
				
				// singly unrolled loop iteration
				lwzu		r31,4(r3)
				
				rlwimi	r30,r26, 0, 0, 7
				rlwimi	r30,r26,24, 8,15
				rlwimi	r30,r26,24,16,23
				rlwimi	r30,r26,16,24,31

				stwu		r30,4(r4)
				
				rlwimi	r28,r26,16, 0, 7
				rlwimi	r28,r26,8 , 8,15
				rlwimi	r28,r26,8 ,16,23
				rlwimi	r28,r26,0 ,24,31

				stwu		r28,4(r4)

				addic.	r29,r29,-8						// update xcount
				bgt			lupe1									// done with line?
				
				add			r3,r3,r6							// skip to next row
				add			r4,r4,r7
				addic.	r9,r9,-1							// update ycount
				
				bgt			lupe0									// done?
				nop
				
				lmw			r22,-40(SP)						// restore registers, how nice
				blr
}
#else

#if 1
asm void SCB21InnerLoop(unsigned char *p, unsigned char *q, unsigned char *q2,
												 long src_margin_width, long margin_width,
												 int xcount, int ycount )
{
				link		a6,#0x0000
				movem.l	d4-d7/a4,-(a7)
				movea.l 8(a6),a4							// p
				movea.l 12(a6),a1							// q
				move.l  24(a6),d6							// margin_width
				move.l  28(a6),d7							// xcount
				move.l  32(a6),d5							// ycount
lupe0:
				move.l	d7,d4									// copy xcount
lupe1:
				move.l	(a4)+,d1							// get 4 pixels
				move.l	d1,d0									// copy
				lsr.l		#8,d0									// expand pixel pair
				lsr.w		#8,d0
				move.l	d0,d2
				lsl.l		#8,d0
				or.l		d0,d2
				move.l	d2,(a1)+							// write pixel pair
				
				swap		d1										// move pair into position
				lsr.l		#8,d1									// expand pixel pair
				lsr.w		#8,d1
				move.l	d1,d2
				lsl.l		#8,d1
				or.l		d1,d2
				move.l	d2,(a1)+							// write pixel pair
				
				subq.l	#4,d4									// next pixel quad
				bgt.s		lupe1
				
				adda.l	20(a6),a4							// advance scanline pointers
				adda.l	d6,a1
				
				subq.l	#1,d5									// next row
				bgt.s		lupe0
				
				movem.l	(a7)+,d4-d7/a4
				unlk		a6
				rts
}
#else
void SCB21InnerLoop(unsigned char *p, unsigned char *q, unsigned char *q2,
												 long src_margin_width, long margin_width,
												 int xcount, int ycount )
{
	short dat;
	
	for (; ycount>0; ycount--)
	{
		for (int x=xcount; x>0; x--) {
			dat = *(p++);
			dat |= dat<<8;
			*((unsigned short *)q) = dat; q+=2;
		}
		p += src_margin_width;
		q += margin_width;
	}
}
#endif

#endif 

void SpeedCopyBits21Fast(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy)
{
	if ((sx1&7)!=0 || (sx2&7)!=7)
		SpeedCopyBits21Slow(im,sx1,sy1,sx2,sy2,dx,dy);
	else
	{
		unsigned char *p;
		unsigned char *q;
		unsigned long srclen = im->width();
		unsigned long src_width = sx2 - sx1 + 1;
		
		p = (unsigned char *)im->scan_line(sy1) + sx1;
		q = (unsigned char *)gVideoMem + gVideoRowBytes*(dy*2+Bounds.top) + (dx*2+Bounds.left);
	
		SCB21InnerLoop(p,q,0,
									 srclen - src_width, (gVideoRowBytes - src_width)*2,
									 src_width, (sy2 - sy1 + 1));
	}
}

void SpeedCopyBits21Slow(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy)
{
	int x,y;
	unsigned char *p,*pp;
	unsigned short *q,*qq,dat;
	unsigned long srclen = im->width();
	
	p = (unsigned char *)im->scan_line(sy1) + sx1;
	q = (unsigned short*)((unsigned char *)gVideoMem + 
												gVideoRowBytes*(dy*2+Bounds.top) + (dx*2+Bounds.left));
	for (y=sy1; y<=sy2; y++)
	{
		pp = p;
		qq = q;
		for (x=sx1; x<=sx2; x++) {
			dat = *(pp++);
			dat |= dat<<8;
			*(qq++) = dat;
		}
		p += srclen;
		q += gVideoRowBytes;
	}
}

#ifdef __POWERPC__
asm void SCB22InnerLoop(unsigned char *p, unsigned char *q, unsigned char *q2,
												 long src_margin_width, long margin_width,
												 int xcount, int ycount )
{
				stmw		r22,-40(SP) 					// save registers used
				addi		r3,r3,-4							// predecrement pointers, so that we can use load w/ update
				addi		r4,r4,-4
				addi		r5,r5,-4
				addi		r6,r6,-4							//  "hacked" optimization to reduce a substraction:
																			//  since we preload data, we need to adjust the pointer
lupe0:
				mr			r29,r8								//  reset counter to xcount
				lwzu		r31,4(r3)							//  preload first data
lupe1:
				lwzu		r26,4(r3)							// preload next data

				rlwimi	r30,r31, 0, 0, 7			// cool shift & mask to expand low pixels
				rlwimi	r30,r31,24, 8,15
				rlwimi	r30,r31,24,16,23
				rlwimi	r30,r31,16,24,31

				stwu		r30,4(r4)							// save off 2 "fat" pixels
				stwu		r30,4(r5)
				
				rlwimi	r28,r31,16, 0, 7			// shift for high pixels
				rlwimi	r28,r31,8 , 8,15
				rlwimi	r28,r31,8 ,16,23
				rlwimi	r28,r31,0 ,24,31

				stwu		r28,4(r4)							// two more "fat" pixels
				stwu		r28,4(r5)
				
				// singly unrolled loop iteration
				lwzu		r31,4(r3)
				
				rlwimi	r30,r26, 0, 0, 7
				rlwimi	r30,r26,24, 8,15
				rlwimi	r30,r26,24,16,23
				rlwimi	r30,r26,16,24,31

				stwu		r30,4(r4)
				stwu		r30,4(r5)
				
				rlwimi	r28,r26,16, 0, 7
				rlwimi	r28,r26,8 , 8,15
				rlwimi	r28,r26,8 ,16,23
				rlwimi	r28,r26,0 ,24,31

				stwu		r28,4(r4)
				stwu		r28,4(r5)

				addic.	r29,r29,-8						// update xcount
				bgt			lupe1									// done with line?
				
				add			r3,r3,r6							// skip to next row
				add			r4,r4,r7
				add			r5,r5,r7
				addic.	r9,r9,-1							// update ycount
				
				bgt			lupe0									// done?
				nop
				
				lmw			r22,-40(SP)						// restore registers, how nice
				blr
}
#else

#if 1
asm void SCB22InnerLoop(unsigned char *p, unsigned char *q, unsigned char *q2,
												 long src_margin_width, long margin_width,
												 int xcount, int ycount )
{
				link		a6,#0x0000
				movem.l	d4-d7/a4,-(a7)
				movea.l 8(a6),a4							// p
				movea.l 12(a6),a1							// q
				movea.l 16(a6),a0							// q2
				move.l  24(a6),d6							// margin_width
				move.l  28(a6),d7							// xcount
				move.l  32(a6),d5							// ycount
lupe0:
				move.l	d7,d4									// copy xcount
lupe1:
				move.l	(a4)+,d1							// get 4 pixels
				move.l	d1,d0									// copy
				lsr.l		#8,d0									// expand pixel pair
				lsr.w		#8,d0
				move.l	d0,d2
				lsl.l		#8,d0
				or.l		d0,d2
				move.l	d2,(a1)+							// write pixel pair
				move.l	d2,(a0)+
				
				swap		d1										// move pair into position
				lsr.l		#8,d1									// expand pixel pair
				lsr.w		#8,d1
				move.l	d1,d2
				lsl.l		#8,d1
				or.l		d1,d2
				move.l	d2,(a1)+							// write pixel pair
				move.l	d2,(a0)+
				
				subq.l	#4,d4									// next pixel quad
				bgt.s		lupe1
				
				adda.l	20(a6),a4							// advance scanline pointers
				adda.l	d6,a1
				adda.l	d6,a0
				
				subq.l	#1,d5									// next row
				bgt.s		lupe0
				
				movem.l	(a7)+,d4-d7/a4
				unlk		a6
				rts
}
#else
void SCB22InnerLoop(unsigned char *p, unsigned char *q, unsigned char *q2,
												 long src_margin_width, long margin_width,
												 int xcount, int ycount )
{
	short dat;
	
	for (; ycount>0; ycount--)
	{
		for (int x=xcount; x>0; x--) {
			dat = *(p++);
			dat |= dat<<8;
			*((unsigned short *)q) = dat; q+=2;
			*((unsigned short *)q2) = dat; q2+=2;
		}
		p += src_margin_width;
		q += margin_width;
		q2 += margin_width;
	}
}
#endif

#endif 

void SpeedCopyBits22Fast(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy)
{
	if ((sx1&7)!=0 || (sx2&7)!=7)
		SpeedCopyBits22Slow(im,sx1,sy1,sx2,sy2,dx,dy);
	else
	{
		unsigned char *p;
		unsigned char *q;
		unsigned long srclen = im->width();
		unsigned long src_width = sx2 - sx1 + 1;
		
		p = (unsigned char *)im->scan_line(sy1) + sx1;
		q = (unsigned char *)gVideoMem + gVideoRowBytes*(dy*2+Bounds.top) + (dx*2+Bounds.left);
	
		SCB22InnerLoop(p,q,((unsigned char*)q + gVideoRowBytes),
									 srclen - src_width, (gVideoRowBytes - src_width)*2,
									 src_width, (sy2 - sy1 + 1));
	}
}

void SpeedCopyBits22Slow(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy)
{
	int x,y;
	unsigned char *p,*pp;
	unsigned short *q,*qq,*qq2,dat;
	unsigned long srclen = im->width();
	
	p = (unsigned char *)im->scan_line(sy1) + sx1;
	q = (unsigned short*)((unsigned char *)gVideoMem + 
												gVideoRowBytes*(dy*2+Bounds.top) + (dx*2+Bounds.left));

	for (y=sy1; y<=sy2; y++)
	{
		pp = p;
		qq = q;
		qq2 = (unsigned short*)((unsigned char*)q + gVideoRowBytes);
		for (x=sx1; x<=sx2; x++) {
			dat = *(pp++);
			dat |= dat<<8;
			*(qq++) = dat;
			*(qq2++) = dat;
		}
		p += srclen;
		q += gVideoRowBytes;
	}
}

#endif

void update_dirty(image *im, int xoff, int yoff)
// go through list of dirty rects & display
{
  int count;
  dirty_rect *dr,*q;
  image *Xim;
  CHECK(im->special);  // make sure the image has the ablity to contain dirty areas
  if (im->special->keep_dirt==0)
//    put_image(im,xoff,yoff);
		q = 0;
  else
  {
    count=im->special->dirties.number_nodes();
    if (!count) return;  // if nothing to update, return
    dr= (dirty_rect *) (im->special->dirties.first());
    while (count>0)
    {
#ifdef VIDEO_DIRECT
			int x1 = dr->dx1,y1 = dr->dy1,x2 = dr->dx2,y2 = dr->dy2,dx1 = xoff + dr->dx1,dy1 = yoff + dr->dy1;
			
    	if (dx1<0)
    	{
    		x1 -= dx1;
    		dx1 = 0;
    	}
    	if (dy1<0)
    	{
    		y1 -= dy1;
    		dy1 = 0;
    	}
    	if (dx1 + x2-x1 >= xres)
    		x2 = (xres - 1) - dx1 + x1;
    	if (dy1 + y2-y1 >= yres)
    		y2 = (yres - 1) - dy1 + y1;

			if (x2>=x1 && y2>=y1)
				(*SpeedCopyBits)(im, x1,y1,x2,y2,dx1,dy1);
#else
  		GWorldPtr gw;
  		Rect src,dst;

    	gw = (GWorldPtr)im->special->extended_descriptor;

    	dst.left = (xoff + dr->dx1)*PixMult;
    	dst.top = (yoff + dr->dy1)*PixMult;
    	dst.right = (xoff + dr->dx2 + 1)*PixMult;
    	dst.bottom = (yoff + dr->dy2 + 1)*PixMult;
    	src.left = dr->dx1;
    	src.top = dr->dy1;
    	src.right = dr->dx2 + 1;
    	src.bottom = dr->dy2 + 1;
    	
			CopyBits((BitMap *) (*(gw->portPixMap)),
					(BitMap*) (*(mainwin->portPixMap)), 
					&src,
					&dst,
					srcCopy, nil);
//      put_part_image(win,im,xoff+dr->dx1,yoff+dr->dy1,dr->dx1,dr->dy1,dr->dx2,dr->dy2);     
//      XDrawRectangle(display,mainwin,gc,xoff+dr->dx1,yoff+dr->dy1,
//		     xoff+dr->dx2-dr->dx1+1,yoff+dr->dy2-dr->dy1+1);
#endif
      q=dr;
      dr=(dirty_rect *) (dr->next());
      im->special->dirties.unlink((linked_node *)q);
      delete q;
      count--;
    }
  }
//  XFlush(display);
}

void fill_image(image *im, int x1, int y1, int x2, int y2);
void clear_put_image(image *im, int x, int y);

int DEBUG_PAL = 0;

void palette::load()
{
	if (DEBUG_PAL)
		return;
	
#ifdef PAL_DIRECT
	ColorSpec *spec;
	VDSetEntryRecord setEntriesRec;
	Ptr				 csPtr;
	QDErr			error;

	spec = (**MacCT).ctTable;
	for (int i=0; i<(**MacCT).ctSize; i++)
	{
		spec[i].rgb.red = red(i) * 256;
		spec[i].rgb.green = green(i) * 256;
		spec[i].rgb.blue = blue(i) * 256;
		spec[i].value = i;
	}
	setEntriesRec.csTable = (ColorSpec *)&(**MacCT).ctTable;
	setEntriesRec.csStart = 0;
	setEntriesRec.csCount = (**MacCT).ctSize;
	csPtr = (Ptr) &setEntriesRec;
	error = Control ((**gd).gdRefNum, cscSetEntries, (Ptr) &csPtr);
	if (error)
		dprintf("aieee! palette problem!\n");
#else
	ColorSpec *spec;
	spec = (**MacCT).ctTable;
	for (int i=0; i<pal_size(); i++)
	{
		spec[i].rgb.red = red(i) * 256;
		spec[i].rgb.green = green(i) * 256;
		spec[i].rgb.blue = blue(i) * 256;
		spec[i].value = i;
	}
//	(**MacCT).ctSeed = GetCTSeed();
	CTab2Palette(MacCT,MacPal,pmTolerant,0);
	NSetPalette((WindowPtr)mainwin,MacPal,pmAllUpdates);
//		HLockHi((Handle)mainwin->portPixMap);
//		LockPixels(mainwin->portPixMap);
//	(**mainwin->portPixMap).pmTable = MacCT;
	ActivatePalette((WindowPtr)mainwin);
#endif
}

void palette::load_nice()
{
	// adapt palette to system's
	load();
}

extern void fade_in(image *im, int steps);
extern void fade_out(int steps);

void Pre_Hack_Mode()
{
#ifdef VIDEO_DIRECT
  // select copy routine
  switch (PixMode)
  {
  	case 1:
  		SpeedCopyBits = &SpeedCopyBits11; 
  		PixMult = 1; 
  		gVideoMem = gGame1VideoMem;
			break;
  	case 2:
  		SpeedCopyBits = &SpeedCopyBits21Fast;
  		PixMult = 2;
  		gVideoMem = gGame2VideoMem;
  		break;
  	case 3:
  		SpeedCopyBits = &SpeedCopyBits22Fast; 
  		PixMult = 2;  
  		gVideoMem = gGame2VideoMem;
  		break;
  	default:
  		dprintf("Aiiieee!  Can't set direct video copier.. it's gonna blow!\n");
  		exit(0);
  		break;
  }
#endif
	xres = 320;
	yres = 200;
	screen->HackW(xres);
	screen->HackH(yres);
	screen->special->resize(xres,yres);
}

void Hack_Mode()
{
//	if (dev & EDIT_MODE)
//		return;

	if (HackMode)
		return;
		
	char *p = gVideoMem;
	for (int y=Bounds.top; y<Bounds.bottom; y++)
	{
		memset(p,0xff,Bounds.right-Bounds.left);
		p += gVideoRowBytes;
	}		

	HackMode = 1;
	Pre_Hack_Mode();
}

void Unhack_Mode()
{
//	if (dev & EDIT_MODE)
//		return;

	HackMode = 0;
	SpeedCopyBits = &SpeedCopyBits11;
	PixMult = 1;
	xres = gRect->right - gRect->left;
	yres = gRect->bottom - gRect->top;
	screen->HackW(xres);
	screen->HackH(yres);
	screen->special->resize(xres,yres);
	gVideoMem = gRealVideoMem;
}

void switch_mode(video_mode new_mode)
{
  if (new_mode==VMODE_320x200)
    Hack_Mode();
  else if (new_mode==VMODE_640x480)
    Unhack_Mode();
}

/*
unsigned char SmoothTable[256][256];

void MakeSmoothTable()
{
	for (int i=0; i<256; i++)
		for (int j=i; j<256; j++)
		{
			int r,b,g,v;
			
			r = (pal->red(i) + pal->red(j))/2;
			g = (pal->green(i) + pal->green(j))/2;
			b = (pal->blue(i) + pal->blue(j))/2;
			
			v = pal->find_color(r,g,b);
			
			SmoothTable[i][j] = v;
			SmoothTable[j][i] = v;
		}
}

*/