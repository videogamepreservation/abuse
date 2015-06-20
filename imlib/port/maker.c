/*
  
*/


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


int line_on,no_include;
enum { LINUX, DOS, AIX, SUN, SGI };

char *plat_names[] = {"Linux (SVGA & X11)",
		      "Watcom for MS-DOS",
		      "IBM AIX for RS6000's",
		      "Sun OS",
		      "Silicon Graphics"};

char *plat_name[] = {"LINUX","DOS","AIX","SUN","SGI"};

		      

char z[5000];

struct var_node
{
  struct var_node *next;
  char *name;
  char *subst;
} *first_var;



  

char *basename="noname";
char *imlib_objs="";
char *add_libs="";
char *ofiles="";
char *imlib_dir="../imlib";
char *plat_stuff=NULL;

void create_dir(char *dir)
{
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
  make_dir(dir);
}

void oextend(char *dir, char *word, char *new_dir, int platform, int optimize);

void store_var(char *name, char *subst)
{
  struct var_node *p;
  for (p=first_var;p && strcmp(p->name,name);p=p->next);
  if (p)  
    free(p->subst);      
  else
  {
    p=(struct var_node *)malloc(sizeof(struct var_node));
    p->name=(char *)strcpy((char *)malloc(strlen(name)+1),name);   
    p->next=first_var;
    first_var=p;
  }
  p->subst=(char *)strcpy((char *)malloc(strlen(subst)+1),subst);
    
}

void expand_line(char *line, char *buffer);

int get_var(char *name, char *buffer)
{
  char *cp,*cp2;
  struct var_node *p;
  char tmp[100];
  for (p=first_var;p && strcmp(p->name,name);p=p->next);
  if (p)
  {
    expand_line(p->subst,buffer);
    return 1;
  }
  else 
  {
    buffer[0]=0;
    return 0;
  }
}

int detect_platform()
{
#ifdef __linux__
  return LINUX;
#endif

#ifdef __WATCOMC__
  return DOS;
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

  printf("Cannot detect platform\n");
  exit(1);


  return 0;
}

int get_word(char **st, char *buffer)
{
  char *bp;
  while (*(*st)==' ') (*st)++;
  if (*(*st)==0 || *(*st)=='\n' || *(*st)=='\r' || *(*st)=='=') 
    return 0;

  for (bp=buffer;*(*st) && *(*st)!=' ' && *(*st)!='\n' && *(*st)!='\r' && *(*st)!='=';(*st)++,bp++)
    *bp=*(*st);
  *(bp)=0;
  return 1;
}

void get_equal(char **st)
{
  while (*(*st)==' ') (*st)++;
  if (*(*st)!='=')
  {
    printf("expecting '=' on line %d\n",line_on);
    exit(0);
  }
  (*st)++;
}

void process_file(char *name);

void expand_line(char *line, char *buffer)
{
  char tmp[100],*cp2;
  for (;*line;)
  {   
    if (*line=='$')
    {
      line++;
      if (*line=='$')
        *(buffer++)=*(line++);
      else if (*line!='(')
      {
	printf("Expecting ( after $\n");
	exit(0);
      } else      
      {
	line++;
	for (cp2=tmp;*line!=')' && *line;line++,cp2++) *cp2=*line; line++;
	*cp2=0;
	get_var(tmp,buffer);
	while (*buffer) buffer++;
      }
    } else *(buffer++)=*(line++);      
  }
  *buffer=0;
}

