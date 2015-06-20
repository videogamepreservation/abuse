#include "atalk.hpp"
#include <stdlib.h>
#include <string.h>

#ifdef __POWERPC__
DDPSocketListenerUPP DDPListener;
UniversalProcPtr InitDDPListener;
#else
#include "ddplisten.hpp"
#endif

#define ATALK_DEBUG
#define PACKET_TYPE 0xca

//atalk_protocol atalk;

short	mppRef;
short	adspRef;

/*
FILE *log_file=NULL;
extern int net_start();
void net_log(char *st, void *buf, long size)
{
    
  if (!log_file) 
  {
    if (net_start())
      log_file=fopen("client.log","wb");
    else
      log_file=fopen("server.log","wb");
  }


    fprintf(log_file,"%s%d - ",st,size);
    int i;
    for (i=0;i<size;i++) 
      if (isprint(*((unsigned char *)buf+i)))
        fprintf(log_file,"%c",*((unsigned char *)buf+i));
      else fprintf(log_file,"~");

    fprintf(log_file," : ");

    for (i=0;i<size;i++) 
    fprintf(log_file,"%02x, ",*((unsigned char *)buf+i),*((unsigned char *)buf+i));
    fprintf(log_file,"\n");
    fflush(log_file);

} */

//////////////////////////////////////////////////////////////////////
//
//  AppleTalk base socket Methods
//

atalk_base::atalk_base(int fd) : num(fd), select_flags(0)
{
}

atalk_base::~atalk_base()
{
	atalk.free_socket(num);
}

//////////////////////////////////////////////////////////////////////
//
//  ADSP Socket Methods
//

adsp_socket::adsp_socket(int fd) : atalk_base(fd)
{
	dsp.ioCRefNum = adspRef;
	dsp.csCode = dspInit;
	dsp.u.initParams.ccbPtr = (TPCCB)&ccb;

	// don't handle exceptions for now
	dsp.u.initParams.userRoutine = 0;
	// myDSPPBPtr->u.initParams.userRoutine = &myConnectionEvtUserRoutine;

	dsp.u.initParams.sendQSize = QSIZE;					// size of send queue
	dsp.u.initParams.sendQueue = &send_queue;		// send-queue buffer
	dsp.u.initParams.recvQSize = QSIZE;					// size of receive queue
	dsp.u.initParams.recvQueue = &recv_queue;		// receive-queue buffer
	dsp.u.initParams.attnPtr = &attn_buff;			// receive-attention buffer
	dsp.u.initParams.localSocket = 0;						// let ADSP assign socket
//	gDspCCB.myA5 = SetCurrentA5();											// save A5 for the user routine

	// establish a connection end

	if (PBControl((ParmBlkPtr) &dsp, FALSE))
		dprintf("ATALK ADSP Initialization Error\n");

	conn = dsp.ccbRefNum;	// save CCB ref num for later
}

int adsp_socket::open(atalk_address *addr)
{
	OSErr err;
	
	dsp.ioCRefNum = adspRef;	// ADSP driver ref num
	dsp.csCode = dspOpen;
	dsp.ccbRefNum = conn;			// connection ref num
	dsp.u.openParams.remoteAddress = addr->addr;	
	dsp.u.openParams.filterAddress.aNet = addr->addr.aNet;
	dsp.u.openParams.filterAddress.aNode = addr->addr.aNode;
	dsp.u.openParams.filterAddress.aSocket = 0;
	dsp.u.openParams.ocMode = ocRequest; 	// open connection mode
	dsp.u.openParams.ocInterval = 6; 			// retry every second
	dsp.u.openParams.ocMaximum = 255;			// don't quit trying

	// open a connection
	if (err = PBControl((ParmBlkPtr)&dsp, FALSE))
	{
		dprintf("ATALK ADSP dspOpen error %d\n",err);
		return err;
	}
	return 0;
}

void adsp_socket::cleanup() 
{
	dsp.ioCRefNum = adspRef;			// ADSP driver ref num
	dsp.csCode = dspRemove;
	dsp.ccbRefNum = conn;					// connection ref num
	dsp.u.closeParams.abort = 0;	// don't close until everything is sent and received

	if (PBControl((ParmBlkPtr) &dsp, FALSE))
		dprintf("ATALK ADSP remove error");
}

int adsp_socket::ready_to_read() 
{
	dsp.csCode = dspStatus;
	PBControl((ParmBlkPtr)&dsp, 0);
	return (dsp.u.statusParams.recvQPending != 0 || error());
}

