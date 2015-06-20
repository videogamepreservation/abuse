#include <AppleTalk.h>
#include <ADSP.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

DDPSocketListenerUPP DDPListener;

#ifdef __POWERPC__
UniversalProcPtr InitDDPListener;
#else
extern asm void DDPEntry(char *buffer);
#endif

#define PACKET_TYPE 0xaa

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

struct sPacket
{
	long Count;
	AddrBlock Addr;
	long Data;
};
struct sDDPBuff
{
	struct sPacket *Start,*End,*Head,*Tail;
	long Count;
	long Data;
};

NamesTableEntry	myNTEName;
MPPParamBlock p;
char buffer[602 + 20 + 602*16];
ATDDPRec ddp;

unsigned char socket = 0;
short	mppRef;

void DoWrite(AddrBlock addr,char *buff, short size)
{
	char wds[14];
	char header[20];

	// the connection with the chosen socket is open, so now send to
	//  the send queue exactly myDataSize number of bytes

	// set up WriteDDP parameters

	fprintf(stderr,"Writing Data\n");
	
	BuildDDPwds((Ptr)wds, (Ptr)header, (Ptr)buff, addr, PACKET_TYPE,size);

	p.DDP.ioCompletion = nil;
	p.DDP.socket = socket;
	p.DDP.checksumFlag = 0;
	p.DDP.u.wdsPointer = (Ptr) &wds;

	macerr(PWriteDDP(&p, FALSE), "Write error");
								// send data to the remote connection
}

void DoRead(AddrBlock *addr, char *buff, short size)
{
	// Now read from the receive queue exactly myDataSize number of bytes.

	fprintf(stderr,"Reading Data\n");

	struct sDDPBuff *db = (struct sDDPBuff *)(&buffer[602]);
	while ( db->Count == 0) ;
	
	db->Count--;

	struct sPacket *pk = db->Tail;
	char *b = (char*)&pk->Data;

	for (int i=0; i<pk->Count && i<size;i++)
		buff[i] = b[i];

	*addr = pk->Addr;

	pk = (struct sPacket *)((char *)pk + 602);
	if (pk>=db->End)
		pk = db->Start;
	db->Tail = pk;
}

int PickSocket(AddrBlock *addr)
{
	EntityName	entity;
	char Buff[105];

	NBPSetEntity((Ptr) &entity, "\p=", "\pAbuseServer", "\p*");
	
	p.NBPinterval = 7;// retransmit every 7*8=56 ticks
	p.NBPcount = 3;	// and retry 3 times
	p.NBPentityPtr = (Ptr) &entity;
	p.NBPretBuffPtr = (Ptr) Buff;
	p.NBPretBuffSize = sizeof(Buff);
	p.NBPmaxToGet = 1;

	macerr(PLookupName(&p,0), "Lookup error");
	
	if (p.NBPnumGotten != 1)
		return 0;
		
	macerr(NBPExtract((Ptr)Buff,p.NBPnumGotten,1,&entity,addr),"Extraction error");

	for (int i=0; i<entity.objStr[0]; i++)
		Buff[i] = entity.objStr[1+i];
	Buff[entity.objStr[0]] = 0;
	
	fprintf(stderr,"Found [%s]\n",Buff);

	return 1;
}

