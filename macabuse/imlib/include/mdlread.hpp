#ifndef _MDL_READ_HPP_
#define _MDL_READ_HPP_
#include "palette.hpp"
#include "image.hpp"

// end ==-1 the read all images  startn==1 first image
short mdl_total_images(char *fn);
image **read_mdl(char *fn, palette *&pal, short startn, short endn, short &total);
void write_mdl(image **images, short total_images, palette *pal,char *fn,
		short firstpage=0, short images_per_page=20);

#endif
