

remap.exe : remap.obj putline.obj
	wlink @remap.lnk

remap.obj : remap.c
	set include=$(%WATCOM)\h;inc;../imlib/include;src/net/inc;src/net/unix;src/net/inc
	wpp386 remap.c -fo=remap.obj /omaxet /zp1 /zq /4r /ei /j

putline.obj : putline.asm
        tasm /mx putline.asm

