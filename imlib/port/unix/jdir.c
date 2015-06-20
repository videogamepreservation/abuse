
#ifdef __WATCOMC__
#include <direct.h>
#else
#include <dirent.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "jmalloc.hpp"
#include <unistd.h>

void get_directory(char *path, char **&files, int &tfiles, char **&dirs, int &tdirs)
{
  
  struct dirent *de;
  files=NULL;
  dirs=NULL;
  tfiles=0;
  tdirs=0;
  DIR *d=opendir(path);
  if (!d) return ;

  char **tlist=NULL;
  int t=0;
  char curdir[200];
  getcwd(curdir,200);
  chdir(path);

  do
  {
    de=readdir(d);
    if (de)
    {
      t++;
      tlist=(char **)jrealloc(tlist,sizeof(char *)*t,"tmp file list");
      tlist[t-1]=strcpy((char *)jmalloc(strlen(de->d_name)+1,"tmp file name"),de->d_name);      
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
}





