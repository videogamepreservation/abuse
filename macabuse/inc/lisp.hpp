#ifndef __LISP_HPP_
#define __LISP_HPP_

#include "lisp_opt.hpp"

//#define L_PROFILE 1 
#ifdef L_PROFILE
#include "timing.hpp"
#endif

#define Cell void
#define MAX_LISP_TOKEN_LEN 200
enum { PERM_SPACE,
       TMP_SPACE,
       USER_SPACE,
       GC_SPACE };
#define CAR(x) ((cons_cell *)x)->car
#define CDR(x) ((cons_cell *)x)->cdr


#define FIXED_TRIG_SIZE 360               // 360 degrees stored in table
extern long sin_table[FIXED_TRIG_SIZE];   // this should be filled in by external module
#define TBS 1662                          // atan table granularity
extern unsigned short atan_table[TBS];
#define NILP(x) (x==NULL)
#define DEFINEDP(x) (x!=l_undefined)
class bFILE;
extern int current_space;
extern bFILE *current_print_file;


enum { L_BAD_CELL,   // error catching type
       L_CONS_CELL, L_NUMBER, L_SYMBOL,     L_SYS_FUNCTION, L_USER_FUNCTION, 
       L_STRING, L_CHARACTER, L_C_FUNCTION, L_C_BOOL,       L_L_FUNCTION, L_POINTER,
       L_OBJECT_VAR, L_1D_ARRAY,
       L_FIXED_POINT, L_COLLECTED_OBJECT };

typedef long ltype;    // make sure structures aren't packed differently on various compiler
                       // and sure that word, etc are word alligned

struct lisp_object_var
{  
  ltype type;
  long number;  
} ;

struct cons_cell
{
  ltype type;
  void *cdr,*car;
} ;

struct lisp_number
{
  ltype type;
  long num;
} ;

struct lisp_collected_object
{
  ltype type;
  void *new_reference;
} ;

struct lisp_symbol
{
  ltype type;
#ifdef L_PROFILE
  float time_taken;
#endif
  void *value, *function, *name;
  lisp_symbol *left,*right;       // tree structure
} ;

struct lisp_sys_function
{
  ltype type;
  short min_args,max_args,fun_number;
  long (*fun)(void *);
  
} ;

struct lisp_user_function
{
  ltype type;
#ifndef NO_LIBS
  long alist,blist;      // id for cached blocks
#else
  void *arg_list,*block_list;
#endif
} ;

struct lisp_1d_array
{
  ltype type;
  unsigned short size; 
  // size * sizeof (void *) follows1
} ;

struct lisp_string
{
  ltype type; 
} ;

struct lisp_character
{
  ltype type;
  short pad;
  unsigned short ch;
} ;

struct lisp_pointer
{
  ltype type; 
  void *addr;
} ;


struct lisp_fixed_point
{
  ltype type;
  long x;
} ;


void perm_space();
void tmp_space();
void use_user_space(void *addr, long size);
#define item_type(c) ((c) ? *((ltype *)c) : (ltype)L_CONS_CELL)
void *lget_array_element(void *a, long x);
void *lpointer_value(void *lpointer);
long lnumber_value(void *lnumber);
char *lstring_value(void *lstring);
unsigned short lcharacter_value(void *c);
long lfixed_point_value(void *c);
void *lisp_atom(void *i);
void *lcdr(void *c);
void *lcar(void *c);
void *lisp_eq(void *n1, void *n2);
void *lisp_equal(void *n1, void *n2);
lisp_symbol *find_symbol(char *name);
long list_length(void *i);
void lprint(void *i);
void *eval(void *prog);
void *eval_block(void *list);
void *eval_function(lisp_symbol *sym, void *arg_list);
void *eval_user_fun(lisp_symbol *sym, void *arg_list);
void *compile(char *&s);
void *symbol_value(void *symbol);
void *symbol_function(void *symbol);
void *set_symbol_number(void *symbol, long num);
void *set_symbol_value(void *symbol, void *value);
void *symbol_name(void *symbol);
void *assoc(void *item, void *list);
void resize_tmp(int new_size);
void resize_perm(int new_size);
lisp_symbol *make_find_symbol(char *name);

void push_onto_list(void *object, void *&list);
lisp_symbol *add_c_object(void *symbol, short number);
lisp_symbol *add_c_function(char *name, short min_args, short max_args, long (*fun)(void *));
lisp_symbol *add_c_bool_fun(char *name, short min_args, short max_args, long (*fun)(void *));
lisp_symbol *add_lisp_function(char *name, short min_args, short max_args, short number);
int read_ltoken(char *&s, char *buffer);
cons_cell *new_cons_cell();
void print_trace_stack(int max_levels);


lisp_number *new_lisp_number(long num);
lisp_pointer *new_lisp_pointer(void *addr);
lisp_character *new_lisp_character(unsigned short ch);
lisp_string *new_lisp_string(char *string);
lisp_string *new_lisp_string(char *string, int length);
lisp_string *new_lisp_string(long length);
lisp_fixed_point *new_lisp_fixed_point(long x);
lisp_object_var *new_lisp_object_var(short number);
lisp_1d_array   *new_lisp_1d_array(unsigned short size, void *rest);
lisp_sys_function *new_lisp_sys_function(int min_args, int max_args, int fun_number);
lisp_sys_function *new_lisp_c_function(int min_args, int max_args, long (*fun)(void *));
lisp_sys_function *new_lisp_c_bool(int min_args, int max_args, long (*fun)(void *));

#ifdef NO_LIBS
lisp_user_function *new_lisp_user_function(void *arg_list, void *block_list);
#else
lisp_user_function *new_lisp_user_function(long arg_list, long block_list);
#endif

lisp_sys_function *new_user_lisp_function(int min_args, int max_args, int fun_number);

int end_of_program(char *s);
void clear_tmp();
void lisp_init(long perm_size, long tmp_size);
void lisp_uninit();
extern lisp_symbol *lsym_root;

extern char *space[4],*free_space[4];
extern int space_size[4];
void *nth(int num, void *list);
long lisp_atan2(long dy, long dx);
long lisp_sin(long x);
long lisp_cos(long x);
void restore_heap(void *val, int heap);
void *mark_heap(int heap);

extern "C" {
void lbreak(const char *format, ...);
} ;

extern void clisp_init();                      // external initalizer call by lisp_init()
extern void *l_caller(long number, void *arg);  // exten lisp function switches on number

extern void *l_obj_get(long number);  // exten lisp function switches on number
extern void l_obj_set(long number, void *arg);  // exten lisp function switches on number
extern void l_obj_print(long number);  // exten lisp function switches on number



#endif
