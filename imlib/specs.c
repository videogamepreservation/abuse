#include "image.hpp"
#include "palette.hpp"
#include "specs.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include "system.h"
#include "jmalloc.hpp"
#include <math.h>
#include "dprint.hpp"
#ifndef __POWERPC__
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef __POWERPC__
extern char *macify_name(char *s);
#endif

char *spec_types[]={"Invalid type",             // 0
                    "Color table",              // 1
                    "Palette",                  // 2
                    "Invalid Type",             // 3
                    "Image",                    // 4
	            			"Fore Tile",
                    "Back Tile",
                    "Character",
                    "8 Morph",
                    "16 Morph",
		    "Grue objs",
		    "Extern WAV",
		    "DMX MUS",
		    "Patched morph",
		    "Normal file",
		    "Compress1 file",
		    "Vector Image",
		    "Light list",
		    "Grue fgmap",
		    "Grue bgmap",
		    "Data array",
		    "Character2",
		    "Particle",
		    "Extern lcache"
};


int total_files_open=0;
char spec_main_file[100];

static char *spec_prefix=NULL;
static char *save_spec_prefix=NULL;

static jFILE spec_main_jfile((FILE*)0);
static int spec_main_fd = -1;
static long spec_main_offset = -1;
static spec_directory spec_main_sd;

static int fast_load_fd = -1;
static int fast_load_mode = 0;

void set_filename_prefix(char *prefix)
{
  if (spec_prefix) { jfree(spec_prefix); }
  if (prefix) 
  {
    spec_prefix=strcpy((char *)jmalloc(strlen(prefix)+2,"prefix_name"),prefix);
    int len=strlen(prefix);
    if (prefix[len-1]!='\\' && prefix[len-1]!='/')
    {
      spec_prefix[len]='/';
      spec_prefix[len+1]=0;
    }
  }

  else spec_prefix=NULL;
}

char *get_filename_prefix()
{
  return spec_prefix;
}


void set_save_filename_prefix(char *save_prefix)
{
  if (save_spec_prefix) { jfree(save_spec_prefix); }
  if (save_prefix) 
  {
    save_spec_prefix=strcpy((char *)jmalloc(strlen(save_prefix)+1,"prefix_name"),save_prefix);
    int len=strlen(save_prefix);
    if (save_prefix[len-1]!='\\' && save_prefix[len-1]!='/')
    {
      save_spec_prefix[len]='/';
      save_spec_prefix[len+1]=0;
    }
  }
  else save_spec_prefix=NULL;
}

char *get_save_filename_prefix()
{
  return save_spec_prefix;
}

int search_order=SPEC_SEARCH_OUTSIDE_INSIDE;

static void (*no_space_handle_fun)()=NULL;

void set_no_space_handler(void (*handle_fun)())
{
  no_space_handle_fun=handle_fun;
}


bFILE::bFILE()
{
  rbuf_size=8192;
  rbuf=(unsigned char *)jmalloc(rbuf_size,"File read buffer");
  rbuf_start=rbuf_end=0;  

  wbuf_size=8192;
  wbuf=(unsigned char *)jmalloc(wbuf_size,"File write buffer");
  wbuf_end=0;  
}

bFILE::~bFILE()
{ 
  if (rbuf) jfree(rbuf);
  flush_writes();
  if (wbuf) jfree(wbuf);
}

int bFILE::flush_writes() 
{
  if (wbuf_end!=0)
  {
    long ret=unbuffered_write(wbuf,wbuf_end);
    if (ret!=wbuf_end && no_space_handle_fun)
      no_space_handle_fun();
      
    wbuf_end=0;
    return ret;
  }
  return 0;
}

int bFILE::read(void *buf, size_t count)       // returns number of bytes read, calls unbuffer_read
{
  if (!allow_read_buffering()) 
    return unbuffered_read(buf,count);

  int total_read=0,error=0;
  if (!count) return 0;
  while (count && !error)
  {
    if (rbuf_start<rbuf_end)
    {
      int avail_size=rbuf_end-rbuf_start;
      int copy_size=avail_size>count ? count : avail_size;
      memcpy(buf,rbuf+rbuf_start,copy_size);
      buf=(void *)(((unsigned char *)buf)+copy_size);
      rbuf_start+=copy_size;
      if (rbuf_start>=rbuf_end)    
      {
				if (rbuf_end!=rbuf_size)  // buffer wasn't full before so there is no way we can complete read
				  error=1;
				rbuf_start=rbuf_end=0;
      }
      total_read+=copy_size;
      count-=copy_size;
    } else
    {
      rbuf_end=unbuffered_read(rbuf,rbuf_size);
      if (rbuf_end==0) error=1;
      rbuf_start=0;
    }
  }
  return total_read;
}


