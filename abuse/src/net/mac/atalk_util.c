/*****************************************************************

	Program:	< ADSP Chat >
	File:		< Atalk.c >
	
	Written by  Pete Helm, Scott Kuechle
	of <Apple Macintosh Developer Technical Support>
	
	modified by Scott Kuechle
	10/92 SRK Converted from Pascal to C
	8/94 SRK Modified to use a queue of parameter
	         blocks.

	Copyright © 1992, 1994 Apple Computer, Inc.
	All rights reserved.
	
*****************************************************************/


/*****************************************************************/
/*  I N C L U D E S
/*****************************************************************/

#include	"ADSP Chat.h"


/********************************************************************
/*  G L O B A L   V A R I A B L E   D E C L A R A T I O N S
/********************************************************************/


ATPPBPtr		gATPPBPtr;			/* the parameter block for GetZoneList call */
XPPParmBlkPtr	gXPBPBPtr;			/* structure for Phase 2 NBP lookups */
short			xppDriverRefNum;
Str255 			gZoneString, gObjStr, gTypeStr;
Str32 			myName;
NamesTableEntry myNTE;
AddrBlock 		theBridgeAddress, ourNetworkAddress;


/*****************************************************************/
/*
/* E X T E R N A L S
/*
/*****************************************************************/

extern SysEnvRec	gMac;				/* set up by Initialize */
extern char 		myLocalSocket;

extern void 		ShowError(short index);
extern void 		CopyPstr(Ptr pSource, Ptr pDest);
extern void 		Terminate();
extern void			Exit(short message);
extern qsort (Ptr base, long n, long size, ProcPtr compare);



/*****************************************************************/
/*
/* R O U T I N E S
/*
/*****************************************************************/

Boolean registerMyName();
void 	removeMyName();
long 	myCompare (Str255 aStr, Str255 bStr);
void 	letsSort (Ptr theBuffPtr, long numZonesGot);
void 	addZonesToBuffer (Ptr LkUpBuffer,
				Ptr BufferForZoneMenu,
				short NumZonesGot,
				short CurrentTotal);
void 	parseItemsAddToMenu (Ptr theBuffPtr,
					MenuHandle zoneMenu,
					short NumZonesGot);;
Boolean zonesPresent();
void 	parseLkupBuffAddToMenu (Ptr theBuffPtr,
						Ptr bigBuffer,
						MenuHandle lookupMenu,
						short NumGot,
						Boolean doObjects);
void 	LookupNames (MenuHandle lookupMenu,
				Boolean doObjects);
void 	BuildZoneListPhase1(MenuHandle zoneMenu);
void 	BuildZoneListPhase2(MenuHandle zoneMenu);
void 	GetZones(MenuHandle ZoneMenu);
void 	getOurZonePhase2();
void 	getOurZonePhase1();
OSErr 	InitAppleTalk();
void 	GetOurZone();


#pragma segment Main
// *****************************************************************
// *	registerMyName
// *
// *	registers our nbp name on the network so we can find other
// *	machines that are running our application.
// *****************************************************************
Boolean registerMyName()
{
	MPPParamBlock pb;
	StringHandle userName;
	

		userName = GetString(-16096);

		myName[0] = 0;
		if (userName != nil)
		{
			if (*userName[0] != 0)
				BlockMove(*userName,&myName,*userName[0]+1);
		}
		

		NBPSetNTE((Ptr)&myNTE, &myName, "\pMoof", "\p*", myLocalSocket);
	
		pb.NBP.interval = 2;
		pb.NBP.count = 3;
		pb.NBP.NBPPtrs.entityPtr = (Ptr)&myNTE;
		pb.NBP.parm.verifyFlag = 1;
		if (PRegisterName(&pb,false) != noErr)
		{
			ShowError(nbpErr);
			return false;
		}
		
		return true;
}

