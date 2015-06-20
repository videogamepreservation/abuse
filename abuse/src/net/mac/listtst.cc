#include "isllist.hpp"
#include <stdio.h>

isllist<int> tst;
typedef isllist<int>::iterator p_int;

main()
{
	tst.insert(5);
	tst.insert(4);
	tst.insert(3);
	tst.insert(2);
	tst.insert(1);

	for ( p_int p = tst.begin(); p != tst.end(); ++p)
		printf("%d, ", *p);
	
	p_int x = tst.begin_prev();
	if (tst.find_prev(x, 3))
		tst.insert_next(x, 9);
	
	for ( p = tst.begin(); p != tst.end(); ++p)
		printf("%d, ", *p);
	
	tst.erase_all();
}
