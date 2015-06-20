/*****************************************************************

	Program:	< ADSP Chat >
	File:		< ADSP Char.h >
	
	Written by  Pete Helm, Scott Kuechle
	of <Apple Macintosh Developer Technical Support>
	
	modified by Scott Kuechle
	10/92 SRK Converted from Pascal to C
	8/94 SRK Modified to use a queue of parameter
	         blocks.

	Copyright © 1992, 1994 Apple Computer, Inc.
	All rights reserved.
	
*****************************************************************/


#include	<types.h>
#include	<quickdraw.h>
#include	<toolutils.h>
#include	<fonts.h>
#include	<events.h>
#include	<windows.h>
#include	<dialogs.h>
#include	<menus.h>
#include	<desk.h>
#include	<textedit.h>
#include	<scrap.h>
#include	<segload.h>
#include	<osevents.h>
#include	<files.h>
#include	<devices.h>
#include	<memory.h>
#include	<appletalk.h>
#include	<lists.h>
#include	<SysEqu.h>
#include	<Script.h>
#include	<CursorCtl.h>
#include	<Resources.h>
#include	<ADSP.h>
#include	<Packages.h>
#include	<String.h>
#include	<Strings.h>
#include	<Traps.h>
#include	<GestaltEqu.h>


/*****************************************************************/
/*
/* D A T A  S T R U C T U R E S
/*
/*****************************************************************/

typedef struct myDSPParamBlock
{
	long 			myA5;			/* save our A5 here */
	ProcPtr			ourCompletion;
	DSPParamBlock 	u;
}myDSPParamBlock;

typedef myDSPParamBlock myDSPParamBlock;
typedef myDSPParamBlock *myDSPParamBlockPtr;


typedef short SICN[16];
typedef SICN *SICNList;
typedef SICNList *SICNHand;


/*****************************************************************/
/*
/* C O N S T A N T S
/*
/*****************************************************************/

/* 1.01 - kMinHeap - This is the minimum result from the following
	 equation:
			
			ORD(GetApplLimit) - ORD(ApplicZone)
			
	 for the application to run. It will insure that enough memory will
	 be around for reasonable-sized scraps, FKEYs, etc. to exist with the
	 application, and still give the application some 'breathing room'.
	 To derive this number, we ran under a MultiFinder partition that was
	 our requested minimum size, as given in the 'SIZE' resource. */
	 
#define		kMinHeap				(29 * 1024)
#define		kMinSpace				(20 * 1024)

#define		_WaitNextEvent			0xA860
#define		_Unimplemented			0xA89F

/* For System 7.0 */
#define 	_Gestalt 				0xA1AD

#define		appleID					128 			/* This is a resource ID */
#define		fileID 					129 			/* ditto */
#define		editID 					130 			/* ditto */


#define 	extremeNeg				-32768
#define 	extremePos				32767 - 1			/* required for old region bug */


	/*constants for positioning the default item within its box*/
#define		osEvent					app4Evt		/* event used by MultiFinder */
#define		resumeMask				1			/* bit of message field for resume vs. suspend */
#define		sysEnvironsVersion		1

#define		kZonesSize				578			/* size of buffer for zone names */
#define		kGetZoneList			8			/* the Zone Information Protocol socket */
#define		kZIPSocket				6			/* the Zone Information Protocol socket */
#define		kMoreZones				0xFF000000 	/* mask to see if more zones to come */
#define		kZoneCount				0x0000FFFF 	/* mask to count zones in buffer */
#define		kHilite					1			/* hilite value for button control */
#define		kDeHilite				0			/* dehilite value for button control */
#define 	theBridgeSocket 		0x6

/* 
 *	constants for window activation 
 */
#define 	kActivateWindow			1
#define 	kDeactivateWindow		0

/* 
 *	A few char codes…
 */
#define 	kEnterKey				(char)3			/* enter key */
#define 	kReturnKey				(char)13		/* return key */
	
/* size of our adsp send/receive queues */
#define		Qsize					300

#define		kATPTimeOutVal			3			/* re-try ATP SendRequest every 3 seconds */
#define		kATPRetryCount			5			/* for five times */

#define		maxZones				250				/* max. zones we can save in our buffer */
#define		ZoneBufSize				maxZones * 33	/* size of our ugly, mondo zone buffer */
#define		BigLookupBuffer			10000			/* another big, ugly buffer - for nbp lookups */
#define		maxNames				250				/* max. names we can save in our buffer */
#define		NameBufSize				maxNames * 33	/* size of our ugly, mondo names buffer */
#define		kLookupBufSize			100

#define 	rMenuBar 				128				/*application's menu bar*/

#define		ZoneMenuID				220				/* zone popup menu */
#define		TypeMenuID				221				/* object type popup menu */
#define		ObjectMenuID			222				/* object popup menu */

#define 	rAboutDialog  			128				/*about alert*/
#define 	rDialog  				129				/*application's window*/
#define 	rErrorDialog  			130				/*error alert window*/

	/* The following constants are used to identify menus and their items. The menu IDs*/
	/* have an "m" prefix and the item numbers within each menu have an "i" prefix.*/

#define 	mApple  				128					/*Apple menu*/
#define 	iAbout  				1

#define 	mFile  					129					/*File menu*/
#define 	iClose  				1
#define 	iQuit  					2

#define 	mEdit  					130					/*Edit menu*/
#define 	iUndo  					1
#define 	iCut  					3
#define 	iCopy  					4
#define 	iPaste  				5
#define 	iClear  				6


#define 	kStandardTriSICN  		-3990

/* dialog items */

