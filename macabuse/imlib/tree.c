#include <stdio.h>
#include <stdlib.h>

class btnode
{
protected :
  btnode *Left, *Right;
public :
	  btnode *left()                   { return Left; }
	  btnode *right()                  { return Right; }
	  void    set_right(btnode *right) { Right=right; }
	  void    set_left (btnode *left)  { Left=left;   }
  virtual int     compare  (btnode *node) = 0;
  virtual char   *name     ()             = 0;
} ;

class btree
{
  void reprint(btnode *n);
protected :
  btnode *root;
public :
	       btree() { root=NULL; }
  virtual void insert(btnode *node);
  virtual void remove(btnode *node);
  void         print() { reprint(root); }
} ;

void btree::insert(btnode *node)
{ btnode *parent,*finder;
  int from_left;
  node->set_right(NULL); node->set_left(NULL);
  if (root)
  { finder=root;
    while (finder)
    { parent=finder;
      if (finder->compare(node)>0)
      { from_left=1; finder=finder->left(); }
      else
      { from_left=0; finder=finder->right(); }
    }
    if (from_left)
      parent->set_left(node);
    else
      parent->set_right(node);
  } else root=node;
}

void btree::remove(btnode *node)
{
}

void btree::reprint(btnode *n)
{
  if (n)
  { reprint(n->left());
    printf("%s\n",n->name());
    reprint(n->right());
  }
}

class btnumber : public btnode
{
  int num;
public :
  btnumber(int x) { num=x; }
  virtual int     compare  (btnode *node)
    { if (num<((btnumber *)node)->num) return -1;
      else return (num> ((btnumber *)node)->num); }
  virtual char   *name     ()
    { static char st[20]; sprintf(st,"%d",num); return st;}
} ;

main()
{
  btree bt;
  for (int i=0;i<20;i++)
    bt.insert((btnode *) new btnumber(random(500)));
  bt.print();
