#include "doscall.c"
#include <dos.h>


#define JC_SEG(x) (((long)x)>>4)
#define JC_OFF(x) (((long)x)&0xf)
#define MKPTR(x)  ((   (((unsigned long)x)&0xffff0000)>>12)+  (((unsigned long)x)&0xffff))
#define MAX_PACKETS 10
#define jmalloc(x,str) malloc(x)
#define jfree(x)       free(x)

#pragma pack (1)

static rminfo rm;
#include <stdlib.h>

struct ECBStructure
{
  ushort Link[2];                /* offset-segment */
  ushort ESRAddress[2];          /* offset-segment */
  uchar  InUseFlag;
  uchar  CompletionCode;
  ushort ECBSocket;              /* high-low */
  uchar  IPXWorkspace[4];        /* N/A */
  uchar  DriverWorkspace[12];    /* N/A */
  uchar  ImmediateAddress[6];    /* high-low */
  ushort FragmentCount;          /* low-high */
  ushort fAddress[2];            /* offset-segment */
  ushort fSize;                  /* low-high */
} ;

struct IPXPacketStructure
{
  ushort PacketCheckSum;         /* high-low */
  ushort PacketLength;           /* high-low */
  uchar  PacketTransportControl;
  uchar  PacketType;

  uchar  dNetwork[4];            /* high-low */
  uchar  dNode[6];               /* high-low */
  ushort dSocket;                /* high-low */

  uchar  sNetwork[4];            /* high-low */
  uchar  sNode[6];               /* high-low */
  ushort sSocket;                /* high-low */
};


struct JC_ipx_packet
{
  ECBStructure ecb;
  IPXPacketStructure ipx;
  ulong time_stamp;
  ushort verify_stamp;       // should be 0cdc  "crack dot com", all others ignored
  uchar buffer[512];
} ;


int ipx_init()
{
  memset(&rm,0,sizeof(rm));
  rm.eax=0x7a00;
  RM_intr(0x2f,&rm);
  return (rm.eax&0xff)==0xff;
}



uchar *ipx_get_local_address()                // same format as above (be sure to jfree this)
{
  uchar *addr=(uchar *)jmalloc(10,"IPX address");  // normal memory return
  uchar *low_addr=(uchar *)alloc_low_memory(10);
  memset(&rm,0,sizeof(rm));
  rm.ebx=9;
  rm.esi=JC_OFF(low_addr);
  rm.es=JC_SEG(low_addr);
  RM_intr(0x7a,&rm);

  memcpy(addr,low_addr,10);
  free_low_memory(low_addr);
  return addr;
}


int ipx_listen(ECBStructure *ecb)
{
  memset(&rm,0,sizeof(rm));
  rm.esi=JC_OFF(ecb);
  rm.es=JC_SEG(ecb);

  rm.ebx=4;
  RM_intr(0x7a,&rm);

  if (rm.eax&0xff)
    return -1;
  return 0;
}

#define htons(x) ((((x)>>8)&0xff)|  (((x)&0xff)<<8))

JC_ipx_packet *ipx_socket(int port)
{
  memset(&rm,0,sizeof(rm));
  rm.ebx=0;                      // Open socket function
  rm.eax=0;                      // close on program exit
  rm.edx=htons(port);                      // dynamically allocate socket number
  RM_intr(0x7a,&rm);
  if (rm.eax&0xff)
  {
    printf("Unable to created ipx socket on port %d\n",port);
    return NULL;
  }
  printf("opened socket %x\n",htons(port));

  printf("created socket %x\n",rm.edx&0xffff);
  int fd=(ushort)htons((rm.edx&0xffff));



  JC_ipx_packet *pk=(JC_ipx_packet *)alloc_low_memory(sizeof(JC_ipx_packet)*MAX_PACKETS);
  if (!pk)
  {
    printf("unable to allocate low memory for packets\n");
    exit(0);
  }
  memset(pk,0,sizeof(JC_ipx_packet)*MAX_PACKETS);


  // setup an outgoing packet structure
  pk[0].ecb.ECBSocket = htons(fd);
  pk[0].ecb.FragmentCount = 1;
  pk[0].ecb.fAddress[0] = JC_OFF(&pk[0].ipx);
  pk[0].ecb.fAddress[1] = JC_SEG(&pk[0].ipx);

//  pk[0].ipx.PacketCheckSum=0xffff;
 
  uchar *my_addr=ipx_get_local_address();
  memcpy(pk[0].ipx.sNetwork,my_addr,10);
  pk[0].ipx.sSocket=htons(fd);

  memcpy(pk[0].ipx.dNetwork,my_addr,4);
  memset(pk[0].ipx.dNode,0xff,6);
  memset(pk[0].ecb.ImmediateAddress,0xff,6);
  pk[0].ipx.dSocket=htons(fd);

  jfree(my_addr);

  
  for (int i=1;i<MAX_PACKETS;i++)
  {
    // setup incoming packet structure
    pk[i].ecb.InUseFlag = 0x1d;
    pk[i].ecb.ECBSocket = htons(fd);
    pk[i].ecb.FragmentCount = 1;
    pk[i].ecb.fAddress[0] = JC_OFF(&pk[i].ipx);
    pk[i].ecb.fAddress[1] = JC_SEG(&pk[i].ipx);
    pk[i].ecb.fSize = sizeof(JC_ipx_packet)-sizeof(ECBStructure);
    if (ipx_listen(&pk[i].ecb)<0)
      printf("listen failed on packet %d\n",i);
    else
      printf("listen success on packet %d\n",i);
  }  

  printf("listen to socket %x\n",htons(fd));
  return pk;
}

int ipx_ready_to_read(JC_ipx_packet *pk)
{
  int i;
  for (i=1 ; i<MAX_PACKETS ; i++)
  {
    if (!pk[i].ecb.InUseFlag)
    {
      printf("<saw a packet>\n");
      if (ipx_listen(&pk[i].ecb)<0)
      { printf("failed to re-listen\n"); }

      return 1;    
    }
  }
  return 0;
}

void ipx_close(JC_ipx_packet *pk)
{
  memset(&rm,0,sizeof(rm));
  rm.ebx=1;
  rm.edx=pk[0].ecb.ECBSocket;
  printf("close socket %x\n",pk[0].ecb.ECBSocket);
  RM_intr(0x7a,&rm);
}


void free_up_memory() { ; }


int ipx_send(JC_ipx_packet *pk)
{
  pk[0].ecb.fSize = sizeof(pk[0].ipx)+10;

  unsigned char *a=(unsigned char *)pk[0].ipx.dNetwork;
  printf("send : %x%x%x%x-%x%x%x%x%x%x (%x)\n",a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],pk[0].ipx.dSocket);

  memset(&rm,0,sizeof(rm));
  rm.ebx=3;                      // send packet function
  rm.esi=JC_OFF(&pk[0]);
  rm.es=JC_SEG(&pk[0]);

  RM_intr(0x7a,&rm); 
  return 1;
}

main()
{
  if (!ipx_init())
    printf("IPX not installed\n");
  else
  {
    JC_ipx_packet *sock=ipx_socket(0x869b);
    if (sock==NULL)
      printf("unable to open socket\n");
    else
    {
      int i=0;
      while (i<5)
      {
	while (!ipx_ready_to_read(sock))
	{
	  fprintf(stderr,".");
	  sleep(1);
	  ipx_send(sock);
	  fprintf(stderr,"\\");
	}

	i++;	 
	printf("saw a packet\n");
      }
      ipx_close(sock);

    }			    
  }
}
