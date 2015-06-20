#ifndef __LOADGAME_HPP__
#define __LOADGAME_HPP__

int show_load_icon();
int load_game(int show_all, char *title);
void get_savegame_name(char *buf);  // buf should be at least 50 bytes
void last_savegame_name(char *buf);
void load_number_icons();
int get_save_spot();

#endif
