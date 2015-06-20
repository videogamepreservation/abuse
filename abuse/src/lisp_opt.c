#ifdef NO_LIBS
#include "fakelib.hpp"
#else
#include "macs.hpp"
#endif

#include "lisp.hpp"
#include "lisp_gc.hpp"

void *true_symbol=NULL,*l_undefined,*list_symbol,*string_symbol,     // in lisp_init()
     *quote_symbol,*backquote_symbol,*comma_symbol,*do_symbol,*in_symbol,*aref_symbol,
     *colon_initial_contents,*colon_initial_element,*if_symbol,
     *progn_symbol,*eq_symbol,*zero_symbol,*eq0_symbol,*car_symbol,*cdr_symbol,
     *load_warning;


void *if_1progn,*if_2progn,*if_12progn,*not_symbol;

void *comp_optimize(void *list)
{
  void *return_val=list;
  p_ref r1(list);
  if (list)
  {
    if (CAR(list)==if_symbol)
    {
      void *eval1=lcar(lcdr(lcdr(list)));
      p_ref r2(eval1);
      void *eval2=lcar(lcdr(lcdr(lcdr(list))));
      p_ref r3(eval2);

      void *ret=NULL;
      p_ref r1(ret);
      if (lcar(list)==eq_symbol && (lcar(lcdr(list))==zero_symbol))  //  simplify (eq 0 x) -> (eq0 x)
      {
	push_onto_list(lcar(lcdr(lcdr(list))),ret);
	push_onto_list(eq0_symbol,ret);
	return_val=comp_optimize(ret);
      } else if (lcar(list)==eq_symbol && 
		 (lcar(lcdr(lcdr(list)))==zero_symbol)) //simplify (eq x 0)-> (eq0 x)
      {
	push_onto_list(lcar(lcdr(list)),ret);
	push_onto_list(eq0_symbol,ret);
	return_val=comp_optimize(ret);
      } else if (lcar(lcar(lcdr(list)))==not_symbol)  // simplify (if (not y) x z) -> (if y z x)
      {      
	push_onto_list(lcar(lcdr(lcdr(list))),ret);
	push_onto_list(lcar(lcdr(lcdr(lcdr(list)))),ret);
	push_onto_list(lcar(lcdr(lcar(lcdr(list)))),ret);
	push_onto_list(if_symbol,ret);
	return_val=comp_optimize(ret);
      } 
      else if (lcar(eval1)==progn_symbol && (eval2==NULL || 
					     item_type(eval2)!=L_CONS_CELL))
      {
	push_onto_list(eval2,ret);
	push_onto_list(lcdr(eval1),ret);
	push_onto_list(lcar(lcdr(list)),ret);
	push_onto_list(if_1progn,ret);
	return_val=comp_optimize(ret);
      } else if (lcar(eval1)==progn_symbol && lcar(eval2)==progn_symbol)
      {
	push_onto_list(lcdr(eval2),ret);
	push_onto_list(lcdr(eval1),ret);
	push_onto_list(lcar(lcdr(list)),ret);
	push_onto_list(if_12progn,ret);
	return_val=comp_optimize(ret);
      } else if (lcar(eval2)==progn_symbol)
      {
	push_onto_list(lcdr(eval2),ret);
	push_onto_list(eval1,ret);
	push_onto_list(lcar(lcdr(list)),ret);
	push_onto_list(if_2progn,ret);
	return_val=comp_optimize(ret);
      }

    }
  }
  return return_val;
}


void l_comp_init()
{
  l_undefined=make_find_symbol(":UNDEFINED");  // this needs to be defined first
  ((lisp_symbol *)l_undefined)->function=NULL;  // collection problems result if we don't do this
  ((lisp_symbol *)l_undefined)->value=NULL;


  true_symbol=make_find_symbol("T");


  list_symbol=make_find_symbol("list");
  string_symbol=make_find_symbol("string");
  quote_symbol=make_find_symbol("quote");
  backquote_symbol=make_find_symbol("backquote");
  comma_symbol=make_find_symbol("comma");
  in_symbol=make_find_symbol("in");
  do_symbol=make_find_symbol("do");
  aref_symbol=make_find_symbol("aref");
  colon_initial_contents=make_find_symbol(":initial-contents");
  colon_initial_element=make_find_symbol(":initial-element");

  if_1progn=make_find_symbol("if-1progn");
  if_2progn=make_find_symbol("if-2progn");
  if_12progn=make_find_symbol("if-12progn");
  if_symbol=make_find_symbol("if");
  progn_symbol=make_find_symbol("progn");
  not_symbol=make_find_symbol("not");
  eq_symbol=make_find_symbol("eq");
  zero_symbol=make_find_symbol("0");
  eq0_symbol=make_find_symbol("eq0");
  car_symbol=make_find_symbol("car");
  cdr_symbol=make_find_symbol("cdr");
  load_warning=make_find_symbol("load_warning");
}
