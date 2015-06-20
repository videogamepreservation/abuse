#include "cache.hpp"
#include "lisp.hpp"
#include "video.hpp"
#include "dprint.hpp"
#include "exitproc.hpp"
#include "lcache.hpp"
#include "status.hpp"
#include "game.hpp"
#include "lisp_gc.hpp"
#include "level.hpp"
#include "status.hpp"
#include "crc.hpp"
#include "specache.hpp"

#ifndef __POWERPC__
#include <sys/stat.h>
#endif

#include <fcntl.h>
char lfname[100]="";          // name of compiled lisp code cache file

#define touch(x) { (x)->last_access=last_access++; \
		   if ((x)->last_access<0) { normalize(); (x)->last_access=1; } }

extern char *symbol_str(char *name);

crc_manager crc_man;

int past_startup=0;

int crc_man_write_crc_file(char *filename)
{
  return crc_man.write_crc_file(filename);
}

int crc_manager::write_crc_file(char *filename)  // return 0 on failure
{
  char msg[100];
  sprintf(msg,symbol_str("calc_crc"));  // this may take some time, show the user a status indicator
  if (stat_man) stat_man->push(msg,NULL);

  int i,total=0;
  for (i=0;i<total_files;i++)
  {
    int failed=0;
    get_crc(i,failed);

    if (failed)
    {
      jFILE *fp=new jFILE(get_filename(i),"rb");
      if (!fp->open_failure())
      {
	set_crc(i,crc_file(fp));
	total++;
      }
      delete fp;
    } else total++;
    if (stat_man)
      stat_man->update(i*100/total_files);
  }
  if (stat_man) stat_man->pop();
  jFILE *fp=new jFILE("#net_crc","wb");
  if (fp->open_failure())
  {
    delete fp;
    return 0; 
  }

  fp->write_short(total);
  total=0;
  for (i=0;i<total_files;i++)
  {
    ulong crc;
    int failed=0;
    crc=get_crc(i,failed);
    if (!failed)
    {
      fp->write_long(crc);
      uchar len=strlen(get_filename(i))+1;
      fp->write_byte(len);
      fp->write(get_filename(i),len);
      total++;
    }
  }
  delete fp;
  return 1;  
}

int crc_manager::load_crc_file(char *filename)
{
  bFILE *fp=open_file(filename,"rb");
  if (fp->open_failure())
  {
    delete fp;
    return 0;
  } else
  {
    short total=fp->read_short();
    int i;
    for (i=0;i<total;i++)
    {
      char name[256];
      ulong crc=fp->read_long();
      uchar len=fp->read_byte();
      fp->read(name,len);
      set_crc(get_filenumber(name),crc);
    }
    delete fp;
  }
  return 1;  
}

void crc_manager::clean_up()
{
  for (int i=0;i<total_files;i++)
    delete files[i];
  if (total_files)
    jfree(files);
  total_files=0;
  files=NULL;
}

crced_file::~crced_file()
{
  jfree(filename);
}

crced_file::crced_file(char *name) 
{ 
  filename=strcpy((char *)jmalloc(strlen(name)+1,"crc_file"),name);
  crc_calculated=0;
}

crc_manager::crc_manager()
{
  total_files=0;
  files=NULL;
}

int crc_manager::get_filenumber(char *filename)
{
  for (int i=0;i<total_files;i++)
    if (!strcmp(filename,files[i]->filename)) return i;
  total_files++;
  files=(crced_file **)jrealloc(files,total_files*sizeof(crced_file *),"crc_file_list");
  files[total_files-1]=new crced_file(filename);
  return total_files-1;
}

char *crc_manager::get_filename(long filenumber)
{
  CHECK(filenumber>=0 && filenumber<total_files);
  return files[filenumber]->filename;
}

ulong crc_manager::get_crc(long filenumber, int &failed)
{    
  CHECK(filenumber>=0 && filenumber<total_files);
  if (files[filenumber]->crc_calculated) 
  {
    failed=0;
    return files[filenumber]->crc;
  }
  failed=1;
  return 0;
}

void crc_manager::set_crc(long filenumber, ulong crc)
{
  CHECK(filenumber>=0 && filenumber<total_files);
  files[filenumber]->crc_calculated=1;
  files[filenumber]->crc=crc;
}

