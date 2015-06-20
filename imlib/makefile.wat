all : opt

debug : .\wat\debug\image.lib .\wat\debug\packet.lib .\wat\debug\gui.lib .\wat\debug\sound.lib .\wat\debug\net.lib .\wat\debug\dir.lib .\wat\debug\winman.lib .\wat\debug\joy.lib .\wat\debug\time.lib
	echo Made debug

opt : .\wat\opt\image.lib .\wat\opt\packet.lib .\wat\opt\gui.lib .\wat\opt\sound.lib .\wat\opt\net.lib .\wat\opt\dir.lib .\wat\opt\winman.lib .\wat\opt\joy.lib .\wat\opt\time.lib
	echo Made opt

WATCOM_image_debug_o_files = &
	.\wat\debug\filter.obj &
	.\wat\debug\gifread.obj &
	.\wat\debug\globals.obj &
	.\wat\debug\image.obj &
	.\wat\debug\linked.obj &
	.\wat\debug\input.obj &
	.\wat\debug\mdlread.obj &
	.\wat\debug\palette.obj &
	.\wat\debug\ppmread.obj &
	.\wat\debug\include.obj &
	.\wat\debug\xwdread.obj &
	.\wat\debug\fonts.obj &
	.\wat\debug\decoder.obj &
	.\wat\debug\loader.obj &
	.\wat\debug\glread.obj &
	.\wat\debug\texture.obj &
	.\wat\debug\specs.obj &
	.\wat\debug\supmorph.obj &
	.\wat\debug\image24.obj &
	.\wat\debug\pcxread.obj &
	.\wat\debug\timage.obj &
	.\wat\debug\jmalloc.obj &
	.\wat\debug\jrand.obj &
	.\wat\debug\lbmread.obj &
	.\wat\debug\targa.obj &
	.\wat\debug\keys.obj &
	.\wat\debug\dprint.obj &
	.\wat\debug\status.obj &
	.\wat\debug\visobj.obj

WATCOM_packet_debug_o_files = &
	.\wat\debug\packet.obj

WATCOM_gui_debug_o_files = &
	.\wat\debug\pmenu.obj &
	.\wat\debug\scroller.obj &
	.\wat\debug\filesel.obj &
	.\wat\debug\tools.obj &
	.\wat\debug\guistat.obj

WATCOM_sound_debug_o_files = &
	port\dos4gw\wat\debug\sound.obj &
	.\wat\debug\readwav.obj &
	port\dos4gw\wat\debug\profile.obj

WATCOM_net_debug_o_files = &
	port\dos4gw\wat\debug\jnet.obj &
	port\dos4gw\wat\debug\ipx.obj &
	port\dos4gw\wat\debug\bwtcp.obj

WATCOM_dir_debug_o_files = &
	port\dos4gw\wat\debug\jdir.obj

WATCOM_winman_debug_o_files = &
	port\dos4gw\wat\debug\video.obj &
	port\dos4gw\wat\debug\mouse.obj &
	port\dos4gw\wat\debug\event.obj &
	port\dos4gw\wat\debug\doscall.obj &
	.\wat\debug\jwindow.obj

WATCOM_joy_debug_o_files = &
	port\dos4gw\wat\debug\joystick.obj

WATCOM_time_debug_o_files = &
	port\dos4gw\wat\debug\timing.obj

.\wat\debug\time.lib : $(WATCOM_time_debug_o_files)
	wlib /n @.\wat\debug\time.lnk

port\dos4gw\wat\debug\timing.obj : port\dos4gw\timing.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\timing.c -fo=port\dos4gw\wat\debug\timing.obj /zq /d2 -DMANAGE_MEM 

port\dos4gw\wat\debug\timing.obj : include\dprint.hpp
port\dos4gw\wat\debug\timing.obj : include\timing.hpp

.\wat\debug\joy.lib : $(WATCOM_joy_debug_o_files)
	wlib /n @.\wat\debug\joy.lnk

port\dos4gw\wat\debug\joystick.obj : port\dos4gw\joystick.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\joystick.c -fo=port\dos4gw\wat\debug\joystick.obj /zq /d2 -DMANAGE_MEM 

port\dos4gw\wat\debug\joystick.obj : include\joy.hpp

.\wat\debug\winman.lib : $(WATCOM_winman_debug_o_files)
	wlib /n @.\wat\debug\winman.lnk

port\dos4gw\wat\debug\video.obj : port\dos4gw\video.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\video.c -fo=port\dos4gw\wat\debug\video.obj /zq /d2 -DMANAGE_MEM 

port\dos4gw\wat\debug\video.obj : include\doscall.hpp
port\dos4gw\wat\debug\video.obj : include\bitmap.h
port\dos4gw\wat\debug\video.obj : include\macs.hpp
port\dos4gw\wat\debug\video.obj : include\dos.h
port\dos4gw\wat\debug\video.obj : include\jmalloc.hpp
port\dos4gw\wat\debug\video.obj : include\specs.hpp
port\dos4gw\wat\debug\video.obj : include\palette.hpp
port\dos4gw\wat\debug\video.obj : include\linked.hpp
port\dos4gw\wat\debug\video.obj : include\image.hpp
port\dos4gw\wat\debug\video.obj : include\video.hpp
port\dos4gw\wat\debug\video.obj : include\system.h
port\dos4gw\wat\debug\video.obj : include\globals.hpp
port\dos4gw\wat\debug\mouse.obj : port\dos4gw\mouse.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\mouse.c -fo=port\dos4gw\wat\debug\mouse.obj /zq /d2 -DMANAGE_MEM 

port\dos4gw\wat\debug\mouse.obj : include\doscall.hpp
port\dos4gw\wat\debug\mouse.obj : include\dprint.hpp
port\dos4gw\wat\debug\mouse.obj : include\mouse.hpp
port\dos4gw\wat\debug\mouse.obj : include\monoprnt.hpp
port\dos4gw\wat\debug\mouse.obj : include\mdlread.hpp
port\dos4gw\wat\debug\mouse.obj : include\filter.hpp
port\dos4gw\wat\debug\mouse.obj : include\macs.hpp
port\dos4gw\wat\debug\mouse.obj : include\sprite.hpp
port\dos4gw\wat\debug\mouse.obj : include\jmalloc.hpp
port\dos4gw\wat\debug\mouse.obj : include\specs.hpp
port\dos4gw\wat\debug\mouse.obj : include\palette.hpp
port\dos4gw\wat\debug\mouse.obj : include\linked.hpp
port\dos4gw\wat\debug\mouse.obj : include\image.hpp
port\dos4gw\wat\debug\mouse.obj : include\system.h
port\dos4gw\wat\debug\mouse.obj : include\video.hpp
port\dos4gw\wat\debug\event.obj : port\dos4gw\event.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\event.c -fo=port\dos4gw\wat\debug\event.obj /zq /d2 -DMANAGE_MEM 

port\dos4gw\wat\debug\event.obj : include\monoprnt.hpp
port\dos4gw\wat\debug\event.obj : include\keys.hpp
port\dos4gw\wat\debug\event.obj : include\event.hpp
port\dos4gw\wat\debug\event.obj : include\conio.h
port\dos4gw\wat\debug\event.obj : include\sprite.hpp
port\dos4gw\wat\debug\event.obj : include\mouse.hpp
port\dos4gw\wat\debug\event.obj : include\macs.hpp
port\dos4gw\wat\debug\event.obj : include\dos.h
port\dos4gw\wat\debug\event.obj : include\video.hpp
port\dos4gw\wat\debug\event.obj : include\mdlread.hpp
port\dos4gw\wat\debug\event.obj : include\jmalloc.hpp
port\dos4gw\wat\debug\event.obj : include\specs.hpp
port\dos4gw\wat\debug\event.obj : include\palette.hpp
port\dos4gw\wat\debug\event.obj : include\linked.hpp
port\dos4gw\wat\debug\event.obj : include\image.hpp
port\dos4gw\wat\debug\event.obj : include\system.h
port\dos4gw\wat\debug\doscall.obj : port\dos4gw\doscall.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\doscall.c -fo=port\dos4gw\wat\debug\doscall.obj /zq /d2 -DMANAGE_MEM 

port\dos4gw\wat\debug\doscall.obj : include\system.h
port\dos4gw\wat\debug\doscall.obj : include\macs.hpp
port\dos4gw\wat\debug\doscall.obj : include\doscall.hpp
.\wat\debug\jwindow.obj : jwindow.c
	set include=$(%WATCOM)\h;include
	wpp386 jwindow.c -fo=.\wat\debug\jwindow.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\jwindow.obj : include\timage.hpp
.\wat\debug\jwindow.obj : include\fonts.hpp
.\wat\debug\jwindow.obj : include\jwindow.hpp
.\wat\debug\jwindow.obj : include\filter.hpp
.\wat\debug\jwindow.obj : include\mouse.hpp
.\wat\debug\jwindow.obj : include\macs.hpp
.\wat\debug\jwindow.obj : include\sprite.hpp
.\wat\debug\jwindow.obj : include\keys.hpp
.\wat\debug\jwindow.obj : include\event.hpp
.\wat\debug\jwindow.obj : include\jmalloc.hpp
.\wat\debug\jwindow.obj : include\specs.hpp
.\wat\debug\jwindow.obj : include\palette.hpp
.\wat\debug\jwindow.obj : include\linked.hpp
.\wat\debug\jwindow.obj : include\image.hpp
.\wat\debug\jwindow.obj : include\system.h
.\wat\debug\jwindow.obj : include\video.hpp

.\wat\debug\dir.lib : $(WATCOM_dir_debug_o_files)
	wlib /n @.\wat\debug\dir.lnk

port\dos4gw\wat\debug\jdir.obj : port\dos4gw\jdir.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\jdir.c -fo=port\dos4gw\wat\debug\jdir.obj /zq /d2 -DMANAGE_MEM 

port\dos4gw\wat\debug\jdir.obj : include\jmalloc.hpp

.\wat\debug\net.lib : $(WATCOM_net_debug_o_files)
	wlib /n @.\wat\debug\net.lnk

port\dos4gw\wat\debug\jnet.obj : port\dos4gw\jnet.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\jnet.c -fo=port\dos4gw\wat\debug\jnet.obj /zq /d2 -DMANAGE_MEM 

