#include <stdio.h>

main()
{
	FILE *f;
	
	f = fopen(":test:tt.txt","wt");
	fprintf(f,"hello, world\n");
	fclose(f);
}