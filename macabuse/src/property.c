#include "property.hpp"
#include "jmalloc.hpp"
#include <stdio.h>
#include "dprint.hpp"
#include <string.h>

class property
{
  public :
  char *name;
  char *def_str;
  double def_num;
  property(char *Name, double Def)
  { name=strcpy((char *)jmalloc(strlen(Name)+1,"Property Name"),Name);
    def_num=Def;
    def_str=NULL;
    next=NULL;
  }

  property(char *Name, char *Def)
  { name=strcpy((char *)jmalloc(strlen(Name)+1,"Property Name"),Name);
    def_str=strcpy((char *)jmalloc(strlen(Def)+1,"Property text"),Def);
    next=NULL;
  }

  void set(double x) 
  { if (def_str) 
    {
      jfree(def_str); 
      def_str=NULL; 
    }
    def_num=x;
  }

  void set(char *x)
  {
    if (def_str) 
    { 
      jfree(def_str); 
      def_str=NULL; 
    }
    def_str=strcpy((char *)jmalloc(strlen(x)+1,"Property text"),x);
  }

  ~property() 
  { 
    if (def_str) 
      jfree(def_str);
    jfree(name); 
  }
  property *next;
} ;

property *property_manager::find(char *name)
{
  for (property *i=first;i;i=i->next)  
    if (!strcmp(i->name,name)) 
      return i;
  return NULL;
}


property_manager::~property_manager()
{
  while (first)
  {
    property *i=first;
    first=first->next;
    delete i;
  }
}

double property_manager::get(char *name, double def)
{
  property *f=find(name);
  if (!f || f->def_str) 
    return def;
  else return f->def_num;
}


char *property_manager::get(char *name,char *def)
{
  property *f=find(name);
  if (!f || !f->def_str) 
    return def;
  else return f->def_str;
}


void property_manager::set(char *name, double def)
{
  property *f=find(name);
  if (f)
    f->set(def);
  else
  {
    f=new property(name,def);
    f->next=first;
    first=f;
  }  
}

void property_manager::set(char *name, char *def)
{
  property *f=find(name);
  if (f)
    f->set(def);
  else
  {
    f=new property(name,def);
    f->next=first;
    first=f;
  }  
}


FILE *open_FILE(char *filename, char *mode);

void property_manager::save(char *filename)
{
  FILE *fp=open_FILE(filename,"wb");
  if (!fp)
    dprintf("Error opening %s to save properties\n",filename);
  else
  {
    for (property *i=first;i;i=i->next)
    {
      fprintf(fp,"%s = ",i->name);
      if (i->def_str)
        fprintf(fp,"\"%s\"\n",i->def_str);
      else
        fprintf(fp,"%g\n",i->def_num);      
    }
    fclose(fp);
  }
}


void property_manager::load(char *filename)
{
  char buf[100],*c1,*c2,name[100],str[100];
  FILE *fp=open_FILE(filename,"rb");
  if (fp)
  {
    while (!feof(fp))
    {
      if (fgets(buf,100,fp))
      {
	for (c1=buf,c2=name;*c1 && *c1!='=';c1++,c2++)
	  *c2=*c1;
	if (*c1==0) { dprintf("Missing = for property line %s in file %s\n",buf,filename); 
		      exit(1);}
	*c2=' ';
	while (*c2==' ') { *c2=0; c2--; }
	c1++; while (*c1==' ') c1++;
	if (*c1=='"')
	{ c1++;
	  for (c2=str;*c1 && *c1!='"';c1++,c2++)
	    *c2=*c1;
	  *c2=0;
	  if (*c1!='"') { dprintf("Missing \" for property name %s in file %s\n",name,filename);
			  exit(1); }
	  set(name,str);
	} else
	{
	  double x;
	  if (sscanf(c1,"%lg",&x))
	    set(name,x);
	  else 
	  { 
	    dprintf("Bad number/string for property name %s in file %s\n",name,filename);
	    exit(1); 
	  }	  
	}			  
      }
    }
  }
}