port\dos4gw\wat\debug\jnet.obj : include\jmalloc.hpp
port\dos4gw\wat\debug\jnet.obj : include\dprint.hpp
port\dos4gw\wat\debug\jnet.obj : include\packet.hpp
port\dos4gw\wat\debug\jnet.obj : include\system.h
port\dos4gw\wat\debug\jnet.obj : include\macs.hpp
port\dos4gw\wat\debug\jnet.obj : include\jnet.hpp
port\dos4gw\wat\debug\ipx.obj : port\dos4gw\ipx.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\ipx.c -fo=port\dos4gw\wat\debug\ipx.obj /zq /d2 -DMANAGE_MEM 

port\dos4gw\wat\debug\ipx.obj : include\timing.hpp
port\dos4gw\wat\debug\ipx.obj : include\doscall.hpp
port\dos4gw\wat\debug\ipx.obj : include\jmalloc.hpp
port\dos4gw\wat\debug\ipx.obj : include\dprint.hpp
port\dos4gw\wat\debug\ipx.obj : include\packet.hpp
port\dos4gw\wat\debug\ipx.obj : include\system.h
port\dos4gw\wat\debug\ipx.obj : include\macs.hpp
port\dos4gw\wat\debug\ipx.obj : include\jnet.hpp
port\dos4gw\wat\debug\bwtcp.obj : port\dos4gw\bwtcp.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\bwtcp.c -fo=port\dos4gw\wat\debug\bwtcp.obj /zq /d2 -DMANAGE_MEM 

port\dos4gw\wat\debug\bwtcp.obj : include\doscall.hpp
port\dos4gw\wat\debug\bwtcp.obj : include\jmalloc.hpp
port\dos4gw\wat\debug\bwtcp.obj : include\dprint.hpp
port\dos4gw\wat\debug\bwtcp.obj : include\packet.hpp
port\dos4gw\wat\debug\bwtcp.obj : include\system.h
port\dos4gw\wat\debug\bwtcp.obj : include\macs.hpp
port\dos4gw\wat\debug\bwtcp.obj : include\jnet.hpp

.\wat\debug\sound.lib : $(WATCOM_sound_debug_o_files)
	wlib /n @.\wat\debug\sound.lnk

port\dos4gw\wat\debug\sound.obj : port\dos4gw\sound.c
	set include=$(%WATCOM)\h;include;c:\sos\include
	wpp386 port\dos4gw\sound.c -fo=port\dos4gw\wat\debug\sound.obj /zq /d2 -DMANAGE_MEM 

port\dos4gw\wat\debug\sound.obj : include\timing.hpp
port\dos4gw\wat\debug\sound.obj : include\macs.hpp
port\dos4gw\wat\debug\sound.obj : include\doscall.hpp
port\dos4gw\wat\debug\sound.obj : include\dprint.hpp
port\dos4gw\wat\debug\sound.obj : include\sound.hpp
port\dos4gw\wat\debug\sound.obj : include\system.h
port\dos4gw\wat\debug\sound.obj : include\jmalloc.hpp
port\dos4gw\wat\debug\sound.obj : include\linked.hpp
port\dos4gw\wat\debug\sound.obj : include\specs.hpp
port\dos4gw\wat\debug\sound.obj : include\readwav.hpp
port\dos4gw\wat\debug\sound.obj : c:\sos\include\sosmdata.h
port\dos4gw\wat\debug\sound.obj : c:\sos\include\sosmfnct.h
port\dos4gw\wat\debug\sound.obj : c:\sos\include\sosm.h
port\dos4gw\wat\debug\sound.obj : c:\sos\include\sosfnct.h
port\dos4gw\wat\debug\sound.obj : c:\sos\include\sosdata.h
port\dos4gw\wat\debug\sound.obj : c:\sos\include\sosdefs.h
port\dos4gw\wat\debug\sound.obj : c:\sos\include\sos.h
.\wat\debug\readwav.obj : readwav.c
	set include=$(%WATCOM)\h;include;c:\sos\include
	wpp386 readwav.c -fo=.\wat\debug\readwav.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\readwav.obj : include\dprint.hpp
.\wat\debug\readwav.obj : include\macs.hpp
.\wat\debug\readwav.obj : include\system.h
.\wat\debug\readwav.obj : include\jmalloc.hpp
.\wat\debug\readwav.obj : include\linked.hpp
.\wat\debug\readwav.obj : include\specs.hpp
.\wat\debug\readwav.obj : include\readwav.hpp
port\dos4gw\wat\debug\profile.obj : port\dos4gw\profile.c
	set include=$(%WATCOM)\h;include;c:\sos\include
	wpp386 port\dos4gw\profile.c -fo=port\dos4gw\wat\debug\profile.obj /zq /d2 -DMANAGE_MEM 

port\dos4gw\wat\debug\profile.obj : c:\sos\include\sosfnct.h
port\dos4gw\wat\debug\profile.obj : c:\sos\include\sosdata.h
port\dos4gw\wat\debug\profile.obj : c:\sos\include\sosdefs.h
port\dos4gw\wat\debug\profile.obj : c:\sos\include\sos.h

.\wat\debug\gui.lib : $(WATCOM_gui_debug_o_files)
	wlib /n @.\wat\debug\gui.lnk

.\wat\debug\pmenu.obj : pmenu.c
	set include=$(%WATCOM)\h;include
	wpp386 pmenu.c -fo=.\wat\debug\pmenu.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\pmenu.obj : include\timage.hpp
.\wat\debug\pmenu.obj : include\fonts.hpp
.\wat\debug\pmenu.obj : include\filter.hpp
.\wat\debug\pmenu.obj : include\mouse.hpp
.\wat\debug\pmenu.obj : include\macs.hpp
.\wat\debug\pmenu.obj : include\sprite.hpp
.\wat\debug\pmenu.obj : include\keys.hpp
.\wat\debug\pmenu.obj : include\event.hpp
.\wat\debug\pmenu.obj : include\specs.hpp
.\wat\debug\pmenu.obj : include\palette.hpp
.\wat\debug\pmenu.obj : include\linked.hpp
.\wat\debug\pmenu.obj : include\image.hpp
.\wat\debug\pmenu.obj : include\system.h
.\wat\debug\pmenu.obj : include\video.hpp
.\wat\debug\pmenu.obj : include\jwindow.hpp
.\wat\debug\pmenu.obj : include\input.hpp
.\wat\debug\pmenu.obj : include\jmalloc.hpp
.\wat\debug\pmenu.obj : include\pmenu.hpp
.\wat\debug\scroller.obj : scroller.c
	set include=$(%WATCOM)\h;include
	wpp386 scroller.c -fo=.\wat\debug\scroller.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\scroller.obj : include\timage.hpp
.\wat\debug\scroller.obj : include\fonts.hpp
.\wat\debug\scroller.obj : include\filter.hpp
.\wat\debug\scroller.obj : include\mouse.hpp
.\wat\debug\scroller.obj : include\macs.hpp
.\wat\debug\scroller.obj : include\sprite.hpp
.\wat\debug\scroller.obj : include\keys.hpp
.\wat\debug\scroller.obj : include\event.hpp
.\wat\debug\scroller.obj : include\jmalloc.hpp
.\wat\debug\scroller.obj : include\specs.hpp
.\wat\debug\scroller.obj : include\palette.hpp
.\wat\debug\scroller.obj : include\linked.hpp
.\wat\debug\scroller.obj : include\image.hpp
.\wat\debug\scroller.obj : include\system.h
.\wat\debug\scroller.obj : include\video.hpp
.\wat\debug\scroller.obj : include\jwindow.hpp
.\wat\debug\scroller.obj : include\input.hpp
.\wat\debug\scroller.obj : include\scroller.hpp
.\wat\debug\filesel.obj : filesel.c
	set include=$(%WATCOM)\h;include
	wpp386 filesel.c -fo=.\wat\debug\filesel.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\filesel.obj : include\jdir.hpp
.\wat\debug\filesel.obj : include\scroller.hpp
.\wat\debug\filesel.obj : include\input.hpp
.\wat\debug\filesel.obj : include\timage.hpp
.\wat\debug\filesel.obj : include\fonts.hpp
.\wat\debug\filesel.obj : include\filter.hpp
.\wat\debug\filesel.obj : include\mouse.hpp
.\wat\debug\filesel.obj : include\macs.hpp
.\wat\debug\filesel.obj : include\sprite.hpp
.\wat\debug\filesel.obj : include\keys.hpp
.\wat\debug\filesel.obj : include\event.hpp
.\wat\debug\filesel.obj : include\jmalloc.hpp
.\wat\debug\filesel.obj : include\specs.hpp
.\wat\debug\filesel.obj : include\palette.hpp
.\wat\debug\filesel.obj : include\linked.hpp
.\wat\debug\filesel.obj : include\image.hpp
.\wat\debug\filesel.obj : include\system.h
.\wat\debug\filesel.obj : include\video.hpp
.\wat\debug\filesel.obj : include\jwindow.hpp
.\wat\debug\filesel.obj : include\filesel.hpp
.\wat\debug\tools.obj : tools.c
	set include=$(%WATCOM)\h;include
	wpp386 tools.c -fo=.\wat\debug\tools.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\tools.obj : include\visobj.hpp
.\wat\debug\tools.obj : include\scroller.hpp
.\wat\debug\tools.obj : include\input.hpp
.\wat\debug\tools.obj : include\timage.hpp
.\wat\debug\tools.obj : include\fonts.hpp
.\wat\debug\tools.obj : include\filter.hpp
.\wat\debug\tools.obj : include\mouse.hpp
.\wat\debug\tools.obj : include\macs.hpp
.\wat\debug\tools.obj : include\sprite.hpp
.\wat\debug\tools.obj : include\keys.hpp
.\wat\debug\tools.obj : include\event.hpp
.\wat\debug\tools.obj : include\jmalloc.hpp
.\wat\debug\tools.obj : include\specs.hpp
.\wat\debug\tools.obj : include\palette.hpp
.\wat\debug\tools.obj : include\linked.hpp
.\wat\debug\tools.obj : include\image.hpp
.\wat\debug\tools.obj : include\system.h
.\wat\debug\tools.obj : include\video.hpp
.\wat\debug\tools.obj : include\jwindow.hpp
.\wat\debug\tools.obj : include\tools.hpp
.\wat\debug\guistat.obj : guistat.c
	set include=$(%WATCOM)\h;include
	wpp386 guistat.c -fo=.\wat\debug\guistat.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\guistat.obj : include\guistat.hpp
