#ifndef __CASHE_HPP_
#define __CASHE_HPP_

#include <stdlib.h>
#include "specs.hpp"
#include "items.hpp"
#include "sound.hpp"
#include "lisp.hpp"

class level;
/* Cache item types :

      foretile,backtile,character,
      sound,
      image,trans_image

*/



struct cache_item
{
  void *data;
  long last_access;   
  unsigned char type;
  short file_number;
  long offset;
} ;



class crced_file
{
  public :
  int crc_calculated;
  ulong crc;
  char *filename;
  crced_file(char *name);
  ~crced_file();
} ;

class crc_manager  // stores crc for each file open so redundant calculations are not done
{
  int total_files;
  crced_file **files;
  public :
  crc_manager();
  int get_filenumber(char *filename);
  ulong get_crc(long filenumber, int &failed);
  void set_crc(long filenumber, ulong crc);
  char *get_filename(long filenumber);
  void clean_up();
  int total_filenames() { return total_files; }
  int write_crc_file(char *filename);
  int load_crc_file(char *filename);
} ;


class memory_file;

class cache_list
{
  cache_item *list;
  long total,last_registered,last_access,poll_start_access;
  short last_file;           // for speed leave the last file accessed open

  bFILE *fp;
  memory_file *cache_mfile;
  spec_directory *last_dir;
  long last_offset;          // store the last offset so we don't have to seek if
                             // we don't need to
  

  short lcache_number;
  long alloc_id();
  void locate(cache_item *i, int local_only=0);    // set up file and offset for this item
  void normalize();
  void unmalloc(cache_item *i);
  int used,                                 // flag set when disk is accessed
      ful;                                  // set when stuff has to be thrown out
  int *prof_data;                           // holds counts for each id
  void preload_cache_object(int type);
  void preload_cache(level *lev);
public :
  void create_lcache();
  cache_list();
  void free_oldest();
  int in_use() { if (used) { used=0; return 1; } else return 0; }
  int full() { if (ful) { ful=0; return 1; } else return 0; }
  long reg_object(char *filename, void *object, int type, int rm_dups);      // lisp object
  long reg(char *filename, char *name, int type=-1, int rm_dups=0);          // returns id to item
  long reg_lisp_block(Cell *block);
  int loaded(int id);
  void unreg(int id);
  void note_need(int id);

  void           expire(int id);   // if loaded item will be freed now
  backtile       *backt(int id);
  foretile       *foret(int id);
  figure          *fig(int id);
  image           *img(int id);
  sound_effect    *sfx(int id);
  Cell         *lblock(int id);
  char_tint     *ctint(int id);

  void prof_init();
  void prof_write(bFILE *fp);
  void prof_uninit();
  int  prof_size();                   // sizeof of spec entry that will be saved
  void prof_poll_start();
  void prof_poll_end();
  int  prof_is_on() { return prof_data!=NULL; }   // so level knows weither to save prof info or not
  int compare(int a, int b);          // compares the ussage counts of 2 entries (used by qsort)
  int offset_compare(int a, int b);

  void load_cache_prof_info(char *filename, level *lev);
  int search(int *sarray, ushort filenum, long offset);  // sarray is a index table sorted by offset/filenum

  void show_accessed();
  void empty();
  ~cache_list();
} ;

extern cache_list cash;
extern crc_manager crc_man;


#endif