void main ()
{
	AddrBlock	myAddrBlk;

	fprintf(stderr,"Open MPP\n");
	macerr(OpenDriver("\p.MPP", &mppRef), "MPP Open Error");	// open .MPP driver

#ifdef __POWERPC__
	// Load 68K listener
	printf("Loading 68K Resource\n");
	Handle listener;

	listener = GetResource('SOCK',0);

	if (!listener)
		return;

	HLock(listener);
	InitDDPListener = (UniversalProcPtr)NewRoutineDescriptor((ProcPtr)(*listener + 0x12),
		kRegisterBased
		 | REGISTER_ROUTINE_PARAMETER(1, kRegisterA1, SIZE_CODE(sizeof(long))),kM68kISA);
	DDPListener = (UniversalProcPtr) (*listener+0x14);
#else
	DDPListener = (DDPSocketListenerUPP)(((char*)(&DDPEntry)) + 6);
#endif

	struct sDDPBuff *db = (struct sDDPBuff *)(&buffer[602]);

	db->Start = (struct sPacket *)(&db->Data);
	db->End = (struct sPacket *)(((char*)db->Start) + 602*16);
	db->Tail = db->Head = db->Start;
	db->Count = 0;

	// start DDP
	fprintf(stderr,"Open DDP socket\n");
	ddp.abResult = 1;
	ddp.ddpActCount = 0;
	ddp.ddpReqCount = 586;
	ddp.ddpDataPtr = buffer;
	p.MPPioCompletion = 0L;
	p.MPPioRefNum = mppRef;
	p.DDPsocket = 0;
	p.DDPlistener = DDPListener;
	macerr(POpenSkt(&p,FALSE), "Open Socket Error");
	socket = p.DDPsocket;

	fprintf(stderr,"Initialize socket listener\n");

#ifdef __POWERPC__
	CallUniversalProc(InitDDPListener, 
				kRegisterBased | 
				REGISTER_ROUTINE_PARAMETER(1, kRegisterA1, SIZE_CODE(sizeof(long))),
			(char*)db);
#else
	DDPEntry((char*)db);
#endif

	short node,net;
	Str255 name;
	
	fprintf(stderr,"Get Address...");	
	macerr(GetNodeAddress(&node,&net),"Can't get node address");
	sprintf((char*)&name[1],"Abuse@%d:%d:%d",net,node,socket);
	name[0] = strlen((char*)&name[1]);

	fprintf(stderr,"I'm %s\n",&name[1]);
	
	NBPSetNTE((Ptr) &myNTEName, "\pDDPtst", "\pAbuseServer", "\p*", socket);
			// set up NBP names table entry

	// set up PRegisterName parameters

	fprintf(stderr,"Registering Name\n");

	p.NBP.interval = 7;													// retransmit every 7*8=56 ticks
	p.NBP.count = 3;														// and retry 3 times
	p.NBP.nbpPtrs.entityPtr = (Ptr) &myNTEName;	// name to register
	p.NBP.parm.verifyFlag = 1;									// verify this name

	if (PRegisterName(&p, FALSE))
	{
		fprintf(stderr,"Couldn't register name\n");

		do
		{
			fprintf(stderr,"Searching for socket\n");
		} while (!PickSocket(&myAddrBlk));

		fprintf(stderr,"Found %d:%d:%d\n",myAddrBlk.aNet,myAddrBlk.aNode,myAddrBlk.aSocket);

		char st[80];

		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		
		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);	
		
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
		DoWrite(myAddrBlk,"Haoel, maestre!",16);
	}
	else
	{
		char st[80];
		
		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoWrite(myAddrBlk,"Hello, other!",14);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

		DoRead(&myAddrBlk,st,sizeof(st));
		fprintf(stderr,"Got [%s] from %d:%d:%d\n",st,
			myAddrBlk.aNet,
			myAddrBlk.aNode,
			myAddrBlk.aSocket
			);

	}

	// we're done with the connection, so remove it

	fprintf(stderr,"Removing Connection\n");

	EntityName	entity;
	
	NBPSetEntity((Ptr) &entity, "\pDDPtst", "\pAbuseServer", "\p*");
	p.MPPioCompletion = 0L;
	p.MPPioRefNum = mppRef;
	p.NBP.nbpPtrs.entityPtr = (Ptr)&entity;
	macerr(PRemoveName(&p,FALSE),"Unregister name error");

	p.DDPsocket = socket;
	macerr(PCloseSkt(&p,FALSE), "Error closing socket");

	fprintf(stderr,"Done.\n");
}	// MyADSP

