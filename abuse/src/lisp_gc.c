/*  Lisp garbage collections :  uses copy/free algorithm
    Places to check :
      symbol 
        values
	functions
	names
      stack


      

*/
#include <stdlib.h>
#include "lisp.hpp"

#ifdef NO_LIBS
#include "fakelib.hpp"
#else
#include "jmalloc.hpp"
#include "macs.hpp"
#endif

#include "stack.hpp"
#include <string.h>


grow_stack<void> l_user_stack(600);       // stack user progs can push data and have it GCed
grow_stack<void *> l_ptr_stack(6000);         // stack of user pointers, user pointers get remapped on GC

int reg_ptr_total=0;
int reg_ptr_list_size=0;
void ***reg_ptr_list=NULL;

void register_pointer(void **addr)
{
  if (reg_ptr_total>=reg_ptr_list_size)
  {
    reg_ptr_list_size+=0x100;
    reg_ptr_list=(void ***)jrealloc(reg_ptr_list,sizeof(void **)*reg_ptr_list_size,"registered ptr list");
  }
  reg_ptr_list[reg_ptr_total++]=addr;
}


void unregister_pointer(void **addr)
{
  int i;
  void ***reg_on=reg_ptr_list;
  for (i=0;i<reg_ptr_total;i++,reg_on++)
  {
    if (*reg_on==addr)
    {
      int j;
      reg_ptr_total--;
      for (j=i;j<reg_ptr_total;j++,reg_on++)
        reg_on[0]=reg_on[1];      
      return ;
    }
  }
  fprintf(stderr,"Unable to locate ptr to unregister");
}

static void *collect_object(void *x);
static void *collect_array(void *x)
{
  long s=((lisp_1d_array *)x)->size;
  lisp_1d_array *a=new_lisp_1d_array(s,NULL);
  void **src,**dst;
  src=(void **)(((lisp_1d_array *)x)+1);
  dst=(void **)(a+1);
  for (int i=0;i<s;i++)
    dst[i]=collect_object(src[i]);

  return a;
}

static uchar *cstart,*cend,*collected_start,*collected_end;

inline void *collect_cons_cell(void *x)
{
  cons_cell *last=NULL,*first;
  if (!x) return x;
  for (;x && item_type(x)==L_CONS_CELL;)
  {
    cons_cell *p=new_cons_cell();
    void *old_car=((cons_cell *)x)->car;
    void *old_cdr=((cons_cell *)x)->cdr;
    void *old_x=x;
    x=CDR(x);
    ((lisp_collected_object *)old_x)->type=L_COLLECTED_OBJECT;
    ((lisp_collected_object *)old_x)->new_reference=p;

    p->car=collect_object(old_car); 
    p->cdr=collect_object(old_cdr); 
	  
    if (last) last->cdr=p;
    else first=p;
    last=p;
  }
  if (x)
    last->cdr=collect_object(x);
  return first;                    // we already set the collection pointers
}