.\wat\debug\guistat.obj : include\timing.hpp
.\wat\debug\guistat.obj : include\timage.hpp
.\wat\debug\guistat.obj : include\fonts.hpp
.\wat\debug\guistat.obj : include\filter.hpp
.\wat\debug\guistat.obj : include\mouse.hpp
.\wat\debug\guistat.obj : include\macs.hpp
.\wat\debug\guistat.obj : include\sprite.hpp
.\wat\debug\guistat.obj : include\keys.hpp
.\wat\debug\guistat.obj : include\event.hpp
.\wat\debug\guistat.obj : include\jmalloc.hpp
.\wat\debug\guistat.obj : include\specs.hpp
.\wat\debug\guistat.obj : include\palette.hpp
.\wat\debug\guistat.obj : include\linked.hpp
.\wat\debug\guistat.obj : include\image.hpp
.\wat\debug\guistat.obj : include\system.h
.\wat\debug\guistat.obj : include\video.hpp
.\wat\debug\guistat.obj : include\jwindow.hpp
.\wat\debug\guistat.obj : include\visobj.hpp
.\wat\debug\guistat.obj : include\status.hpp

.\wat\debug\packet.lib : $(WATCOM_packet_debug_o_files)
	wlib /n @.\wat\debug\packet.lnk

.\wat\debug\packet.obj : packet.c
	set include=$(%WATCOM)\h;include
	wpp386 packet.c -fo=.\wat\debug\packet.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\packet.obj : include\jmalloc.hpp
.\wat\debug\packet.obj : include\system.h
.\wat\debug\packet.obj : include\macs.hpp
.\wat\debug\packet.obj : include\packet.hpp

.\wat\debug\image.lib : $(WATCOM_image_debug_o_files)
	wlib /n @.\wat\debug\image.lnk

.\wat\debug\filter.obj : filter.c
	set include=$(%WATCOM)\h;include
	wpp386 filter.c -fo=.\wat\debug\filter.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\filter.obj : include\filter.hpp
.\wat\debug\filter.obj : include\macs.hpp
.\wat\debug\filter.obj : include\system.h
.\wat\debug\filter.obj : include\jmalloc.hpp
.\wat\debug\filter.obj : include\specs.hpp
.\wat\debug\filter.obj : include\palette.hpp
.\wat\debug\filter.obj : include\linked.hpp
.\wat\debug\filter.obj : include\image.hpp
.\wat\debug\gifread.obj : gifread.c
	set include=$(%WATCOM)\h;include
	wpp386 gifread.c -fo=.\wat\debug\gifread.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\gifread.obj : include\macs.hpp
.\wat\debug\gifread.obj : include\dir.h
.\wat\debug\gifread.obj : include\dos.h
.\wat\debug\gifread.obj : include\std.h
.\wat\debug\gifread.obj : include\gifdecod.hpp
.\wat\debug\gifread.obj : include\video.hpp
.\wat\debug\gifread.obj : include\system.h
.\wat\debug\gifread.obj : include\jmalloc.hpp
.\wat\debug\gifread.obj : include\specs.hpp
.\wat\debug\gifread.obj : include\palette.hpp
.\wat\debug\gifread.obj : include\linked.hpp
.\wat\debug\gifread.obj : include\image.hpp
.\wat\debug\gifread.obj : include\gifread.hpp
.\wat\debug\globals.obj : globals.c
	set include=$(%WATCOM)\h;include
	wpp386 globals.c -fo=.\wat\debug\globals.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\globals.obj : globals.c
.\wat\debug\image.obj : image.c
	set include=$(%WATCOM)\h;include
	wpp386 image.c -fo=.\wat\debug\image.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\image.obj : include\macs.hpp
.\wat\debug\image.obj : include\system.h
.\wat\debug\image.obj : include\jmalloc.hpp
.\wat\debug\image.obj : include\specs.hpp
.\wat\debug\image.obj : include\palette.hpp
.\wat\debug\image.obj : include\linked.hpp
.\wat\debug\image.obj : include\image.hpp
.\wat\debug\linked.obj : linked.c
	set include=$(%WATCOM)\h;include
	wpp386 linked.c -fo=.\wat\debug\linked.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\linked.obj : include\linked.hpp
.\wat\debug\input.obj : input.c
	set include=$(%WATCOM)\h;include
	wpp386 input.c -fo=.\wat\debug\input.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\input.obj : include\timage.hpp
.\wat\debug\input.obj : include\fonts.hpp
.\wat\debug\input.obj : include\filter.hpp
.\wat\debug\input.obj : include\mouse.hpp
.\wat\debug\input.obj : include\macs.hpp
.\wat\debug\input.obj : include\sprite.hpp
.\wat\debug\input.obj : include\keys.hpp
.\wat\debug\input.obj : include\event.hpp
.\wat\debug\input.obj : include\jmalloc.hpp
.\wat\debug\input.obj : include\specs.hpp
.\wat\debug\input.obj : include\palette.hpp
.\wat\debug\input.obj : include\linked.hpp
.\wat\debug\input.obj : include\image.hpp
.\wat\debug\input.obj : include\system.h
.\wat\debug\input.obj : include\video.hpp
.\wat\debug\input.obj : include\jwindow.hpp
.\wat\debug\input.obj : include\input.hpp
.\wat\debug\mdlread.obj : mdlread.c
	set include=$(%WATCOM)\h;include
	wpp386 mdlread.c -fo=.\wat\debug\mdlread.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\mdlread.obj : include\macs.hpp
.\wat\debug\mdlread.obj : include\image.hpp
.\wat\debug\mdlread.obj : include\system.h
.\wat\debug\mdlread.obj : include\jmalloc.hpp
.\wat\debug\mdlread.obj : include\specs.hpp
.\wat\debug\mdlread.obj : include\linked.hpp
.\wat\debug\mdlread.obj : include\palette.hpp
.\wat\debug\mdlread.obj : include\mdlread.hpp
.\wat\debug\palette.obj : palette.c
	set include=$(%WATCOM)\h;include
	wpp386 palette.c -fo=.\wat\debug\palette.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\palette.obj : include\filter.hpp
.\wat\debug\palette.obj : include\video.hpp
.\wat\debug\palette.obj : include\dos.h
.\wat\debug\palette.obj : include\macs.hpp
.\wat\debug\palette.obj : include\image.hpp
.\wat\debug\palette.obj : include\system.h
.\wat\debug\palette.obj : include\jmalloc.hpp
.\wat\debug\palette.obj : include\specs.hpp
.\wat\debug\palette.obj : include\linked.hpp
.\wat\debug\palette.obj : include\palette.hpp
.\wat\debug\ppmread.obj : ppmread.c
	set include=$(%WATCOM)\h;include
	wpp386 ppmread.c -fo=.\wat\debug\ppmread.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\ppmread.obj : include\ppmread.hpp
.\wat\debug\ppmread.obj : include\macs.hpp
.\wat\debug\ppmread.obj : include\system.h
.\wat\debug\ppmread.obj : include\jmalloc.hpp
.\wat\debug\ppmread.obj : include\specs.hpp
.\wat\debug\ppmread.obj : include\palette.hpp
.\wat\debug\ppmread.obj : include\linked.hpp
.\wat\debug\ppmread.obj : include\image.hpp
.\wat\debug\include.obj : include.c
	set include=$(%WATCOM)\h;include
	wpp386 include.c -fo=.\wat\debug\include.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\include.obj : include\system.h
.\wat\debug\include.obj : include\jmalloc.hpp
.\wat\debug\include.obj : include\specs.hpp
.\wat\debug\include.obj : include\palette.hpp
.\wat\debug\include.obj : include\linked.hpp
.\wat\debug\include.obj : include\image.hpp
.\wat\debug\include.obj : include\include.hpp
.\wat\debug\xwdread.obj : xwdread.c
	set include=$(%WATCOM)\h;include
	wpp386 xwdread.c -fo=.\wat\debug\xwdread.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\xwdread.obj : include\image24.hpp
.\wat\debug\xwdread.obj : include\main.hpp
.\wat\debug\xwdread.obj : include\dos.h
.\wat\debug\xwdread.obj : include\video.hpp
.\wat\debug\xwdread.obj : include\mouse.hpp
.\wat\debug\xwdread.obj : include\macs.hpp
.\wat\debug\xwdread.obj : include\sprite.hpp
.\wat\debug\xwdread.obj : include\keys.hpp
.\wat\debug\xwdread.obj : include\event.hpp
.\wat\debug\xwdread.obj : include\system.h
.\wat\debug\xwdread.obj : include\jmalloc.hpp
.\wat\debug\xwdread.obj : include\specs.hpp
.\wat\debug\xwdread.obj : include\palette.hpp
.\wat\debug\xwdread.obj : include\linked.hpp
.\wat\debug\xwdread.obj : include\image.hpp
.\wat\debug\xwdread.obj : include\filter.hpp
.\wat\debug\fonts.obj : fonts.c
	set include=$(%WATCOM)\h;include
	wpp386 fonts.c -fo=.\wat\debug\fonts.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\fonts.obj : include\filter.hpp
.\wat\debug\fonts.obj : include\macs.hpp
.\wat\debug\fonts.obj : include\timage.hpp
.\wat\debug\fonts.obj : include\system.h
.\wat\debug\fonts.obj : include\jmalloc.hpp
.\wat\debug\fonts.obj : include\specs.hpp
.\wat\debug\fonts.obj : include\palette.hpp
.\wat\debug\fonts.obj : include\linked.hpp
.\wat\debug\fonts.obj : include\image.hpp
.\wat\debug\fonts.obj : include\fonts.hpp
.\wat\debug\decoder.obj : decoder.c
	set include=$(%WATCOM)\h;include
	wpp386 decoder.c -fo=.\wat\debug\decoder.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\decoder.obj : include\macs.hpp
