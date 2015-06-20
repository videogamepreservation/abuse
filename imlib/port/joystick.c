#include "joy.hpp"

/* NULL joystick should be used for system where the joy
   stick has not yet been ported */

int joy_fd;

int joy_init(int argc, char **argv)
{
  return 0;
}

void joy_status(int &b1, int &b2, int &b3, int &xv, int &yv)
{
  xv=yv=b1=b2=b3=0;
}

