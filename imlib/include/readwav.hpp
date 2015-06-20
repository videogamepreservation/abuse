#ifndef __READ_WAV_HPP_
#define __READ_WAV_HPP_
#include "specs.hpp"
unsigned char *read_wav(char *filename, long &sample_rate, long &data_size);
void write_wav(char *filename, long sample_rate, long data_size, unsigned char *data);


#endif



