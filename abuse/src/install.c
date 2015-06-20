#define NO_LIBS 1
#include "lisp.c"
#include "trig.c"
#include "lisp_gc.c"
#include "lisp_opt.c"
#include "text_gui.c"


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


long c_caller(long number, void *arg)  // exten c function switches on number
{
  switch (number)
  {
    case 0 : 
    {
      char cd[100];
      getcwd(cd,100);
      int t=change_dir(lstring_value(CAR(arg)));
      change_dir(cd);
      return t;
    } break;    
    case 1 :
    {
      if (change_dir(lstring_value(eval(CAR(arg)))))
        return 1;
      else return 0;
    } break; 
    case 2 :
    {
      return K_avail(lstring_value(CAR(arg)));
    } break;
    case 3 :
    {
      void *title=eval(CAR(arg)); arg=CDR(arg);   p_ref r1(title);
      void *source=eval(CAR(arg)); arg=CDR(arg);  p_ref r2(source);
      void *dest=eval(CAR(arg)); arg=CDR(arg);    p_ref r3(dest);

      return nice_copy(lstring_value(title),lstring_value(source),lstring_value(dest));
    } break;
    case 4 :
    {
      if (access(lstring_value(eval(CAR(arg))),R_OK)==0)
        return 1;
      else
        return 0;
    } break;
  }
  return 0;
}


int nice_copy(char *title, char *source, char *dest);

 void *l_obj_get(long number) { return NULL; }  // exten lisp function switches on number
 void l_obj_set(long number, void *arg) { ; }  // exten lisp function switches on number
 void l_obj_print(long number) { ; }  // exten lisp function switches on number

void clisp_init() 
{                      // external initalizer call by lisp_init()
  void *platform=make_find_symbol("platform");
  set_symbol_value(platform,make_find_symbol(plat_name[detect_platform()]));  
  add_lisp_function("system",1,1,                   0);
  add_lisp_function("split_filename",2,2,           1);
  add_lisp_function("convert_slashes",2,2,          2);
  add_lisp_function("make_dir",1,1,                 3);
  add_lisp_function("extension",1,1,                4);
  add_lisp_function("nice_input",3,3,               5);  // title, prompt, default -> returns input
  add_lisp_function("nice_menu",3,3,                6);  // title, menu_title, list -> return selection number
  add_lisp_function("show_yes_no",4,4,              7);
  add_lisp_function("get_cwd",0,0,                  8);
  add_lisp_function("getenv",1,1,                   9);
  add_lisp_function("modify_install_path",1,1,     10);
 

  add_c_bool_fun("dir_exsist",1,1,                  0);
  add_c_bool_fun("chdir",1,1,                       1); 
  add_c_function("K_avail",1,1,                     2);  // path
  add_c_bool_fun("nice_copy",3,3,                   3);  // source file, dest file
  add_c_bool_fun("file_exsist",1,1,                 4);
  
  char esc_str[2]={27,0};
  set_symbol_value(make_find_symbol("ESC_string"),new_lisp_string(esc_str));
}




void *l_caller(long number, void *arg) 
{
  p_ref r1(arg);
  void *ret=NULL;
  switch (number)
  {
    case 0 :
    { system(lstring_value(eval(CAR(arg)))); } break;
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
      char cd[100];
      getcwd(cd,100);

      char name_so_far[100];
      char *dir=lstring_value(eval(CAR(arg)));
      char *d,ch;
      d=dir;
      int err=0;
      while (*d && !err)
      {
	if ((*d=='\\' || *d=='/') && d!=dir && *(d-1)!=':')
	{
	  ch=*d;
	  *d=0;
	  if (!change_dir(dir))
	    if (make_dir(dir)!=0)
	      err=1;

	  *d=ch;
	  
	}
	d++;
      }
      change_dir(cd);

      if (err)
        ret=NULL;
      else ret=true_symbol;
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
      void *tit=eval(CAR(arg));  arg=CDR(arg);
      p_ref r1(tit);
      void *prompt=eval(CAR(arg));  arg=CDR(arg);
      p_ref r2(prompt);
      void *def=eval(CAR(arg));  arg=CDR(arg);
      p_ref r3(def);

      return nice_input(lstring_value(tit),lstring_value(prompt),lstring_value(def));
    } break;
    case 6 :
    {
      return nice_menu(CAR(arg),CAR(CDR(arg)),CAR(CDR(CDR(arg))));
    } break;
    case 7 :
    {
      return show_yes_no(CAR(arg),CAR(CDR(arg)),CAR(CDR(CDR(arg))),CAR(CDR(CDR(CDR(arg)))));
    } break;
    case 8 :
    {
      char cd[150];
      getcwd(cd,100);
      return new_lisp_string(cd);
    } break;
    case 9 :
    {
      return new_lisp_string(getenv(lstring_value(eval(CAR(arg)))));
    } break;
    case 10 :
    {
      char str[200];
      strcpy(str,lstring_value(eval(CAR(arg))));
      modify_install_path(str);
      return new_lisp_string(str);
    } break;
  }
  return ret;
}


 // exten lisp function switches on number


main(int argc, char **argv)
{
  lisp_init(100000,0x2000);
  char *use_file="install.lsp";
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

