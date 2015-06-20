char *mac_replace[]={"(if $cond (progn $list) nil)"
		     "(if-1progn $cond $list)",

		     "(if $cond (progn $list1) (progn $list2))"
		     "(if-12progn $list1 $list2)",
		     
		     "(if $cond nil (progn $list))"
		     "(if-2progn $cond $list)",

		     "(if (not $cond) $x $y)"
		     "(if $cond $y $x)",

		     "(with_object $x (progn $y))"
		     "(with_object $x $y)",
		     
		     "(with_object (get_object 0) $x)"
		     "(with_obj0 $x)",

		     "(eq 0 $x)"
		     "(eq0 $x)",

		     "(eq $x 0)"
		     "(eq0 $x)",

		     "(not (eq0 $x))"
		     "(noteq0 $x)",

		     NULL
		   };


void **find_rules,
     **replace_rules;

int trules;    // total rules

void mac_replace_init()
{
  char **s=mac_replace;
  int trules=0;
  while (*s) { trules++; s++; }   // count how many rules we have
  find_rules=(void **)jmalloc(sizeof(void *)*trules,"mac find rules");
  replace_rules=(void **)jmalloc(sizeof(void *)*trules,"mac replace rules");

  s=mac_replace;
  int i;
  for (int i=0;i<trules;i++,s++)
  {
    char *cs=*s;
    find_rules[i]=compile(cs);
    l_ptr_stack.push(&find_rules[i]);

    replace_rules[i]=compile(cs);
    l_ptr_stack.push(&replace_rules[i]);
  }
}


class mac_binding
{
  public :
  void *var;
  void *value;
  mac_binding *next;
  mac_binding(void *Var, void *Value, mac_binding *Next)
  {
    var=Var;
    value=Value;
    next=Next;
  }
} ;


mac_binding *match_rule(void *x, void *rule)
{
  if (item_type(x)==L_CONS_CELL
}

void apply_rules(void *x)
{
  
}
