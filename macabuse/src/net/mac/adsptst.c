#include <AppleTalk.h>
#include <ADSP.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

int eprintf(char *form, ...)
{
	va_list arg;
	
	va_start(arg,form);
	vfprintf(stderr,form,arg);
	va_end(arg);
	
	exit(0);

	return 0;
}

#define assert(x,y) ((x)? 0 : eprintf("Line %d: %s, error\n",__LINE__, y))
#define macerr(x,y) ((x)? eprintf("Line %d: %s, error %d\n",__LINE__, y, x) : 0)

unsigned char *pstrcpy(unsigned char *d, unsigned char *s)
{
	unsigned int i = (unsigned char)s[0];

	BlockMove(s,d,i+1);
	
	return d;
}

#define	qSize	600	// queue space

unsigned char	*dspSendQPtr;
unsigned char	*dspRecvQPtr;
unsigned char	*dspAttnBufPtr;
unsigned char	*myData2ReadPtr;
unsigned char	*myData2WritePtr;
unsigned char	*myAttnMsgPtr;

DSPPBPtr	myDSPPBPtr;
MPPPBPtr	myMPPPBPtr;

short	drvrRefNum;
short	mppRef;
short	connRefNum;

Boolean	gReceivedAnEvent;
TRCCB	gDspCCB;

void DoWrite(char *buff, short size)
{
	// the connection with the chosen socket is open, so now send to
	//  the send queue exactly myDataSize number of bytes

	// set up dspWrite parameters

	fprintf(stderr,"Writing Data\n");
	
	myDSPPBPtr->ioCRefNum = drvrRefNum;	// ADSP driver ref num
	myDSPPBPtr->csCode = dspWrite;
	myDSPPBPtr->ccbRefNum = connRefNum;	// connection ref num
	myDSPPBPtr->u.ioParams.reqCount = size;
							// write this numberof bytes
	myDSPPBPtr->u.ioParams.dataPtr = buff;	// pointer to
										// send queue
	myDSPPBPtr->u.ioParams.eom = 1;	// 1 means last byte is logical
								// end-of-message
	myDSPPBPtr->u.ioParams.flush = 1;	// 1 means send data now

	macerr(PBControl((ParmBlkPtr) myDSPPBPtr, FALSE), "Write error");
								// send data to the remote connection
}

void DoRead(char *buff, short size)
{
	// Now read from the receive queue exactly myDataSize number of bytes.

	fprintf(stderr,"Reading Data\n");
	
	// set up dspRead parameters
	myDSPPBPtr->ioCRefNum = drvrRefNum;	// ADSP driver ref num
	myDSPPBPtr->csCode = dspRead;
	myDSPPBPtr->ccbRefNum = connRefNum;	// connection ref num
	myDSPPBPtr->u.ioParams.reqCount = size;
								// read this number of bytes
	myDSPPBPtr->u.ioParams.dataPtr = (Ptr)buff;	// pointer to
										// read buffer

	macerr(PBControl((ParmBlkPtr) myDSPPBPtr, FALSE), "read error");
							// read data from the remote  connection
}

int PickSocket(AddrBlock *addr)
{
	EntityName	entity;
	char Buff[105];

	NBPSetEntity((Ptr) &entity, "\p=", "\pAbuseServer", "\p*");
	
	myMPPPBPtr->NBPinterval = 7;// retransmit every 7*8=56 ticks
	myMPPPBPtr->NBPcount = 3;	// and retry 3 times
	myMPPPBPtr->NBPentityPtr = (Ptr) &entity;
	myMPPPBPtr->NBPretBuffPtr = (Ptr) Buff;
	myMPPPBPtr->NBPretBuffSize = sizeof(Buff);
	myMPPPBPtr->NBPmaxToGet = 1;

	macerr(PLookupName(myMPPPBPtr,0), "Lookup error");
		
	macerr(NBPExtract((Ptr)Buff,myMPPPBPtr->NBPnumGotten,1,&entity,addr),"Extraction error");

	for (int i=0; i<entity.objStr[0]; i++)
		Buff[i] = entity.objStr[1+i];
	Buff[entity.objStr[0]] = 0;
	
	fprintf(stderr,"Found [%s]\n",Buff);

	return (myMPPPBPtr->NBPnumGotten == 1);
}

