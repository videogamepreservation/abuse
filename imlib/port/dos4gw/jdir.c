#include <direct.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "jmalloc.hpp"


void get_directory(char *path, char **&files, int &tfiles, char **&dirs, int &tdirs)
{
  struct dirent *de;
  files=NULL;
  dirs=NULL;
  tfiles=0;
  tdirs=0;
  DIR *d=opendir(path);
  if (!d) return ;
  char curdir[200];
  getcwd(curdir,200);
  chdir(path);

  do
  {
    de=readdir(d);
    if (de)
    {
      if (de->d_attr&_A_SUBDIR)
      {      
	tdirs++;
	dirs=(char **)jrealloc(dirs,sizeof(char *)*tdirs,"dir list");
	dirs[tdirs-1]=strcpy((char *)jmalloc(strlen(de->d_name)+1,"tmp file name"),de->d_name);
      } else
      {
	tfiles++;
	files=(char **)jrealloc(files,sizeof(char *)*tfiles,"dir list");
	files[tfiles-1]=strcpy((char *)jmalloc(strlen(de->d_name)+1,"tmp file name"),de->d_name);
      }
    }
  } while (de);
  closedir(d);
  chdir(curdir);
}