void cache_list::unmalloc(cache_item *i)
{
  switch (i->type)
  {
    case SPEC_CHARACTER2 :
    case SPEC_CHARACTER : delete ((figure *)i->data);   break;
    case SPEC_FORETILE : delete ((foretile *)i->data);  break;
    case SPEC_BACKTILE : delete ((backtile *)i->data);  break;
    case SPEC_IMAGE    : delete ((image *)i->data);     break;
    case SPEC_EXTERN_SFX : delete ((sound_effect *)i->data); break;
    case SPEC_PARTICLE : delete ((part_frame *)i->data); break;
    case SPEC_EXTERNAL_LCACHE : if (i->data) jfree(i->data); break;
    case SPEC_PALETTE : delete ((char_tint *)i->data); break;
    default :
      printf("Trying to unmalloc unknown type\n");
  }
  i->data=NULL;
  i->last_access=-1;
}



void cache_list::prof_init()
{
  if (prof_data)
    jfree(prof_data);
  
  prof_data=(int *)jmalloc(sizeof(int)*total,"cache profile");
  memset(prof_data,0,sizeof(int)*total);
}

static int c_sorter(const void *a, const void *b)
{
  return cash.compare(*(int *)a,*(int *)b);
}

int cache_list::compare(int a, int b)
{
  if (prof_data[a]<prof_data[b])
    return 1;
  else if (prof_data[a]>prof_data[b])
    return -1;
  else return 0;
}


int cache_list::prof_size()
{
  int size=0;     // count up the size for a spec enrty
  size+=2;        // total filenames
  int i;
  for (i=0;i<crc_man.total_filenames();i++)
      size+=strlen(crc_man.get_filename(i))+2;    // filename + 0 + size of string

  size+=4;       // number of entries saved

  for (i=0;i<total;i++)
    if (list[i].last_access>0)       // don't save unaccessed counts
      size+=2+4+1;                   // filenumber & offset & type

  return size;
}


void cache_list::prof_write(bFILE *fp)
{
  if (prof_data)
  {
    int *ordered_ids=(int *)jmalloc(sizeof(int)*total,"profile order");
    int i;
    for (i=0;i<total;i++) ordered_ids[i]=i;
    qsort(ordered_ids,total,sizeof(int),c_sorter);

    if (fp)
    {
      fp->write_short(crc_man.total_filenames());
      for (i=0;i<crc_man.total_filenames();i++)
      {
	int l=strlen(crc_man.get_filename(i))+1;
        fp->write_byte(l);
	fp->write(crc_man.get_filename(i),l);
      }

      int tsaved=0;
      for (i=0;i<total;i++)
        if (list[i].last_access>0) tsaved++;
      fp->write_long(tsaved);

      for (i=0;i<total;i++)
      {
	int id=ordered_ids[i];
        if (list[id].last_access>0)       // don't save unaccessed counts      
	{
	  fp->write_byte(list[id].type);    // save type, if type changed on reload 
	                                    // don't cache in-> its a different refrence
	  fp->write_short(list[id].file_number);
	  fp->write_long(list[id].offset);
	}
      }      
    }

    jfree(ordered_ids);

  } else dprintf("Cache profiling was not initialized\n");
}

void cache_list::prof_uninit()
{
  if (prof_data)
  {
    jfree(prof_data);
    prof_data=NULL;
  }
}

int *sorted_id_list;


static int s_offset_compare(const void *a, const void *b)
{
  return cash.offset_compare(*(int *)a,*(int *)b);
}

int cache_list::offset_compare(int a, int b)
{
  if (list[a].offset<list[b].offset)
    return -1;
  else if (list[a].offset>list[b].offset)
    return 1;
  else if (list[a].file_number<list[b].file_number)
    return -1;
  else if (list[a].file_number>list[b].file_number)
    return 1;
  else return 0;
}


int cache_list::search(int *sarray, ushort filenum, long offset)
{
  int x1=0,x2=total-1;
  int split;
  do
  {
    split=(x1+x2)/2;
    cache_item *e=list+sarray[split];

    int comp;
    if (e->offset<offset)      // search to the right    
      x1=split+1;
    else if (e->offset>offset)
      x2=split-1;
    else if (e->file_number<filenum)
      x1=split+1;
    else if (e->file_number>filenum)
      x2=split-1;
    else return sarray[split];
  } while (x1<=x2);
  return -1;
}

