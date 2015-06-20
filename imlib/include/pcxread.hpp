#ifndef __PCX_READ_HPP__
#define __PCX_READ_HPP__

#include "image.hpp"
#include "image24.hpp"
#include "palette.hpp"

enum PCX_type { not_PCX, PCX_8, PCX_24 };

PCX_type PCX_file_type(char *filename);
image24 *read_PCX24(char *filename);
image *read_PCX(char *filename, palette *&pal);
void write_PCX(image *im, palette *pal,char *filename);

#endif