.\wat\debug\decoder.obj : include\system.h
.\wat\debug\decoder.obj : include\jmalloc.hpp
.\wat\debug\decoder.obj : include\specs.hpp
.\wat\debug\decoder.obj : include\palette.hpp
.\wat\debug\decoder.obj : include\linked.hpp
.\wat\debug\decoder.obj : include\image.hpp
.\wat\debug\decoder.obj : include\errs.h
.\wat\debug\loader.obj : loader.c
	set include=$(%WATCOM)\h;include
	wpp386 loader.c -fo=.\wat\debug\loader.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\loader.obj : include\targa.hpp
.\wat\debug\loader.obj : include\lbmread.hpp
.\wat\debug\loader.obj : include\pcxread.hpp
.\wat\debug\loader.obj : include\glread.hpp
.\wat\debug\loader.obj : include\xwdread.hpp
.\wat\debug\loader.obj : include\ppmread.hpp
.\wat\debug\loader.obj : include\mdlread.hpp
.\wat\debug\loader.obj : include\filter.hpp
.\wat\debug\loader.obj : include\macs.hpp
.\wat\debug\loader.obj : include\image24.hpp
.\wat\debug\loader.obj : include\system.h
.\wat\debug\loader.obj : include\jmalloc.hpp
.\wat\debug\loader.obj : include\specs.hpp
.\wat\debug\loader.obj : include\palette.hpp
.\wat\debug\loader.obj : include\linked.hpp
.\wat\debug\loader.obj : include\image.hpp
.\wat\debug\loader.obj : include\loader.hpp
.\wat\debug\glread.obj : glread.c
	set include=$(%WATCOM)\h;include
	wpp386 glread.c -fo=.\wat\debug\glread.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\glread.obj : include\video.hpp
.\wat\debug\glread.obj : include\jmalloc.hpp
.\wat\debug\glread.obj : include\specs.hpp
.\wat\debug\glread.obj : include\palette.hpp
.\wat\debug\glread.obj : include\linked.hpp
.\wat\debug\glread.obj : include\image.hpp
.\wat\debug\glread.obj : include\system.h
.\wat\debug\glread.obj : include\macs.hpp
.\wat\debug\texture.obj : texture.c
	set include=$(%WATCOM)\h;include
	wpp386 texture.c -fo=.\wat\debug\texture.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\texture.obj : include\mouse.hpp
.\wat\debug\texture.obj : include\sprite.hpp
.\wat\debug\texture.obj : include\keys.hpp
.\wat\debug\texture.obj : include\event.hpp
.\wat\debug\texture.obj : include\macs.hpp
.\wat\debug\texture.obj : include\video.hpp
.\wat\debug\texture.obj : include\system.h
.\wat\debug\texture.obj : include\jmalloc.hpp
.\wat\debug\texture.obj : include\specs.hpp
.\wat\debug\texture.obj : include\palette.hpp
.\wat\debug\texture.obj : include\linked.hpp
.\wat\debug\texture.obj : include\image.hpp
.\wat\debug\specs.obj : specs.c
	set include=$(%WATCOM)\h;include
	wpp386 specs.c -fo=.\wat\debug\specs.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\specs.obj : include\dprint.hpp
.\wat\debug\specs.obj : include\system.h
.\wat\debug\specs.obj : include\jmalloc.hpp
.\wat\debug\specs.obj : include\specs.hpp
.\wat\debug\specs.obj : include\palette.hpp
.\wat\debug\specs.obj : include\linked.hpp
.\wat\debug\specs.obj : include\image.hpp
.\wat\debug\supmorph.obj : supmorph.c
	set include=$(%WATCOM)\h;include
	wpp386 supmorph.c -fo=.\wat\debug\supmorph.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\supmorph.obj : include\jrand.hpp
.\wat\debug\supmorph.obj : include\video.hpp
.\wat\debug\supmorph.obj : include\timing.hpp
.\wat\debug\supmorph.obj : include\filter.hpp
.\wat\debug\supmorph.obj : include\macs.hpp
.\wat\debug\supmorph.obj : include\system.h
.\wat\debug\supmorph.obj : include\specs.hpp
.\wat\debug\supmorph.obj : include\palette.hpp
.\wat\debug\supmorph.obj : include\linked.hpp
.\wat\debug\supmorph.obj : include\image.hpp
.\wat\debug\supmorph.obj : include\timage.hpp
.\wat\debug\supmorph.obj : include\jmalloc.hpp
.\wat\debug\supmorph.obj : include\supmorph.hpp
.\wat\debug\image24.obj : image24.c
	set include=$(%WATCOM)\h;include
	wpp386 image24.c -fo=.\wat\debug\image24.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\image24.obj : include\image.hpp
.\wat\debug\image24.obj : include\filter.hpp
.\wat\debug\image24.obj : include\macs.hpp
.\wat\debug\image24.obj : include\system.h
.\wat\debug\image24.obj : include\jmalloc.hpp
.\wat\debug\image24.obj : include\specs.hpp
.\wat\debug\image24.obj : include\linked.hpp
.\wat\debug\image24.obj : include\palette.hpp
.\wat\debug\image24.obj : include\image24.hpp
.\wat\debug\pcxread.obj : pcxread.c
	set include=$(%WATCOM)\h;include
	wpp386 pcxread.c -fo=.\wat\debug\pcxread.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\pcxread.obj : include\filter.hpp
.\wat\debug\pcxread.obj : include\macs.hpp
.\wat\debug\pcxread.obj : include\image24.hpp
.\wat\debug\pcxread.obj : include\system.h
.\wat\debug\pcxread.obj : include\jmalloc.hpp
.\wat\debug\pcxread.obj : include\specs.hpp
.\wat\debug\pcxread.obj : include\palette.hpp
.\wat\debug\pcxread.obj : include\linked.hpp
.\wat\debug\pcxread.obj : include\image.hpp
.\wat\debug\pcxread.obj : include\pcxread.hpp
.\wat\debug\timage.obj : timage.c
	set include=$(%WATCOM)\h;include
	wpp386 timage.c -fo=.\wat\debug\timage.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\timage.obj : include\filter.hpp
.\wat\debug\timage.obj : include\macs.hpp
.\wat\debug\timage.obj : include\system.h
.\wat\debug\timage.obj : include\jmalloc.hpp
.\wat\debug\timage.obj : include\specs.hpp
.\wat\debug\timage.obj : include\palette.hpp
.\wat\debug\timage.obj : include\linked.hpp
.\wat\debug\timage.obj : include\image.hpp
.\wat\debug\timage.obj : include\timage.hpp
.\wat\debug\jmalloc.obj : jmalloc.c
	set include=$(%WATCOM)\h;include
	wpp386 jmalloc.c -fo=.\wat\debug\jmalloc.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\jmalloc.obj : include\jmalloc.hpp
.\wat\debug\jmalloc.obj : include\system.h
.\wat\debug\jmalloc.obj : include\macs.hpp
.\wat\debug\jmalloc.obj : include\doscall.hpp
.\wat\debug\jrand.obj : jrand.c
	set include=$(%WATCOM)\h;include
	wpp386 jrand.c -fo=.\wat\debug\jrand.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\jrand.obj : include\jrand.hpp
.\wat\debug\lbmread.obj : lbmread.c
	set include=$(%WATCOM)\h;include
	wpp386 lbmread.c -fo=.\wat\debug\lbmread.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\lbmread.obj : include\system.h
.\wat\debug\lbmread.obj : include\jmalloc.hpp
.\wat\debug\lbmread.obj : include\specs.hpp
.\wat\debug\lbmread.obj : include\palette.hpp
.\wat\debug\lbmread.obj : include\linked.hpp
.\wat\debug\lbmread.obj : include\image.hpp
.\wat\debug\lbmread.obj : include\lbmread.hpp
.\wat\debug\targa.obj : targa.c
	set include=$(%WATCOM)\h;include
	wpp386 targa.c -fo=.\wat\debug\targa.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\targa.obj : include\system.h
.\wat\debug\targa.obj : include\jmalloc.hpp
.\wat\debug\targa.obj : include\specs.hpp
.\wat\debug\targa.obj : include\palette.hpp
.\wat\debug\targa.obj : include\linked.hpp
.\wat\debug\targa.obj : include\image.hpp
.\wat\debug\keys.obj : keys.c
	set include=$(%WATCOM)\h;include
	wpp386 keys.c -fo=.\wat\debug\keys.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\keys.obj : include\keys.hpp
.\wat\debug\dprint.obj : dprint.c
	set include=$(%WATCOM)\h;include
	wpp386 dprint.c -fo=.\wat\debug\dprint.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\dprint.obj : include\system.h
.\wat\debug\dprint.obj : include\macs.hpp
.\wat\debug\status.obj : status.c
	set include=$(%WATCOM)\h;include
	wpp386 status.c -fo=.\wat\debug\status.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\status.obj : include\dprint.hpp
.\wat\debug\status.obj : include\timage.hpp
.\wat\debug\status.obj : include\fonts.hpp
.\wat\debug\status.obj : include\filter.hpp
.\wat\debug\status.obj : include\mouse.hpp
.\wat\debug\status.obj : include\sprite.hpp
.\wat\debug\status.obj : include\keys.hpp
.\wat\debug\status.obj : include\event.hpp
.\wat\debug\status.obj : include\jmalloc.hpp
.\wat\debug\status.obj : include\specs.hpp
.\wat\debug\status.obj : include\palette.hpp
.\wat\debug\status.obj : include\linked.hpp
.\wat\debug\status.obj : include\image.hpp
.\wat\debug\status.obj : include\video.hpp
.\wat\debug\status.obj : include\jwindow.hpp
.\wat\debug\status.obj : include\visobj.hpp
.\wat\debug\status.obj : include\status.hpp
.\wat\debug\status.obj : include\system.h
.\wat\debug\status.obj : include\macs.hpp
.\wat\debug\visobj.obj : visobj.c
	set include=$(%WATCOM)\h;include
	wpp386 visobj.c -fo=.\wat\debug\visobj.obj /zq /d2 -DMANAGE_MEM 