int bFILE::write(void *buf, size_t count)      // returns number of bytes written
{ 
  if (allow_write_buffering())
  {
    int total_written=0;
    while (count)
    {
      int copy_size=wbuf_end+count<=wbuf_size ? count :  wbuf_size-wbuf_end;           
      memcpy(wbuf+wbuf_end,buf,copy_size);
      wbuf_end+=copy_size;
      count-=copy_size;
      buf=(void *)(((char *)buf)+copy_size);
      if (wbuf_end==wbuf_size)
        if (flush_writes()!=wbuf_size)
	  return total_written;
      
      total_written+=copy_size;      
    }
    return total_written;
  } else
  {
    long ret=unbuffered_write(buf,count);
    if (ret!=count && no_space_handle_fun)
      no_space_handle_fun();
  }
  return 0;
}

int bFILE::seek(long offset, int whence) // whence=SEEK_SET, SEEK_CUR, SEEK_END, ret=0=success
{  
//    rbuf_start=rbuf_end=0;
//    unbuffered_seek(offset,SEEK_SET);

  long realpos=unbuffered_tell();
  long curpos=realpos-rbuf_end+rbuf_start;
  if (whence==SEEK_CUR) offset+=curpos;
  else if (whence==SEEK_END) offset=file_size()-offset;

  if (offset<realpos-rbuf_end || offset>=realpos)
  {
    rbuf_start=rbuf_end=0;
    unbuffered_seek(offset,SEEK_SET);
  } else  
    rbuf_start=rbuf_end-(realpos-offset);
  return 1;
}

int bFILE::tell()
{
  return unbuffered_tell()-rbuf_end+rbuf_start+
         wbuf_end;    // if this a write file, add on how much we've written
}

int bFILE::allow_read_buffering() { return 1; }
int bFILE::allow_write_buffering() { return 1; }

void bFILE::set_read_buffer_size(long size)
{
  unbuffered_seek(tell(),SEEK_SET);
  rbuf_start=rbuf_end=0;
  if (rbuf)
    jfree(rbuf);
  rbuf_size=size;
  rbuf=(unsigned char *)jmalloc(rbuf_size,"File buffer");
}

void set_spec_main_file(char *filename, int Search_order)
{
  dprintf("Specs : main file set to %s\n",filename);
  strcpy(spec_main_file,filename);
  search_order=Search_order;
  
#ifdef __POWERPC__
  spec_main_jfile.open_external(filename,"rb",O_BINARY|O_RDONLY);
  spec_main_fd = spec_main_jfile.get_fd();
  spec_main_sd.startup(&spec_main_jfile);
#else
  spec_main_jfile.open_external(filename,"rb",O_RDONLY);
  spec_main_fd = spec_main_jfile.get_fd();
  spec_main_sd.startup(&spec_main_jfile);
#endif
}

void fast_load_start_recording(char *filename)
{
#ifdef __POWERPC__
  fast_load_fd = ::open(macify_name(filename),O_BINARY|O_CREAT|O_RDWR);
#else
  fast_load_fd = ::open(filename,O_CREAT|O_RDWR,S_IRWXU | S_IRWXG | S_IRWXO);
#endif
	fast_load_mode = 1;
}

void fast_load_stop_recording()
{
	fast_load_mode = 0;
}

void fast_load_start_reloading(char *filename)
{
#ifdef __POWERPC__
  fast_load_fd = ::open(macify_name(filename),O_BINARY|O_RDONLY);
#else
  fast_load_fd = ::open(filename,O_RDONLY);
#endif
	fast_load_mode = 2;
}

void fast_load_stop_reloading()
{
	fast_load_mode = 0;
}

jFILE::jFILE(FILE *file_pointer)                       // assumes fp is at begining of file
{
  access=0;
  fd=-1;
  file_length=0;
  start_offset=0;
  flags=JFILE_CLONED;
}

