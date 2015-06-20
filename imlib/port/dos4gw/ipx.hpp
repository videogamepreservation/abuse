#ifndef __IPX_HHP__
#define __IPX_HHP__

#include "jnet.hpp"
#define MAX_PACKETS 10

#pragma pack (1)

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


class ipx_out_socket : public out_socket
{
  public :

  int local_socket,      // fetched dymnamically from IPX driver
      remote_socket;     // fetched from remote host through connection port
  ulong local_time,
        remote_time;
  JC_ipx_packet *pk;                        // list of ecb's
  ipx_out_socket(int port);                 // port==0 means allocate dynamically
                                            // allocates 1 sending ecb, and 5 recieving

  int try_connect(char *rhost, int port);  // sends connection request to port and
                                            // waits for a acceptance reply, 2 second time out
  virtual int ready_to_read();              // true if at any "recieve" packets are clear
  virtual int ready_to_write();             // true if send packet is clear
  virtual int send(packet &pk);
  virtual int get(packet &pk);              
  virtual ~ipx_out_socket();
} ;


class ipx_in_socket : public in_socket
{
  int port;
  public :
  ipx_in_socket(int Port);
  ipx_out_socket *listener;
  virtual out_socket *check_for_connect();
  virtual ~ipx_in_socket();
} ;

int ipx_init();
void ipx_uninit();
uchar *ipx_get_local_address();

#endif






