#ifndef __JOYSTICK_HPP_
#define __JOYSTICK_HPP_

int joy_init(int argc, char **argv);              // returns 0 if joy stick not avaible
void joy_status(int &b1, int &b2, int &b3, int &xv, int &yv);
void joy_calibrate();

#endif
