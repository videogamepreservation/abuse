#ifndef __LCACHE_HPP_
#define __LCACHE_HPP_
#include "lisp.hpp"

#ifdef SCADALISP
#define can_cache_lisp() 0
#else
#define can_cache_lisp() 1
#endif
class memory_file;
long block_size(Cell *level);              // return number of bytes to save this block of code
void write_level(memory_file *fp, Cell *level);
Cell *load_block(memory_file *fp);

#endif