// *****************************************************************
// *	removeMyName
// *
// *	removes our nbp name from the network.
// *****************************************************************
void removeMyName()
{
	MPPParamBlock pb;
	OSErr err;

		pb.NBP.NBPPtrs.entityPtr = (Ptr)&myNTE.nt.entityData;
		
		err = PRemoveName(&pb, false);
}



// *****************************************************************
// *	myCompare
// *
// *	this uses stand International Utilites package for sorting
// *****************************************************************
long myCompare (Str255 aStr, Str255 bStr)
	{
		return ((long)IUCompString(bStr, aStr));
	}


// *****************************************************************
// *	letsSort
// *
// * calls a standard C qSort routine compiled with Pascal calling 
// * conventions the 33 is the standard size of a Str32 type, which
// * is the standard size of an AppleTalk NBP object.
// *****************************************************************
void letsSort (Ptr theBuffPtr, long numZonesGot)
	{
		qsort(theBuffPtr, numZonesGot, 33, &myCompare);
	}

// *****************************************************************
// *	addZonesToBuffer
// *
// *	
// *****************************************************************
void addZonesToBuffer (Ptr LkUpBuffer,
				Ptr BufferForZoneMenu,
				short NumZonesGot,
				short CurrentTotal)
{	
	char curLength;
	long index;
	short i;

		index = 0;
		curLength = 0;
		for (i=CurrentTotal;i<=(NumZonesGot + CurrentTotal) && (i < maxZones);++i)
		{
			curLength = *(LkUpBuffer + index);
			BlockMove((LkUpBuffer + index), (BufferForZoneMenu + (i * 33)), 33);
			index = index + curLength + 1;
		}
}

// *****************************************************************
// *	parseItemsAddToMenu
// *
// *	this routine adds zone name items to our zones popup menu
// *****************************************************************
void parseItemsAddToMenu (Ptr theBuffPtr,
					MenuHandle zoneMenu,
					short NumZonesGot)
{	
	long index;
	Str255 tempString;
	short i;

		letsSort(theBuffPtr, NumZonesGot);
	
		index = 0;
		for (i=0; i<=(NumZonesGot - 1); ++i)
			{
				BlockMove((theBuffPtr + i * 33), &tempString, 33);
					/* initially insert a blank item so meta-characters aren't used */
				AppendMenu(zoneMenu, "\p ");
					/* use SetItem so we can display meta-chars in zone names */
				SetItem(zoneMenu, i + 1, tempString);
	
			}
}

// *****************************************************************
// *	zonesPresent
// *
// * this checks to see if there is a local bridge available
// *****************************************************************
Boolean zonesPresent()
{	
	short myNode,ignore;


		theBridgeAddress.aNode = 0;
	
		theBridgeAddress.aNode = GetBridgeAddress();
		ignore = GetNodeAddress(&myNode, &ourNetworkAddress.aNet);
	
		ourNetworkAddress.aNode = myNode;
		theBridgeAddress.aNet = ourNetworkAddress.aNet;
		
		if (ourNetworkAddress.aNode != 0) 
		{
			theBridgeAddress.aSocket = theBridgeSocket;
			return true;
		}
		else
		{
			CopyPstr("\p*", &gZoneString);
			return false;
		}
}



