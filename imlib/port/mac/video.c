#include <QDOffscreen.h>
#include <Palettes.h>
#include <Displays.h>
#include <Video.h>
#include <Menus.h>

#include "video.hpp"

#include "filter.hpp"
#include "globals.hpp"
#include "system.h"
#include "dos.h"
#include "macs.hpp"
#include "bitmap.h"
#include "image.hpp"
#include "jmalloc.hpp"
#include <GUSI.h>

#include "RequestVideo.h"

// Resource id for crack window
#define 	WIND_CRACK 	1000

#define PAL_DIRECT    // direct palette routines - yes!
#define VIDEO_DIRECT  // direct video routines - yes!

unsigned char current_background;
extern unsigned int xres,yres;

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
Rect *gRect;

#ifdef VIDEO_DIRECT
void (*SpeedCopyBits)(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy);
void SpeedCopyBits1(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy);
void SpeedCopyBits2(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy);
#endif

struct DisplayModeRequest
{
	// Returned values
	unsigned short csMode;
	unsigned long csData;

	// Provided values
	long DesiredWidth;
	long DesiredHeight;
	long DesiredDepth;
};

pascal void DisplayModeCallback(void* userData, DMListIndexType,
	DMDisplayModeListEntryPtr pModeInfo)
{
	DisplayModeRequest *pRequest = (DisplayModeRequest*)userData;

	// Get timing info and make sure this is an OK display mode
	VDTimingInfoRec TimingInfo = *(pModeInfo->displayModeTimingInfo);
	if (TimingInfo.csTimingFlags & 1<<kModeValid)
	{
		// How many modes are being enumerated here?
		unsigned long DepthCount =
			pModeInfo->displayModeDepthBlockInfo->depthBlockCount;

		// Filter through each of the modes provided here
		VDSwitchInfoRec *pSwitchInfo;
		VPBlock *pVPBlockInfo;
		for (short Count = 0; Count < DepthCount; ++Count)
		{
			// This provides the csMode and csData information
			pSwitchInfo =
				pModeInfo->displayModeDepthBlockInfo->
				depthVPBlock[Count].depthSwitchInfo;

			// This tells us the resolution and pixel depth
			pVPBlockInfo =
				pModeInfo->displayModeDepthBlockInfo->
				depthVPBlock[Count].depthVPBlock;

			if (pVPBlockInfo->vpPixelSize == pRequest->DesiredDepth &&
				pVPBlockInfo->vpBounds.right == pRequest->DesiredWidth &&
				pVPBlockInfo->vpBounds.bottom == pRequest->DesiredHeight)
			{
				// Found a mode that matches the request!
				pRequest->csMode = pSwitchInfo->csMode;
				pRequest->csData = pSwitchInfo->csData;
			}
		}
	}
}

class CMacStartup {
public:
	CMacStartup()
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

		GUSIDefaultSetup();

    MaxApplZone();
    MoreMasters();

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
			fprintf(stderr,"Can't get current video mode\n");
			exit(1);
		}

		// Get current setting
		originalRec.screenDevice = requestRec.screenDevice;		// this screen
		RVGetCurrentVideoSetting(&originalRec);
		
		RVSetVideoRequest (&requestRec);
//		if (noErr != RVConfirmVideoRequest (&requestRec))
//			RVSetVideoRequest (&originalRec);
		
		PixMapHandle PMH;

		gd = requestRec.screenDevice;
		PMH = (*gd)->gdPMap;
		LockPixels(PMH);
		gVideoRowBytes = (*PMH)->rowBytes & 0x3FFF;
		gVideoMem = GetPixBaseAddr(PMH);
		gRect = &(*gd)->gdRect;

#if 0

		for (int y=gRect->top; y<gRect->bottom; y++)
			for (int x=gRect->left; x<gRect->right; x++)
				*(gVideoMem + (y*gVideoRowBytes+x)) = 0;
#endif

		// set color table for 8 bit mode
		
#ifdef PAL_DIRECT
		MacCT = (**((**gd).gdPMap)).pmTable;