.\wat\debug\visobj.obj : include\timage.hpp
.\wat\debug\visobj.obj : include\fonts.hpp
.\wat\debug\visobj.obj : include\filter.hpp
.\wat\debug\visobj.obj : include\mouse.hpp
.\wat\debug\visobj.obj : include\macs.hpp
.\wat\debug\visobj.obj : include\sprite.hpp
.\wat\debug\visobj.obj : include\keys.hpp
.\wat\debug\visobj.obj : include\event.hpp
.\wat\debug\visobj.obj : include\jmalloc.hpp
.\wat\debug\visobj.obj : include\specs.hpp
.\wat\debug\visobj.obj : include\palette.hpp
.\wat\debug\visobj.obj : include\linked.hpp
.\wat\debug\visobj.obj : include\image.hpp
.\wat\debug\visobj.obj : include\system.h
.\wat\debug\visobj.obj : include\video.hpp
.\wat\debug\visobj.obj : include\jwindow.hpp
.\wat\debug\visobj.obj : include\visobj.hpp

WATCOM_image_opt_o_files = &
	.\wat\opt\filter.obj &
	.\wat\opt\gifread.obj &
	.\wat\opt\globals.obj &
	.\wat\opt\image.obj &
	.\wat\opt\linked.obj &
	.\wat\opt\input.obj &
	.\wat\opt\mdlread.obj &
	.\wat\opt\palette.obj &
	.\wat\opt\ppmread.obj &
	.\wat\opt\include.obj &
	.\wat\opt\xwdread.obj &
	.\wat\opt\fonts.obj &
	.\wat\opt\decoder.obj &
	.\wat\opt\loader.obj &
	.\wat\opt\glread.obj &
	.\wat\opt\texture.obj &
	.\wat\opt\specs.obj &
	.\wat\opt\supmorph.obj &
	.\wat\opt\image24.obj &
	.\wat\opt\pcxread.obj &
	.\wat\opt\timage.obj &
	.\wat\opt\jmalloc.obj &
	.\wat\opt\jrand.obj &
	.\wat\opt\lbmread.obj &
	.\wat\opt\targa.obj &
	.\wat\opt\keys.obj &
	.\wat\opt\dprint.obj &
	.\wat\opt\status.obj &
	.\wat\opt\visobj.obj

WATCOM_packet_opt_o_files = &
	.\wat\opt\packet.obj

WATCOM_gui_opt_o_files = &
	.\wat\opt\pmenu.obj &
	.\wat\opt\scroller.obj &
	.\wat\opt\filesel.obj &
	.\wat\opt\tools.obj &
	.\wat\opt\guistat.obj

WATCOM_sound_opt_o_files = &
	port\dos4gw\wat\opt\sound.obj &
	.\wat\opt\readwav.obj &
	port\dos4gw\wat\opt\profile.obj

WATCOM_net_opt_o_files = &
	port\dos4gw\wat\opt\jnet.obj &
	port\dos4gw\wat\opt\ipx.obj &
	port\dos4gw\wat\opt\bwtcp.obj

WATCOM_dir_opt_o_files = &
	port\dos4gw\wat\opt\jdir.obj

WATCOM_winman_opt_o_files = &
	port\dos4gw\wat\opt\video.obj &
	port\dos4gw\wat\opt\mouse.obj &
	port\dos4gw\wat\opt\event.obj &
	port\dos4gw\wat\opt\doscall.obj &
	.\wat\opt\jwindow.obj

WATCOM_joy_opt_o_files = &
	port\dos4gw\wat\opt\joystick.obj

WATCOM_time_opt_o_files = &
	port\dos4gw\wat\opt\timing.obj

.\wat\opt\time.lib : $(WATCOM_time_opt_o_files)
	wlib /n @.\wat\opt\time.lnk

port\dos4gw\wat\opt\timing.obj : port\dos4gw\timing.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\timing.c -fo=port\dos4gw\wat\opt\timing.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

port\dos4gw\wat\opt\timing.obj : include\dprint.hpp
port\dos4gw\wat\opt\timing.obj : include\timing.hpp

.\wat\opt\joy.lib : $(WATCOM_joy_opt_o_files)
	wlib /n @.\wat\opt\joy.lnk

port\dos4gw\wat\opt\joystick.obj : port\dos4gw\joystick.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\joystick.c -fo=port\dos4gw\wat\opt\joystick.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

port\dos4gw\wat\opt\joystick.obj : include\joy.hpp

.\wat\opt\winman.lib : $(WATCOM_winman_opt_o_files)
	wlib /n @.\wat\opt\winman.lnk

port\dos4gw\wat\opt\video.obj : port\dos4gw\video.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\video.c -fo=port\dos4gw\wat\opt\video.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

port\dos4gw\wat\opt\video.obj : include\doscall.hpp
port\dos4gw\wat\opt\video.obj : include\bitmap.h
port\dos4gw\wat\opt\video.obj : include\macs.hpp
port\dos4gw\wat\opt\video.obj : include\dos.h
port\dos4gw\wat\opt\video.obj : include\jmalloc.hpp
port\dos4gw\wat\opt\video.obj : include\specs.hpp
port\dos4gw\wat\opt\video.obj : include\palette.hpp
port\dos4gw\wat\opt\video.obj : include\linked.hpp
port\dos4gw\wat\opt\video.obj : include\image.hpp
port\dos4gw\wat\opt\video.obj : include\video.hpp
port\dos4gw\wat\opt\video.obj : include\system.h
port\dos4gw\wat\opt\video.obj : include\globals.hpp
port\dos4gw\wat\opt\mouse.obj : port\dos4gw\mouse.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\mouse.c -fo=port\dos4gw\wat\opt\mouse.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

port\dos4gw\wat\opt\mouse.obj : include\doscall.hpp
port\dos4gw\wat\opt\mouse.obj : include\dprint.hpp
port\dos4gw\wat\opt\mouse.obj : include\mouse.hpp
port\dos4gw\wat\opt\mouse.obj : include\monoprnt.hpp
port\dos4gw\wat\opt\mouse.obj : include\mdlread.hpp
port\dos4gw\wat\opt\mouse.obj : include\filter.hpp
port\dos4gw\wat\opt\mouse.obj : include\macs.hpp
port\dos4gw\wat\opt\mouse.obj : include\sprite.hpp
port\dos4gw\wat\opt\mouse.obj : include\jmalloc.hpp
port\dos4gw\wat\opt\mouse.obj : include\specs.hpp
port\dos4gw\wat\opt\mouse.obj : include\palette.hpp
port\dos4gw\wat\opt\mouse.obj : include\linked.hpp
port\dos4gw\wat\opt\mouse.obj : include\image.hpp
port\dos4gw\wat\opt\mouse.obj : include\system.h
port\dos4gw\wat\opt\mouse.obj : include\video.hpp
port\dos4gw\wat\opt\event.obj : port\dos4gw\event.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\event.c -fo=port\dos4gw\wat\opt\event.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

port\dos4gw\wat\opt\event.obj : include\monoprnt.hpp
port\dos4gw\wat\opt\event.obj : include\keys.hpp
port\dos4gw\wat\opt\event.obj : include\event.hpp
port\dos4gw\wat\opt\event.obj : include\conio.h
port\dos4gw\wat\opt\event.obj : include\sprite.hpp
port\dos4gw\wat\opt\event.obj : include\mouse.hpp
port\dos4gw\wat\opt\event.obj : include\macs.hpp
port\dos4gw\wat\opt\event.obj : include\dos.h
port\dos4gw\wat\opt\event.obj : include\video.hpp
port\dos4gw\wat\opt\event.obj : include\mdlread.hpp
port\dos4gw\wat\opt\event.obj : include\jmalloc.hpp
port\dos4gw\wat\opt\event.obj : include\specs.hpp
port\dos4gw\wat\opt\event.obj : include\palette.hpp
port\dos4gw\wat\opt\event.obj : include\linked.hpp
port\dos4gw\wat\opt\event.obj : include\image.hpp
port\dos4gw\wat\opt\event.obj : include\system.h
port\dos4gw\wat\opt\doscall.obj : port\dos4gw\doscall.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\doscall.c -fo=port\dos4gw\wat\opt\doscall.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

port\dos4gw\wat\opt\doscall.obj : include\system.h
port\dos4gw\wat\opt\doscall.obj : include\macs.hpp
port\dos4gw\wat\opt\doscall.obj : include\doscall.hpp
.\wat\opt\jwindow.obj : jwindow.c
	set include=$(%WATCOM)\h;include
	wpp386 jwindow.c -fo=.\wat\opt\jwindow.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\jwindow.obj : include\timage.hpp
.\wat\opt\jwindow.obj : include\fonts.hpp
.\wat\opt\jwindow.obj : include\jwindow.hpp
.\wat\opt\jwindow.obj : include\filter.hpp
.\wat\opt\jwindow.obj : include\mouse.hpp
.\wat\opt\jwindow.obj : include\macs.hpp
.\wat\opt\jwindow.obj : include\sprite.hpp
.\wat\opt\jwindow.obj : include\keys.hpp
.\wat\opt\jwindow.obj : include\event.hpp
.\wat\opt\jwindow.obj : include\jmalloc.hpp
.\wat\opt\jwindow.obj : include\specs.hpp
.\wat\opt\jwindow.obj : include\palette.hpp
.\wat\opt\jwindow.obj : include\linked.hpp
.\wat\opt\jwindow.obj : include\image.hpp
.\wat\opt\jwindow.obj : include\system.h
.\wat\opt\jwindow.obj : include\video.hpp

.\wat\opt\dir.lib : $(WATCOM_dir_opt_o_files)
	wlib /n @.\wat\opt\dir.lnk

port\dos4gw\wat\opt\jdir.obj : port\dos4gw\jdir.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\jdir.c -fo=port\dos4gw\wat\opt\jdir.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

port\dos4gw\wat\opt\jdir.obj : include\jmalloc.hpp

.\wat\opt\net.lib : $(WATCOM_net_opt_o_files)
	wlib /n @.\wat\opt\net.lnk

port\dos4gw\wat\opt\jnet.obj : port\dos4gw\jnet.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\jnet.c -fo=port\dos4gw\wat\opt\jnet.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

port\dos4gw\wat\opt\jnet.obj : include\jmalloc.hpp
port\dos4gw\wat\opt\jnet.obj : include\dprint.hpp
port\dos4gw\wat\opt\jnet.obj : include\packet.hpp
port\dos4gw\wat\opt\jnet.obj : include\system.h
port\dos4gw\wat\opt\jnet.obj : include\macs.hpp
port\dos4gw\wat\opt\jnet.obj : include\jnet.hpp
port\dos4gw\wat\opt\ipx.obj : port\dos4gw\ipx.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\ipx.c -fo=port\dos4gw\wat\opt\ipx.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

