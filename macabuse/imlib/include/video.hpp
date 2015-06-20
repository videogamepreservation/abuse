#ifndef _VIDEO_HPP_
#define _VIDEO_HPP_
#include "system.h"
#include "image.hpp"
#include "globals.hpp"

enum video_mode { VMODE_320x200,  VMODE_640x480 };

extern unsigned char current_background;
extern int xoff,yoff;
extern image *screen;

void set_mode(video_mode mode, int argc=0, char **argv=NULL);
void switch_mode(video_mode new_mode);
void close_graphics();
void fill_image(image *im, int x1, int y1, int x2, int y2);
void update_dirty(image *im, int xoff=0, int yoff=0);


void clear_put_image(image *im, int x, int y);
int get_vmode();

#endif
