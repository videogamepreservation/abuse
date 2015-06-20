## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libimage.a libpacket.a libgui.a libsound.a libnet.a \
	libdir.a libwinman.a libjoy.a libtime.a $(LIBSVGA)
EXTRA_LIBRARIES = libsvgawin.a

libimage_a_SOURCES = filter.c gifread.c globals.c image.c linked.c \
	input.c mdlread.c palette.c ppmread.c include.c xwdread.c \
	fonts.c decoder.c loader.c glread.c texture.c specs.c \
	supmorph.c image24.c pcxread.c timage.c jmalloc.c jrand.c \
	lbmread.c targa.c keys.c dprint.c status.c visobj.c

libpacket_a_SOURCES = packet.c

libgui_a_SOURCES = pmenu.c scroller.c filesel.c tools.c guistat.c

libsound_a_SOURCES = readwav.c
libsound_a_LIBADD = $(SOUND)
libsound_a_DEPENDENCIES = $(SOUND)
EXTRA_libsound_a_SOURCES = port/sgi/sound.c port/aix/sound.c \
	port/unix/sound.c
libnet_a_SOURCES = port/unix/jnet.c
libdir_a_SOURCES = port/unix/jdir.c
libwinman_a_SOURCES = port/x11/video.c port/x11/mouse.c port/x11/event.c \
	jwindow.c
libsvgawin_a_SOURCES = port/svga/video.c port/svga/mouse.c \
	port/svga/event.c jwindow.c
libjoy_a_SOURCES = 
libjoy_a_LIBADD = $(JOYSTICK)
libjoy_a_DEPENDENCIES = $(JOYSTICK)
EXTRA_libjoy_a_SOURCES = port/unix/joystick.c port/linux/joystick.c
libtime_a_SOURCES =
libtime_a_LIBADD = $(TIMING)
libtime_a_DEPENDENCIES = $(TIMING)
EXTRA_libtime_a_SOURCES = port/unix/timing.c port/sgi/timing.c

INCLUDES = -I$(srcdir)/include $(X_CFLAGS)

MYDEFS = -DMANAGE_MEM -DNO_CHECK
CXXFLAGS = -g -O2

MOSTLYCLEANFILES = $(SOUND) $(JOYSTICK) $(TIMING)

COMPILE = $(CXX) $(DEFS) $(MYDEFS) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) $(ADDCXXFLAGS) -o $@

debug:
	$(MAKE) DEFS='-DMANAGE_MEM -DMEM_CHECK' CXXFLAGS=-g
opt:
	$(MAKE) DEFS='-DMANAGE_MEM -DNO_CHECK' CXXFLAGS=-O2
profile:
	$(MAKE) DEFS='-DMANAGE_MEM -DNO_CHECK' CXXFLAGS='-O2 -g -pg'