port\dos4gw\wat\opt\ipx.obj : include\timing.hpp
port\dos4gw\wat\opt\ipx.obj : include\doscall.hpp
port\dos4gw\wat\opt\ipx.obj : include\jmalloc.hpp
port\dos4gw\wat\opt\ipx.obj : include\dprint.hpp
port\dos4gw\wat\opt\ipx.obj : include\packet.hpp
port\dos4gw\wat\opt\ipx.obj : include\system.h
port\dos4gw\wat\opt\ipx.obj : include\macs.hpp
port\dos4gw\wat\opt\ipx.obj : include\jnet.hpp
port\dos4gw\wat\opt\bwtcp.obj : port\dos4gw\bwtcp.c
	set include=$(%WATCOM)\h;include
	wpp386 port\dos4gw\bwtcp.c -fo=port\dos4gw\wat\opt\bwtcp.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

port\dos4gw\wat\opt\bwtcp.obj : include\doscall.hpp
port\dos4gw\wat\opt\bwtcp.obj : include\jmalloc.hpp
port\dos4gw\wat\opt\bwtcp.obj : include\dprint.hpp
port\dos4gw\wat\opt\bwtcp.obj : include\packet.hpp
port\dos4gw\wat\opt\bwtcp.obj : include\system.h
port\dos4gw\wat\opt\bwtcp.obj : include\macs.hpp
port\dos4gw\wat\opt\bwtcp.obj : include\jnet.hpp

.\wat\opt\sound.lib : $(WATCOM_sound_opt_o_files)
	wlib /n @.\wat\opt\sound.lnk

port\dos4gw\wat\opt\sound.obj : port\dos4gw\sound.c
	set include=$(%WATCOM)\h;include;c:\sos\include
	wpp386 port\dos4gw\sound.c -fo=port\dos4gw\wat\opt\sound.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

port\dos4gw\wat\opt\sound.obj : include\timing.hpp
port\dos4gw\wat\opt\sound.obj : include\macs.hpp
port\dos4gw\wat\opt\sound.obj : include\doscall.hpp
port\dos4gw\wat\opt\sound.obj : include\dprint.hpp
port\dos4gw\wat\opt\sound.obj : include\sound.hpp
port\dos4gw\wat\opt\sound.obj : include\system.h
port\dos4gw\wat\opt\sound.obj : include\jmalloc.hpp
port\dos4gw\wat\opt\sound.obj : include\linked.hpp
port\dos4gw\wat\opt\sound.obj : include\specs.hpp
port\dos4gw\wat\opt\sound.obj : include\readwav.hpp
port\dos4gw\wat\opt\sound.obj : c:\sos\include\sosmdata.h
port\dos4gw\wat\opt\sound.obj : c:\sos\include\sosmfnct.h
port\dos4gw\wat\opt\sound.obj : c:\sos\include\sosm.h
port\dos4gw\wat\opt\sound.obj : c:\sos\include\sosfnct.h
port\dos4gw\wat\opt\sound.obj : c:\sos\include\sosdata.h
port\dos4gw\wat\opt\sound.obj : c:\sos\include\sosdefs.h
port\dos4gw\wat\opt\sound.obj : c:\sos\include\sos.h
.\wat\opt\readwav.obj : readwav.c
	set include=$(%WATCOM)\h;include;c:\sos\include
	wpp386 readwav.c -fo=.\wat\opt\readwav.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\readwav.obj : include\dprint.hpp
.\wat\opt\readwav.obj : include\macs.hpp
.\wat\opt\readwav.obj : include\system.h
.\wat\opt\readwav.obj : include\jmalloc.hpp
.\wat\opt\readwav.obj : include\linked.hpp
.\wat\opt\readwav.obj : include\specs.hpp
.\wat\opt\readwav.obj : include\readwav.hpp
port\dos4gw\wat\opt\profile.obj : port\dos4gw\profile.c
	set include=$(%WATCOM)\h;include;c:\sos\include
	wpp386 port\dos4gw\profile.c -fo=port\dos4gw\wat\opt\profile.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

port\dos4gw\wat\opt\profile.obj : c:\sos\include\sosfnct.h
port\dos4gw\wat\opt\profile.obj : c:\sos\include\sosdata.h
port\dos4gw\wat\opt\profile.obj : c:\sos\include\sosdefs.h
port\dos4gw\wat\opt\profile.obj : c:\sos\include\sos.h

.\wat\opt\gui.lib : $(WATCOM_gui_opt_o_files)
	wlib /n @.\wat\opt\gui.lnk

.\wat\opt\pmenu.obj : pmenu.c
	set include=$(%WATCOM)\h;include
	wpp386 pmenu.c -fo=.\wat\opt\pmenu.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\pmenu.obj : include\timage.hpp
.\wat\opt\pmenu.obj : include\fonts.hpp
.\wat\opt\pmenu.obj : include\filter.hpp
.\wat\opt\pmenu.obj : include\mouse.hpp
.\wat\opt\pmenu.obj : include\macs.hpp
.\wat\opt\pmenu.obj : include\sprite.hpp
.\wat\opt\pmenu.obj : include\keys.hpp
.\wat\opt\pmenu.obj : include\event.hpp
.\wat\opt\pmenu.obj : include\specs.hpp
.\wat\opt\pmenu.obj : include\palette.hpp
.\wat\opt\pmenu.obj : include\linked.hpp
.\wat\opt\pmenu.obj : include\image.hpp
.\wat\opt\pmenu.obj : include\system.h
.\wat\opt\pmenu.obj : include\video.hpp
.\wat\opt\pmenu.obj : include\jwindow.hpp
.\wat\opt\pmenu.obj : include\input.hpp
.\wat\opt\pmenu.obj : include\jmalloc.hpp
.\wat\opt\pmenu.obj : include\pmenu.hpp
.\wat\opt\scroller.obj : scroller.c
	set include=$(%WATCOM)\h;include
	wpp386 scroller.c -fo=.\wat\opt\scroller.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\scroller.obj : include\timage.hpp
.\wat\opt\scroller.obj : include\fonts.hpp
.\wat\opt\scroller.obj : include\filter.hpp
.\wat\opt\scroller.obj : include\mouse.hpp
.\wat\opt\scroller.obj : include\macs.hpp
.\wat\opt\scroller.obj : include\sprite.hpp
.\wat\opt\scroller.obj : include\keys.hpp
.\wat\opt\scroller.obj : include\event.hpp
.\wat\opt\scroller.obj : include\jmalloc.hpp
.\wat\opt\scroller.obj : include\specs.hpp
.\wat\opt\scroller.obj : include\palette.hpp
.\wat\opt\scroller.obj : include\linked.hpp
.\wat\opt\scroller.obj : include\image.hpp
.\wat\opt\scroller.obj : include\system.h
.\wat\opt\scroller.obj : include\video.hpp
.\wat\opt\scroller.obj : include\jwindow.hpp
.\wat\opt\scroller.obj : include\input.hpp
.\wat\opt\scroller.obj : include\scroller.hpp
.\wat\opt\filesel.obj : filesel.c
	set include=$(%WATCOM)\h;include
	wpp386 filesel.c -fo=.\wat\opt\filesel.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\filesel.obj : include\jdir.hpp
.\wat\opt\filesel.obj : include\scroller.hpp
.\wat\opt\filesel.obj : include\input.hpp
.\wat\opt\filesel.obj : include\timage.hpp
.\wat\opt\filesel.obj : include\fonts.hpp
.\wat\opt\filesel.obj : include\filter.hpp
.\wat\opt\filesel.obj : include\mouse.hpp
.\wat\opt\filesel.obj : include\macs.hpp
.\wat\opt\filesel.obj : include\sprite.hpp
.\wat\opt\filesel.obj : include\keys.hpp
.\wat\opt\filesel.obj : include\event.hpp
.\wat\opt\filesel.obj : include\jmalloc.hpp
.\wat\opt\filesel.obj : include\specs.hpp
.\wat\opt\filesel.obj : include\palette.hpp
.\wat\opt\filesel.obj : include\linked.hpp
.\wat\opt\filesel.obj : include\image.hpp
.\wat\opt\filesel.obj : include\system.h
.\wat\opt\filesel.obj : include\video.hpp
.\wat\opt\filesel.obj : include\jwindow.hpp
.\wat\opt\filesel.obj : include\filesel.hpp
.\wat\opt\tools.obj : tools.c
	set include=$(%WATCOM)\h;include
	wpp386 tools.c -fo=.\wat\opt\tools.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\tools.obj : include\visobj.hpp
.\wat\opt\tools.obj : include\scroller.hpp
.\wat\opt\tools.obj : include\input.hpp
.\wat\opt\tools.obj : include\timage.hpp
.\wat\opt\tools.obj : include\fonts.hpp
.\wat\opt\tools.obj : include\filter.hpp
.\wat\opt\tools.obj : include\mouse.hpp
.\wat\opt\tools.obj : include\macs.hpp
.\wat\opt\tools.obj : include\sprite.hpp
.\wat\opt\tools.obj : include\keys.hpp
.\wat\opt\tools.obj : include\event.hpp
.\wat\opt\tools.obj : include\jmalloc.hpp
.\wat\opt\tools.obj : include\specs.hpp
.\wat\opt\tools.obj : include\palette.hpp
.\wat\opt\tools.obj : include\linked.hpp
.\wat\opt\tools.obj : include\image.hpp
.\wat\opt\tools.obj : include\system.h
.\wat\opt\tools.obj : include\video.hpp
.\wat\opt\tools.obj : include\jwindow.hpp
.\wat\opt\tools.obj : include\tools.hpp
.\wat\opt\guistat.obj : guistat.c
	set include=$(%WATCOM)\h;include
	wpp386 guistat.c -fo=.\wat\opt\guistat.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\guistat.obj : include\guistat.hpp