static int load_chars()  // returns 0 if cache filled
{
  int i;
  for (i=0;i<total_objects;i++)
  {
    if (figures[i]->get_cflag(CFLAG_NEED_CACHE_IN))
    {
      figures[i]->set_cflag(CFLAG_CACHED_IN,0);
      figures[i]->cache_in();
      figures[i]->set_cflag(CFLAG_NEED_CACHE_IN,0);
    }
  }
  return 1;
  
}

void cache_list::note_need(int id)
{
  if (list[id].last_access<0)
    list[id].last_access=-2;
  else
    list[id].last_access=2;
}

void cache_list::preload_cache_object(int type)
{
  if (type<0xffff)
  {
    if (!figures[type]->get_cflag(CFLAG_NEED_CACHE_IN))  // see if it's already marked
    {        
      figures[type]->set_cflag(CFLAG_NEED_CACHE_IN,1);
      void *cache_fun=figures[type]->get_fun(OFUN_GET_CACHE_LIST);

      if (cache_fun)
      {
	int sp=current_space;
	current_space=PERM_SPACE;

	void *call_with=NULL;
	push_onto_list(new_lisp_number(type),call_with);

	void *cache_list=eval_function((lisp_symbol *)cache_fun,call_with);
	p_ref r1(cache_list);

	if (cache_list && lcar(cache_list))
	{
	  void *obj_list=lcar(cache_list);
	  while (obj_list)
	  {
	    int t=lnumber_value(CAR(obj_list));
	    if (t<0 || t>=total_objects)
	      lbreak("Get cache list returned a bad object number %d\n",t);
	    else
	      preload_cache_object(t);
	    obj_list=CDR(obj_list);
	  }
	} 
	if (cache_list && lcdr(cache_list))
	{
	  void *id_list=lcar(lcdr(cache_list));
	  while (id_list)
	  {
	    int id=lnumber_value(CAR(id_list));
	    if (id<0 || id>=total)
	      lbreak("Get cache list returned a bad id number %d\n",id);
	    else if (list[id].last_access<0)
	      list[id].last_access=-2;
	    else list[id].last_access=2;

	    id_list=CDR(id_list);
	  }
	}
	current_space=sp;

      }
    }
  }
}

void cache_list::preload_cache(level *lev)
{
  game_object *f;
  int i;
  for (i=0;i<total_objects;i++)                       // mark all types as not needing loading
    figures[i]->set_cflag(CFLAG_NEED_CACHE_IN,0);

  for (f=lev->first_object();f;f=f->next)               // go through each object and get requested items to cache in
    preload_cache_object(f->otype);


  int j;
  ushort *fg_line;
  for (j=0;j<lev->foreground_height();j++)
  {
    fg_line=lev->get_fgline(j);
    for (i=0;i<lev->foreground_width();i++,fg_line++)
    {
      int id=foretiles[fgvalue(*fg_line)];
      if (id>=0 && id<nforetiles)
      {
	if (list[id].last_access<0)
          list[id].last_access=-2;
	else list[id].last_access=2;      
      }
    }      
  }

  ushort *bg_line;
  for (j=0;j<lev->background_height();j++)
  {
    bg_line=lev->get_bgline(j);
    for (i=0;i<lev->background_width();i++,bg_line++)
    {    
      int id=backtiles[bgvalue(*bg_line)];
      if (id>=0 && id<nbacktiles)
      {
	if (list[id].last_access<0)
          list[id].last_access=-2;
	else list[id].last_access=2;      
      }
    }      
  }

  load_chars();
}

