#include "system.h"

main()
{
#if BYTE_ORDER==LITTLE_ENDIAN
  printf("Hello little\n");
#else 
  printf("hi big\n");
#endif
}