.\wat\opt\guistat.obj : include\timing.hpp
.\wat\opt\guistat.obj : include\timage.hpp
.\wat\opt\guistat.obj : include\fonts.hpp
.\wat\opt\guistat.obj : include\filter.hpp
.\wat\opt\guistat.obj : include\mouse.hpp
.\wat\opt\guistat.obj : include\macs.hpp
.\wat\opt\guistat.obj : include\sprite.hpp
.\wat\opt\guistat.obj : include\keys.hpp
.\wat\opt\guistat.obj : include\event.hpp
.\wat\opt\guistat.obj : include\jmalloc.hpp
.\wat\opt\guistat.obj : include\specs.hpp
.\wat\opt\guistat.obj : include\palette.hpp
.\wat\opt\guistat.obj : include\linked.hpp
.\wat\opt\guistat.obj : include\image.hpp
.\wat\opt\guistat.obj : include\system.h
.\wat\opt\guistat.obj : include\video.hpp
.\wat\opt\guistat.obj : include\jwindow.hpp
.\wat\opt\guistat.obj : include\visobj.hpp
.\wat\opt\guistat.obj : include\status.hpp

.\wat\opt\packet.lib : $(WATCOM_packet_opt_o_files)
	wlib /n @.\wat\opt\packet.lnk

.\wat\opt\packet.obj : packet.c
	set include=$(%WATCOM)\h;include
	wpp386 packet.c -fo=.\wat\opt\packet.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\packet.obj : include\jmalloc.hpp
.\wat\opt\packet.obj : include\system.h
.\wat\opt\packet.obj : include\macs.hpp
.\wat\opt\packet.obj : include\packet.hpp

.\wat\opt\image.lib : $(WATCOM_image_opt_o_files)
	wlib /n @.\wat\opt\image.lnk

.\wat\opt\filter.obj : filter.c
	set include=$(%WATCOM)\h;include
	wpp386 filter.c -fo=.\wat\opt\filter.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\filter.obj : include\filter.hpp
.\wat\opt\filter.obj : include\macs.hpp
.\wat\opt\filter.obj : include\system.h
.\wat\opt\filter.obj : include\jmalloc.hpp
.\wat\opt\filter.obj : include\specs.hpp
.\wat\opt\filter.obj : include\palette.hpp
.\wat\opt\filter.obj : include\linked.hpp
.\wat\opt\filter.obj : include\image.hpp
.\wat\opt\gifread.obj : gifread.c
	set include=$(%WATCOM)\h;include
	wpp386 gifread.c -fo=.\wat\opt\gifread.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\gifread.obj : include\macs.hpp
.\wat\opt\gifread.obj : include\dir.h
.\wat\opt\gifread.obj : include\dos.h
.\wat\opt\gifread.obj : include\std.h
.\wat\opt\gifread.obj : include\gifdecod.hpp
.\wat\opt\gifread.obj : include\video.hpp
.\wat\opt\gifread.obj : include\system.h
.\wat\opt\gifread.obj : include\jmalloc.hpp
.\wat\opt\gifread.obj : include\specs.hpp
.\wat\opt\gifread.obj : include\palette.hpp
.\wat\opt\gifread.obj : include\linked.hpp
.\wat\opt\gifread.obj : include\image.hpp
.\wat\opt\gifread.obj : include\gifread.hpp
.\wat\opt\globals.obj : globals.c
	set include=$(%WATCOM)\h;include
	wpp386 globals.c -fo=.\wat\opt\globals.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\globals.obj : globals.c
.\wat\opt\image.obj : image.c
	set include=$(%WATCOM)\h;include
	wpp386 image.c -fo=.\wat\opt\image.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\image.obj : include\macs.hpp
.\wat\opt\image.obj : include\system.h
.\wat\opt\image.obj : include\jmalloc.hpp
.\wat\opt\image.obj : include\specs.hpp
.\wat\opt\image.obj : include\palette.hpp
.\wat\opt\image.obj : include\linked.hpp
.\wat\opt\image.obj : include\image.hpp
.\wat\opt\linked.obj : linked.c
	set include=$(%WATCOM)\h;include
	wpp386 linked.c -fo=.\wat\opt\linked.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\linked.obj : include\linked.hpp
.\wat\opt\input.obj : input.c
	set include=$(%WATCOM)\h;include
	wpp386 input.c -fo=.\wat\opt\input.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\input.obj : include\timage.hpp
.\wat\opt\input.obj : include\fonts.hpp
.\wat\opt\input.obj : include\filter.hpp
.\wat\opt\input.obj : include\mouse.hpp
.\wat\opt\input.obj : include\macs.hpp
.\wat\opt\input.obj : include\sprite.hpp
.\wat\opt\input.obj : include\keys.hpp
.\wat\opt\input.obj : include\event.hpp
.\wat\opt\input.obj : include\jmalloc.hpp
.\wat\opt\input.obj : include\specs.hpp
.\wat\opt\input.obj : include\palette.hpp
.\wat\opt\input.obj : include\linked.hpp
.\wat\opt\input.obj : include\image.hpp
.\wat\opt\input.obj : include\system.h
.\wat\opt\input.obj : include\video.hpp
.\wat\opt\input.obj : include\jwindow.hpp
.\wat\opt\input.obj : include\input.hpp
.\wat\opt\mdlread.obj : mdlread.c
	set include=$(%WATCOM)\h;include
	wpp386 mdlread.c -fo=.\wat\opt\mdlread.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\mdlread.obj : include\macs.hpp
.\wat\opt\mdlread.obj : include\image.hpp
.\wat\opt\mdlread.obj : include\system.h
.\wat\opt\mdlread.obj : include\jmalloc.hpp
.\wat\opt\mdlread.obj : include\specs.hpp
.\wat\opt\mdlread.obj : include\linked.hpp
.\wat\opt\mdlread.obj : include\palette.hpp
.\wat\opt\mdlread.obj : include\mdlread.hpp
.\wat\opt\palette.obj : palette.c
	set include=$(%WATCOM)\h;include
	wpp386 palette.c -fo=.\wat\opt\palette.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\palette.obj : include\filter.hpp
.\wat\opt\palette.obj : include\video.hpp
.\wat\opt\palette.obj : include\dos.h
.\wat\opt\palette.obj : include\macs.hpp
.\wat\opt\palette.obj : include\image.hpp
.\wat\opt\palette.obj : include\system.h
.\wat\opt\palette.obj : include\jmalloc.hpp
.\wat\opt\palette.obj : include\specs.hpp
.\wat\opt\palette.obj : include\linked.hpp
.\wat\opt\palette.obj : include\palette.hpp
.\wat\opt\ppmread.obj : ppmread.c
	set include=$(%WATCOM)\h;include
	wpp386 ppmread.c -fo=.\wat\opt\ppmread.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\ppmread.obj : include\ppmread.hpp
.\wat\opt\ppmread.obj : include\macs.hpp
.\wat\opt\ppmread.obj : include\system.h
.\wat\opt\ppmread.obj : include\jmalloc.hpp
.\wat\opt\ppmread.obj : include\specs.hpp
.\wat\opt\ppmread.obj : include\palette.hpp
.\wat\opt\ppmread.obj : include\linked.hpp
.\wat\opt\ppmread.obj : include\image.hpp
.\wat\opt\include.obj : include.c
	set include=$(%WATCOM)\h;include
	wpp386 include.c -fo=.\wat\opt\include.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\include.obj : include\system.h
.\wat\opt\include.obj : include\jmalloc.hpp
.\wat\opt\include.obj : include\specs.hpp
.\wat\opt\include.obj : include\palette.hpp
.\wat\opt\include.obj : include\linked.hpp
.\wat\opt\include.obj : include\image.hpp
.\wat\opt\include.obj : include\include.hpp
.\wat\opt\xwdread.obj : xwdread.c
	set include=$(%WATCOM)\h;include
	wpp386 xwdread.c -fo=.\wat\opt\xwdread.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\xwdread.obj : include\image24.hpp
.\wat\opt\xwdread.obj : include\main.hpp
.\wat\opt\xwdread.obj : include\dos.h
.\wat\opt\xwdread.obj : include\video.hpp
.\wat\opt\xwdread.obj : include\mouse.hpp
.\wat\opt\xwdread.obj : include\macs.hpp
.\wat\opt\xwdread.obj : include\sprite.hpp
.\wat\opt\xwdread.obj : include\keys.hpp
.\wat\opt\xwdread.obj : include\event.hpp
.\wat\opt\xwdread.obj : include\system.h
.\wat\opt\xwdread.obj : include\jmalloc.hpp
.\wat\opt\xwdread.obj : include\specs.hpp
.\wat\opt\xwdread.obj : include\palette.hpp
.\wat\opt\xwdread.obj : include\linked.hpp
.\wat\opt\xwdread.obj : include\image.hpp
.\wat\opt\xwdread.obj : include\filter.hpp
.\wat\opt\fonts.obj : fonts.c
	set include=$(%WATCOM)\h;include
	wpp386 fonts.c -fo=.\wat\opt\fonts.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\fonts.obj : include\filter.hpp
.\wat\opt\fonts.obj : include\macs.hpp
.\wat\opt\fonts.obj : include\timage.hpp
.\wat\opt\fonts.obj : include\system.h
.\wat\opt\fonts.obj : include\jmalloc.hpp
.\wat\opt\fonts.obj : include\specs.hpp
.\wat\opt\fonts.obj : include\palette.hpp
.\wat\opt\fonts.obj : include\linked.hpp
.\wat\opt\fonts.obj : include\image.hpp
.\wat\opt\fonts.obj : include\fonts.hpp
.\wat\opt\decoder.obj : decoder.c
	set include=$(%WATCOM)\h;include
	wpp386 decoder.c -fo=.\wat\opt\decoder.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\decoder.obj : include\macs.hpp
.\wat\opt\decoder.obj : include\system.h
.\wat\opt\decoder.obj : include\jmalloc.hpp
.\wat\opt\decoder.obj : include\specs.hpp
.\wat\opt\decoder.obj : include\palette.hpp
.\wat\opt\decoder.obj : include\linked.hpp
.\wat\opt\decoder.obj : include\image.hpp
.\wat\opt\decoder.obj : include\errs.h
.\wat\opt\loader.obj : loader.c
	set include=$(%WATCOM)\h;include
	wpp386 loader.c -fo=.\wat\opt\loader.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\loader.obj : include\targa.hpp