void cache_list::load_cache_prof_info(char *filename, level *lev)
{
  int j;
  for (j=0;j<this->total;j++)
    if (list[j].last_access>=0)      // reset all loaded cache items to 0, all non-load to -1
      list[j].last_access=0;

  preload_cache(lev);                // preliminary guesses at stuff to load

  int load_fail=1;
  bFILE *fp=open_file(filename,"rb");
  if (!fp->open_failure())
  {
    spec_directory sd(fp);
    spec_entry *se=sd.find("cache profile info");   // see if the cache profile info is in the file
    if (se)
    {
      fp->seek(se->offset,0);

      char name[255];
      int tnames=0;
      int *fnum_remap;    // remaps old filenumbers into current ones
      
      tnames=fp->read_short();
      if (tnames)                     /// make sure there isn't bad info in the file
      {
	fnum_remap=(int *)jmalloc(sizeof(int)*tnames,"pfname remap");

	int i;
	for (i=0;i<tnames;i++)
	{
	  fp->read(name,fp->read_byte());
	  fnum_remap[i]=-1;                    // initialize the map to no-map

	  int j;
	  for (j=0;j<crc_man.total_filenames();j++)
	    if (!strcmp(crc_man.get_filename(j),name))
	      fnum_remap[i]=j;
	}
	
	long tsaved=fp->read_long();


	int *priority=(int *)jmalloc(tsaved*sizeof(int),"priorities");
	memset(priority,0xff,tsaved*sizeof(int));   // initialize to -1
	int tmatches=0;

	sorted_id_list=(int *)jmalloc(sizeof(int)*total,"sorted ids");
	for (j=0;j<total;j++) sorted_id_list[j]=j;
	qsort(sorted_id_list,total,sizeof(int),s_offset_compare);

	for (i=0;i<tsaved;i++)
	{
	  uchar type=fp->read_byte();
	  short file_num=fp->read_short();
	  if (file_num>=tnames)  // bad data?
	    file_num=-1;
	  else file_num=fnum_remap[file_num];

	  ulong offset=fp->read_long();

	  // search for a match 
	  j=search(sorted_id_list,file_num,offset);	 
	  if (j!=-1)
	  {	      
	    if (list[j].last_access<0)  // if not loaded
	      list[j].last_access=-2;      // mark as needing loading
	    else list[j].last_access=2;   // mark as loaded and needing to stay that way
	    priority[i]=j;
	    tmatches++;
	  }
	}

	jfree(sorted_id_list);            // was used for searching, no longer needed

	for (j=0;j<total;j++)
	  if (list[j].last_access==0)
	    unmalloc(list+j);             // free any cache entries that are not accessed at all in the level


	ful=0;
	int tcached=0;
	for (j=0;j<total;j++)    // now load all of the objects until full
	{
//	  stat_man->update(j*70/total+25);
	  if (list[j].file_number>=0 && list[j].last_access==-2)
	  {
	    list[j].last_access=-1;
	    if (!ful)
	    {
	      switch (list[j].type)
	      {
		case SPEC_BACKTILE : backt(j); break;
		case SPEC_FORETILE : foret(j); break;
		case SPEC_CHARACTER :
		case SPEC_CHARACTER2 : fig(j); break;
		case SPEC_IMAGE : img(j); break;
		case SPEC_PARTICLE : part(j); break;
		case SPEC_EXTERN_SFX : sfx(j); break;
		case SPEC_EXTERNAL_LCACHE : lblock(j); break;
		case SPEC_PALETTE : ctint(j); break;
	      }
	      tcached++;
	    }
	  }
	}
	load_fail=0;
//	if (full())
//	  dprintf("Cache filled while loading\n");

	if (tsaved>tmatches)
	  tmatches=tsaved+1;

	last_access=tmatches+1;
	for (i=0;i<tsaved;i++)      // reorder the last access of each cache to reflect prioirties
	{
	  if (priority[i]!=-1)
	  {
	    if (list[priority[i]].last_access!=-1)            // make sure this wasn't the last item
	      list[priority[i]].last_access=tmatches--;
	  }
	} 

	jfree(priority);
	jfree(fnum_remap);


      }
    }    
  }

  if (load_fail) // no cache file, go solely on above gueses
  {
    int j;
    for (j=0;j<total;j++)    // now load all of the objects until full, don't free old stuff
    {
//      stat_man->update(j*70/total+25);

      if (list[j].file_number>=0 && list[j].last_access==-2)
      {
	list[j].last_access=-1;
	if (!ful)
	{
	  switch (list[j].type)
	  {
	    case SPEC_BACKTILE : backt(j); break;
	    case SPEC_FORETILE : foret(j); break;
	    case SPEC_CHARACTER :
	    case SPEC_CHARACTER2 : fig(j); break;
	    case SPEC_IMAGE : img(j); break;
	    case SPEC_PARTICLE : part(j); break;
	    case SPEC_EXTERN_SFX : sfx(j); break;
	    case SPEC_EXTERNAL_LCACHE : lblock(j); break;
	    case SPEC_PALETTE : ctint(j); break;
	  }
	}
      }
    }
    if (full())
      dprintf("Cache filled while loading\n");
  }
  delete fp;
}