// *****************************************************************
// *	parseLkupBuffAddToMenu
// *
// *	this routine takes all the names returned from the nbp lookup
// *	and checks to see if the item is already in the list. If not,
// *	the item is added to the appropriate popup menu.
// *****************************************************************
void parseLkupBuffAddToMenu (Ptr NamesReturnedBuffer,		/* lookup buffer where names were returned */
						Ptr NonDuplicateNamesBuffer,		/* check for duplicates then store all names here */
						MenuHandle lookupMenu,
						short NumGot,						/* number of items returned in this lookup */
						Boolean doObjects)					/* flag - are we saving nbp type or object items? */
{	
	Str255 menuString, str;
	short i, j, k;
	EntityName abEntity;
	AddrBlock address;
	OSErr resultCode;
	Boolean weHaveDupe;


		k = 1;
		for (i=1; i<=NumGot; ++i)
		{
				/* get an item from the list */
			resultCode = NBPExtract(NamesReturnedBuffer, NumGot, i, &abEntity, &address);
				/* save off current item */
			if (doObjects)	/* do object strings */
				BlockMove(&abEntity.objStr,&menuString,abEntity.objStr[0]+1);
			else	/* do type strings */
				BlockMove(&abEntity.typeStr,&menuString,abEntity.typeStr[0]+1);

				/* check for duplicates here */
			weHaveDupe = false;
			j = 0;
			do
			{
				j = j + 1;
				BlockMove((NonDuplicateNamesBuffer + (j - 1) * 33), &str, 33);
					/* do we already have this item in the list? */
				if (EqualString(str, menuString, false, true))
					weHaveDupe = true;
			}
			while ((j <= k) && (weHaveDupe == false));

				/* if we dont have a duplicate item and if the item is not our own node (we dont
					support self-send) then add it to our list */
			if ((weHaveDupe == false) && (address != ourNetworkAddress))
			{
				BlockMove(&menuString, (NonDuplicateNamesBuffer + (k - 1) * 33), 33);
				k = k + 1;
			}
		}
		
			/* now put all our new items into the actual menu */
		parseItemsAddToMenu(NonDuplicateNamesBuffer, lookupMenu, k - 1);
}

// *****************************************************************
// *	LookupNames
// *
// *	issues an nbp lookup for the desired object, type and zone
// *	the user has specified with the target machine popup menus.
// *****************************************************************
void LookupNames (MenuHandle lookupMenu,
				Boolean doObjects)			/* flag - do we want nbp type or object items? */
{	
	Str32 NBPObject, NBPType, NBPZone;
	NamesTableEntry lookupEntity;
	Ptr NBPLookupBuffer; 					/* totally gross mondo buffer for returned names */
	Ptr NonDuplicateNamesBuffer;			/* put all non-duplicate into this buffer */
	MPPParamBlock pbLKP;

		NBPLookupBuffer = nil;
		NonDuplicateNamesBuffer = nil;
		
		NBPLookupBuffer = NewPtr(BigLookupBuffer);
		if (NBPLookupBuffer == nil)
			return;

		NonDuplicateNamesBuffer = NewPtr(NameBufSize);
		if (NonDuplicateNamesBuffer == nil)
			goto Exit;

		CopyPstr("\p=", NBPObject);

		if (!doObjects)
			CopyPstr("\p=", NBPType);
		else
			BlockMove(&gTypeStr,&NBPType,gTypeStr[0]+1);

		if (zonesPresent()) 
			BlockMove(&gZoneString,&NBPZone,gZoneString[0]+1);
		else
			CopyPstr("\p*", NBPZone);

		NBPSetEntity((Ptr)&lookupEntity.nt.entityData, NBPObject, NBPType, NBPZone);

		pbLKP.NBP.ioCompletion = nil;
		pbLKP.NBP.interval = 3;
		pbLKP.NBP.count = 3;
		pbLKP.NBPentityPtr = &lookupEntity.nt.entityData;
		pbLKP.NBPretBuffSize = BigLookupBuffer;
		pbLKP.NBPretBuffPtr = NBPLookupBuffer;
		pbLKP.NBPmaxToGet = (BigLookupBuffer/sizeof(NTElement));

		if (PLookupName(&pbLKP, false) == noErr)
			if (pbLKP.NBPnumGotten > 0)
				parseLkupBuffAddToMenu(NBPLookupBuffer, NonDuplicateNamesBuffer, lookupMenu, pbLKP.NBPnumGotten, doObjects);

Exit:
		if (NBPLookupBuffer != nil)
			DisposPtr(NBPLookupBuffer);
			
		if (NonDuplicateNamesBuffer != nil)
			DisposPtr(NonDuplicateNamesBuffer);

}