int adsp_socket::error() 
{
	return ( ccb.userFlags>=4 );
}

int adsp_socket::ready_to_write() 
{
	dsp.ioCRefNum = adspRef;		        // ADSP driver ref num
	dsp.csCode = dspStatus;
	dsp.ioCompletion = 0;
	dsp.ccbRefNum = conn;
	PBControl((ParmBlkPtr)&dsp, 0);
	return (dsp.u.statusParams.sendQPending == 0 || error());
}

int adsp_socket::read(void *buf, int size, net_address **addr) 
{
  if (addr) 
  	*addr=NULL;

	do
	{
		if (error())
			return 0;
			
		dsp.ioCRefNum = adspRef;		        // ADSP driver ref num
		dsp.csCode = dspStatus;
		dsp.ioCompletion = 0;
		dsp.ccbRefNum = conn;
		PBControl((ParmBlkPtr)&dsp, 0);
	} while (dsp.u.statusParams.recvQPending < size);

	int fu = dsp.u.statusParams.recvQPending;

	dsp.ioCRefNum = adspRef;		        // ADSP driver ref num
	dsp.csCode = dspRead;
	dsp.ccbRefNum = conn;	        			// connection ref num
	dsp.u.ioParams.reqCount = size;     // read this number of bytes
	dsp.u.ioParams.dataPtr = (Ptr)buf;	// pointer to read buffer

	// perform read
	if (PBControl( (ParmBlkPtr)&dsp, FALSE))
	{
		dprintf("ATALK ADSP read error\n");
		return 0;
	}

//	dprintf("ATALK DSP read %d of %d, wanted %d\n", 
//		dsp.u.ioParams.actCount,
//		fu,
//		size);

  return dsp.u.ioParams.actCount;
}

int adsp_socket::write(void *buf, int size, net_address *addr)
{ 
  if (addr)
  	dprintf("Cannot change address for this socket type\n");

	while (!ready_to_write()) 
		if (error())
			return 0;

	dsp.ioCRefNum = adspRef;	       // ADSP driver ref num
	dsp.csCode = dspWrite;
	dsp.ccbRefNum = conn;     	     // connection ref num
	dsp.u.ioParams.reqCount = size;  // write this numberof bytes
	dsp.u.ioParams.dataPtr = buf; 	 // pointer to send queue
	dsp.u.ioParams.eom = 0;	         // 1 means last byte is logical end-of-message
	dsp.u.ioParams.flush = 1;	       // 1 means send data now

	// perform write
	if (PBControl( (ParmBlkPtr)&dsp, FALSE))
	{
		dprintf("ATALK ADSP Write error\n");
		return 0;
	}

	int ret = dsp.u.ioParams.actCount;

	while (!ready_to_write()) 
		if (error())
			return 0;

//	dprintf("ATALK DSP wrote %d of %d\n",ret,size);

  return ret; 
}

//////////////////////////////////////////////////////////////////////
//
//  ADSP Listener Socket Methods
//

adsp_listener::adsp_listener(int fd) : atalk_base(fd)
{
	// initialize listener with socket
	dsp.ioCRefNum = adspRef;
	dsp.csCode = dspCLInit;
	dsp.u.initParams.ccbPtr = (TPCCB)&ccb;
	dsp.u.initParams.localSocket = 0;  // get new "port"
//	gDspCCB.myA5 = SetCurrentA5();											// save A5 for the user routine

	if (PBControl((ParmBlkPtr) &dsp, FALSE))
		dprintf("ATALK Listener Initialization Error\n");

	conn = dsp.ccbRefNum;	// save CCB ref num for later
}

int adsp_listener::listen(int port)
{
	// perform listen
	dsp.ioCRefNum = adspRef;
	dsp.csCode = dspCLListen;
	dsp.ccbRefNum = conn;
	dsp.ioResult = 1;
	
	// don't filter any addresses
	dsp.u.openParams.filterAddress.aNet = 0;
	dsp.u.openParams.filterAddress.aNode = 0;
	dsp.u.openParams.filterAddress.aSocket = 0;
//	gDspCCB.myA5 = SetCurrentA5();											// save A5 for the user routine

	if (PBControl((ParmBlkPtr) &dsp, TRUE))
	{
		dprintf("ATALK Listener Error\n");
		return 0;
	}
	listening = 1;
	return 1;
}