void jFILE::open_external(char *filename, char *mode, int flags)
{
  int skip_size=0;
  char tmp_name[200];
  if (spec_prefix && filename[0] != '/')
    sprintf(tmp_name,"%s%s",spec_prefix,filename);
  else strcpy(tmp_name,filename);

//  int old_mask=umask(S_IRWXU | S_IRWXG | S_IRWXO);
#ifdef __WATCOMC__
  flags|=O_BINARY;
#endif
  if (flags&O_WRONLY)
  {
    if ((flags&O_APPEND)==0) 
    {
      skip_size=1;
      int errval = unlink(tmp_name);
    }

    flags-=O_WRONLY;
    flags|=O_CREAT|O_RDWR;

#ifdef __POWERPC__
		fd=open(macify_name(tmp_name),flags);
#else
    fd=open(tmp_name,flags,S_IRWXU | S_IRWXG | S_IRWXO);
#endif
  } else 
#ifdef __POWERPC__
    fd=open(macify_name(tmp_name),flags);
#else
    fd=open(tmp_name,flags);
#endif

//  umask(old_mask);
  if (fd>=0 && !skip_size)
  {
    file_length=lseek(fd,0,SEEK_END);
    if ((flags&O_APPEND)==0)
      lseek(fd,0,SEEK_SET);
    else
    	current_offset = file_length;
    start_offset=0;
  } else 
  {
    file_length=0;
    start_offset=0;
  }
}


class null_file : public bFILE     // this file type will use virtual opens inside of a spe
{
  public :
  virtual int open_failure() { return 1; }
  virtual int unbuffered_read(void *buf, size_t count)   { return 0; }
  virtual int unbuffered_write(void *buf, size_t count)  { return 0; }
  virtual int unbuffered_seek(long offset, int whence)   { return 0; }

  virtual int unbuffered_tell() { return 0; }
  virtual int file_size() { return 0; }
  virtual ~null_file() { ; }
} ; 


static bFILE *(*open_file_fun)(char *,char *)=NULL;
int (*verify_file_fun)(char *,char *)=NULL;

void set_file_opener(bFILE *(*open_fun)(char *, char *))
{
  open_file_fun=open_fun;
}

bFILE *open_file(char *filename, char *mode)
{
  if (!verify_file_fun || verify_file_fun(filename,mode))
  {
    if (open_file_fun)
      return open_file_fun(filename,mode);
    else return new jFILE(filename,mode);
  } else return new null_file;
}

void jFILE::open_internal(char *filename, char *mode, int flags)
{
  int wr=0;
  for (char *s=mode;*s;s++) 
    if (toupper(*s)=='A' || toupper(*s)=='W')
      wr=1;

  if (wr)
    fd=-1;                 // only allow extern file openings for writing
  else
  {
   	fd = spec_main_fd;
    if (fd>=0)                    // if we were able to open the main file, see if it's in there
    {
      start_offset=0;
      spec_entry *se=spec_main_sd.find(filename);
      if (se)    
      {
				start_offset=se->offset;
				current_offset = 0;
				file_length=se->size;
				rbuf_start=rbuf_end=0;
      } else 
      {
				close(fd);
				fd=-1;
      }
    }  
  }
}

jFILE::jFILE(char *filename, char *access_string)      // same as fopen parameters
{
 flags=access=0;  
 char *s=access_string;
  for (;*s;s++) 
    if (toupper(*s)=='R') access=O_RDONLY;

  for (s=access_string;*s;s++) 
    if (toupper(*s)=='W')      
      if (access)
        access=O_RDWR;
      else access=O_WRONLY;

  for (s=access_string;*s;s++) 
    if (toupper(*s)=='A')      
      access|=O_APPEND|O_WRONLY;

  file_length=start_offset=-1;
  current_offset = 0;

  fd=-1;
  if (search_order==SPEC_SEARCH_OUTSIDE_INSIDE)
    open_external(filename,access_string,access);

  if (fd<0)
    open_internal(filename,access_string,access);

  if (fd<0 && search_order==SPEC_SEARCH_INSIDE_OUTSIDE)
    open_external(filename,access_string,access);

  total_files_open++;
}

jFILE::~jFILE()
{
  flush_writes();
  if (fd>=0 && !(flags&JFILE_CLONED))
  {
    total_files_open--;
    if (fd != spec_main_fd)
	    close(fd);
  }
}

