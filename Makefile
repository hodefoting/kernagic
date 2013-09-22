BIN_PKGMODULES=cairo gtk+-2.0
PROJECT_NAME = kernagic

CFLAGS += -O2 -g
LD_FLAGS += -lm

include .mm/magic
include .mm/bin

# the following mingw32 target relies on the correct compiler being in path,
# and resources as described by
# http://wiki.gimp.org/index.php/Hacking:Building/Windows residing in /windev
# 

WINROOT=/windev/grab/usr/i686-w64-mingw32/sys-root/mingw
kernagic.exe: *.c *.h Makefile
	@echo -n "CC $@"; i686-w64-mingw32-gcc *.c -Wall -std=c99 \
	-I$(WINROOT)/lib/glib-2.0/include  \
	-I$(WINROOT)/lib/gtk-2.0/include  \
	-I$(WINROOT)/include/glib-2.0/ \
	-I$(WINROOT)/include/gtk-2.0/ \
	-I$(WINROOT)/include/cairo/ \
	-I$(WINROOT)/include/pango-1.0/ \
	-I$(WINROOT)/include/gdk-pixbuf-2.0/ \
	-I$(WINROOT)/include/atk-1.0/ \
	-L$(WINROOT)/lib \
	-lglib-2.0.dll -lgobject-2.0.dll \
	-lgdk-win32-2.0.dll -lgtk-win32-2.0.dll -lcairo.dll \
	-lm \
	-mms-bitfields \
	 -o $@ && echo