void cache_list::prof_poll_start()
{
  poll_start_access=last_access;  
}

void cache_list::prof_poll_end()
{
  if (prof_data)
  {
    int i=0;
    for (;i<total;i++)
    {
      if (list[i].last_access>=poll_start_access)
        prof_data[i]++;
    }
  }
}

void cache_list::unreg(int id)
{
  if (list[id].file_number)
  {
    unmalloc(&list[id]);
    list[id].file_number=-1;
  }
  else 
    printf("Error : trying to unregister free object\n");
}

void cache_cleanup2()
{ unlink(lfname); 
}

void cache_cleanup(int ret, void *arg)
{ unlink(lfname); 
}

FILE *open_FILE(char *filename, char *mode);
extern char *macify_name(char *s);

void cache_list::create_lcache()
{
#ifdef __WATCOMC__
  char *prefix="c:\\";
#else
  char *prefix="/tmp/";     // for UNIX store lisp cache in tmp dir
  int flags=O_CREAT | O_RDWR;
#endif
 
  int cfail=1,num=0;
  do
  {
    sprintf(lfname,"%slcache%02d.tmp",prefix,num);

#if defined( __WATCOMC__ ) || defined( __POWERPC__ )
    unlink(lfname);
#ifdef __POWERPC__
		macify_name(lfname);
#endif
    FILE *fp=fopen(lfname,"wb");
    if (fp) 
    {
    	fclose(fp);
    	cfail=0;
		}
#else
    int fd=open(lfname,flags,S_IRWXU | S_IRWXG | S_IRWXO);     // can we get exclusive rights to this file?
    if (fd<0) close(fd); else cfail=0;
#endif

    if (cfail)
      num++;

  } while (cfail && num<15);
  if (cfail)
  {
    fprintf(stderr,"Error : Unable to open cache file for compiled code.\n"
	    "        Please delete all files named lcacheXX.tmp\n"
	    "        and make sure you have write permission to\n"
	    "        directory (%s)\n",prefix);
    exit(0);
  } else
  {
    exit_proc(cache_cleanup,cache_cleanup2);    // make sure this file gets deleted on exit..
  }
  lcache_number=-1;
}

cache_list::cache_list()
{
  // start out with a decent sized cache buffer because it's going to get allocated anyway. 
  total=0; 
  list=NULL;
  last_registered=-1;   
  cache_file=fp=NULL; 
  last_access=1;
  used=ful=0;
  last_dir=NULL;
  last_file=-1;
  prof_data=NULL;
  cache_read_file=NULL;
  create_lcache();
}

cache_list::~cache_list()
{ 
}

void cache_list::empty()
{
  for (int i=0;i<total;i++)
  {
    if (list[i].file_number>=0 && list[i].last_access!=-1)
      unmalloc(&list[i]);
  }
  jfree(list);
  if (fp) delete fp;
  if (last_dir) delete last_dir;
  if (cache_file)
  {
    delete cache_file;
    cache_file=NULL;    
  }
  unlink(lfname); 

  if (prof_data)
  {
    delete prof_data;
    prof_data=NULL;
  }

  total=0;                    // reinitalize
  list=NULL;
  last_registered=-1;   
  cache_file=fp=NULL; 
  if (cache_read_file) 
  {
    delete cache_read_file;
    cache_read_file=NULL;
  }

  last_access=1;
  used=ful=0;
  last_dir=NULL;
  last_file=-1;
  prof_data=NULL;
}