// *****************************************************************
// *	BuildZoneListPhase1
// *
// *	Create the list of zones on the network. Find a bridge to talk to , if one is
// * 	present, then ask it for zone names. Add the names to the list in the dialog.
// *****************************************************************
void BuildZoneListPhase1(MenuHandle zoneMenu)
{
	BDSElement	dBDS;				/* the BDS for GetZoneList call */
	ATPPBPtr	gATPPBPtr;
	Ptr			gZones,sortBuffer;
	long		tempUserData;
	short		zIndex,zoneCallType;
	Boolean		DontGetMoreZones;
	short		NumZonesGot, totalZones;
	

		
		gATPPBPtr = nil;											/* init some important variables*/
		gZones = nil;
		sortBuffer = nil;
	
		if (zonesPresent() == false)
			return;
	
		gATPPBPtr = (ATPPBPtr)NewPtr(sizeof(ATPParamBlock));
		if (gATPPBPtr == nil)
			return;
			
		gZones = NewPtr(kZonesSize);
		if (gZones == nil)
			goto Exit;
			
		sortBuffer = NewPtr(ZoneBufSize);
		if (sortBuffer == nil)
			goto Exit;

		zoneCallType = kGetZoneList;
		zIndex = 1;
			
		dBDS.buffSize = kZonesSize;									/* set up BDS */
		dBDS.buffPtr = gZones;
		dBDS.dataSize = 0;
		dBDS.userBytes = 0;
	
		gATPPBPtr->ATPatpFlags = 0;

	
		gATPPBPtr->ATPaddrBlock.aNet = theBridgeAddress.aNet;
		gATPPBPtr->ATPaddrBlock.aNode = theBridgeAddress.aNode;		/* get node of bridge */
		gATPPBPtr->ATPaddrBlock.aSocket = kZIPSocket;				/* the socket we want */
		gATPPBPtr->ATPreqLength = 0;
		gATPPBPtr->ATPreqPointer = nil;
		gATPPBPtr->ATPbdsPointer = (Ptr) &dBDS;
		gATPPBPtr->ATPnumOfBuffs = 1;
		gATPPBPtr->ATPtimeOutVal = kATPTimeOutVal;
		gATPPBPtr->ATPretryCount = kATPRetryCount;
	
		NumZonesGot = 0;
		totalZones = 0;
		DontGetMoreZones = false;
	
			/* keep going until none left (and we haven't exceeded our buffer size) */
		while (!DontGetMoreZones && (totalZones <= maxZones)) 
		{
			zIndex += NumZonesGot;			/* index count. 1 for start */

			BlockMove((Ptr) &zoneCallType + 1, (Ptr) &tempUserData, 1L);
			BlockMove((Ptr) &zIndex, (Ptr)&tempUserData + 2, 2L);

			gATPPBPtr->ATPuserData = tempUserData;						/* indicate GetZoneList request */
			
			if (PSendRequest(gATPPBPtr, false) != noErr)		/* send sync request */
				Exit(DrvrErr);
			
			tempUserData = dBDS.userBytes;
			BlockMove((Ptr) &tempUserData, (Ptr) &DontGetMoreZones, 1); /* the highbyte will be nonzero if its the last packet of zones */
			BlockMove((Ptr)&tempUserData + 2, (Ptr) &NumZonesGot, 2);
	
			addZonesToBuffer(dBDS.buffPtr,sortBuffer,NumZonesGot,totalZones);			

			totalZones += NumZonesGot;
		}

		totalZones = (totalZones <= maxZones) ? totalZones : maxZones;
		parseItemsAddToMenu(sortBuffer,zoneMenu,totalZones);

		Exit:
			if (gATPPBPtr != nil)
				DisposePtr((Ptr)gATPPBPtr);
			if (gZones != nil)
				DisposePtr(gZones);
			if (sortBuffer != nil)
				DisposePtr(sortBuffer);

} /* BuildZoneList */

