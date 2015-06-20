#ifndef _LISP_HPP_INCLUDED_
#define _LISP_HPP_INCLUDED_

#include	<sys/types.h>
#include	<stdtool.h>




extern "C"
{
#include	"li_modul.h"
#include	"li_proto.h"
}

/* ------------------------- COMPATIBILITY ------------------------- */

#define L_SYMBOL		LI_tSYMBOL
#define L_STRING		LI_tSTRING
#define L_CONS_CELL		LI_tCONS
#define L_NUMBER		LI_tLONG
#define L_CHARACTER		LI_tLONG
#define L_POINTER		LI_tUSER
#define L_FIXED_POINT	LI_tFIXEDPOINT

#define PERM_SPACE	0
#define TEMP_SPACE	0
#define cons_cell	Cell
#define lisp_symbol	Cell

/* ------------------------- GLOBAL VARS ------------------------- */

extern int current_space;
extern void *enviroment;

/* ------------------------- GLOBAL LISP VARS ------------------------- */

extern Cell *l_difficulty,*l_easy,*l_hard,*l_medium,*l_main_menu,
            *l_logo,*l_state_art,*l_abilities,*l_state_sfx,
			*l_song_list,*l_filename,*l_sfx_directory,*l_max_hp,
			*l_default_font,*l_morph,*l_max_power,*l_default_abilities,
			*l_default_ai_function,*l_tile_files,*l_empty_cache,*l_range,
			*l_joy_file,*l_hurt_all,*l_death_handler,*l_title_screen,
			*l_console_font,*l_fields,*l_dist,*l_pushx,*l_pushy,
			*l_object,*l_tile;

/* variables for the status bar */

extern Cell *l_statbar_ammo_x,*l_statbar_ammo_y,
            *l_statbar_ammo_w,*l_statbar_ammo_h,
			*l_statbar_ammo_bg_color,

			*l_statbar_health_x,*l_statbar_health_y,
			*l_statbar_health_w,*l_statbar_health_h,
			*l_statbar_health_bg_color,

			*l_statbar_logo_x,*l_statbar_logo_y;

extern Cell *true_symbol;

/* ------------------------- TRIGONOMETRY -------------------- */

#define FIXED_TRIG_SIZE 360               // 360 degrees stored in table
extern long sin_table[FIXED_TRIG_SIZE];   // this should be filled in by external module
#define TBS 1662                          // atan table granularity
extern unsigned short atan_table[TBS];

/* ------------------------- COMPATIBILITY -------------------- */

typedef Cell	lisp_symbol;
#undef CAR
#undef CDR
#undef SCAR
#undef SCDR
#define SCAR(x)			(((Cell*)x)->type == LI_tCONS ? ((Cell*)x)->v.cons.car : Nil)
#define SCDR(x)			(((Cell*)x)->type == LI_tCONS ? ((Cell*)x)->v.cons.cdr : Nil)
#define CAR(x)			(((Cell*)x)->v.cons.car)
#define CDR(x)			(((Cell*)x)->v.cons.cdr)

/* ------------------------- FUNCTION PROTOTYPES ------------------------- */

void lisp_init (long a, long b);
void lisp_uninit (void);
char* lstring_value (void* x);
long lnumber_value (void* x);
char lcharacter_value (void* x);
Cell* new_lisp_number (long x);
Cell* new_lisp_character (unsigned short x);
Cell* new_lisp_string (long x);
Cell* new_lisp_pointer (void* x);
Cell* new_cons_cell (void);
Cell* lcar (void* x);
Cell* lcdr (void* x);
Cell* lprint (void* x);
Cell* set_symbol_number (void* x, long y);
Cell* set_symbol_value(void *symbol, void *value);
Cell* symbol_value (void* x);
Cell* make_find_symbol (char* name);
Cell* find_symbol (char* name);
Cell* symbol_function (void* symbol);
void use_user_space(void *addr, long size);
Cell* eval_function(void *sym, void *arg_list, void *env);
void clear_tmp();
void resize_tmp (int size);
Cell* eval (void* item, void* env);
long list_length(void *i);
Cell* nth (int num, void *list);
Cell* compile (char*& str);
int get_lprop_number (void* symbol, int number);
Cell* assoc (void* vlists, void* vtarget);
char* symbol_name (void* symbol);
int item_type (void* cell);
long lisp_cos(long x);
long lisp_sin(long x);
long lisp_atan2(long dy, long dx);
void push_onto_list(Cell *object, Cell *&list);

void* lpointer_value (void* x);
long lfixed_point_value(void* x);
void lisp_init_globals ();
int lisp_init_lisp_fns ();
void lisp_init_c_fns ();

extern "C" void lbreak (const char* format, ...);

#endif /* _LISP_HPP_INCLUDED_ */
