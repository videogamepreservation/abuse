#include "lisp.hpp"

char *lang_string(char *symbol)
{
  void *v=find_symbol(symbol);
  if (!v || !DEFINEDP(symbol_value(v))) return "Language symbol missing!";
  else return lstring_value(symbol_value(v));  
}