void adsp_listener::cleanup() 
{
	dsp.csCode = dspCLRemove;
	dsp.ioCompletion = 0;
	dsp.ioCRefNum = adspRef;			// ADSP driver ref num
	dsp.ccbRefNum = conn;					// connection ref num
	dsp.u.closeParams.abort = 1;	// close outstanding requests

	if (PBControl((ParmBlkPtr) &dsp, FALSE))
		dprintf("ATALK ADSP Listener remove error");
}

int adsp_listener::ready_to_read()
{
	return dsp.ioResult != 1;
}

net_socket *adsp_listener::accept(net_address *&addr)
{
	if (!ready_to_read())
	{
		dprintf("Listener not ready!\n");
		return 0;
	}
	int fd = atalk.new_socket();
	
	adsp_socket *sock = new adsp_socket(fd);
	atalk.socket[fd] = sock;
	
	addr = new atalk_address(&dsp.u.openParams.remoteAddress);
	
	sock->dsp.ioCRefNum = adspRef;
	sock->dsp.csCode = dspOpen;
	sock->dsp.ccbRefNum  = sock->conn;
	sock->dsp.u.openParams.ocMode = ocAccept;
	sock->dsp.u.openParams.ocInterval = 6;
	sock->dsp.u.openParams.ocMaximum = 3;
	sock->dsp.u.openParams.remoteCID = dsp.u.openParams.remoteCID;
	sock->dsp.u.openParams.remoteAddress = dsp.u.openParams.remoteAddress;
	sock->dsp.u.openParams.sendSeq = dsp.u.openParams.sendSeq;
	sock->dsp.u.openParams.attnSendSeq = dsp.u.openParams.attnSendSeq;
	sock->dsp.u.openParams.sendWindow = dsp.u.openParams.sendWindow;

	if (PBControl((ParmBlkPtr) &sock->dsp, 0))
		dprintf("ATALK Accept Error\n");

	dsp.ioResult = 1;
	listen(0);
	
	return sock;
}

//////////////////////////////////////////////////////////////////////
//
//  DDP Socket Methods
//

ddp_socket::ddp_socket(int fd) : atalk_base(fd)
{
	dprintf("DDP new\n");
	ddp.abResult = 1;
	ddp.ddpActCount = 0;
	ddp.ddpReqCount = 586;
	ddp.ddpDataPtr = buffer;

	struct sDDPBuff *db = (struct sDDPBuff *)(&buffer[602]);

	db->Start = (struct sPacket *)(&db->Data);
	db->End = (struct sPacket *)((char*)db->Start + 602*16);
	db->Tail = db->Head = db->Start;
	db->Count = 0;

	// note!!!! can only do once!
	CallUniversalProc(InitDDPListener,
		kRegisterBased
		 	| REGISTER_ROUTINE_PARAMETER(1, kRegisterA1, SIZE_CODE(sizeof(long))),
		(char*)db);

	atalk.mpp.MPPioRefNum = mppRef;
	atalk.mpp.MPPioCompletion = 0L;
	atalk.mpp.DDPsocket = 0;
	atalk.mpp.DDPlistener = DDPListener;
	
	if (POpenSkt(&atalk.mpp,FALSE))
		dprintf("ATALK DDP Open Socket Error\n");
		
	socket = atalk.mpp.DDPsocket;
	
	dprintf("allocated DDP at socket %d\n",socket);
	
	def_addr.aNet = 0;
	def_addr.aNode = 0;
	def_addr.aSocket = 0;
}

void ddp_socket::cleanup()
{
	atalk.mpp.MPPioRefNum = mppRef;
	atalk.mpp.DDPsocket = socket;
	
	if (PCloseSkt(&atalk.mpp,FALSE))
		dprintf("Error closing socket");
}

int ddp_socket::ready_to_read()
{
	return ( ((struct sDDPBuff *)(&buffer[602]))->Count > 0);
}

int ddp_socket::error()
{
	return 0;
}

//extern void do_flip(int num);

