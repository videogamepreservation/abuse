#define NO_LIBS 1
#include <unistd.h>
#include "lisp.c"
#include "trig.c"
#include "lisp_gc.c"
#include "lisp_opt.c"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef __WATCOMC__
#include <sys\types.h>
#include <direct.h>
#define make_dir(dir) mkdir(dir)
#else

#include <sys/stat.h>
#define make_dir(dir) mkdir(dir,511)
#endif



enum { LINUX, WATCOM, AIX, SUN, SGI };

char *plat_names[] = {"Linux (SVGA & X11)",
		      "Watcom for MS-DOS",
		      "IBM AIX for RS6000's",
		      "Sun OS",
		      "Silicon Graphics"};

char *plat_name[] = {"LINUX","WATCOM","AIX","SUN","SGI"};


int detect_platform()
{
#ifdef __linux__
  return LINUX;
#endif

#ifdef __WATCOMC__
  return WATCOM;
#endif

#ifdef _AIX
  return AIX;
#endif

#ifdef sun
  return SUN;
#endif

#ifdef SUN3
  return SUN;
#endif

#ifdef SUN4
  return SUN;
#endif

#ifdef __sgi
  return SGI;
#endif

  printf("Cannot detect platform\n");
  exit(1);

  return 0;
}


 void *l_obj_get(long number) { return NULL; }  // exten lisp function switches on number
 void l_obj_set(long number, void *arg) { ; }  // exten lisp function switches on number
 void l_obj_print(long number) { ; }  // exten lisp function switches on number

void clisp_init() 
{                      // external initalizer call by lisp_init()
  void *platform=make_find_symbol("platform");
  set_symbol_value(platform,make_find_symbol(plat_name[detect_platform()]));  
  add_lisp_function("get_depends",3,3,              0);
  add_lisp_function("split_filename",2,2,           1);
  add_lisp_function("convert_slashes",2,2,          2);
  add_lisp_function("make_dir",1,1,                 3);
  add_lisp_function("extension",1,1,                4);
  add_lisp_function("system",1,1,                   5);
  add_lisp_function("get_cwd",0,0,                  6);

  add_lisp_function("mangle_oname",1,1,             7);

  add_c_bool_fun("chdir",1,1,                       1); 
}


#ifdef __WATCOMC__
#include <dos.h>
#endif

int change_dir(char *path)
{
#ifdef __WATCOMC__
  unsigned cur_drive;
  _dos_getdrive(&cur_drive);
  if (path[1]==':')
  {
    unsigned total;
    _dos_setdrive(toupper(path[0])-'A'+1,&total);


    unsigned new_drive;
    _dos_getdrive(&new_drive);

    if (new_drive!=toupper(path[0])-'A'+1)
    {
      return 0;
    }

    path+=2;
  }
  
  int er=chdir(path);
  if (er)
  {
    unsigned total;
    _dos_setdrive(cur_drive,&total);
  }
  return !er;
#else
  int ret=chdir(path);    // weird
  ret=chdir(path);
  return ret==0;
#endif  
}

long c_caller(long number, void *arg) // exten c function switches on number
{
  switch (number)
  {
    case 1 :
    {
      if (change_dir(lstring_value(eval(CAR(arg)))))
        return 1;
      else return 0;
    } break;
  }
}

void get_depends(char *fn, char *slash, void *ilist, void *&ret)
{
  p_ref r8(ret);  
  p_ref r1(ilist);
  void *v=ret;
  p_ref r2(v);
  for (;v;v=CDR(v)) 
    if (!strcmp(fn,lstring_value(CAR(v)))) return ;     // check to see if file already in list

  char tmp_name[200];  
  strcpy(tmp_name,fn);
  FILE *fp=fopen(fn,"rb");
  if (!fp)
  {
    for (v=ilist;!fp && v;v=CDR(v))     
    {
      sprintf(tmp_name,"%s%s%s",lstring_value(CAR(v)),slash,fn);
      for (void *v=ret;v;v=CDR(v)) 
        if (!strcmp(tmp_name,lstring_value(CAR(v)))) return ; 
      // check to see if file already in list
      fp=fopen(tmp_name,"rb");
    }
  }
  if (fp)
  {
    push_onto_list(new_lisp_string(tmp_name),ret);

    char line[200];
    while (!feof(fp))
    {
      fgets(line,200,fp);
      if (!feof(fp))
      {
	if (memcmp(line,"#include",8)==0)
	{ 
	  char *ch,*ch2;
	  for (ch=line+8;*ch==' ' || *ch=='\t';ch++);
	  if (*ch=='"')
	  {
	    ch++;
	    for (ch2=line;*ch!='"';ch++,ch2++)
	    { *ch2=*ch; }
	    *ch2=0;
	    get_depends(line,slash,ilist,ret);
	  }
	}
      }
    }
    fclose(fp);
  }
}

