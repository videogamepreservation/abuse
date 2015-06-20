
EXENAME		= boat

X11_FILES = x11_vid.c x11_evt.c
SVGA_FILES = svga_vid.c svga_evt.c

CSRC =  $(X11_FILES) \
        floater.c \
        boat.c \
        bomb.c \
        image.c \
        linked.c \
        palette.c \
        jmalloc.c \
        specs.c \
        filter.c \
        dprint.c \
        rectlist.c \
        texture.c \
        draw.c \
        timing.c \
        main.c \
        boatcnst.c \
        boatdraw.c \
        bombdraw.c \
        anims.c \
        animate.c \
        boatgfx.c

CCSRC =  \
		
EXTDEP =  \

EXEDEP =	$(CCSRC:%.cc=%.o) $(CSRC:%.c=%.o)
SRCDEP =	$(CCSRC:%.cc=.%.d) $(CSRC:%.c=.%.d)	
DIRTYFILES = \
	*.bak \
	$(EXEDEP) \
	$(SRCDEP) \
	$(EXENAME)

# }}} ################################################################
# {{{ Translator Definitions

CC              = g++
CXX             = g++
SHELL		= /bin/sh

INCLUDE		= -Iinc

LIBS		= -lm -lXext -lX11
DEFS		= -DMEM_CHECK

CDEBUG		= -O2 -g
CPPFLAGS	= $(CDEBUG) $(INCLUDE) $(DEFS)
LDFLAGS		= -O2 -g


OFILES = src/mono_cfg.obj src/specache.obj imlib/sprite.obj src/netcfg.obj src/text_gui.obj \
        src/key_cfg.obj \
        src/innet.obj   src/net/unix/gserver.obj \
        src/net/unix/gclient.obj  src/net/unix/fileman.obj  src/net/sock.obj \
        src/chat.obj src/endgame.obj src/setup.obj src/version.obj src/loadgame.obj src/profile.obj \
	src/cop.obj src/statbar.obj src/compiled.obj src/ant.obj src/sensor.obj \
        src/lisp_opt.obj src/demo.obj src/lcache.obj src/lisp_gc.obj \
        src/nfclient.obj src/username.obj src/clisp2.obj src/gui.obj \
        src/transp.obj src/collide.obj src/trig.obj src/property.obj \
	src/lisp src/cache src/objects \
	src/extend src/console src/ability \
	src/items src/dev src/chars src/level src/smallfnt \
	src/help src/intsect src/loader2 src/seq \
	src/points src/fnt6x13 src/morpher src/menu src/director src/view \
	src/config src/game src/light src/devsel \
	src/crc src/gamma src/language

.c.obj
        $(CXX) /omaxne /zp1 /zq $*.c

CXX              = wpp386
