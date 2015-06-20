// linked.hpp  - linked list and linked list node classes
// written June 2, 1992 by Jonathan Clark  (at home)
// these classes provide the basic groundwork for any future linked list
// please derive your own linked_node subclass and define the virtual
// function compare.
// example compare function
//   virtual int compare(void *n1, int field)
//        {return ((classname *) n1)->data > data);}
//  should return (1 if n1 is greater than (self)) else return 0
//  field is the value determined by linked_list::set_sort_field
//  this defaults to 1


#ifndef linkman         
#define linkman
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define loop(controll,first,inside) { (linked_node *)controll=first; \
  if (first) do { inside (linked_node *) controll=controll->next(); } \
  while ((linked_node *) controll!=first); }

#define loopt(type,controll,first,inside) { controll=(type *)(first); \
  if (first) do { inside controll=(type *)(controll->next()); } \
  while (controll!=(type *)(first)); }


#define loop_rev(controll,last,inside) { (linked_node *)controll=last; \
  if (first) do { inside (linked_node *) controll=controll->last(); } \
  while ((linked_node *) controll!=last); }

#define loopct(type,controll,first,cond,inside) { controll=(type *)first; \
  if (first && (cond)) do { inside controll=(type *)controll->next(); } \
  while (controll!=(type *)first && (cond)); }

#define loop_fort(type,controll,first,x) \
  int x=0; \
  if (first) \
     for (controll=(type *)(first); \
          (!x || (controll)!=(type *)(first));\
          controll=(type *)(controll->next()),x++)

#define loop_forct(type,controll,first,cond,x) int x=0; if (first) for \
  (controll=(type *)(first);cond && (!x || controll!=(type *)(first));\
  controll=(type *)(controll->next()),x++)

class linked_node
{
  class linked_node *nextp, *lastp;
public:
  virtual int compare(void *n1, int field)     {return(0);}  // default is = (equal)
  class linked_node *next()              {return nextp;}
  class linked_node *last()              {return lastp;}
  void set_next(class linked_node *p)    {nextp=p;}
  void set_last(class linked_node *p)    {lastp=p;}
  virtual ~linked_node() { ; }
  linked_node() { nextp=NULL; lastp=NULL; }
};

// this is the basic class for all linked_list
// it's features should beself explanitory
// openly use the functions listed after the keyword PUBLIC
// type conversions may be nessary if you derive a class of nodes of your own
// for example shape is an class derived from linked_node.
// to add a shape to linked lis I have to say
// mylist.add_end( (linked_node *) myshape_pointer);
// unlink removes a node from the list via pointers but does not deallocate
// it from the heap
// the destructor for linked_list will get dispose of all the nodes as
// well, so if you don't want something deleted then you must unlink
// it from the list before the destructor is called

class linked_list
{
  class linked_node *fn, *cn;     // first and current nodes
  int nn; char sortby;
public :
  linked_list(linked_node *first=NULL);
  void add_front(class linked_node *p);
  void add_end(class linked_node *p);
  void insert(class linked_node *p);
  void set_sort_field(int x) {sortby=x;}   // this is passed to compare
  class linked_node *current() {return cn;}
  class linked_node *first() {return fn;}
  class linked_node *last() {return fn->last();}
  class linked_node *get_node(int x);
  void set_current(class linked_node *p) {cn=p;}
  void go_first() {cn=fn;}
  void go_end() {cn=fn->last();}
  void go_next() {cn=cn->next();}
  void go_last() {cn=cn->last();}
  int number_nodes()  {return nn;}
  int node_number(linked_node *p);
  int unlink(linked_node *p);
  ~linked_list();
};

#endif