.\wat\opt\loader.obj : include\lbmread.hpp
.\wat\opt\loader.obj : include\pcxread.hpp
.\wat\opt\loader.obj : include\glread.hpp
.\wat\opt\loader.obj : include\xwdread.hpp
.\wat\opt\loader.obj : include\ppmread.hpp
.\wat\opt\loader.obj : include\mdlread.hpp
.\wat\opt\loader.obj : include\filter.hpp
.\wat\opt\loader.obj : include\macs.hpp
.\wat\opt\loader.obj : include\image24.hpp
.\wat\opt\loader.obj : include\system.h
.\wat\opt\loader.obj : include\jmalloc.hpp
.\wat\opt\loader.obj : include\specs.hpp
.\wat\opt\loader.obj : include\palette.hpp
.\wat\opt\loader.obj : include\linked.hpp
.\wat\opt\loader.obj : include\image.hpp
.\wat\opt\loader.obj : include\loader.hpp
.\wat\opt\glread.obj : glread.c
	set include=$(%WATCOM)\h;include
	wpp386 glread.c -fo=.\wat\opt\glread.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\glread.obj : include\video.hpp
.\wat\opt\glread.obj : include\jmalloc.hpp
.\wat\opt\glread.obj : include\specs.hpp
.\wat\opt\glread.obj : include\palette.hpp
.\wat\opt\glread.obj : include\linked.hpp
.\wat\opt\glread.obj : include\image.hpp
.\wat\opt\glread.obj : include\system.h
.\wat\opt\glread.obj : include\macs.hpp
.\wat\opt\texture.obj : texture.c
	set include=$(%WATCOM)\h;include
	wpp386 texture.c -fo=.\wat\opt\texture.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\texture.obj : include\mouse.hpp
.\wat\opt\texture.obj : include\sprite.hpp
.\wat\opt\texture.obj : include\keys.hpp
.\wat\opt\texture.obj : include\event.hpp
.\wat\opt\texture.obj : include\macs.hpp
.\wat\opt\texture.obj : include\video.hpp
.\wat\opt\texture.obj : include\system.h
.\wat\opt\texture.obj : include\jmalloc.hpp
.\wat\opt\texture.obj : include\specs.hpp
.\wat\opt\texture.obj : include\palette.hpp
.\wat\opt\texture.obj : include\linked.hpp
.\wat\opt\texture.obj : include\image.hpp
.\wat\opt\specs.obj : specs.c
	set include=$(%WATCOM)\h;include
	wpp386 specs.c -fo=.\wat\opt\specs.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\specs.obj : include\dprint.hpp
.\wat\opt\specs.obj : include\system.h
.\wat\opt\specs.obj : include\jmalloc.hpp
.\wat\opt\specs.obj : include\specs.hpp
.\wat\opt\specs.obj : include\palette.hpp
.\wat\opt\specs.obj : include\linked.hpp
.\wat\opt\specs.obj : include\image.hpp
.\wat\opt\supmorph.obj : supmorph.c
	set include=$(%WATCOM)\h;include
	wpp386 supmorph.c -fo=.\wat\opt\supmorph.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\supmorph.obj : include\jrand.hpp
.\wat\opt\supmorph.obj : include\video.hpp
.\wat\opt\supmorph.obj : include\timing.hpp
.\wat\opt\supmorph.obj : include\filter.hpp
.\wat\opt\supmorph.obj : include\macs.hpp
.\wat\opt\supmorph.obj : include\system.h
.\wat\opt\supmorph.obj : include\specs.hpp
.\wat\opt\supmorph.obj : include\palette.hpp
.\wat\opt\supmorph.obj : include\linked.hpp
.\wat\opt\supmorph.obj : include\image.hpp
.\wat\opt\supmorph.obj : include\timage.hpp
.\wat\opt\supmorph.obj : include\jmalloc.hpp
.\wat\opt\supmorph.obj : include\supmorph.hpp
.\wat\opt\image24.obj : image24.c
	set include=$(%WATCOM)\h;include
	wpp386 image24.c -fo=.\wat\opt\image24.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\image24.obj : include\image.hpp
.\wat\opt\image24.obj : include\filter.hpp
.\wat\opt\image24.obj : include\macs.hpp
.\wat\opt\image24.obj : include\system.h
.\wat\opt\image24.obj : include\jmalloc.hpp
.\wat\opt\image24.obj : include\specs.hpp
.\wat\opt\image24.obj : include\linked.hpp
.\wat\opt\image24.obj : include\palette.hpp
.\wat\opt\image24.obj : include\image24.hpp
.\wat\opt\pcxread.obj : pcxread.c
	set include=$(%WATCOM)\h;include
	wpp386 pcxread.c -fo=.\wat\opt\pcxread.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\pcxread.obj : include\filter.hpp
.\wat\opt\pcxread.obj : include\macs.hpp
.\wat\opt\pcxread.obj : include\image24.hpp
.\wat\opt\pcxread.obj : include\system.h
.\wat\opt\pcxread.obj : include\jmalloc.hpp
.\wat\opt\pcxread.obj : include\specs.hpp
.\wat\opt\pcxread.obj : include\palette.hpp
.\wat\opt\pcxread.obj : include\linked.hpp
.\wat\opt\pcxread.obj : include\image.hpp
.\wat\opt\pcxread.obj : include\pcxread.hpp
.\wat\opt\timage.obj : timage.c
	set include=$(%WATCOM)\h;include
	wpp386 timage.c -fo=.\wat\opt\timage.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\timage.obj : include\filter.hpp
.\wat\opt\timage.obj : include\macs.hpp
.\wat\opt\timage.obj : include\system.h
.\wat\opt\timage.obj : include\jmalloc.hpp
.\wat\opt\timage.obj : include\specs.hpp
.\wat\opt\timage.obj : include\palette.hpp
.\wat\opt\timage.obj : include\linked.hpp
.\wat\opt\timage.obj : include\image.hpp
.\wat\opt\timage.obj : include\timage.hpp
.\wat\opt\jmalloc.obj : jmalloc.c
	set include=$(%WATCOM)\h;include
	wpp386 jmalloc.c -fo=.\wat\opt\jmalloc.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\jmalloc.obj : include\jmalloc.hpp
.\wat\opt\jmalloc.obj : include\system.h
.\wat\opt\jmalloc.obj : include\macs.hpp
.\wat\opt\jmalloc.obj : include\doscall.hpp
.\wat\opt\jrand.obj : jrand.c
	set include=$(%WATCOM)\h;include
	wpp386 jrand.c -fo=.\wat\opt\jrand.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\jrand.obj : include\jrand.hpp
.\wat\opt\lbmread.obj : lbmread.c
	set include=$(%WATCOM)\h;include
	wpp386 lbmread.c -fo=.\wat\opt\lbmread.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\lbmread.obj : include\system.h
.\wat\opt\lbmread.obj : include\jmalloc.hpp
.\wat\opt\lbmread.obj : include\specs.hpp
.\wat\opt\lbmread.obj : include\palette.hpp
.\wat\opt\lbmread.obj : include\linked.hpp
.\wat\opt\lbmread.obj : include\image.hpp
.\wat\opt\lbmread.obj : include\lbmread.hpp
.\wat\opt\targa.obj : targa.c
	set include=$(%WATCOM)\h;include
	wpp386 targa.c -fo=.\wat\opt\targa.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\targa.obj : include\system.h
.\wat\opt\targa.obj : include\jmalloc.hpp
.\wat\opt\targa.obj : include\specs.hpp
.\wat\opt\targa.obj : include\palette.hpp
.\wat\opt\targa.obj : include\linked.hpp
.\wat\opt\targa.obj : include\image.hpp
.\wat\opt\keys.obj : keys.c
	set include=$(%WATCOM)\h;include
	wpp386 keys.c -fo=.\wat\opt\keys.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\keys.obj : include\keys.hpp
.\wat\opt\dprint.obj : dprint.c
	set include=$(%WATCOM)\h;include
	wpp386 dprint.c -fo=.\wat\opt\dprint.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\dprint.obj : include\system.h
.\wat\opt\dprint.obj : include\macs.hpp
.\wat\opt\status.obj : status.c
	set include=$(%WATCOM)\h;include
	wpp386 status.c -fo=.\wat\opt\status.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\status.obj : include\dprint.hpp
.\wat\opt\status.obj : include\timage.hpp
.\wat\opt\status.obj : include\fonts.hpp
.\wat\opt\status.obj : include\filter.hpp
.\wat\opt\status.obj : include\mouse.hpp
.\wat\opt\status.obj : include\sprite.hpp
.\wat\opt\status.obj : include\keys.hpp
.\wat\opt\status.obj : include\event.hpp
.\wat\opt\status.obj : include\jmalloc.hpp
.\wat\opt\status.obj : include\specs.hpp
.\wat\opt\status.obj : include\palette.hpp
.\wat\opt\status.obj : include\linked.hpp
.\wat\opt\status.obj : include\image.hpp
.\wat\opt\status.obj : include\video.hpp
.\wat\opt\status.obj : include\jwindow.hpp
.\wat\opt\status.obj : include\visobj.hpp
.\wat\opt\status.obj : include\status.hpp
.\wat\opt\status.obj : include\system.h
.\wat\opt\status.obj : include\macs.hpp
.\wat\opt\visobj.obj : visobj.c
	set include=$(%WATCOM)\h;include
	wpp386 visobj.c -fo=.\wat\opt\visobj.obj /omaxne /zp1 /zq -DNO_CHECK -DMANAGE_MEM 

.\wat\opt\visobj.obj : include\timage.hpp
.\wat\opt\visobj.obj : include\fonts.hpp
.\wat\opt\visobj.obj : include\filter.hpp
.\wat\opt\visobj.obj : include\mouse.hpp
.\wat\opt\visobj.obj : include\macs.hpp
.\wat\opt\visobj.obj : include\sprite.hpp
.\wat\opt\visobj.obj : include\keys.hpp
.\wat\opt\visobj.obj : include\event.hpp
.\wat\opt\visobj.obj : include\jmalloc.hpp
.\wat\opt\visobj.obj : include\specs.hpp
.\wat\opt\visobj.obj : include\palette.hpp
.\wat\opt\visobj.obj : include\linked.hpp
.\wat\opt\visobj.obj : include\image.hpp
.\wat\opt\visobj.obj : include\system.h
.\wat\opt\visobj.obj : include\video.hpp
.\wat\opt\visobj.obj : include\jwindow.hpp
.\wat\opt\visobj.obj : include\visobj.hpp

