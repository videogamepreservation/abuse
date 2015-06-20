#include "isllist.hpp"
#include <stdio.h>

typedef isllist<int>::iterator p_int;

main()
{
  isllist<int> l1,l2;
  p_int p,q;
  int i;

  printf("Adding to first list:\n");
  l1.insert(5);
  l1.insert(4);
  l1.insert(3);
  l1.insert(2);
  l1.insert(1);

  printf("List 1: ");
  for (p = l1.begin(); p!=l1.end(); p++)
    printf("%d ",*p);
  printf("\n");

  printf("List 2: ");
  for (p = l2.begin(); p!=l2.end(); p++)
    printf("%d ",*p);
  printf("\n");

  printf("Moving elements!\n");

  l1.move_next(l1.begin_prev(),l2.begin_prev());
  l1.move_next(l1.begin_prev(),l2.begin_prev());
  l1.move_next(l1.begin_prev(),l2.begin_prev());
  l1.move_next(l1.begin_prev(),l2.begin_prev());
  l1.move_next(l1.begin_prev(),l2.begin_prev());
  
  printf("List 1: ");
  for (p = l1.begin(); p!=l1.end(); p++)
    printf("%d ",*p);
  printf("\n");

  printf("List 2: ");
  for (p = l2.begin(); p!=l2.end(); p++)
    printf("%d ",*p);
  printf("\n");

  scanf("%d",&i);
  printf("%d %s.\n",i,(l2.find(i))? "found" : "not found");

  printf("Erasing\n");
  l1.erase_all();
  l2.erase_all();

  printf("List 1: ");
  for (p = l1.begin(); p!=l1.end(); p++)
    printf("%d ",*p);
  printf("\n");

  printf("List 2: ");
  for (p = l2.begin(); p!=l2.end(); p++)
    printf("%d ",*p);
  printf("\n");

}