static void *collect_object(void *x)
{
  void *ret=x;

  if (((uchar *)x)>=cstart && ((uchar *)x)<cend)
  {
    switch (item_type(x))
    {
      case L_BAD_CELL :
      { lbreak("error : GC corrupted cell\n"); } break;

      case L_NUMBER : 
      { ret=new_lisp_number(((lisp_number *)x)->num); } break;


      case L_SYS_FUNCTION :
      { ret=new_lisp_sys_function( ((lisp_sys_function *)x)->min_args,
				      ((lisp_sys_function *)x)->max_args,
				      ((lisp_sys_function *)x)->fun_number);
      } break;
      case L_USER_FUNCTION :
      {
#ifndef NO_LIBS
	ret=new_lisp_user_function( ((lisp_user_function *)x)->alist,
				       ((lisp_user_function *)x)->blist);

#else
	void *arg=collect_object(((lisp_user_function *)x)->arg_list);
	void *block=collect_object(((lisp_user_function *)x)->block_list);
	ret=new_lisp_user_function(arg,block);
#endif
      } break;
      case L_STRING :
      { ret=new_lisp_string(lstring_value(x)); } break;

      case L_CHARACTER :
      { ret=new_lisp_character(lcharacter_value(x)); } break; 

      case L_C_FUNCTION :
      {
	ret=new_lisp_c_function( ((lisp_sys_function *)x)->min_args,
				      ((lisp_sys_function *)x)->max_args,
				      ((lisp_sys_function *)x)->fun_number);
      } break;

      case L_C_BOOL :
      {
	ret=new_lisp_c_bool( ((lisp_sys_function *)x)->min_args,
				      ((lisp_sys_function *)x)->max_args,
				      ((lisp_sys_function *)x)->fun_number);
      } break;
      case L_L_FUNCTION :
      {
	ret=new_user_lisp_function( ((lisp_sys_function *)x)->min_args,
				      ((lisp_sys_function *)x)->max_args,
				      ((lisp_sys_function *)x)->fun_number);
      } break;

      case L_POINTER :
      { ret=new_lisp_pointer(lpointer_value(x)); } break;
      

      case L_1D_ARRAY :
      { ret=collect_array(x); } break;

      case L_FIXED_POINT :
      { ret=new_lisp_fixed_point(lfixed_point_value(x)); } break;

      case L_CONS_CELL :
      { ret=collect_cons_cell((cons_cell *)x); } break;

      case L_OBJECT_VAR :
      {
	ret=new_lisp_object_var( ((lisp_object_var *)x)->number);
      } break;
      case L_COLLECTED_OBJECT :
      {
	ret=((lisp_collected_object *)x)->new_reference;
      } break;

      default :
      { lbreak("shouldn't happen. collecting bad object\n"); } break;      
    }
    ((lisp_collected_object *)x)->type=L_COLLECTED_OBJECT;
    ((lisp_collected_object *)x)->new_reference=ret;
  } else if ((uchar *)x<collected_start || (uchar *)x>=collected_end)  
  {
    if (item_type(x)==L_CONS_CELL) // still need to remap cons_cells outside of space
    {
      for (;x && item_type(x)==L_CONS_CELL;x=CDR(x))
        ((cons_cell *)x)->car=collect_object(((cons_cell *)x)->car);
      if (x)
        ((cons_cell *)x)->cdr=collect_object(((cons_cell *)x)->cdr);
    }
  }

  return ret;
}

static void collect_symbols(lisp_symbol *root)
{
  if (root)
  {
    root->value=collect_object(root->value);
    root->function=collect_object(root->function);
    root->name=collect_object(root->name);
    collect_symbols(root->left);
    collect_symbols(root->right);
  }
}

static void collect_stacks()
{
  long t=l_user_stack.son;
  void **d=l_user_stack.sdata;
  int i=0;
  for (;i<t;i++,d++)
    *d=collect_object(*d);

  t=l_ptr_stack.son;
  void ***d2=l_ptr_stack.sdata;
  for (i=0;i<t;i++,d2++)
  {
    void **ptr=*d2;
    *ptr=collect_object(*ptr);
  }

  d2=reg_ptr_list;
  for (t=0;t<reg_ptr_total;t++,d2++)
  {
    void **ptr=*d2;
    *ptr=collect_object(*ptr);
  }    

}

void collect_space(int which_space) // should be tmp or permenant
{
  int old_space=current_space;
  cstart=(uchar *)space[which_space];
  cend=(uchar *)free_space[which_space];

  space_size[GC_SPACE]=space_size[which_space];
  void *new_space=jmalloc(space_size[GC_SPACE],"collect lisp space");
  current_space=GC_SPACE;
  free_space[GC_SPACE]=space[GC_SPACE]=(char *)new_space;

  collected_start=(uchar *)new_space;
  collected_end=(((uchar *)new_space)+space_size[GC_SPACE]);

  collect_symbols(lsym_root);
  collect_stacks();

  memset(space[which_space],0,space_size[which_space]);  // for debuging clear it out
  jfree(space[which_space]);

  space[which_space]=(char *)new_space;
  free_space[which_space]=((char *)new_space)+
         (((uchar *)free_space[GC_SPACE])-((uchar *)space[GC_SPACE]));
  current_space=old_space;
}

