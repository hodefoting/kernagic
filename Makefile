BIN_PKGMODULES=cairo gtk+-2.0
PROJECT_NAME = kernagic

CFLAGS += -O2 -g -Wall -Wextra
LD_FLAGS += -lm

include .mm/magic
include .mm/bin





# Win32 cross-compilation catch-all gtk2 build, relies on the correct compiler
# i686-w64-mingw32-gcc being in path, and dependencies' include and lib dirs
# to exist as symlinks from the build location. 
#
# Install mingw-w64 for your distro
#
# fetch gtk+-bundle_2.24.10-20120208_win32.zip from the net
#
# To download the compiler and header files and dll stubs needed, follow
# instructions in:
#
# http://wiki.gimp.org/index.php/Hacking:Building/Windows 
#
# note that you can trim away quite a few of the packages listed in the
# grabstuff script, though some image format libraries are still needed by
# gtk-pixbuf and gtk.

$(PROJECT_NAME).exe: *.c *.h Makefile
	i686-w64-mingw32-gcc *.c -Wall -Wextra -Wno-unused-parameter \
	-Ilib/glib-2.0/include  \
	-Ilib/gtk-2.0/include  \
	-Iinclude/glib-2.0/ \
	-Iinclude/gtk-2.0/ \
	-Iinclude/cairo/ \
	-Iinclude/pango-1.0/ \
	-Iinclude/gdk-pixbuf-2.0/ \
	-Iinclude/atk-1.0/ \
	-Llib \
	-lglib-2.0.dll -lgobject-2.0.dll \
	-lgdk-win32-2.0.dll -lgtk-win32-2.0.dll -lcairo.dll \
	-lm \
	-O2 \
	-std=c99 \
	-mms-bitfields \
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
