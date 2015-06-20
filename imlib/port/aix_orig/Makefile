#CC=xlC
#CFLAGS=-+ -O2 -I/usr/include/UMS -I/usr/lpp/som/include
#LIBS=-lUMSobj
CC=cc
CFLAGS=-qcpluscmt -O2 -I/usr/include/UMS -I/usr/lpp/som/include
LIBS=-lUMSobj

all : /usr/local/bin/aix_sdrv

/usr/local/bin/aix_sdrv : aix_sdrv.c gen_drv.c
	$(CC) $(CFLAGS) aix_sdrv.c -o /usr/local/bin/aix_sdrv $(LIBS)