void process_line(char *st, FILE *fp)
{
  char word[100],*wp,*ws,skip,rd;
  expand_line(st,z);
  st=z;

  if (get_word(&st,word))   
  {
    if (!strcmp(word,"INCLUDE"))
    {
      if (!get_word(&st,word))
      {
	fprintf(stderr,"expecting filename after INCLUDE on line %d\n",line_on);
	exit(1);
      } 
      process_file(word);
    } else if (!strcmp(word,"SECTION"))
    {
      skip=0; rd=0;
      wp=st;
      do
      {
	while (*wp==' ') wp++;
	ws=wp;
	if (*wp==0) skip=1;
	else
	{
	  while (*wp!=' ' && *wp) wp++;
	  *wp=0;
	  if (!strcmp(ws,plat_name[detect_platform()]))
	    rd=1;
	}
      } while (!skip && !rd);
      do
      {
	fgets(word,100,fp);
	while (word[strlen(word)-1]=='\n' || word[strlen(word)-1]=='\r')
	  word[strlen(word)-1]=0;
	strcat(word,"\n");
	if (rd)
	{
	  if (strcmp(word,"END\n"))
	  {
	    if (plat_stuff)
	    {
	      plat_stuff=realloc(plat_stuff,strlen(plat_stuff)+1+strlen(word));
	      strcat(plat_stuff,word);
	    }
	    else
	    {
	      plat_stuff=malloc(strlen(plat_stuff)+1+strlen(word));
	      strcpy(plat_stuff,word);
	    }	    
	  }
	}
      } while (strcmp(word,"END\n"));


    } else 
    {
      get_equal(&st);
      store_var(word,st);
    }
  }  
}


void dos_path(char *path)
{
  if (detect_platform()==DOS)
  {
    for (;*path;path++)
      if (*path=='/') *path='\\';
  }
}

char *include_path(int platform)
{
  char tmp[200];
  if (platform==DOS)
  {
    z[0]=0;
/*    dos_path(imlib_dir,tmp);
    sprintf(z,"-ic:\\watcom\\h;%s\\include;-i. ",tmp);  */
  }
  else
    sprintf(z,"-I%s/include",imlib_dir);
  return z;
}


void debug_flags(FILE *fp, int platform)
{
  switch (platform)
  {
    case LINUX :
    case SUN :
    case AIX :
    case SGI :
    {
      fprintf(fp,"-g");
    } break;
    case DOS :
    {
      fprintf(fp,"/zq /d2");
    } break;
  }
}

char *compiler(int platform)
{
  switch(platform)
  {
    case DOS : 
    { return "wpp386"; } break;
    default : 
    { return "g++"; } break;		 
  }
}

int fetch_word(char **ch, char *buffer)
{
  while (*(*ch)==' ' || *(*ch)=='\t') (*ch)++;
  if (*(*ch)==0) return 0;
  while (*(*ch) && *(*ch)!=' ' && *(*ch)!='\t')
  {
    *(buffer++)=*(*ch);
    (*ch)++;
  }
  *buffer=0;
  return 1;
}

void list_o_files(char *name, FILE *fp, char *fl, char *dir, char *ext, int platform, int optimize)
{
  char fname[200],ofname[200],*ch,sl;
  if (platform==DOS) sl='\\'; else sl='/';
  if (optimize)
    fprintf(fp,"%s_O = ",name);
  else
    fprintf(fp,"%s = ",name);

  while (*fl==' ' || *fl=='\t') fl++;
  while (*fl)
  {
    if (dir && dir[0])
      sprintf(fname,"%s%c",dir,sl);
    else fname[0]=0;
    ch=fname+strlen(fname);


    while (*fl && *fl!=' ' && *fl!='\t')
      *(ch++)=*(fl++);
    *ch=0;
    oextend(NULL,fname,ofname,platform,optimize);
    fprintf(fp,"%s",ofname);

    while (*fl==' ' || *fl=='\t') fl++;
    if (*fl)
    {
      if (platform==DOS)
        fprintf(fp," &\n\t");
      else
        fprintf(fp," \\\n\t");
    }
    else fprintf(fp,"\n\n");
  }
}

char *object_extension(int platform)
{
  if (platform==DOS)
    return ".obj";
  else return ".o";
}

