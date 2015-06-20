#ifndef __EXITPROC_HPP_
#define __EXITPROC_HPP_

#include <stdlib.h>
#include "system.h"

#if defined( __AIX ) || defined( __sgi ) || defined( __WATCOMC__ ) || defined( __MAC__ )
#define exit_proc(x,y) atexit(y)
#else
#define exit_proc(x,y) on_exit(x,NULL)
#endif


#endif
