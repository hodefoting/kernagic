BIN_PKGMODULES=cairo gtk+-2.0
PROJECT_NAME = kernagic

CFLAGS += -O2 -g -Wall -Wextra
LD_FLAGS += -lm

include .mm/magic
include .mm/bin

# Install mingw-w64 for your distro this should contain the
# i686-w64-mingw32-gcc compiler which is used for mingw cross-compiling of
# 32bit windows applications.
# 
# dependencies are downloaded automatically from files described at
# http://www.gtk.org/download/win32.php

gtk+-mingw-bundle/gtk+-bundle_2.24.10-20120208_win32.zip:
	(cd gtk+-mingw-bundle/; wget http://ftp.gnome.org/pub/gnome/binaries/win32/gtk+/2.24/gtk+-bundle_2.24.10-20120208_win32.zip; )

gtk+-mingw-bundle/unpacked.stamp: gtk+-mingw-bundle/gtk+-bundle_2.24.10-20120208_win32.zip
	(cd gtk+-mingw-bundle;\
	 unzip gtk+-bundle_2.24.10-20120208_win32.zip;\
	 touch unpacked.stamp)

$(PROJECT_NAME).exe: *.c *.h Makefile gtk+-mingw-bundle/unpacked.stamp
	i686-w64-mingw32-gcc *.c -Wall -Wextra -Wno-unused-parameter \
	-Igtk+-mingw-bundle/lib/glib-2.0/include  \
	-Igtk+-mingw-bundle/lib/gtk-2.0/include  \
	-Igtk+-mingw-bundle/include/glib-2.0/ \
	-Igtk+-mingw-bundle/include/gtk-2.0/ \
	-Igtk+-mingw-bundle/include/cairo/ \
	-Igtk+-mingw-bundle/include/pango-1.0/ \
	-Igtk+-mingw-bundle/include/gdk-pixbuf-2.0/ \
	-Igtk+-mingw-bundle/include/atk-1.0/ \
	\
	-lm -O2 -std=c99 -mms-bitfields \
	\
	-mwindows -Wl,--subsystem,windows \
	\
	-Lgtk+-mingw-bundle/lib -lgio-2.0.dll \
	-lglib-2.0.dll -lgobject-2.0.dll -lpango-1.0.dll -lpangocairo-1.0.dll \
	-lgdk-win32-2.0.dll -lgtk-win32-2.0.dll -lcairo.dll \
	\
	 -o $@ && echo
	i686-w64-mingw32-strip $@

# the kernagic.exe binary should work if you drop it into a folder with all
# the dlls downloaded above. To make the installer; only the neccesary dlls
# should be selected and placed in a dll directory. Then typing make
# kernagic-installer.exe would bundle these dls's with a freshly built exe
# producing an installer.

kernagic-installer.exe: kernagic.exe kernagic.nsis
	@makensis kernagic.nsis
	@chmod a+x $@
	@echo
	@ls -sh *.exe
	@echo
	@sha1sum *.exe

clean: clean-extra
clean-extra:
	rm -f *.exe kernagic
	rm -f tests/output/*

sync:
	cp tests/output/* tests/reference
