#include <Slots.h>
#include <ROMDefs.h>
#include <ENET.h>
#include <string.h>
#include <stdio.h>
#include <memory.h>

#define NETBUFFERSIZE 8888
#define ETHERTYPEID 0x8137

int gENETDone;
short ENETVRefNum;
char gMultiCastAddr[6];

unsigned char *pstrcpy(unsigned char *d, unsigned char *s)
{
	unsigned int i = (unsigned char)s[0];

	BlockMove(s,d,i+1);
	
	return d;
}

short Get_ENET_Driver()
{
	SpBlock mySBlk;
	ParamBlockRec myPBRec;
	OSErr err;
	short found, ENETRefNum;
	Str15 EnetStr,Enet0Str;
	char slot,id;
	
	memset(&mySBlk,0,sizeof(mySBlk));
	
	found = 0;
	ENETRefNum = 0;
	mySBlk.spParamData = 1;
	mySBlk.spCategory = catNetwork;
	mySBlk.spCType = typeEtherNet;
	mySBlk.spDrvrSW = 0;
	mySBlk.spDrvrHW = 0;
	mySBlk.spTBMask = 3;
	mySBlk.spSlot = 0;
	mySBlk.spID = 0;
	mySBlk.spExtDev = 0;
	
	do {
		err = SGetTypeSRsrc(&mySBlk);
		if (err == noErr)
		{
			found++;
			slot = mySBlk.spSlot;
			id = mySBlk.spID;
		}
	} while ( err != smNoMoresRsrcs );

	if (found) 
	{
		memset(&myPBRec,0,sizeof(myPBRec));
	
		printf(".ENET\n");
		pstrcpy(EnetStr, "\p.ENET");
		myPBRec.slotDevParam.ioCompletion = 0;
		myPBRec.slotDevParam.ioNamePtr = EnetStr;
		myPBRec.slotDevParam.ioSPermssn = fsCurPerm;
		myPBRec.slotDevParam.ioSFlags = 0;
		myPBRec.slotDevParam.ioSlot = slot;
		myPBRec.slotDevParam.ioID = id;
		err = OpenSlot(&myPBRec, false);
		if (err == noErr)
		{
			ENETVRefNum = myPBRec.slotDevParam.ioVRefNum;
			ENETRefNum = myPBRec.slotDevParam.ioSRefNum;		
		}
	}
	else
	{
		printf(".ENET0\n");
		pstrcpy(Enet0Str,"\p.ENET0");
		err = OpenDriver(Enet0Str, &ENETRefNum);
	}
	
	return ENETRefNum;
}

OSErr Send_ENET( short ENETRefNum )
{
//	BlockMove(
	return noErr;
}

pascal void MyCompProc(EParamBlkPtr thePBPtr)
{
	Ptr aptr;
	OSErr err;
	
	if (thePBPtr->ioResult < noErr)
	{
		if (thePBPtr->ioResult != reqAborted)
		{
			printf("Aiiieee! Completion failed!\n");
		}
		else
		{
			printf("Read aborted\n");
		}
	}
	else
	{
		aptr = thePBPtr->u.EParms1.ePointer;
		printf("Whoopee!\n");
	}
	
	if (!gENETDone)
	{
		err = ERead((EParamBlkPtr)thePBPtr, true);
		if (err != noErr)
			printf("Aiiee! Completion read failed!\n");
	}
}

OSErr Detach_ENET_Packet_Handler( short ENETRefNum )
{
	EParamBlock myPB;
	EParamBlkPtr myEPBPtr;
	Ptr aptr;
	OSErr err;
	
	myEPBPtr = &myPB;
	myPB.u.EParms1.eProtType = ETHERTYPEID;
	myPB.ioRefNum = ENETRefNum;
	err = EDetachPH((EParamBlkPtr)myEPBPtr, false);
	
	if (err != noErr)
	{
		printf("Aieeee! Detach failed!\n");
	}
	gENETDone = 1;

	return err;
}

OSErr Attach_ENET_Packet_Handler( short ENETRefNum )
{
	EParamBlock myPB;
	EParamBlkPtr myEPBPtr;
	Ptr aptr;
	OSErr err;
	
	memset(&myPB,0,sizeof(myPB));
	
	myEPBPtr = &myPB;
	myPB.u.EParms1.eProtType = ETHERTYPEID;
	myPB.u.EParms1.ePointer = 0;
//	myPB.ioCompletion = 0;
//	myPB.ioVRefNum = ENETVRefNum;
	myPB.ioRefNum = ENETRefNum;
//	myPB.csCode = ENetAttachPH;
	err = EAttachPH(&myPB, 0);
	
	if (err != noErr)
	{
		printf("Aieeee! Attach failed!\n");
	}
	else
	{
		aptr = NewPtr(NETBUFFERSIZE);
		myPB.ioCompletion = NewENETCompletionProc(&MyCompProc);
		myPB.u.EParms1.eProtType = ETHERTYPEID;
		myPB.u.EParms1.ePointer = aptr;
		myPB.u.EParms1.eBuffSize = NETBUFFERSIZE;
		myPB.ioRefNum = ENETRefNum;
		
		err = ERead((EParamBlkPtr)myEPBPtr, true);
		
		if (err != noErr)
		{
			printf("Aiiiiieeeee! Read failed!\n");
			Detach_ENET_Packet_Handler(ENETRefNum);
		}
	}
	return err;
}

main()
{
	short eref;
	
	printf("Macs suck!!!!\n\n");
	printf("%d\n",eref = Get_ENET_Driver());

	while (!Button())
		;
		
	gENETDone = 0;
	Attach_ENET_Packet_Handler(eref);
	
	while (!Button())
		;
		
	Detach_ENET_Packet_Handler(eref);
	printf("Ok! Done\n");
	return 1;
}