void list_wlink_files(FILE *fp, char *var, char *dir, int optimize)
{
  char tmp[2000],name[100],*prep,*last_slash,*np;
  get_var(var,tmp);

  if (!dir) dir=".";
  dos_path(dir);
  var=tmp;
  while (*var)
  {
    while (*var==' ' || *var=='\t') var++;
    if (*var)
    {
      for (prep=name,last_slash=NULL;*var!=' ' && *var!='\t' && *var;prep++,var++) 
      {
	*prep=*var;
	if (*var=='/') 
	{
	  last_slash=prep;
	  *prep='\\';
	}
      }
      *prep=0;
      if (last_slash) 
      {
	*last_slash=0;
	fprintf(fp,"file %s\\%s\\DOS\\",dir,name);
	np=last_slash+1;
      } else 
      {
	fprintf(fp,"file %s\\DOS\\",dir);
	np=name;	
      }    
      if (optimize)
        fprintf(fp,"opt\\");
      else fprintf(fp,"debug\\");
      fprintf(fp,"%s.obj\n",np);
    }
  }  
}

void make_program(FILE *fp, char *name, char *plat_base, int platform, int optimize)
{
  FILE *lfp;
  char tmp[2000],tmp2[200],de[5],*oe,*oe2;
  sprintf(tmp2,"%s_FILES",plat_base);
  get_var(tmp2,tmp);
  list_o_files(tmp2,fp,tmp,imlib_dir,object_extension(platform),platform,optimize);  
  if (platform==DOS)
    strcpy(de,".exe");
  else
    de[0]=0;

  if (optimize) { oe="o"; oe2="_O";}  else { oe=""; oe2=""; }

  fprintf(fp,"%s%s%s : $(%s_FILES%s) $(IMLIB_OBJS%s) $(PROG_OBJS%s)\n",name,oe,de,plat_base,oe2,oe2,oe2);

  if (platform!=DOS)
  {
    if (optimize)
      fprintf(fp,"\t$(CC) -o %s%s $(%s_FILES_O) $(IMLIB_OBJS_O) $(PROG_OBJS_O)",
	    name,oe,plat_base);
    else
      fprintf(fp,"\t$(CC) -o %s%s $(%s_FILES) $(IMLIB_OBJS) $(PROG_OBJS)",
	    name,oe,plat_base);

    sprintf(tmp,"%s_LIBS",plat_base);
    if (get_var(tmp,tmp))
      fprintf(fp," %s",tmp);
    fprintf(fp,"\n");
  } else
  {
    if (optimize)    
      sprintf(tmp,"%so.lnk",name);
    else
      sprintf(tmp,"%s.lnk",name);

    fprintf(fp,"\twlink @%s\n",tmp);
    lfp=fopen(tmp,"wb");
    if (!lfp)
    {
      printf("Unable to open linker file %s\n",tmp);
      exit(0);
    }
    if (!optimize)
      fprintf(lfp,"debug all\n");
    fprintf(lfp,"system dos4g\n"
	    "option caseexact\n");
    if (optimize)
      fprintf(lfp,"name %so%s\n",name,de);
    else
      fprintf(lfp,"name %s%s\n",name,de);

    if (get_var("STACK_SIZE",tmp2))
      fprintf(lfp,"option stack=%s\n",tmp2);
    else fprintf(lfp,"option stack=8k\n");

    
    
    sprintf(tmp,"%s_FILES",plat_base);
    list_wlink_files(lfp,tmp,imlib_dir,optimize);
    sprintf(tmp,"IMLIB_OBJS");
    list_wlink_files(lfp,tmp,imlib_dir,optimize);
    sprintf(tmp,"O_FILES");
    list_wlink_files(lfp,tmp,NULL,optimize);
    fclose(lfp);
  }

  fprintf(fp,"\n");
}


struct dep_file
{
  struct dep_file *next;
  char *name;
} ;


struct dep_file *make_depend_list(struct dep_file *first, char *name, int platform)
{
  FILE *fp;
  struct dep_file *p;
  char tmp[200],*ch,*ch2;
  strcpy(tmp,name);
  fp=fopen(tmp,"rb"); 
  if (!fp)
  {
    if (platform==DOS)    
      sprintf(tmp,"%s\\include\\%s",imlib_dir,name);
    else
      sprintf(tmp,"%s/include/%s",imlib_dir,name);
    fp=fopen(tmp,"rb");
  }