int ddp_socket::read(void *buf, int size, net_address **addr)
{
	int cnt = size;
	
	while (!ready_to_read()) ;
	
//	do_flip(0);
	
	struct sDDPBuff *db = (struct sDDPBuff *)(&buffer[602]);
	struct sPacket *pk = db->Tail;

	cnt = (pk->Count<cnt)? pk->Count : cnt;
	db->Count--;
	memcpy(buf,&pk->Data,cnt);

//		dprintf("DDP Got %d of %d from %d:%d:%d\n",cnt,size,
//			ddp.ddpAddress.aNet,ddp.ddpAddress.aNode,ddp.ddpAddress.aSocket);
		
	if (addr)
		*addr = new atalk_address(&pk->Addr);

	pk = (struct sPacket *)((char *)pk + 602);
	if (pk>=db->End)
		pk = db->Start;
	db->Tail = pk;

	return cnt;
}

int ddp_socket::write(void *buf, int size, net_address *address)
{
	char wds[14];
	char header[20];
	AddrBlock *addr;

	if (address)
		addr = &((atalk_address*)address)->addr;
	else
		addr = &def_addr;
		
	BuildDDPwds((Ptr)wds, (Ptr)header, (Ptr)buf, *addr, PACKET_TYPE, size);

	atalk.mpp.DDP.ioCompletion = nil;
	atalk.mpp.DDP.socket = socket;
	atalk.mpp.DDP.checksumFlag = 0;
	atalk.mpp.DDP.u.wdsPointer = (Ptr) &wds;

	if (PWriteDDP(&atalk.mpp, FALSE))
		dprintf("ATALK ADD Write error\n");

//	dprintf("DDP Wrote %d to %d:%d:%d\n",size,
//		addr->aNet, addr->aNode, addr->aSocket);

	return 0;
}

//////////////////////////////////////////////////////////////////////
//
//  ATALK Protocol Methods
//

net_address *atalk_protocol::get_local_address()
{
	short node,net;
	
	GetNodeAddress(&node,&net);
  atalk_address *a=new atalk_address(net,node);

	return a;
}

net_address *atalk_protocol::get_node_address(char *&server_name, int def_port, int force_port)
{
	// not really used now

	return 0;
}

net_socket *atalk_protocol::connect_to_server(net_address *addr, net_socket::socket_type sock_type)
{
  if (addr->protocol_type()!=net_address::ATALK)
  {
    fprintf(stderr,"Procotol type not supported in the executable\n");
    return NULL;
  }

#ifdef ATALK_DEBUG
	char s[256];
	atalk_address *tmp = (atalk_address *)get_local_address();
	tmp->store_string(s,256);
	dprintf("[%s] trying to connect\n",s);

	delete tmp;
#endif

	int fd = new_socket();

  if (sock_type==net_socket::SOCKET_SECURE)
  {
  	adsp_socket *s;
    socket[fd] = s = new adsp_socket(fd);
    s->open((atalk_address *)addr);
    return socket[fd];
  }
  else
  {
  	ddp_socket *d;
    socket[fd] = d = new ddp_socket(fd);
    d->def_addr = ((atalk_address*)addr)->addr;
    return socket[fd];
  }
}

net_socket *atalk_protocol::create_listen_socket(int &port, net_socket::socket_type sock_type)
{
	int fd = new_socket();

	if (fd<0)
		return 0;
		
	if (sock_type == net_socket::SOCKET_SECURE)
	{
		adsp_listener *l;

		socket[fd] = l = new adsp_listener(fd);

		listener = l->dsp.u.initParams.localSocket;
		port = listener;
	
	  if (l->listen(port)==0)
  	{   
    	delete l;
    	return 0;
  	}
	}
	else
	{
		ddp_socket *d;
		
		socket[fd] = d = new ddp_socket(fd);
		port = d->get_socket();

		// get rid of this when real.  used only by test program
//		listener = port;
	}

	return socket[fd];
}

atalk_protocol::atalk_protocol()
: ok(0), net_protocol(), registered(0), listener(0), querying(0)
{
	if (OpenDriver("\p.MPP", &mppRef))
		return;

	if (OpenDriver("\p.DSP", &adspRef))
		return;

	for (int i=0; i<MAXSOCKS; i++)
		socket[i] = 0;
	for (int i=0; i<MAXSOCKS-1; i++)
		usage[i] = i+1;
	usage[MAXSOCKS-1] = -1;
	free = 0;

	Handle listener;

	listener = GetResource('SOCK',0);

	if (!listener)
		return;

	HLock(listener);
	InitDDPListener = (UniversalProcPtr)NewRoutineDescriptor((ProcPtr)(*listener+0x12),
		kRegisterBased
		 | REGISTER_ROUTINE_PARAMETER(1, kRegisterA1, SIZE_CODE(sizeof(long))),kM68kISA);
	DDPListener = (UniversalProcPtr) (*listener+0x14);

	// zone info initialization
	last_block = 0;
	last_pos = 1024;
	num_zones = 0;
	MyZone[0] = 1;
	MyZone[1] = '*';

	ok = 1;
}