// *****************************************************************
// *	BuildZoneListPhase2
// *
// *	Create the list of zones on the network. Find a bridge to talk to , if one is
// * 	present, then ask it for zone names. Add the names to the list in the dialog.
// *****************************************************************
void BuildZoneListPhase2(MenuHandle zoneMenu)
{
	Ptr			bigBuffer;
	short		TotalZones;
	Ptr			gZones; 	/* the data buffer for GetZoneList call */


		gXPBPBPtr = nil;											/* init some important variables*/
		gZones = nil;
		bigBuffer = nil;
	
	
		if (zonesPresent() == false)
			return;
	
		gXPBPBPtr = (XPPParmBlkPtr)NewPtr(sizeof(XCallParam));
		if (gXPBPBPtr == nil)
			return;
		
		gZones = NewPtr(kZonesSize);
		if (gZones == nil)
			goto Exit;
	
			/* big, ugly mondo buffer to hold complete zone list so we can sort them */
		bigBuffer = NewPtr(ZoneBufSize);
		if (bigBuffer == nil)
			goto Exit;
	
		gXPBPBPtr->XCALL.zipInfoField[0] = 0;	/* ALWAYS 0 on first call.  has state info on subsequent calls */
		gXPBPBPtr->XCALL.zipInfoField[1] = 0;	/* ALWAYS 0 on first call.  has state info on subsequent calls */
		gXPBPBPtr->XCALL.zipLastFlag = 0;
	
		gXPBPBPtr->XCALL.ioRefNum = xppDriverRefNum;
		gXPBPBPtr->XCALL.csCode = xCall;
		gXPBPBPtr->XCALL.xppSubCode = zipGetZoneList;
		gXPBPBPtr->XCALL.xppTimeout = kATPTimeOutVal;
		gXPBPBPtr->XCALL.xppRetry = kATPRetryCount;
		gXPBPBPtr->XCALL.zipBuffPtr = (Ptr) gZones;
	
		TotalZones = 0;
	
		do
		{
			if (PBControl((ParmBlkPtr) gXPBPBPtr, false) != noErr)
				Exit(DrvrErr);

			addZonesToBuffer(gZones,bigBuffer,gXPBPBPtr->XCALL.zipNumZones,TotalZones);
			TotalZones = TotalZones + gXPBPBPtr->XCALL.zipNumZones;						/* find out how many returned */
	
		} while ((gXPBPBPtr->XCALL.zipLastFlag == 0) && (TotalZones <= maxZones));				/*	 keep going until none left */
		
		TotalZones = (TotalZones <= maxZones) ? TotalZones : maxZones;
		parseItemsAddToMenu(bigBuffer,zoneMenu,TotalZones);
	
	Exit:
	
		if (gXPBPBPtr != nil)
			DisposPtr((Ptr)gXPBPBPtr);			/* get rid of pb block */
	
		if (gZones != nil)
			DisposPtr(gZones);					/* and buffer */
	
		if (bigBuffer != nil)
			DisposPtr(bigBuffer);					/* and buffer */

} /* BuildZoneList */

// *****************************************************************
// *	GetZones
// *
// *	checks which version of AppleTalk we are using and then calls
// * 	the appropriate routine for building the zone list.
// *****************************************************************
void GetZones(MenuHandle ZoneMenu)
{
		/* are we using AppleTalk Phase 1 or 2 ? */
	if(gMac.atDrvrVersNum > 52)
		BuildZoneListPhase2(ZoneMenu);			/*	put the stuff into the list */
	else
		BuildZoneListPhase1(ZoneMenu);			/*	put the stuff into the list */
}


