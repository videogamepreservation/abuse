## Process this file with automake to produce Makefile.in

bin_SCRIPTS = abuse
pkglib_PROGRAMS = abuse.x11R6 $(PROGS)
EXTRA_PROGRAMS = abuse.console keydrv lnx_sdrv sgi_sdrv aix_sdrv
pkgdata_DATA = abuse.lsp light.tbl

abuse_x11R6_SOURCES = src/net/mac/tcpip.c src/specache.c src/netcfg.c \
	src/text_gui.c src/innet.c src/net/unix/gserver.c \
	src/net/unix/gclient.c src/net/unix/fileman.c src/net/sock.c \
	src/chat.c src/endgame.c src/setup.c src/version.c \
	src/loadgame.c src/profile.c src/cop.c src/statbar.c \
	src/compiled.c src/ant.c src/sensor.c src/lisp_opt.c src/demo.c \
	src/lcache.c src/lisp_gc.c src/nfclient.c src/username.c \
	src/clisp.c src/gui.c src/transp.c src/collide.c src/trig.c \
	src/property.c src/lisp.c src/cache.c src/particle.c \
	src/objects.c src/extend.c src/console.c src/ability.c \
	src/items.c src/dev.c src/chars.c src/level.c src/smallfnt.c \
	src/automap.c src/help.c src/intsect.c src/loader2.c src/seq.c \
	src/points.c src/fnt6x13.c src/morpher.c src/menu.c \
	src/director.c src/view.c src/config.c src/game.c src/light.c \
	src/devsel.c src/crc.c src/gamma.c src/language.c \
	../imlib/sprite.c
abuse_console_SOURCES = $(abuse_x11R6_SOURCES)
keydrv_SOURCES = src/keydrv.c

lnx_sdrv_SOURCES = ../imlib/port/linux/lnx_sdrv.c
sgi_sdrv_SOURCES = ../imlib/port/sgi/sgi_sdrv.c
sgi_sdrv_LDADD = -laudio
aix_sdrv_SOURCES = ../imlib/port/aix/aix_sdrv.c
aix_sdrv_LDADD = -lUMSobj

IMLIBS = ../imlib/libimage.a ../imlib/libgui.a \
	../imlib/libdir.a ../imlib/libtime.a ../imlib/libsound.a \
	../imlib/libjoy.a ../imlib/libpacket.a
abuse_x11R6_LDADD = $(IMLIBS) ../imlib/libwinman.a \
	$(X_LIBS) -lX11 -lXext -lm
abuse_console_LDADD = $(IMLIBS) ../imlib/libsvgawin.a -lvga -lm

INCLUDES = -I$(srcdir)/inc -I$(srcdir)/../imlib/include \
	   -I$(srcdir)/src/net/inc -I$(srcdir)/src/net/unix

MYDEFS = -DMANAGE_MEM -DNO_CHECK
CXXFLAGS = -g -O2

COMPILE = $(CXX) $(DEFS) $(MYDEFS) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) $(ADDCXXFLAGS) -o $@

debug:
	$(MAKE) DEFS='-DMANAGE_MEM -DMEM_CHECK' CXXFLAGS=-g
opt:
	$(MAKE) DEFS='-DMANAGE_MEM -DNO_CHECK' CXXFLAGS=-O2
profile:
	$(MAKE) DEFS='-DMANAGE_MEM -DNO_CHECK' CXXFLAGS='-O2 -g -pg'

../imlib/port/aix/aix_sdrv.o: ../imlib/port/aix/aix_sdrv.c
	cc -qcpluscmt -O2 -I/usr/lpp/UMS/include -I/usr/lpp/som/include -o $@ -c $<

install-data-local:
	for f in `(cd $(srcdir); find addon art levels lisp register sfx -type d -print)`; do \
	  $(mkinstalldirs) $(pkgdatadir)/$$f; \
	done
	for f in `(cd $(srcdir); find addon art levels lisp register sfx -type f -print)`; do \
	  echo " $(INSTALL_DATA) $(srcdir)/$$f $(pkgdatadir)/$$f"; \
	  $(INSTALL_DATA) $(srcdir)/$$f $(pkgdatadir)/$$f; \
	done

uninstall-local:
	rm -rf $(pkgdatadir) $(pkglibdir)

install-exec-local:
	-if [ -f $(pkglibdir)/abuse.console ]; then \
	  : *** abuse.console needs to run setuid root; \
	  : *** if this fails, set it up yourself; \
	  chown root $(pkglibdir)/abuse.console \
	  && chmod 4755 $(pkglibdir)/abuse.console ; \
	fi