  if (fp)
  {
    for (p=first;p;p=p->next)
    {
      if (!strcmp(p->name,tmp))   /* make sure we have not already processed this file */
      {
	fclose(fp);
        return first;
      }
    }
    p=(struct dep_file *)malloc(sizeof(struct dep_file));
    p->next=first;
    p->name=(char *)strcpy((char *)malloc(strlen(tmp)+1),tmp);
    first=p;

    if (!no_include)
    {
      while (!feof(fp))
      {
	fgets(tmp,200,fp);
	if (!feof(fp))
	{
	  if (memcmp(tmp,"#include",8)==0)
	  {
	    for (ch=tmp+8;*ch==' ' || *ch=='\t';ch++);
	    if (*ch=='"')
	    {
	      ch++;
	      for (ch2=tmp;*ch!='"';ch++,ch2++)
	      { *ch2=*ch; }
	      *ch2=0;
	      first=make_depend_list(first,tmp,platform);
	    }
	  }
	}
      }
    }
    fclose(fp);
  }
  return first;
}

void list_c_depends(FILE *fp, char *file, int platform, int optimize)
{
  char nn[200],nn2[200],*ndir;
  struct dep_file *first,*p;
  first=make_depend_list(NULL,file,platform);
  fprintf(stderr,"checking dependancies for : %s                      \r",file);
  strcpy(nn,file);
  nn[strlen(nn)-2]=0;
  oextend(NULL,nn,nn2,platform,optimize);
  if (nn2[0]=='.' && (nn2[1]=='\\' || nn2[1]=='/'))
    ndir=nn2+2;
  else ndir=nn2;
  while (first)
  {
    if (strcmp(first->name,file))
      fprintf(fp,"%s : %s\n",ndir,first->name);
    p=first;
    first=first->next;
    free(p);
  }
}

void oextend(char *dir, char *word, char *new_dir, int platform, int optimize)
{
  char slash,*sl,*last_slash,ext[200],small_word[100],mk[200];
  if (platform==DOS) slash='\\'; else slash='/';
  new_dir[0]=0;
  if (dir && strcmp(dir,".") && strcmp(dir,".\\") && strcmp(dir,"./"))
    sprintf(new_dir,"%s%c%s",dir,slash,word);
  else strcpy(new_dir,word);

  for (sl=new_dir,last_slash=NULL;*sl;sl++)
    if (*sl=='\\' || *sl=='/')
      last_slash=sl;

  if (last_slash)
  {
    strcpy(small_word,last_slash+1);
    *(last_slash+1)=0;
  }
  else 
  {
    strcpy(small_word,new_dir);
    new_dir[0]=0;
  }
 
  if (dir && strcmp(dir,".") && strcmp(dir,".\\") && strcmp(dir,"./"))
    sprintf(mk,"%s%c%s",dir,slash,word);
  else 
    strcpy(mk,word);


  sprintf(mk,"%s%c%s",dir,slash,word);

  for (sl=mk;*sl;sl++);
  while (*sl!='/' && *sl!='\\') sl--;
  sl++; 
  if (optimize)
    sprintf(sl,"%s/%s",plat_name[platform],"opt");
  else
    sprintf(sl,"%s/%s",plat_name[platform],"debug");
  create_dir(mk);


  if (optimize)
    sprintf(ext,"%s/%s/%s%s",plat_name[platform],"opt",small_word,object_extension(platform));
  else
    sprintf(ext,"%s/%s/%s%s",plat_name[platform],"debug",small_word,object_extension(platform));

  strcat(new_dir,ext);
  if (platform==DOS)
  {    
    for (sl=new_dir;*sl;sl++)
      if (*sl=='/') *sl='\\';
  }
}