#define 	kobjectItemID 				1		/* popup menu */
#define 	ktypeItemID  				2		/* popup menu */
#define 	kzoneItemID  				3		/* popup menu */

#define 	kRemoteMacsTimeBorderID		4
#define 	kRemoteMacsTimeID 			5
#define 	kConnectButtonID  			6		/* connect button */
#define 	kQuitButtonID 				7

#define 	kIncomingMessageBorderID	8
#define		kIncomingTextID 			9

#define 	kOutgoingMessageBorderID	10
#define 	kOutgoingTextID 			11
#define 	kConnectedToString			12

#define 	kMoofFilterCheckBox			13
#define 	kPopupBorderID				14


#define 	kHintText					15
#define 	kRemoteMacsNameID 			16

#define 	kConnectStatusBorder		20
#define 	kConnectionStatusString		21

/*  our attention codes */

#define 	kDisplayTime				5
#define 	kDisplayTheirName			6

#define 	maxQElements				30

/* our error message codes	*/

#define 	atalkErr					1
#define 	memErr						2
#define 	menuErr						3
#define 	nbpErr						4
#define 	noTargetErr					5

#define 	noConnErr					7
#define 	writeNotDoneErr				8
#define 	badROMsErr					9
#define 	heapErr						10
#define 	noMemErr					11
#define 	DrvrErr						12
#define 	ListenErr					13
#define 	dspInitErr					14
#define 	dspOpenErr					15
#define 	dspRemoveErr				16


/*****************************************************************/
/*
/* R O U T I N E S
/*
/*****************************************************************/

Boolean doADSPinit(unsigned char *localSocket,
					short *ccbRefNum,
					TPCCB ccbPtr);
Boolean setUpADSPbuffers();
void 	initializeADSP();
void 	FlashReadEnd();
void 	FlashReadStart();
void 	changeConnectButtonState();
void 	DisplayTime();
pascal 	void saveThatA5();
pascal 	void GetMyTRCCBA5();
pascal 	void restoreThatA5();
void 	DisplayTheirName();
void 	DisplayIncomingText(DSPPBPtr dspPBPtr);
void 	removeADSPBuffers();
void 	removeConnectionEnd(short ccbRefNum);
Boolean WaitForConnectionRequest();
void 	CloseConnection();
pascal 	void sendAttnMsgCompRoutine(DSPPBPtr dspPBPtr);
void 	sendAttnMsg(DSPPBPtr dspPBPtr,
				 short buffSize,
				 Ptr attnData,
				 short msg,
				 short ccbRefNum);
pascal 	void adspOpenRqstCompletionRtn(DSPPBPtr dspPBPtr);
void 	sendAnOpenConnReq (DSPPBPtr dspPBPtr,
						AddrBlock theirAddress,
						short ccbRefNum);
void 	connectToPeer();
pascal 	void readIncomingComp(DSPPBPtr dspPBPtr);
void 	readIncoming(DSPPBPtr dspPBPtr,
					short ccbRefNum);
pascal 	void writeComp(DSPPBPtr dspPBPtr);
void 	writeOutgoing(DSPPBPtr dspPBPtr,
					short ccbRefNum,
					Ptr dataPtr,
					short reqCount);
void 	sendTime();
void 	sendMyName();
void 	signalConnect();
void 	checkAttnMsgs();
void 	DoConnectionEvents();
void 	ADSPLoop();
void 	DoPassiveOpen(DSPPBPtr dspPBPtr,
					short ccbRefNum);
pascal 	void OpenPassiveCompletionRtn(DSPPBPtr dspPBPtr);
void 	CloseTheConnection(short ccbRefNum);
OSErr 	InitQueues();
DSPPBPtr GetQElement(QHdrPtr qHdrPtr);
void 	CheckCompletedReads();
void 	CheckDoneQueue();
void 	ShowADSPError(DSPPBPtr dspPBPtr);
void	SetUpADSPError(OSErr err, StringPtr displayStr);
void 	SetOurCompletionRoutine(ProcPtr procPtr,
							DSPPBPtr dspPBPtr);


void	Terminate();
void 	DialogEditing (short menuItem);
void	DoActivate (WindowPtr window, Boolean becomingActive);
void	CheckEnvirons();
void	drawPopUpTri (WindowPtr whichWindow, Rect r);
void 	CopyPstr(Ptr pSource, Ptr pDest);
void 	PStrCat(Ptr sourceStr, Ptr destinationStr);
void	DisplayCurrentStatus(Ptr displayStr);
void 	ShowError(short index);
void 	FatalError(error);
Boolean IsAppWindow(window);
Boolean IsDAWindow(WindowPtr window);
void 	DoCloseWindow (WindowPtr window);
void 	HiliteConnectButton (short mode);
void 	outlinePopUpMenus (WindowPtr whichWindow, Rect r, Str255 itemString);
pascal 	void UpdateUserItems (WindowPtr whichWindow, short theItem);
void 	DoModeless (DialogPtr whichDialog, short whichItem);
void 	setEachUserItem (short item);
void 	DoMenuCommand (long menuResult);
void 	AdjustMenus();
void 	DisposeQueueMemory(QHdrPtr qHdrPtr);
void 	Exit(short message);
void 	DoIdleProc();
void 	AdjustCursor (Point mouse, RgnHandle region);
void 	UpdateItemBorder (short item, Rect r);
void 	PlotSICN (Rect theRect,SICNHand theSICN, short theIndex);
void 	DoEvent (EventRecord event);
void 	EventLoop();
Boolean TrapAvailable(tNumber,tType);
void 	SetupUserItems();
void 	Initialize();
void 	zeroOutStrings();
