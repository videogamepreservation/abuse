#include <appletalk.h>
#include <stdio.h>

char ourName[60];

short mpp = 0, socket = 0;
NamesTableEntry *NTPtr = 0L;
MPPParamBlock p;
ATDDPRec ddp;
char buffer[1000];
AddrBlock *users;

int Setup()
{
	printf("NetTst 1.0\n\nReady\n");

	return 1;	
}

short DoRegName()
{
	EntityName name;
	short err, len;
	short index;
	StringHandle H;
	
	ourName[0] = 1;
	ourName[1] = '?';

/*
	p.MPPioCompletion = 0L;
	p.MPPioRefNum = mpp;
	p.DDPsocket = 0;
	p.DDPlistener = (Ptr) StripAddress((Ptr) (ProcPtr) DDPListener);
	err = POpenSkt(&p,true);
*/
	socket = 0;
	err = DDPOpenSocket(&socket, nil);

/*	
	while (p.MPPioResult == 1)
		;
*/
	if (err) {
		Message("\pCould not open AppleTalk socket !!!");
		return(err);
	}
//	socket = p.DDPsocket;
	name.zoneStr[0] = 1;
	name.zoneStr[1] = '*';
	BlockMove("\pNeighborhood Watch",name.typeStr, 20L);
	len = sizeof(NamesTableEntry);
	
	NTPtr = (NamesTableEntry*) NewPtrClear(len);
	if (!NTPtr)
		return(MemErr);
	NTPtr->nt.nteAddress.aSocket = socket;
	p.NBPinterval = 3;
	p.NBPcount = 3;
	p.NBPverifyFlag = true;
	p.NBPntQElPtr = (Ptr) NTPtr;
	BlockMove(ourName,&(NTPtr->nt.entityData[0]),33L);
	index = ourName[0] + 1;
	BlockMove(name.typeStr,&(NTPtr->nt.entityData[index]),33L);
	index += name.typeStr[0] + 1;
	BlockMove(name.zoneStr,&(NTPtr->nt.entityData[index]),33L);
	err = PRegisterName(&p,true);
	while (p.MPPioResult == 1)
		;
	err = p.MPPioResult;
	while (err == nbpDuplicate) {
		ourName[0]++;
		ourName[ourName[0]] = '1';
		BlockMove(ourName,&(NTPtr->nt.entityData[0]),33L);
		index = ourName[0] + 1;
		BlockMove(name.typeStr,&(NTPtr->nt.entityData[index]),33L);
		index += name.typeStr[0] + 1;
		BlockMove(name.zoneStr,&(NTPtr->nt.entityData[index]),33L);
		err = PRegisterName(&p,true);
		while (p.MPPioResult == 1)
			;
		err = p.MPPioResult;
	}
	if (err)
		Message("\pCould not register name in network.");

//	InitDDPListener(&ddp);

	return(noErr);
}

void Names()
{
	EntityName name;
	short err, counter;
	Handle buffer = 0L;
	short found;
	MPPParamBlock p;
	char Entity[110];
	
	p.MPPioCompletion = 0L;
	p.MPPioRefNum = mpp;
	buffer = NewHandle(LOOKUPBUFSIZE);
	if (!buffer) {
		return;
	}
	Message("\pFindind Neighbors...");
	HLock(buffer);
	BlockMove("\pNeighborhood Watch",name.typeStr, 20L);
	name.objStr[0] = 1;
	name.objStr[1] = '=';
	name.zoneStr[0] = 1;
	name.zoneStr[1] = '*';
	BlockMove(name.objStr,Entity,33L);
	counter = Entity[0] + 1;
	BlockMove(name.typeStr,&Entity[counter],33L);
	counter += name.typeStr[0] + 1;
	BlockMove(name.zoneStr,&Entity[counter],33L);
	p.NBPinterval = 5;
	p.NBPcount = 4;
	p.NBPentityPtr = Entity;
	p.NBPretBuffPtr = *buffer;
	p.NBPretBuffSize = LOOKUPBUFSIZE;
	p.NBPmaxToGet = LOOKUPBUFSIZE / 110;
	p.NBPnumGotten = 0;
	err = PLookupName(&p,false);
	err = p.MPPioResult;
	if (!err) {
		found = p.NBPnumGotten;
		for (counter = 0; counter < found; counter++)
			err = myNBPExtract(*buffer,found,counter + 1,&name, &(users[counter]));
			Message(name.objStr);
	}
	if (buffer)
		DisposHandle(buffer);
	buffer = 0L;
}

main()
{
	short err, len;	
	char st[256];

	if (!Setup())
		return;
	err = OpenDriver("\p.MPP",&mpp);
	if (err) {
		Message("\pAppleTalk is not available");
		return;
	}
	err = DoRegName();
	Names();
	BlockMove("\pEntering the network...",st,30L);
	len = ourName[0];
	BlockMove(&ourName[1], &st[st[0] + 1], (long) len);
	st[0] += len;
	err = DoDDP(st);
	if (err)
		Message("\pError writing to network.");
	while (Working())
		;
	BlockMove("\pLeaving the network...",st,30L);
	len = ourName[0];
	BlockMove(&ourName[1], &st[st[0] + 1], (long) len);
	st[0] += len;
	err = DoDDP(st);
	CleanTalk();
	
}