int atalk_protocol::new_socket()
{
	int ret = free;
	
	if (ret >=0)
	{
		free = usage[free];
		usage[ret] = -2;
	}
	else
	{
		dprintf("Out of atalk_protocol sockets");
		exit(0);
	}

	dprintf("ATALK created fd = %d\n",ret);

	return ret;
}

void atalk_protocol::free_socket(int num)
{
	if (usage[num] != -2)
	{
		dprintf("Freed bad atalk_socket %d\n",num);
//		exit(0);
	}
	usage[num] = free;
	free = num;
	socket[num] = 0;

	dprintf("ATALK freed fd = %d\n",num);
}

void atalk_protocol::cleanup()
{
	if (registered)
		end_notify();
	registered = 0;	
	
	for (int i=0; i<MAXSOCKS; i++)
		if (socket[i])
		{
			delete socket[i];
			free_socket(i);
		}
}

int atalk_protocol::select(int block)
{
	int i,count=0;

	for (i=0; i<MAXSOCKS; i++)
		if (socket[i])
			count += socket[i]->select_count();

	return count;
}

net_socket *atalk_protocol::start_notify(int port, void *data, int len)
{
	if (registered)
		return 0;
		
	// set up NBP names table entry
	Name[0] = len;
	memcpy(&Name[1],data,len);
	NBPSetNTE((Ptr) &NTEName, Name, "\pAbuseServer", "\p*", listener);

	// set up PRegisterName parameters
	mpp.MPPioCompletion = 0L;
	mpp.MPPioRefNum = mppRef;
	mpp.NBP.interval = 7;												// retransmit every 7*8=56 ticks
	mpp.NBP.count = 3;													// and retry 3 times
	mpp.NBP.nbpPtrs.entityPtr = (Ptr) &NTEName;	// name to register
	mpp.NBP.parm.verifyFlag = 1;								// verify this name

#ifdef ATALK_DEBUG
	char s[256];
	atalk_address *tmp = (atalk_address *)get_local_address();
	tmp->set_port(listener);
	tmp->store_string(s,256);
	dprintf("registering [%s] as %s\n",s,(char*)data);
	
	delete tmp;
#endif

	if (PRegisterName(&mpp, FALSE))
		dprintf("ATALK Couldn't Register name\n");

	registered = 1;

	return 0;
}

void atalk_protocol::end_notify()
{
	EntityName	entity;

	NBPSetEntity((Ptr) &entity, Name, "\pAbuseServer", "\p*");
	mpp.MPPioCompletion = 0L;
	mpp.MPPioRefNum = mppRef;
	mpp.NBP.nbpPtrs.entityPtr = (Ptr)&entity;
	if (PRemoveName(&mpp,FALSE))
		dprintf("ATALK Unregister name error\n");
}

