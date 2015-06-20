
#if defined( __WATCOMC__ )
#include <direct.h>
#elif defined( __POWERPC__ )
#else
#include <dirent.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "jmalloc.hpp"
#include <unistd.h>

void get_directory(char *path, char **&files, int &tfiles, char **&dirs, int &tdirs)
{
	// get files in directory in path into files & tfiles
	// get dirs in directory in path into dirs & tdirs
}

char MacName[512];

extern char *macify_name(char *s);
char *macify_name(char *s)
{
	char *p;
/*
	
	p = &MacName[0];
	while (*p = *s++) {
		if (*p=='/')
			*p = '_';
		p++;
	}
	
	return MacName;
*/
	p = s;
	while (*p) {
		if (*p=='/')
			*p = '_';
		p++;
	}
	return s;
}


