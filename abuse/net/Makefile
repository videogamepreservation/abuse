
STORE=/tmp

INCLUDE=-I../inc

all : $(STORE)/undrv $(STORE)/tdrv



$(STORE)/undrv : $(STORE)/undrv.o
	g++ $(STORE)/undrv.o -g -o $(STORE)/undrv

$(STORE)/undrv.o : abuse_ndrv.c ../inc/netface.hpp
	g++ $(INCLUDE) abuse_ndrv.c -g -c -o $(STORE)/undrv.o
	

$(STORE)/tdrv : tdrv.c ../inc/netface.hpp
	g++ $(INCLUDE) tdrv.c -g -o $(STORE)/tdrv
