#include "ddplisten.hpp"

#define ddpResult			2
#define ddpType				8
#define ddpSocket			10
#define ddpAddress			12
#define ddpReqCount			16
#define ddpActCount			18
#define ddpDataPtr			20
#define ddpNodeID			24


#define tDDPRec			 0
#define tDDPBuffer		 4
#define tDDPLen			 8

// if on entry D0 = INIT, then call is to initialize, a1 must have a DDP handle.
// with VM this code must be locked down as well as the buffer and the handle !!!

pascal asm void DDPListener()
{
		cmp.l #'INIT',d0					// is this my init call ?
		beq saveDDP							// save the address of my DDP handle
		move.w SR,-(sp)						// save status register
		ori.w #0x2600,SR					// disable interrupts
		move.l a5,-(sp)						// save this a5
		lea data,a5							// put my data in a5
		move.w d1,tDDPLen(a5)				// save the len of this packet
		moveq #0,d3							// clear d3
		move.w d1,d3						// we want to read the whole packet
		move.l tDDPBuffer(a5),a3			// set buffer address in a3
		move.l (sp)+,a5						// recover a5
		jsr 2(a4)							// go to ReadRest
		beq.s getAddr						// if there was no error lets fill in DDP info
		move.w (sp)+,SR						// recover status register
		rts									// return otherwise (error)

getAddr:
		move.l a5,-(sp)						// save a5
		lea data,a5							// put my data in a5
		move.w tDDPLen(a5),d1				// put packet length back in d1
		move.l tDDPRec(a5),a5				// put the DDP pointer in a5
		clr.l d3							// clear d3
		clr.w ddpResult(a5)							// set result to zero (noErr)
		move.w d1,ddpActCount(a5)						// set actual count read
		move.b 1(a2),d3									// put destination node in d3
		move.w d3,ddpNodeID(a5)							// set destination node
		cmpi.b #2,3(a2)									// is this long lap header ?
		beq.s longhead
		move.b 6(a2),d3									// put destination socket in d3
		move.w d3,ddpSocket(a5)							// set destination socket
		move.b 8(a2),d3									// put ddpType in d3
		move.w d3,ddpType(a5)							// set ddpType
		move.w 0x1A(a2),ddpAddress(a5)					// set source network address
		move.b 2(a2),ddpAddress+2(a5)					// set source node address
		move.b 7(a2),ddpAddress+3(a5)					// set source socket address
		move.l (sp)+,a5									// recover a5
		move.w (sp)+,SR									// recover status register
		rts												// return to protocol handler

longhead:
		move.w 10(a2),ddpAddress(a5)					// set source network address
		move.b 13(a2),ddpAddress+2(a5)					// set source node address
		move.b 15(a2),ddpAddress+3(a5)					// set source socket number
		move.b 14(a2),d3								// put destination socket in d3
		move.w d3,ddpSocket(a5)							// set destination socket
		move.b 16(a2),d3								// put ddpType in d3
		move.w d3,ddpType(a5)							// set ddpType
		move.l (sp)+,a5									// recover a5
		move.w (sp)+,SR									// recover status register
		rts												// return to protocol handler

saveDDP:
		lea data,a0								// put this data in a0
		move.l a1,tDDPRec(a0)					// save my DDP pointer there
		move.l ddpDataPtr(a1),tDDPBuffer(a0)	// save address of buffer there too
		rts										// return to caller
data:
		dc.l	0	// DDP record pointer
		dc.l	0	// DDPBuffer address
		dc.w	0	// packet lenght
}

asm void InitDDPListener(ATDDPRec *ddp)
{
	move.l #'INIT',d0	// set our init flag in d0
	move.l 4(sp),a1		// put the ddp record pointer we are passed in a1
	jsr DDPListener		// go initialaze our socket listener
	rts
}
