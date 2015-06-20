all : opt

debug : ipxtest.exe
	echo Made debug

opt : ipxtesto.exe
	echo Made opt

WATCOM_ipxtest_debug_o_files = &
	.\wat\debug\ipxtest.obj

ipxtest.exe : $(WATCOM_ipxtest_debug_o_files)
	wlink @ipxtest.lnk

.\wat\debug\ipxtest.obj : ipxtest.c
	set include=$(%WATCOM)\h;../../include
	wpp386 ipxtest.c -fo=.\wat\debug\ipxtest.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\ipxtest.obj : ../../include\system.h
.\wat\debug\ipxtest.obj : ../../include\macs.hpp
.\wat\debug\ipxtest.obj : ../../include\doscall.hpp
.\wat\debug\ipxtest.obj : doscall.c

WATCOM_ipxtesto_opt_o_files = &
	.\wat\opt\ipxtest.obj

ipxtesto.exe : $(WATCOM_ipxtesto_opt_o_files)
	wlink @ipxtesto.lnk

.\wat\opt\ipxtest.obj : ipxtest.c
	set include=$(%WATCOM)\h;../../include
	wpp386 ipxtest.c -fo=.\wat\opt\ipxtest.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\ipxtest.obj : ../../include\system.h
.\wat\opt\ipxtest.obj : ../../include\macs.hpp
.\wat\opt\ipxtest.obj : ../../include\doscall.hpp
.\wat\opt\ipxtest.obj : doscall.c

