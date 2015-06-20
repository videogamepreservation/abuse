all : ipxtest.exe

INCLUDE=..\..\..\include


wat\debug\doscall.obj : doscall.c
	set include=$(%WATCOM)\h;$(INCLUDE)
	wpp386 doscall.c -fo=wat\debug\doscall.obj /zq /d2 -DMANAGE_MEM 


wat\debug\ipx.obj : ipx.c
	set include=$(%WATCOM)\h;$(INCLUDE)
	wpp386 ipx.c -fo=wat\debug\ipx.obj /zq /d2 -DMANAGE_MEM 

wat\debug\ipxtest.obj : ipxtest.c
	set include=$(%WATCOM)\h;$(INCLUDE)
	wpp386 ipxtest.c -fo=wat\debug\ipxtest.obj /zq /d2 -DMANAGE_MEM 

ipxtest.exe : wat\debug\ipxtest.obj wat\debug\ipx.obj wat\debug\doscall.obj
	wlink @ipxtest.lnk	
