#include "ddplisten.hpp"

#define ddpResult			2
#define ddpType				8
#define ddpSocket			10
#define ddpAddress			12
#define ddpReqCount			16
#define ddpActCount			18
#define ddpDataPtr			20
#define ddpNodeID			24


#define myStart			0
#define myEnd			4
#define myHeadPtr		8
#define myTailPtr		12
#define myBuffCnt       16

// if on entry D0 = INIT, then call is to initialize, a1 must have a DDP handle.
// with VM this code must be locked down as well as the buffer and the handle !!!

#define PPC_DDP

#ifdef PPC_DDP
asm void main();

asm void main()
{
#else
asm void DDPEntry(char *buffer)
{
		move.l 4(sp),a1
#endif
InitDDPListener:
		bra.s saveDDP									// jump to save, expects address of buffer in a1
DDPListener:
		move.w SR,-(sp)									// save status register
		ori.w #0x2600,SR								// disable interrupts
		
		move.l a5,-(sp)									// save this a5
		
		move.l data,a5									// put buffer struct in a5

		move.l myHeadPtr(a5),a3							// get head pointer

		moveq #0,d3										// clear d3
		move.w d1,d3									// we want to read the whole packet

		move.l d3,(a3)									// save length as long

		lea 8(a3),a3									// skip to buffer store

		move.l (sp)+,a5									// recover a5
		jsr 2(a4)										// go to ReadRest
		beq.s getAddr									// if there was no error lets fill in DDP info
		
		move.w (sp)+,SR									// recover status register
		rts												// return otherwise (error)

saveDDP:
		lea data,a0
		move.l a1,(a0)									// put this data in a0
		rts												// return to caller

getAddr:
		move.l a5,-(sp)									// save a5
		move.l a3,-(sp)									// save a3

		move.l data,a5									// put buffer struct a5
		addi.l #1,myBuffCnt(a5)							// increment count

		move.l myHeadPtr(a5),a3							// put head address in a3
		
		cmpi.b #2,3(a2)									// is this long lap header ?
		beq.s longhead

shorthead:
		move.w 0x1A(a2),4(a3)							// set source network address
		move.b 2(a2),	6(a3)							// set source node address
		move.b 7(a2),	7(a3)							// set source socket address

		bra.s cont1

longhead:
		move.w 10(a2),4(a3)								// set source network address
		move.b 13(a2),6(a3)								// set source node address
		move.b 15(a2),7(a3)								// set source socket number

cont1:
		move.l myHeadPtr(a5),d3							// put head address in d3
		addi.l #602,d3									// next buffer
		
		cmp.l myEnd(a5),d3								// check against end

		blo.s no_reset
		move.l myStart(a5),d3							// restart
no_reset:
		move.l d3,myHeadPtr(a5)

		move.l (sp)+,a3									// recover a3
		move.l (sp)+,a5									// recover a5
		move.w (sp)+,SR									// recover status register
		rts												// return to protocol handler

data:
		dc.l	0										// data buffer address
}