int jFILE::unbuffered_tell()
{
//	int ret = ::lseek(fd,0,SEEK_CUR) - start_offset;
//	if (ret != current_offset)
//		fprintf(stderr,"Bad tell %d\n",current_offset);
	return current_offset;
}

int jFILE::unbuffered_read(void *buf, size_t count)
{
	long len;

	if (fd == spec_main_fd)
	{
		switch (fast_load_mode)
		{
		case 0:
			if (current_offset+start_offset != spec_main_offset)
				spec_main_offset = lseek(fd, start_offset+current_offset, SEEK_SET);
			
			len = ::read(fd,(char*)buf,count);
			break;
		case 1:
			if (current_offset+start_offset != spec_main_offset)
				spec_main_offset = lseek(fd, start_offset+current_offset, SEEK_SET);
			
			len = ::read(fd,(char*)buf,count);
			::write(fast_load_fd,(char*)&len,sizeof(len));
			::write(fast_load_fd,(char*)buf,count);
			break;
		case 2:
			::read(fast_load_fd,(char*)&len,sizeof(len));
			len = ::read(fast_load_fd,(char*)buf,len);
			break;
		}
		
		spec_main_offset += len;
	}
	else
	{
		switch (fast_load_mode)
		{
		case 0:
	  	len = ::read(fd,(char*)buf,count);
	  	break;
		case 1:
	  	len = ::read(fd,(char*)buf,count);
			::write(fast_load_fd,(char*)&len,sizeof(len));
			::write(fast_load_fd,(char*)buf,count);
	  	break;
		case 2:
			::read(fast_load_fd,(char*)&len,sizeof(len));
			len = ::read(fast_load_fd,(char*)buf,len);
			if (count != len)
				printf("short read! %d:%d\n",current_offset,len);
	  	break;
	  }
	}
	current_offset += len;
	return len;
}

int jFILE::unbuffered_write(void *buf, size_t count)
{
  long ret = ::write(fd,(char*)buf,count);
	current_offset += ret;
	return ret;
}

int jFILE::unbuffered_seek(long offset, int whence) // whence=SEEK_SET, SEEK_CUR, SEEK_END, ret=0=success
{
	long ret;
	
	if (fast_load_mode == 2)
	{
		switch (whence)
		{
    case SEEK_SET : 
     	current_offset = start_offset+offset;
     	break;
    case SEEK_END : 
    	current_offset = start_offset+file_length-offset;
    	break;
    case SEEK_CUR : 
    	current_offset += offset;
    	break;
    default:
    	ret = -1;
    	break;
    }
		return current_offset;
	}
	
  switch (whence)
  {
    case SEEK_SET : 
    { ret = lseek(fd,start_offset+offset,SEEK_SET); } break;
    case SEEK_END : 
    { ret = lseek(fd,start_offset+file_length-offset,SEEK_SET); } break;
    case SEEK_CUR : 
    { ret = lseek(fd,offset,SEEK_CUR); } break;
    default:
    	ret = -1;
    	break;
  }
  if (ret>=0)
  {
  	current_offset = ret - start_offset;
  	if (spec_main_fd == fd)
  		spec_main_offset = ret;
  	return ret;
  }
  else
	  return -1;  // if a bad whence, then failure
}


unsigned char bFILE::read_byte()
{ unsigned char x;
  read(&x,1);
  return x;
}

unsigned short bFILE::read_short()
{ 
  unsigned short x;
  read(&x,2); 
  return int_to_local(x);
}


unsigned long bFILE::read_long()
{ 
  unsigned long x;
  read(&x,4); 
  return long_to_local(x);
}

void bFILE::write_byte(unsigned char x)
{
  write(&x,1);
}

void bFILE::write_short(unsigned short x)
{ 
  x=int_to_local(x);
  write(&x,2);
}


void bFILE::write_long(unsigned long x)
{
  x=long_to_local(x);
  write(&x,4);
}

void bFILE::write_double(double x)
{
  double a;
  write_long((long)(modf(x,&a)*(double)(1<<32-1)));
  write_long((long)a);
}

double bFILE::read_double()
{
  long a,b;
  a=read_long();
  b=read_long();
  return (double)b+a/(double)(1<<32-1);
}

spec_directory::~spec_directory()
{

  if (total)
  {
    jfree(data);
    jfree(entries);
  }
}

void spec_entry::print()
{
  printf("%15s%25s%8ld%8ld\n",spec_types[type],name,size,offset);
}

