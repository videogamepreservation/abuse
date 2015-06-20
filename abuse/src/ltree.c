
lisp_symbol *find_symbol(char *name)
{
  lisp_symbol *p=lsym_root;
  while (p)
  {
    int cmp=strcmp(name,((char *)((lisp_symbol *)cs->car)->name)+sizeof(lisp_string));
    if (cmp==0) return p;
    else if (cmp<0) p=p->left;
    else p=p->right;
  }
  return NULL;
}



lisp_symbol *make_find_symbol(char *name)
{
  lisp_symbol *p=lsym_root;
  lisp_symbol **parent=&lsym_root;
  while (p)
  {
    int cmp=strcmp(name,((char *)((lisp_symbol *)cs->car)->name)+sizeof(lisp_string));
    if (cmp==0) return p;
    else if (cmp<0) 
    { 
      parent=&p->left;
      p=p->left;
    }
    else 
    {
      parent=&p->right;
      p=p->right;
    }
  }

  p=jmalloc(sizeof(lisp_symbol),"lsymbol");
  p->type=L_SYMBOL;
  p->name=new_lisp_string(name);
  p->value=l_undefined;
  p->function=l_undefined;
  p->call_counter=0;
  p->left=p->right=NULL;
  *parent=p;
  return p;
}


