all : /usr/local/bin/sgi_sdrv

/usr/local/bin/sgi_sdrv : sgi_sdrv.c gen_drv.c
	g++ sgi_sdrv.c -g -o /usr/local/bin/sgi_sdrv -laudio
	