void spec_directory::calc_offsets()
{
  spec_entry **e;
  int i;
  long o=SPEC_SIG_SIZE+2;
  if (total)
  { 
    for (i=0,e=entries;i<total;i++,e++)          // calculate the size of directory info
    {
      o+=1+1+strlen((*e)->name)+1 +1 +8;
    }

    for (i=0,e=entries;i<total;i++,e++)          // calculate offset for each entry
    {
      (*e)->offset=o;
      o+=(*e)->size;
    }
  }
}

spec_entry *spec_directory::find(char *name, int type)
{
  int i;
  spec_entry **e;
  for (i=0,e=entries;i<total;i++,e++)
    if (!strcmp((*e)->name,name) && (*e)->type==type)
      return (*e);
  return NULL;
}

spec_entry *spec_directory::find(char *name)
{
  int i;
  spec_entry **e;
  for (i=0,e=entries;i<total;i++,e++)
    if (!strcmp((*e)->name,name))
      return (*e);
  return NULL;
}

long spec_directory::find_number(char *name)
{
  int i;
  spec_entry **e;
  for (i=0,e=entries;i<total;i++,e++)
    if (!strcmp((*e)->name,name))
      return i;
  return -1;
}

spec_entry *spec_directory::find(int type)
{
  int i;
  spec_entry **e;
  for (i=0,e=entries;i<total;i++,e++)
    if ((*e)->type==type)
      return (*e);
  return NULL;
}

long spec_directory::type_total(int type)
{
  int i,x=0;
  spec_entry **e;
  for (i=0,e=entries;i<total;i++,e++)
    if ((*e)->type==type) x++;
  return x;
}

long spec_directory::find_number(int type)
{
  int i;
  spec_entry **e;
  for (i=0,e=entries;i<total;i++,e++)
    if ((*e)->type==type)
      return i;
  return -1;
}

void spec_directory::print()
{
  spec_entry **se;
  int i;
  printf("[   Entry type   ][   Entry name   ][  Size  ][ Offset ]\n"); 
  for (i=0,se=entries;i<total;i++,se++)
    (*se)->print();
}


void spec_directory::startup(bFILE *fp)
{
  char buf[256];
  fp->read(buf,8);
  buf[9]=0;
  size=0;
  if (!strcmp(buf,SPEC_SIGNATURE))
  {    
    total=fp->read_short();   
    entries=(spec_entry **)jmalloc(sizeof(spec_entry *)*total,"spec_directory::entries");
    long start=fp->tell();

    int i;
    for (i=0;i<total;i++)
    {            
      fp->read(buf,2);
      long entry_size=sizeof(spec_entry)+(unsigned char)buf[1];
      entry_size=(entry_size+3)&(~3);
      fp->read(buf,(unsigned char)buf[1]);
      fp->read(buf,9);

      size+=entry_size;
    }
    data=jmalloc(size,"spec_directory::data");
    char *dp=(char *)data;
    fp->seek(start,SEEK_SET);
    for (i=0;i<total;i++)
    {            
      spec_entry *se=(spec_entry *)dp;
      entries[i]=se;

      unsigned char len,flags,type;
      fp->read(&type,1);      
      fp->read(&len,1); 
      se->type=type;
      se->name=dp+sizeof(spec_entry);
      fp->read(se->name,len);
      fp->read(&flags,1); 

      se->size=fp->read_long();
      se->offset=fp->read_long();
      dp+=((sizeof(spec_entry)+len)+3)&(~3);
    }
  }
  else 
  {
    total=0;
    data=NULL;
    entries=NULL;
  }  
}


spec_directory::spec_directory(bFILE *fp)
{ startup(fp); }

spec_directory::spec_directory(FILE *fp)
{
  jFILE jfp(fp);
  startup(&jfp);
}

spec_directory::spec_directory()
{
  size=0;
  total=0;
  data=NULL;
  entries=NULL;
}

/*
spec_directory::spec_directory(char *filename)
{
  jFILE *fp;
  if (filename)
  {
    fp=new jFILE(filename,"rb");
    if (!fp->open_failure())
      startup(fp);
    else
    {
      total=0;
      entries=NULL;
    }
    delete fp;
  } else printf("NULL filename to spec_directory::spec_directory\n");
}*/

int write_string(bFILE *fp, char *st)
{
  unsigned char length=strlen(st)+1;
  if (fp->write(&length,1)!=1) return 0;
  if (fp->write(st,length)!=length) return 0;
  return 1;
}