void list_o_depends(FILE *fp, char *var, char *dir, int platform, int optimize)
{
  char *ch,word[100],tmp[200],o_name[200],sl,*tail;
  if (platform==DOS) sl='\\'; else sl='/';
  get_var(var,z);
  ch=z;
  if (!dir) dir=".";
  dos_path(dir);
  if (optimize) tail="_O"; else tail="_D";
  
  while (fetch_word(&ch,word))
  {
    oextend(dir,word,o_name,platform,optimize);
    if (!get_var(o_name,z))
    { 
      dos_path(word);
      if (platform!=DOS)
      {
	if (!strcmp(dir,"."))
          fprintf(fp,"%s : %s.c\n",o_name,word);
	else
          fprintf(fp,"%s : %s%c%s.c\n",o_name,dir,sl,word);
      }
      else
        fprintf(fp,"%s : .%c%s%c%s.c\n",o_name,sl,dir,sl,word);

      if (platform==DOS)
        fprintf(fp,"\twpp386 %s%c%s.c $(CFLAGS%s) -fo=%s\n",dir,sl,word,tail,o_name);
      else 
        fprintf(fp,"\t$(CC) %s%c%s.c $(CFLAGS%s) -c -o %s\n",dir,sl,word,tail,o_name);


      store_var(o_name,"O");
      if (dir)      
	sprintf(tmp,"%s%c%s.c",dir,sl,word);
      else
      	sprintf(tmp,"%s.c",word);
      list_c_depends(fp,tmp,platform,optimize);
    }
  }

}


void write_flags(FILE *fp,int platform, int optimize)
{
  char tmp[1000];
  char nm[100];
  if (optimize)
    fprintf(fp,"CFLAGS_O=%s",include_path(platform));
  else
    fprintf(fp,"CFLAGS_D=%s",include_path(platform));

  if (optimize) 
    sprintf(nm,"%s_OPTIMIZE",plat_name[platform]);
  else
    sprintf(nm,"%s_DEBUG",plat_name[platform]);

  get_var(nm,tmp);
  fprintf(fp," %s ",tmp);


  if (platform!=DOS && platform!=LINUX)
    fprintf(fp,"-DBIG_ENDIANS ");
  if (platform==SUN)
    fprintf(fp,"-I/lusr/X11R5/include ");
  if (get_var("CFLAGS",z))
    fprintf(fp,"%s ",z);

  sprintf(tmp,"%s_FLAGS",plat_name[platform]);
  if (get_var(tmp,z))
    fprintf(fp,"%s ",z);


  fprintf(fp,"\n");
}

