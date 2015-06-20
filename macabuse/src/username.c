#include "system.h"

#if defined( __WATCOMC__ ) || defined( __MAC__ )
char *get_username() { return "DOS user"; }

#else
  
#include	<stdio.h>
#include	<pwd.h>
#include	<sys/types.h>
#include        <unistd.h>

char *get_username()
{
  struct passwd		*pw;
  char			*name;

  if (!(name = getlogin()))
  {
    if ((pw = getpwuid (getuid())))
      return pw->pw_name;
    else
      return "UNIX user";
  } else return name; 
}

#endif


