int change_dir(char *path);
long K_avail(char *path);
void set_cursor(int x, int y);
void put_char(int x, int y, int val, int color=0x17);
unsigned short get_char(int x, int y, int val);
void put_string(int x,int y,char *s, int c);
void bar(int x1, int y1, int x2, int y2, int v, int c);
void cls();
void box(int x1, int y1, int x2, int y2, int c);
void put_title(char *t);
int nice_copy(char *title, char *source, char *dest);
void *nice_input(char *t, char *p, char *d);
void *nice_menu(void *main_title, void *menu_title, void *list);
void center_tbox(void *list, int c);
void *show_yes_no(void *t, void *msg, void *y, void *n);
void modify_install_path(char *path);



