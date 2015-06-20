#include "dirent.h"

#include <stdlib.h>
#include <string.h>
#include "jmalloc.hpp"
#include <unistd.h>

char MacName[512];

extern char *macify_name(char *s);
char *macify_name(char *s)
{
  char *p = s;
  int macify = 0;
  int premacify = 1;
  int slashed = 0;
  
  while (*p) {
    if (*p=='/')
      macify = 1;
    if (*p==':')
      premacify = 0;
    p++;
  }
  
  if (!macify)
    return s;
  
  p = MacName;
  
  if (s[0] != '.' )
  { 
    if (premacify)
      *p++ = ':';
  }
  else
    s++;
  while (*p = *s++) {
    if (*p=='/')
      *p = ':';
    if (*p != ':')
    {
      slashed = 0;
      p++;
    }
    else
      if (!slashed)
      {
        slashed = 1;
        p++;
      }
  }
  
  return MacName;
/*
  p = s;
  while (*p) {
    if (*p=='/')
      *p = '_';
    p++;
  }
  return s;
*/
}

void get_directory(char *path, char **&files, int &tfiles, char **&dirs, int &tdirs)
{
  
  struct dirent *de;
  files=NULL;
  dirs=NULL;
  tfiles=0;
  tdirs=0;
  DIR *d=opendir(macify_name(path));
  if (!d) return ;

  char **tlist=NULL;
  int t=0;
  char curdir[200];
  getcwd(curdir,200);
  chdir(macify_name(path));

  do
  {
    de=readdir(d);
    if (de)
    {
      t++;
      tlist=(char **)jrealloc(tlist,sizeof(char *)*t,"tmp file list");
      tlist[t-1]=strcpy((char *)jmalloc(strlen((char*)de->d_name)+1,"tmp file name"),(char*)de->d_name);      
    }
  } while (de);
  closedir(d);

  for (int i=0;i<t;i++)
  {
    d=opendir(tlist[i]);
    if (d)
    {
      tdirs++;
      dirs=(char **)jrealloc(dirs,sizeof(char *)*tdirs,"dir list");
      dirs[tdirs-1]=strcpy((char *)jmalloc(strlen(tlist[i])+1,"tmp file name"),tlist[i]);
      closedir(d);
    } else
    {
      tfiles++;
      files=(char **)jrealloc(files,sizeof(char *)*tfiles,"dir list");
      files[tfiles-1]=strcpy((char *)jmalloc(strlen(tlist[i])+1,"tmp file name"),tlist[i]);
    }
    jfree(tlist[i]);
  }
  if (t)
    jfree(tlist);
  chdir(curdir);

  // get files in directory in path into files & tfiles
  // get dirs in directory in path into dirs & tdirs
}