void cache_list::locate(cache_item *i, int local_only)
{
//  dprintf("cache in %s, type %d, offset %d\n",crc_man.get_filename(i->file_number),i->type,i->offset);
  if (i->file_number!=last_file)
  {
    if (fp) delete fp;
    if (last_dir) delete last_dir; 
    if (local_only)
      fp=new jFILE(crc_man.get_filename(i->file_number),"rb");
    else
      fp=open_file(crc_man.get_filename(i->file_number),"rb");


    if (fp->open_failure())
    {
      printf("Ooch. Could not open file %s\n",crc_man.get_filename(i->file_number));
      delete fp;
      exit(0);
    }

    last_offset=-1;
    last_dir=new spec_directory(fp);
    last_file=i->file_number;
  }
  if (i->offset!=last_offset)
  {
    fp->seek(i->offset,SEEK_SET);
    last_offset=i->offset;
  }
  used=1;
}

long cache_list::alloc_id()
{
  int id;
  if (prof_data)
  {
    the_game->show_help("new id allocated, cache profiling turned off\n");
    prof_uninit();
  }

  // see if we previously allocated an id, if so check the next spot in the array
  // otherwise we will have to scan the whole list for a free id and possible 
  // grow the list.
  if (last_registered+1<total && list[last_registered+1].file_number<0)
    id=last_registered+1;
  else
  {
    int i;
    cache_item *ci=list;
    for (i=0,id=-1;i<total && id<0;i++,ci++)        // scan list for a free id
    {
      if (ci->file_number<0)
        id=i;
    }

    if (id<0)                                 // if no free id's then make list bigger
    {
      int add_size=20;
      list=(cache_item *)jrealloc(list,(sizeof(cache_item)*(total+add_size)),"Cache list");
      for (i=0;i<add_size;i++)
      {
        list[total+i].file_number=-1;         // mark new entries as new
	list[total+i].last_access=-1;
	list[total+i].data=NULL;
      }
      id=total;
      if (prof_data)                          // new id's have been added old prof_data size won't work
      { jfree(prof_data); prof_data=NULL; }
      total+=add_size;
    }
  }
  last_registered=id;
  return id;
}



long cache_list::reg_lisp_block(Cell *block)
{ 
  long s;
  if (lcache_number==-1)
    lcache_number=crc_man.get_filenumber(lfname);

  if (can_cache_lisp())
  {
    if (!cache_file)
    {
      if (cache_read_file)
      {
				delete cache_read_file;
				cache_read_file=NULL;
			
				cache_file=new jFILE(lfname,"ab");	
      } else cache_file=new jFILE(lfname,"wb");	 // first time we opened

    }
    if (cache_file->open_failure()) 
    { 
      delete cache_file;
      lprint(block);
      fprintf(stderr,"Unable to open lisp cache file name %s\n",lfname);
      exit(0);
    }
  }
  int id=alloc_id(),i,fn=crc_man.get_filenumber(lfname);
  cache_item *ci=list+id;
  CHECK(id<total && list[id].file_number<0);

  ci->file_number=fn;
  ci->last_access=-1;
  ci->type=SPEC_EXTERNAL_LCACHE;
  if (!can_cache_lisp()) 
  {
    ci->data=(void *)block;                // we can't cache it out so it must be in memory
    return id;
  } 
  ci->data=NULL;                  // assume that it is in tmp memory, need to cache in on access
  ci->offset=cache_file->tell();

  s=block_size(block);
  cache_file->write_long(s);
  write_level(cache_file,block);
  return id;    
}

long cache_list::reg_object(char *filename, void *object, int type, int rm_dups)
{ 
  char *name;
  if (item_type(object)==L_CONS_CELL)      // see if we got a object with a filename included
  {
    filename=lstring_value(lcar(object));
    name=lstring_value(lcdr(object));
  }
  else name=lstring_value(object);        // otherwise should be a string
  return reg(filename,name,type,rm_dups);
}

extern int total_files_open;

