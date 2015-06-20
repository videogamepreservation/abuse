#ifndef __GLREAD_HPP_
#define __GLREAD_HPP_
#include "palette.hpp"
#include "image.hpp"
image *read_glfont(char *fn);
image *read_pic(char *fn, palette *&pal);
image *read_clp(char *fn);

#endif