void *l_caller(long number, void *arg) 
{
  p_ref r1(arg);
  void *ret=NULL;
  switch (number)
  {
    case 0 :
    {
      void *fn=eval(CAR(arg));  arg=CDR(arg);
      p_ref r1(fn);
      void *sl=eval(CAR(arg));   arg=CDR(arg);
      p_ref r2(sl);

      void *ilist=eval(CAR(arg));
      p_ref r3(ilist);

      char filename[200];
      strcpy(filename,lstring_value(fn));

      char slash[10];
      strcpy(slash,lstring_value(sl)); 

      get_depends(filename,slash,ilist,ret);
      void *v=ret;
      if (v && CDR(v))
      {
	for (;CDR(CDR(v));v=CDR(v)); CDR(v)=NULL;  //chop of self
      }
    } break;
    case 1 :
    {
      void *fn=eval(CAR(arg));  arg=CDR(arg);
      p_ref r1(fn);
      char *current_dir=lstring_value(eval(CAR(arg)));
      char *filename=lstring_value(fn);

      char *last=NULL,*s=filename,*dp;
      char dir[200],name[200];
      while (*s) { if (*s=='\\' || *s=='/') last=s+1; s++; }
      if (last)
      {
	for (dp=dir,s=filename;s!=last;dp++,s++) { *dp=*s; }
	*dp=0;
	strcpy(name,last);
      } else
      {
	strcpy(dir,current_dir);
	strcpy(name,filename);
      }
      void *cs=(void *)new_cons_cell();
      p_ref r24(cs);
      ((cons_cell *)cs)->car=new_lisp_string(dir);
      ((cons_cell *)cs)->cdr=new_lisp_string(name);
      ret=cs;
    } break;
    case 2 :
    {
      void *fn=eval(CAR(arg)); arg=CDR(arg);
      p_ref r1(fn);
      char *slash=lstring_value(eval(CAR(arg)));
      char *filename=lstring_value(fn);

      char tmp[200],*s=filename,*tp;
      
      for (tp=tmp;*s;s++,tp++)
      {
	if (*s=='/' || *s=='\\') 
	{
	  *tp=*slash;
//	  if (*slash=='\\') 
//	  { tp++; *tp='\\'; }
	}
	else *tp=*s;
      }
      *tp=0;
      ret=new_lisp_string(tmp);
    } break;
    case 3 :
    {
      char name_so_far[100];
      char *dir=lstring_value(eval(CAR(arg)));
      char *d,ch;
      d=dir;
      while (*d)
      {
	if (*d=='\\' || *d=='/')
	{
	  ch=*d;
	  *d=0;
	  make_dir(dir);
	  *d=ch;
	  
	}
	d++;
      }
      ret=NULL;
    } break;
    case 4 :
    {
      char *fn=lstring_value(eval(CAR(arg)));
      char *l=NULL,*s=fn;
      while (*s) { if (*s=='.') l=s; s++; }
      if (l) ret=new_lisp_string(l);
      else ret=new_lisp_string("");
    } break;
    case 5 :
    {
      ret=new_lisp_number(system(lstring_value(eval(CAR(arg)))));
    } break;
    case 6 :
    {
      char cd[150];
      getcwd(cd,100);
      return new_lisp_string(cd);
    } break;
    case 7 :
    {
      char *fn=lstring_value(eval(CAR(arg)));
      uchar c1=0,c2=0,c3=0,c4=0;
      while (*fn)
      {
	c1+=*fn;
	c2+=c1;
	c3+=c2;
	c4+=c3;
	fn++;
      }
      char st[15];
      sprintf(st,"%02x%02x%02x%02x",c1,c2,c3,c4);
      return new_lisp_string(st);            
    } break;
  }
  return ret;
}


 // exten lisp function switches on number


main(int argc, char **argv)
{
  lisp_init(5*1000*1024,5*1000*1024);
  char *use_file="maker.lsp";
  for (int i=1;i<argc;i++)
  {
    if (!strcmp(argv[i],"-f"))
    {
      i++;
      use_file=argv[i];
    }
  }

  char prog[100],*s;
  sprintf(prog,"(compile-file \"%s\")\n",use_file);
  s=prog;
  if (!eval(compile(s)))
  {
    printf("unable to open file %s",use_file);
    exit(0);
  }
  return 0;
}