long cache_list::reg(char *filename, char *name, int type, int rm_dups)
{ 
  int id=alloc_id(),i,fn=crc_man.get_filenumber(filename);
  cache_item *ci=list+id;
  CHECK(id<total && list[id].file_number<0);

  if (type==SPEC_EXTERN_SFX)    // if a extern sound effect then just make sure it's there
  {
    bFILE *check=open_file(filename,"rb");
    if (check->open_failure())
    {
      delete check;
      printf("Unable to open file '%s' for reading\n",filename);
      exit(0);
    }
    char buf[4];
    check->read(buf,4);
    delete check;
    if (memcmp(buf,"RIFF",4))
    {
      printf("File %s is not a WAV file\n",filename);
      exit(0);
    }
    ci->file_number=fn;
    ci->last_access=-1;
    ci->data=NULL;
    ci->offset=0;
    ci->type=type;  
    return id;  
  }

  spec_directory *sd=sd_cache.get_spec_directory(filename);
  
  if (!sd)
  {
    printf("Unable to open filename %s for requested item %s\n",filename,name);
    exit(0);
  }

  spec_entry *se;
  if (type!=-1)
  {
    se=sd->find(name,type);
    if (!se) se=sd->find(name);
  }
  else se=sd->find(name);
  
  
  if (!se)
  {    
    printf("No such item %s in file %s\n",name,filename);
    exit(0);
  }
  else if (type>=0 && (type!=se->type && ((type!=SPEC_CHARACTER2 && type!=SPEC_CHARACTER)  ||
					  (se->type!=SPEC_CHARACTER && se->type!=SPEC_CHARACTER2))))
  {
    printf("Item %s of file %s should be type %s\n",name,filename,spec_types[type]);
    exit(0);
  }

      

  if (rm_dups)
  {
    for (i=0;i<total;i++)
      if (list[i].file_number==fn && list[i].offset==se->offset)
        return i;
  }

  ci->file_number=fn;
  ci->last_access=-1;
  ci->data=NULL;
  ci->offset=se->offset;
  ci->type=se->type;  
  return id;  
}


void cache_list::normalize()
{
  int j;
  cache_item *ci=list;
  last_access=-1;
  for (j=0;j<total;j++,ci++)
  {
    if (ci->last_access>=0)
      ci->last_access=ci->last_access>>16;        // shift everything over by 16
    if (ci->last_access>last_access)            //  and find new largest timestamp
      last_access=ci->last_access;
  }
  last_access++;
}

backtile *cache_list::backt(int id)
{
  cache_item *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");

  if (me->last_access>=0)  
  {
    touch(me);
    return (backtile *)me->data;
  }
  else
  {
    touch(me);
    locate(me);
    int sp=alloc_space; alloc_space=ALLOC_SPACE_CACHE;
    me->data=(void *)new backtile(fp);
    alloc_space=sp;
    last_offset=fp->tell();
    return (backtile *)me->data;
  }  
}


foretile *cache_list::foret(int id)
{
  cache_item *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");

  if (me->last_access>=0)  
  {
    touch(me);
    return (foretile *)me->data;
  }
  else
  {
    touch(me);
    locate(me);
    int sp=alloc_space; alloc_space=ALLOC_SPACE_CACHE;
    me->data=(void *)new foretile(fp);
    alloc_space=sp;
    last_offset=fp->tell();
    return (foretile *)me->data;
  }  
}

figure *cache_list::fig(int id)
{
  cache_item *me=list+id;
//  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");
  if (me->last_access>=0)  
  {
    touch(me);
    return (figure *)me->data;
  }
  else
  {
    touch(me);
    locate(me);
    int sp=alloc_space; alloc_space=ALLOC_SPACE_CACHE;
    me->data=(void *)new figure(fp,me->type);
    alloc_space=sp;
    last_offset=fp->tell();
    return (figure *)me->data;
  }  
}

image *cache_list::img(int id)
{
  cache_item *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");
  if (me->last_access>=0)  
  {
    touch(me);
    return (image *)me->data;
  }
  else
  {
    touch(me);                                           // hold me, feel me, be me!
    locate(me);
    int sp=alloc_space; alloc_space=ALLOC_SPACE_CACHE;
    image *im=new image(fp);
    alloc_space=sp;
    me->data=(void *)im;
    last_offset=fp->tell();

    return (image *)me->data;
  }  
}

