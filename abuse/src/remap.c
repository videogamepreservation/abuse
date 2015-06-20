#include <stdio.h>

#include <stdlib.h>



extern "C" {

void remap_line_asm(void *scr, void *remap, void *seq, int seqlen);

}



main()

{



	unsigned char *s = (unsigned char *) malloc(320*200);

	unsigned char *r, *seq;

	int i,j;



	r = (unsigned char *) malloc(256*256*2);

	r = (unsigned char *) (((unsigned) r + (256*256-1)) & ~(256*256-1));

	seq = (unsigned char *) malloc(40);



	for (j=0 ; j<64 ; j++)

		for (i=0 ; i<256; i++)

			r[j*256+i] = i+j;

	for (i=0 ; i<320*200 ; i++) s[i] = i&0xff;

	for (i=0 ; i<40 ; i++) seq[i] = i;



	for (j=0 ; j<200 ; j++)

                remap_line_asm(s+j*320, r, seq, 40);





}



