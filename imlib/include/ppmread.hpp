#ifndef __PPMREAD__
#define __PPMREAD__
#include "image.hpp"
#include "palette.hpp"
#define PPM_R3G3B2 1
#define PPM_BW 2
#define PPM_REG 3
extern image *read_ppm(char *fn,palette *&pal, int pal_type=0);
extern void write_ppm(image *im,palette *pal,char *fn);
#endif