void main ()
{
	NamesTableEntry	myNTEName;
	AddrBlock	myAddrBlk;
	short	myAttnCode;
	Byte	tempFlag;
	short	tempCFlag;

	fprintf(stderr,"Open MPP\n");
	macerr(OpenDriver("\p.MPP", &mppRef), "MPP Open Error");	// open .MPP driver
	fprintf(stderr,"Open DSP\n");
	macerr(OpenDriver("\p.DSP", &drvrRefNum), "DSP Open Error");	// open .DSP driver

	// allocate memory for data buffers
	fprintf(stderr,"Alloc structures\n");

	dspSendQPtr = (unsigned char *) NewPtr(qSize);	// ADSP use only
	dspRecvQPtr = (unsigned char *) NewPtr(qSize);	// ADSP use only
	dspAttnBufPtr = (unsigned char *) NewPtr(attnBufSize);// ADSP use only
	myAttnMsgPtr = (unsigned char *) NewPtr(128);
	myDSPPBPtr = (DSPPBPtr) NewPtr(sizeof(DSPParamBlock));
	myMPPPBPtr = (MPPPBPtr) NewPtr(sizeof(MPPParamBlock));

	// set up dspInit parameters

	fprintf(stderr,"DSP init\n");

	myDSPPBPtr->ioCRefNum = drvrRefNum;	// ADSP driver ref num
	myDSPPBPtr->csCode = dspInit;
	myDSPPBPtr->u.initParams.ccbPtr = (TPCCB) &gDspCCB;	// ptr to CCB

	// don't handle exceptions for now
	myDSPPBPtr->u.initParams.userRoutine = 0;
//	myDSPPBPtr->u.initParams.userRoutine = &myConnectionEvtUserRoutine;

	myDSPPBPtr->u.initParams.sendQSize = qSize;	// size of send queue
	myDSPPBPtr->u.initParams.sendQueue = dspSendQPtr;// send-queue buffer
	myDSPPBPtr->u.initParams.recvQSize = qSize;	// size of receive queue
	myDSPPBPtr->u.initParams.recvQueue = dspRecvQPtr;	// receive-queue
														//buffer
	myDSPPBPtr->u.initParams.attnPtr = dspAttnBufPtr;	// receive-
														//attention buffer
	myDSPPBPtr->u.initParams.localSocket = 0;	// let ADSP assign socket

	gReceivedAnEvent = FALSE;
//	gDspCCB.myA5 = SetCurrentA5();	// save A5 for the user routine

	// establish a connection end

	macerr(PBControl((ParmBlkPtr) myDSPPBPtr, FALSE), "Initialization Error");

	connRefNum = myDSPPBPtr->ccbRefNum;	// save CCB ref num for later

	short node,net;
	Str255 name;
	
	fprintf(stderr,"Get Address...");	
	macerr(GetNodeAddress(&node,&net),"Can't get node address");
	sprintf((char*)&name[1],"Abuse@%d:%d:%d",net,node,myDSPPBPtr->u.initParams.localSocket);
	name[0] = strlen((char*)&name[1]);

	fprintf(stderr,"I'm %s\n",&name[1]);
	
	NBPSetNTE((Ptr) &myNTEName, "\pA1", "\pAbuseServer", "\p*",
			myDSPPBPtr->u.initParams.localSocket);
			// set up NBP names table entry

	// set up PRegisterName parameters

	fprintf(stderr,"Registering Name...");

	myMPPPBPtr->NBP.interval = 7;// retransmit every 7*8=56 ticks
	myMPPPBPtr->NBP.count = 3;	// and retry 3 times
	myMPPPBPtr->NBP.nbpPtrs.entityPtr = (Ptr) &myNTEName;
								// name to register
	myMPPPBPtr->NBP.parm.verifyFlag = 1;	// verify this name

	if (PRegisterName(myMPPPBPtr, FALSE) == noErr)
	{
		fprintf(stderr,"Ok.\n");
		fprintf(stderr,"Waiting for connection...");
				
		// open a connection passive
	
		// set up dspOpen parameters
	
		myDSPPBPtr->ioCRefNum = drvrRefNum;	// ADSP driver ref num
		myDSPPBPtr->csCode = dspOpen;
		myDSPPBPtr->ccbRefNum = connRefNum;	// connection ref num
	
		myDSPPBPtr->u.openParams.filterAddress.aNet = 0;
		myDSPPBPtr->u.openParams.filterAddress.aNode = 0;
		myDSPPBPtr->u.openParams.filterAddress.aSocket = 0;
							// address filter, don't filter
	
		myDSPPBPtr->u.openParams.ocMode = ocPassive; 	// open connection
											//mode
		myDSPPBPtr->u.openParams.ocInterval = 7; 	// use default retry interval
		myDSPPBPtr->u.openParams.ocMaximum = 3;
										// use default retry maximum
	
		macerr(PBControl((ParmBlkPtr)myDSPPBPtr, FALSE), "dspOpen error");
									// open a connection
									
		myAddrBlk = myDSPPBPtr->u.openParams.remoteAddress;
		fprintf(stderr,"Found %d:%d:%d\n",myAddrBlk.aNet,myAddrBlk.aNode,myAddrBlk.aSocket);
	}
	else
	{
		fprintf(stderr,"Failed.\n");
		fprintf(stderr,"Searching for connection...");
				
	// set up dspOptions parameters

//	myDSPPBPtr->ioCRefNum = drvrRefNum;	// ADSP driver ref num
//	myDSPPBPtr->csCode = dspOptions;
//	myDSPPBPtr->ccbRefNum = connRefNum;	// connection ref num
//	myDSPPBPtr->u.optionParams.sendBlocking = blockFact;
								// quantum for data packet
//	myDSPPBPtr->u.optionParams.badSeqMax = 0;	// use default
//	myDSPPBPtr->u.optionParams.useCheckSum = 0;	// don't calculate
													//checksum

//	macerr(PBControl((ParmBlkPtr) myDSPPBPtr, FALSE), "Set Options error");
										// set options

		// routine using the PLookupName function to pick a socket
		// that will be used to establish an open connection

		assert(PickSocket(&myAddrBlk),"Can't find socket");

		fprintf(stderr,"Found %d:%d:%d\n",myAddrBlk.aNet,myAddrBlk.aNode,myAddrBlk.aSocket);

		// open a connection with the chosen socket
	
		// set up dspOpen parameters

		fprintf(stderr,"Opening Socket\n");
	
		myDSPPBPtr->ioCRefNum = drvrRefNum;	// ADSP driver ref num
		myDSPPBPtr->csCode = dspOpen;
		myDSPPBPtr->ccbRefNum = connRefNum;	// connection ref num
		myDSPPBPtr->u.openParams.remoteAddress = myAddrBlk;
						//address of remote socket from PLookupName function
	
		myDSPPBPtr->u.openParams.filterAddress = myAddrBlk;
							// address filter, specified socket address only
	
		myDSPPBPtr->u.openParams.ocMode = ocRequest; 	// open connection
											//mode
		myDSPPBPtr->u.openParams.ocInterval = 0; 	// use default retry interval
		myDSPPBPtr->u.openParams.ocMaximum = 0;
										// use default retry maximum
	
		macerr(PBControl((ParmBlkPtr)myDSPPBPtr, FALSE), "dspOpen error");
									// open a connection
	}
	
	char st[80];
	
	DoWrite("Hello, other!",14);
	
	DoRead(st,sizeof(st));

	fprintf(stderr,"Got [%s]\n",st);

	// we're done with the connection, so remove it

	// set up dspRemove parameters

	fprintf(stderr,"Removing Connection\n");

	myDSPPBPtr->ioCRefNum = drvrRefNum;	// ADSP driver ref num
	myDSPPBPtr->csCode = dspRemove;
	myDSPPBPtr->ccbRefNum = connRefNum;	// connection ref num
	myDSPPBPtr->u.closeParams.abort = 0;	// don't close until everything
									//is sent and received

	macerr(PBControl((ParmBlkPtr) myDSPPBPtr, FALSE), "remove error");
									// close and remove the connection

	// you're done with this connection, so give back the memory
	DisposPtr((Ptr) dspSendQPtr);
	DisposPtr((Ptr) dspRecvQPtr);
	DisposPtr((Ptr) dspAttnBufPtr);
	DisposPtr((Ptr) myData2ReadPtr);
	DisposPtr((Ptr) myData2WritePtr);
	DisposPtr((Ptr) myAttnMsgPtr);
	DisposPtr((Ptr) myDSPPBPtr);
	DisposPtr((Ptr) myMPPPBPtr);

	fprintf(stderr,"Done.\n");
}	// MyADSP

