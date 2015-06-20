#ifndef __READXWD_HPP
#define __READXWD_HPP
#include "image.hpp"
#include "palette.hpp"
#include "image24.hpp"

image *read_bmp(palette *&pal, char *filename);
image24 *read_bmp24(char *filename);
int bmp_bits(char *filname);
image *readxwd(char *input_file,palette *&pal);
void write_bmp(image *im, palette *pal, char *filename);
void deskjet_print(int argc, char **argv);


#endif