net_address *atalk_protocol::find_address(int port, char *name)
// name should be a 256 byte buffer
{
	EntityName	entity;
	atalk_address addr;

	if (!querying)
	{
		NBPSetEntity((Ptr) &entity, "\p=", "\pAbuseServer", MyZone);
		
		nbp.MPPioRefNum = mppRef;
		nbp.MPPioCompletion = 0L;
		nbp.NBPinterval = 7;// retransmit every 7*8=56 ticks
		nbp.NBPcount = 3;	// and retry 3 times
		nbp.NBPentityPtr = (Ptr) &entity;
		nbp.NBPretBuffPtr = (Ptr) Buff;
		nbp.NBPretBuffSize = sizeof(Buff);
		nbp.NBPmaxToGet = 10;
		nbp.MPPioResult = 1;
		nbp.NBPnumGotten = 0;
	
		if (PLookupName(&nbp,TRUE))
			dprintf("ATALK Name Lookup error\n");

		querying = 1;
		
		return 0;
	}
	else
	{
		if (nbp.MPPioResult == 1)
			return 0;

		querying = 0;
		
		if (nbp.MPPioResult != 0)
			return 0;
		
		for (int i=0; i<nbp.NBPnumGotten; i++)
		{
			int found = 0;
			
			NBPExtract((Ptr)Buff,nbp.NBPnumGotten,i+1,&entity,&addr.addr);
	
	    for (p_request p = servers.begin(); !found && p!=servers.end(); ++p)
	    	if ( *((*p)->addr) == addr )
	    		found = 1;
	    for (p_request q = returned.begin(); !found && q!=returned.end(); ++q)
	    	if ( *((*q)->addr) == addr )
	    		found = 1;
	    		
			if (!found) 
			{
				RequestItem *r = new RequestItem;
	
				for (int i=0; i<entity.objStr[0]; i++)
					r->name[i] = entity.objStr[1+i];
				r->name[entity.objStr[0]] = 0;
	
				r->addr = new atalk_address(&addr.addr);
	      servers.insert(r);
#ifdef ATALK_DEBUG
				char s[256];
				
				addr.store_string(s,256);
				dprintf("accepted %s\n",s);
#endif
			}
		}
	
		if (servers.empty())
			return 0;
	
	  servers.move_next(servers.begin_prev(), returned.begin_prev());
		atalk_address *ret = (atalk_address*)(*returned.begin())->addr->copy();
		strcpy(name,(*returned.begin())->name);
	
#ifdef ATALK_DEBUG
		char s[256];
		
		ret->store_string(s,256);
		dprintf("Found [%s]\n",s);
#endif

	  return ret;
	}
}

void atalk_protocol::reset_find_list()
{
	p_request p;
	
	for (p=servers.begin(); p!=servers.end(); ++p)
		delete (*p)->addr;
	for (p=returned.begin(); p!=returned.end(); ++p)
		delete (*p)->addr;
		
  servers.erase_all();
  returned.erase_all();
}

char ** atalk_protocol::GetZones(int &num)
{
	if (num_zones==0)
	{
		XPPParamBlock xppPB;
		char buffer[578];
		OSErr err = noErr;
	
		xppPB.XCALL.xppTimeout = 3;
		xppPB.XCALL.xppRetry = 4;	
		xppPB.XCALL.zipBuffPtr = buffer;	
		xppPB.XCALL.zipLastFlag = 0;	
		xppPB.XCALL.zipInfoField[1] = 0;	
		xppPB.XCALL.zipInfoField[2] = 0;	
		
		while (xppPB.XCALL.zipLastFlag == 0 && err==noErr)
		{
			err = GetZoneList(&xppPB, FALSE);
			if (err)
				dprintf("ATALK GetZoneList error %d\n",err);
			else
			{
				unsigned char *p = (unsigned char *)buffer;
				for (int i=0; i<xppPB.XCALL.zipNumZones; i++)
				{
					AddZone(p);
					p += (*p) + 1;
				} 
			}
		}
	}
	
	num = num_zones;
	return ZoneName;
}

void atalk_protocol::SetZone(char *name)
{
	int l = strlen(name);
	MyZone[0] = l;
	memcpy(&MyZone[1],name,l);
}

int atalk_protocol::GetMyZone(char *name)
{
	XPPParamBlock xppPB;
	char buffer[33];
	OSErr err = noErr;

	xppPB.XCALL.xppTimeout = 3;
	xppPB.XCALL.xppRetry = 4;	
	xppPB.XCALL.zipBuffPtr = buffer;	
	xppPB.XCALL.zipLastFlag = 0;	
	xppPB.XCALL.zipInfoField[1] = 0;	
	xppPB.XCALL.zipInfoField[2] = 0;	
		
	if (::GetMyZone(&xppPB, FALSE) == noErr)
	{
		memcpy(name,(char*)&buffer[1],buffer[0]);
		name[buffer[0]] = 0;
		
		return 0;
	}
	else
		return -1;
}

void atalk_protocol::AddZone(unsigned char *name)
{
	if (num_zones>=MAXZONENAMES)
		return;

	if (name[0] + last_pos + 1> 1024)
	{
		zones.insert(last_block = new sZoneBlock);
		last_pos = 0;
	}
	char *ret = &last_block->Name[last_pos];
	last_pos += name[0] + 1;
	
	memcpy(ret,(char*)&name[1],name[0]);
	ret[name[0]] = 0;
	
	ZoneName[num_zones++] = ret;
}

void atalk_protocol::FreeZones()
{
	for (p_zoneblock p = zones.begin(); p != zones.end(); ++p)
		delete (*p);
		
	zones.erase_all();
	num_zones = 0;
}