#pragma segment Initialize
// *****************************************************************
// *	getOurZonePhase2
// *
// *	gets our local zone using AppleTalk Phase 2 calls
// *****************************************************************
void getOurZonePhase2()
{

	gXPBPBPtr = nil;			/* init some important variables*/
	gZoneString[0] = 0;


	if (zonesPresent() == false)
		return;
	
	gXPBPBPtr = (XPPParmBlkPtr)NewPtr(sizeof(XCallParam));
	if (gXPBPBPtr == nil)
		return;

	gXPBPBPtr->XCALL.zipInfoField[0] = 0;	/* ALWAYS 0 on first call.  has state info on subsequent calls */
	gXPBPBPtr->XCALL.zipInfoField[1] = 0;	/* ALWAYS 0 on first call.  has state info on subsequent calls */
	gXPBPBPtr->XCALL.zipLastFlag = 0;

	gXPBPBPtr->XCALL.ioRefNum = xppDriverRefNum;
	gXPBPBPtr->XCALL.csCode = xCall;
	gXPBPBPtr->XCALL.xppSubCode = zipGetMyZone;
	gXPBPBPtr->XCALL.xppTimeout = kATPTimeOutVal;
	gXPBPBPtr->XCALL.xppRetry = kATPRetryCount;
	gXPBPBPtr->XCALL.zipBuffPtr = (Ptr)&gZoneString;

	if (PBControl((ParmBlkPtr) gXPBPBPtr, false) != noErr)		/* send sync control call */
		Exit(DrvrErr);

	DisposPtr((Ptr)gXPBPBPtr);			/* get rid of pb block */
	
}


// *****************************************************************
// *	getOurZonePhase1
// *
// *	gets our local zone using AppleTalk Phase 1 calls
// *****************************************************************
void getOurZonePhase1()
{	
	BDSElement myZoneBDS;
	ATPParamBlock ZonePB;
	long tempUserData;
	Ptr theBufferPtr;
	char zoneCallType;

		zoneCallType = 7;  /*8 for zone list   7 returns my zone */
		theBufferPtr = nil;
		gZoneString[0] = 0;

		
		if (zonesPresent() == true)
		{
			theBufferPtr = NewPtr(33);
			if (theBufferPtr != nil) 
			{
				myZoneBDS.buffSize = 33;
				myZoneBDS.buffPtr = theBufferPtr;
				myZoneBDS.dataSize = 0;
				myZoneBDS.userBytes = 0;

				ZonePB.ATP.atpFlags = 0;
				ZonePB.ATP.ioCompletion = nil;

				ZonePB.ATP.userData = 0;/*  ATP user data  */
				ZonePB.ATP.addrBlock = theBridgeAddress;
				ZonePB.ATP.reqLength = 0;
				ZonePB.ATP.reqPointer = nil;
				ZonePB.ATP.bdsPointer = (Ptr)&myZoneBDS;
				ZonePB.ATPnumOfBuffs = 1;
				ZonePB.ATPtimeOutVal = 2;
				ZonePB.ATPretryCount = 3;

				/* 0 this out so bottom three bytes are 0 */
				tempUserData = 0;
				BlockMove(&zoneCallType, (Ptr)&tempUserData, 1);

				ZonePB.ATP.userData = tempUserData;

				gZoneString[0] = 0;
				if (PSendRequest(&ZonePB, false) == noErr) 
				{
					BlockMove(myZoneBDS.buffPtr, &gZoneString, 33);
				}
				DisposPtr(theBufferPtr);
			}
		}
}


// *****************************************************************
// *	InitAppleTalk
// *
// *	opens the appropriate AppleTalk drivers
// *****************************************************************
OSErr InitAppleTalk()
{
	OSErr err;
	short ref;
	
		if (gMac.atDrvrVersNum > 52)
		{
			err = OpenDriver("\p.MPP",&ref);
			if (err != noErr)
				return err;
	
			err = OpenDriver("\p.XPP",&xppDriverRefNum);
		}
		else
			err = MPPOpen();
	
		return err;
	
}

// *****************************************************************
// *	GetOurZone
// *
// *	checks which version of AppleTalk we are using and then calls
// *	the appropriate phase 1 or phase 2 call to get the local zone.
// *****************************************************************
void GetOurZone()
{
	if(gMac.atDrvrVersNum > 52)
		getOurZonePhase2();									/*	put the stuff into the list */
	else
		getOurZonePhase1();									/*	put the stuff into the list */

}