#else
		MacCT = GetCTable(8);
		MacPal = NewPalette(256,nil,pmTolerant,0);
		(**MacCT).ctSeed = GetCTSeed();
#endif
		
		PixMult = 1;
	}
	
	~CMacStartup()
	{
		FlushEvents(everyEvent, 0);
		RVSetVideoRequest (&originalRec);
		RVSetVideoAsScreenPrefs ();
	}
} MacStartup;

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

void HideMenu()
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

void RestoreMenu()
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

void set_mode(int mode, int argc, char **argv)
{
	Rect CurBounds;
	GrafPtr savePort;

#ifdef VIDEO_DIRECT
  // select copy routine
  switch (PixMult)
  {
  	case 1:  SpeedCopyBits = &SpeedCopyBits1; break;
  	case 2:  SpeedCopyBits = &SpeedCopyBits2; break;
  	default:
  		fprintf(stderr,"Aiiieee!  Can't set direct video copier.. it's gonna blow!\n");
  		break;
  }
#endif

	HideMenu();	

	Bounds = *gRect;

	backwin = (CWindowPtr)NewCWindow(nil, &Bounds, "\p", TRUE, 2, (WindowPtr)-1L, FALSE, 0);

	Bounds.left = (gRect->left+gRect->right)/2 - xres/2 * PixMult;
	Bounds.right = (gRect->left+gRect->right)/2 + xres/2 * PixMult;
	Bounds.top = (gRect->top+gRect->bottom)/2 - yres/2 * PixMult;
	Bounds.bottom = (gRect->top+gRect->bottom)/2 + yres/2 * PixMult;
	mainwin = (CWindowPtr)NewCWindow(nil, &Bounds, "\p", TRUE, 2, (WindowPtr)-1L, FALSE, 0);
	SetGWorld((GWorldPtr)mainwin,gd);
	CurBounds = mainwin->portRect;

	xres = (Bounds.right - Bounds.left)/PixMult;
	yres = (Bounds.bottom - Bounds.top)/PixMult;

	// save old palette
	saved_pal = (**((**gd).gdPMap)).pmTable;
	HandToHand((Handle*)&saved_pal);
	
	// erase cursor
	ShieldCursor(&CurBounds,topLeft((**(mainwin->portPixMap)).bounds));

	// create screen memory
  screen=new image(xres,yres,NULL,2);  

	// clear screen
  screen->clear();
  update_dirty(screen);
  
}

void close_graphics()
{
  delete screen;

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
		fprintf(stderr,"aieee! palette problem!\n");
#else
	// restore palette
	MacPal = NewPalette((**saved_pal).ctSize,saved_pal,pmTolerant,0);
	NSetPalette((WindowPtr)mainwin,MacPal,pmAllUpdates);
	ActivatePalette((WindowPtr)mainwin);
#endif
	// restore cursor
	ShowCursor();

	CloseWindow((WindowPtr)mainwin);
	CloseWindow((WindowPtr)backwin);
	
	RestoreMenu();
}

#ifdef VIDEO_DIRECT

void SpeedCopyBits1(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy)
{
	int x,y;
	unsigned char *p,*q,*pp,*qq;
	unsigned long srclen = im->width();
	
	p = (unsigned char *)im->scan_line(sy1) + sx1;
	q = (unsigned char *)gVideoMem + gVideoRowBytes*(dy+Bounds.top) + (dx+Bounds.left);
	for (y=sy1; y<=sy2; y++)
	{
		pp = p;
		qq = q;
		for (x=sx1; x<=sx2; x++)
			*(qq++) = *(pp++);
		p += srclen;
		q += gVideoRowBytes;
	}
}

void SpeedCopyBits2(image *im, int sx1,int sy1, int sx2,int sy2, int dx,int dy)
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
			(*SpeedCopyBits)(im, dr->dx1,dr->dy1, dr->dx2,dr->dy2, 
										xoff + dr->dx1, yoff + dr->dy1);
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

void palette::load()
{
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
		fprintf(stderr,"aieee! palette problem!\n");
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

