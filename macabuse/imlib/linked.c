#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linked.hpp"

// unlink take a node out of the linked list, but does not dispose of the memory
int linked_list::unlink(linked_node *p)
{
  linked_node *q;
  if (first())        // if there are any nodes..
  {
    if (first()==p)  // if they want to unlinkt the first node
    {
      fn->last()->set_next(fn->next());   // set the first's last's next to the first's next
      fn->next()->set_last(fn->last());    // set the first next's last to the first last
      if (fn->next()==fn)                   // if there is ony one node
      { fn=NULL; cn=NULL; }                   // clear the list
      else fn=p->next();                  // else point the first pointer to the next node
      nn--;                              // decrement the number of nodes in the list
      return 1;
    }
    else
    {
      q=first()->next();
      while (q!=p && q!=first())    // find the node in the list
	q=q->next();
      if (q!=first())                  // is it in the list at all?
      {  q->last()->set_next(q->next());   // yes unlink the pointers
	 q->next()->set_last(q->last());
	 nn--;
 	 return 1;
      }                                    // decrement the number of nodes
    }
  }
  return 0;
}


// this function clears all the nodes in a linked list and dispose of the
// memory for each one by calling is't destructor
linked_list::~linked_list()
{
  if (fn)
    fn->last()->set_next(NULL);  // set the last nodes next to NULL
                                 // so we can go until we hit NULL
  while (fn != NULL)          // repeat until we get to that node
  {
    cn=fn->next();
    delete fn;                // delete the old node
    fn=cn;
  }
  cn=NULL;                   // clear the list
  nn=0;                     // set the number of nodes to 0
}

// this function returns the node number a node is in a linked list
// it start at the node and goes backwards until it reaches the first
// node
int linked_list::node_number(linked_node *p)
{
  int x;
  x=1;
  while (p!=first())
  { x++; p=p->last(); }
  return x;
}


// this function returns a pointer to the xth node
class linked_node *linked_list::get_node(int x)
{
  class linked_node *p;
  p=fn;             // start tat the first node
  while (x--!=1) p=p->next();  // go forward X-1 nodes
  return p;
}


// this function adds a node to the end of a linked_list
void linked_list::add_end(class linked_node *p)
{
  nn++;
  if (fn==NULL)        // if there are no nodes, then this one becomes the first
  { fn=p;
    fn->set_next(fn);  // and it poits to itself for first and last
    fn->set_last(fn);
  }
  else
  {
    p->set_last(fn->last());  // otherwise add it in at the end
    p->set_next(fn);
    fn->last()->set_next(p);
    fn->set_last(p);          // the first's last pointer points to the node we just added
  }
}


// to add a node at the fron of the list, just add it at the end and set
// the first pointer to it
void linked_list::add_front(class linked_node *p)
{
  add_end(p);
  fn=p;
}


// insert adds a node in the list according to is sort value
void linked_list::insert(class linked_node *p)
{
  class linked_node *q;

  // if there are nodes, or it belongs at the beginin call add front
  if ((fn==NULL) || (p->compare(fn,sortby)>0))
    add_front(p);
  // else if it goes at the ned call add_end
  else if (p->compare(fn->last(),sortby)<=0)
    add_end(p);
  else      // otherwise we have to find the right spot for it.
  {
    nn++;
    q=fn;
    while (q!=fn->last())  // q starts at the front
    { q=q->next();
      if (p->compare(q,sortby)>0)  // repeat until we find a value greater than the one we are inserting
      { p->set_next(q);
	p->set_last(q->last());     // insert it with pointers here
	q->last()->set_next(p);
	q->set_last(p);
	q=fn->last();         // set q to the last node so we exit the loop
      }
    }
  }
}

linked_list::linked_list(linked_node *first)
{ linked_node *prev;
  fn=first; cn=first; sortby=1; nn=0;
  if (first)
  { cn=first;
    do
    {
      nn++;
      prev=cn;
      cn=cn->next();
    } while (cn && cn!=first);
    if (cn==NULL)
    {
      fn->set_last(prev);
      prev->set_next(fn);
    }
    cn=first;

  }
}
