#ifndef _CRC_HPP_
#define _CRC_HPP_
#include "specs.hpp"
#include "macs.hpp"

unsigned short calc_crc(unsigned char *buf, long len);
ulong crc_file(bFILE *fp);


#endif