void make_makefile(int platform)
{
  char tmp[100],*s,de[5]; 
  FILE *fp;
  if (platform==DOS)
    strcpy(de,".exe");
  else de[0]=0;
  
  if (!get_var("MAKEFILE_NAME",tmp))
  {
    if (platform==DOS)
      strcpy(tmp,"makefile.wat");
    else
      strcpy(tmp,"Makefile");
  }

  fp=fopen(tmp,"w");

  if (!fp)
  {
    printf("Unable to open %s for writing\n",tmp);
    exit(0);
  }
  
  fprintf(fp,"CC=%s\n",compiler(platform));

  write_flags(fp,platform,0);
  write_flags(fp,platform,1);



  list_o_files("IMLIB_OBJS",fp,imlib_objs,imlib_dir,object_extension(platform),platform,0);
  list_o_files("IMLIB_OBJS",fp,imlib_objs,imlib_dir,object_extension(platform),platform,1);
  list_o_files("PROG_OBJS",fp,ofiles,NULL,object_extension(platform),platform,0);
  list_o_files("PROG_OBJS",fp,ofiles,NULL,object_extension(platform),platform,1);
  

  if (platform==LINUX)        /* for linux make two versions of program,  X11 & SVGA */
  {
    sprintf(tmp,"%sx",basename);
    fprintf(fp,"all : %s %s\n\n",basename,tmp);
    
    make_program(fp,basename,"LINUX_SVGA",platform,0);
    make_program(fp,tmp,"LINUX_X",platform,0); 

    fprintf(fp,"opt : %so %so\n\n",basename,tmp);
    
    make_program(fp,basename,"LINUX_SVGA",platform,1);
    make_program(fp,tmp,"LINUX_X",platform,1); 
  }
  else 
  {
    fprintf(fp,"all : %s%s\n\n",basename,de);    
    make_program(fp,basename,plat_name[platform],platform,0);    

    fprintf(fp,"opt : %so%s\n\n",basename,de);
    make_program(fp,basename,plat_name[platform],platform,1);    
  }
  sprintf(tmp,"%s_FILES",plat_name[platform]);

  list_o_depends(fp,tmp,imlib_dir,platform,0);
  list_o_depends(fp,tmp,imlib_dir,platform,1);
  list_o_depends(fp,"IMLIB_OBJS",imlib_dir,platform,0);
  list_o_depends(fp,"IMLIB_OBJS",imlib_dir,platform,1);
  list_o_depends(fp,"O_FILES",NULL,platform,0);
  list_o_depends(fp,"O_FILES",NULL,platform,1);
  if (platform!=DOS)
  {
    fprintf(fp,"clean :\n\t"
	    "rm -f $(%s_FILES) $(IMLIB_OBJS) $(O_FILES)\n",
	    plat_name[platform]);
    fprintf(fp,"cleano :\n\t"
	    "rm -f $(%s_FILES_O) $(IMLIB_OBJS_O) $(O_FILES_O)\n",
	    plat_name[platform]);
  }
  if (plat_stuff)
    fprintf(fp,"%s",plat_stuff);   /* add any platform specific additions  */

  fclose(fp);
}


void process_file(char *name)
{
  char line[500],tmp[3000],*chp;
  int ol;
  int get_next_line,i;
  FILE *fp;
  ol=line_on;
  fp=fopen(name,"rb");
  if (!fp)
  {
    printf("Unable to open makefile template (%s)\n",name);
    exit(1);
  }

  line_on=1;
  while (!feof(fp))
  {
    tmp[0]=0;   
    do
    {
      get_next_line=0;
      fgets(line,1000,fp);
     
      for (chp=line;*chp && *chp!=';';chp++);
      *chp=0;

      while (line[strlen(line)-1]=='\n' || line[strlen(line)-1]=='\r' || line[strlen(line)-1]==' ') 
        line[strlen(line)-1]=0;

      if (line[strlen(line)-1]=='\\')
      {
	line[strlen(line)-1]=0;
	get_next_line=1;
      }
      strcat(tmp,line);
      
    } while (get_next_line && !feof(fp));
	   

    if (!feof(fp))
      process_line(tmp,fp);
    line_on++;
  }

  fclose(fp);
  line_on=ol;
}

char *template_file="maker.tmp";
main(int argc, char **argv)
{
  int pf,i;
  char tmp[1000];
  first_var=NULL;
  no_include=0;

  for (i=1;i<argc;i++)
  {
    if (!strcmp(argv[i],"-f"))
    {
      i++;
      template_file=argv[i];
    }
    if (!strcmp(argv[i],"-noi"))
      no_include=1;
  }

  process_file(template_file);
  pf=detect_platform();
  printf("Genering Makefile for %s\n",plat_names[pf]);

  if (get_var("O_FILES",tmp))
    ofiles=(char *)strcpy((char *)malloc(strlen(tmp)+1),tmp);
  if (get_var("BASE_NAME",tmp))
    basename=(char *)strcpy((char *)malloc(strlen(tmp)+1),tmp);
  if (get_var("IMLIB_DIR",tmp))
    imlib_dir=(char *)strcpy((char *)malloc(strlen(tmp)+1),tmp);
  if (get_var("IMLIB_OBJS",tmp))
    imlib_objs=(char *)strcpy((char *)malloc(strlen(tmp)+1),tmp);

 
  make_makefile(pf);
  printf("done                                               \n");
}