sound_effect *cache_list::sfx(int id)
{
  cache_item *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");
  if (me->last_access>=0)  
  {
    touch(me);                                           // hold me, feel me, be me!
    return (sound_effect *)me->data;
  }
  else
  {
    touch(me);                                           // hold me, feel me, be me!
    char *fn=crc_man.get_filename(me->file_number);
    int sp=alloc_space; alloc_space=ALLOC_SPACE_CACHE;
    me->data=(void *)new sound_effect(fn);
    alloc_space=sp;
    return (sound_effect *)me->data;
  }  
}


part_frame *cache_list::part(int id)
{
  cache_item *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");
  if (me->last_access>=0)  
  {
    touch(me);                                           // hold me, feel me, be me!
    return (part_frame *)me->data;
  }
  else
  {
    touch(me);
    locate(me);
    int sp=alloc_space; alloc_space=ALLOC_SPACE_CACHE;
    me->data=(void *)new part_frame(fp);
    alloc_space=sp;
    last_offset=fp->tell();
    return (part_frame *)me->data;
  }  
}


Cell *cache_list::lblock(int id)
{
  cache_item *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");
  if (!can_cache_lisp()) return (Cell *)me->data;
  if (me->last_access>=0)  
  {
    touch(me);
    return (Cell *)me->data;
  }
  else
  {
    if (cache_file)
    {
      delete cache_file;
      cache_file=NULL;
    } 
    touch(me);

    if (!cache_read_file)
    {
      cache_read_file=new jFILE(crc_man.get_filename(me->file_number),"rb");
      
      int cache_size=80*1024;                   // 80K
      cache_read_file->set_read_buffer_size(cache_size); 
      uchar mini_buf;
      cache_read_file->read(&mini_buf,1);       // prime the buffer
    }

    cache_read_file->seek(me->offset,0);

    int sp=alloc_space; alloc_space=ALLOC_SPACE_CACHE;

    long size=cache_read_file->read_long();
    void *space;

    if (size)
      space=jmalloc(size,"cached lisp block");
    else space=NULL;

    int cs=current_space;
    use_user_space(space,size);    
    load_block(cache_read_file);
    current_space=cs;
    
    alloc_space=sp;
    if (size)
      me->data=(Cell *)space;
    else me->data=NULL;
    return (Cell *)me->data;
  }  
}

cache_list cash;

void free_up_memory()
{
  cash.free_oldest();
}

void cache_list::free_oldest()
{
  long i,old_time=last_access;
  cache_item *ci=list,*oldest=NULL;
  ful=1;

  for (i=0;i<total;i++,ci++)
  {
    if (ci->data && ci->last_access<old_time)
    {
      oldest=ci;
      old_time=ci->last_access;
    }
  }
  if (oldest)
  {
    dprintf("mem_maker : freeing %s\n",spec_types[oldest->type]);
    unmalloc(oldest);    
  }
  else
  {
    close_graphics();
    printf("Out of memory, please remove any TSR's device drivers you can\n");
    mem_report("out_of_mem");
    exit(0);
  }         
}


void cache_list::show_accessed()
{
  int old=last_access,new_old_accessed;
  cache_item *ci,*new_old;
  
  do
  {
    new_old_accessed=-1;
    new_old=NULL;
    ci=list;
    for (int i=0;i<total;i++,ci++)  
    {
      if (ci->last_access<old && ci->last_access>0 && ci->last_access>new_old_accessed)    
      {
	new_old_accessed=ci->last_access;
        new_old=ci;    
      }
    }
    if (new_old)
    {
      ci=new_old;
      old=ci->last_access;
      printf("type=(%20s) file=(%20s) access=(%6d)\n",spec_types[ci->type],
	     crc_man.get_filename(ci->file_number),
	     ci->last_access);
    }
  } while (new_old);
}


int cache_list::loaded(int id)
{
  cache_item *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");
  if (me->last_access>=0)  
    return 1;
  else return 0;
}



char_tint *cache_list::ctint(int id)
{
  cache_item *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id" && me->type==SPEC_PALETTE);
  if (me->last_access>=0)  
  {
    touch(me);
    return (char_tint *)me->data;
  }
  else
  {
    touch(me);
    locate(me);
    int sp=alloc_space; alloc_space=ALLOC_SPACE_CACHE;
    me->data=(void *)new char_tint(fp);
    alloc_space=sp;
    last_offset=fp->tell();
    return (char_tint *)me->data;
  }    
}