long spec_directory::data_start_offset()
{
  spec_entry *e;
  long o=0,i;
  for (i=0;i<total;i++) 
    return entries[i]->offset;

  return SPEC_SIG_SIZE+2;     // if no entries, then no data, but return where it would
                              // start anyway
} 

long spec_directory::data_end_offset()
{
  spec_entry **e;
  long o=0,i;
  for (i=total-1,e=entries;i>=0;i--,e++)
    return (*e)->offset+(*e)->size;

  return SPEC_SIG_SIZE+2;
} 

int spec_directory::write(bFILE *fp)
{

  char sig[SPEC_SIG_SIZE];
  unsigned short tentries;
  unsigned char flags=0;
  unsigned long offset,data_size;
  spec_entry **e;
  strcpy(sig,SPEC_SIGNATURE);

  if (fp->write(sig,sizeof(sig))!=sizeof(sig))    return 0;
  fp->write_short(total);


  int i;
  for (i=0,e=entries;i<total;i++,e++)
  {
    if (fp->write(&(*e)->type,1)!=1)                 return 0;
    if (!write_string(fp,(*e)->name))                return 0;
    flags=0;
    if (fp->write(&flags,1)!=1)                     return 0; 

    data_size=long_to_intel((*e)->size); 
    if (fp->write((char *)&data_size,4)!=4)              return 0; 
    offset=long_to_intel((*e)->offset);
    if (fp->write((char *)&offset,4)!=4)                  return 0; 

  }
  return 1; 
}

jFILE *spec_directory::write(char *filename)
{
  jFILE *fp;
  fp=new jFILE(filename,"wb");
  if (fp->open_failure()) { delete fp; return NULL; }
  if (!write(fp))
  {
    delete fp;
    return NULL;
  } else return fp;

}

unsigned short read_short(FILE *fp)
{
  unsigned short x;
  fread(&x,1,2,fp); 
  return int_to_local(x);
}

unsigned long read_long(FILE *fp)
{
  unsigned long x;
  fread(&x,1,4,fp);
  return (long)long_to_local(x);
}
void write_short(FILE *fp, unsigned short x)
{
  x=int_to_local(x);
  fwrite(&x,1,2,fp);
}

void write_long(FILE *fp, unsigned long x)
{
  x=long_to_local(x);
  fwrite(&x,1,4,fp);
}

unsigned char read_byte(FILE *fp) { return fgetc(fp)&0xff; }
void write_byte(FILE *fp, unsigned char x) { fputc(x,fp); }

unsigned short read_other_long(FILE *fp)
{
  unsigned long x;
  fread(&x,1,4,fp);
  return big_long_to_local(x);
}

unsigned long read_other_short(FILE *fp)
{
  unsigned short x;
  fread(&x,1,2,fp);
  return big_short_to_local(x);
}


void write_other_short(FILE *fp, unsigned short x)
{
  x=big_short_to_local(x);
  fwrite(&x,1,2,fp);
}

void write_other_long(FILE *fp, unsigned long x)
{
  x=big_long_to_local(x);
  fwrite(&x,1,4,fp);
}

void spec_directory::remove(spec_entry *e) 
{ 
  int i;
  for (i=0;i<total && entries[i]!=e;i++);            // find the entry in the array first
  
  if (entries[i]==e)                                 // make sre it was found
  {
    delete e;
    total--;
    for (;i<total;i++)                               // compact the pointer array
      entries[i]=entries[i+1];
    entries=(spec_entry **)jrealloc(entries,sizeof(spec_entry *)*total,"spec_directory::entries");
  }
  else
    printf("Spec_directory::remove bad entry pointer\n");
}



void spec_directory::add_by_hand(spec_entry *e) 
{ 
  total++;
  entries=(spec_entry **)jrealloc(entries,sizeof(spec_entry *)*total,"spec_directory::entries");
  entries[total-1]=e;
}

void spec_directory::delete_entries()   // if the directory was created by hand instead of by file
{
  int i;
  for (i=0;i<total;i++)
    delete entries[i];

  if (total)
    jfree(entries);
}


void note_open_fd(int fd, char *str)
{
  total_files_open++;
}

void note_close_fd(int fd)
{
  total_files_open--;
}


void list_open_fds()
{
  printf("Total open dos fds=%d\n",total_files_open);
}

