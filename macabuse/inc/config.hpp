#ifndef __CONFIG_HPP_
#define __CONFIG_HPP_

enum { HIGH_DETAIL,
       MEDIUM_DETAIL,
       LOW_DETAIL,
       POOR_DETAIL };


void key_bindings(int player, int &left, int &right, int &up, int &down, int &b1, int &b2, int &b3,  int &b4);
void get_key_bindings();
void get_movement(int player, int &x, int &y, int &b1, int &b2, int &b3, int &b4);
void config_cleanup();  // free any memory allocated
int get_keycode(char *str);  // -1 means not a valid key code

#endif
