#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([lbreakouthd],[1.1.3])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

AC_USE_SYSTEM_EXTENSIONS
AM_GNU_GETTEXT([external])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB(m, main,, AC_MSG_ERROR(lib math is needed))
AC_CHECK_LIB(SDL2, main,, AC_MSG_ERROR(lib SDL2 is needed))
AC_CHECK_LIB(SDL2_ttf, main,, AC_MSG_ERROR(lib SDL2 ttf is needed))
AC_CHECK_LIB(SDL2_image, main,, AC_MSG_ERROR(lib SDL2 image is needed))
AC_CHECK_LIB(SDL2_mixer, main,, AC_MSG_ERROR(lib SDL2 mixer is needed))

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([memset strchr strdup strrchr])

dnl allow changing config dir
configdir=.lbreakouthd
AC_ARG_WITH( configdir,
[  --with-configdir=CONFIGDIR       Set directory for config/data files in home directory (default: .lbreakouthd).],
configdir="$withval",)

# other flags
datdir=$datadir/$PACKAGE
hiscoredir=$localstatedir
debugflags=""
AC_ARG_ENABLE( install,
[  --disable-install       No installation. Start from source directory.], 
datdir="." hiscoredir="." configdir=".")
AC_ARG_ENABLE( debug,
[  --enable-debug       Add debug symbols, no optimization.], 
debugflags="-O0 -g")
AC_SUBST(datdir)
AC_SUBST(hiscoredir)

CFLAGS="$CFLAGS -Wno-int-conversion -Wno-format -Wno-implicit-function-declaration"
CFLAGS="$CFLAGS $debugflags -DSRC_DIR=\\\"$datdir\\\" -DCONFIG_DIR_NAME=\\\"$configdir\\\""
CXXFLAGS="$CXXFLAGS -std=c++11 -Wall $debugflags -DLOCALEDIR=\\\"$localedir\\\" -DCONFIGDIR=\\\"$configdir\\\" -DDATADIR=\\\"$datdir\\\" -DHISCOREDIR=\\\"$hiscoredir\\\""

if test "x$host" = "xx86_64-w64-mingw32"; then
	CFLAGS="$CFLAGS -DWIN32 -I/usr/$host/sys-root/mingw/include"
	CXXFLAGS="$CXXFLAGS -DWIN32 -I/usr/$host/sys-root/mingw/include"
	CXXFLAGS="$CXXFLAGS $(/usr/$host/sys-root/mingw/bin/sdl2-config --cflags)"
	# we can't use sdl2-config as we want to skip -mwindows as it
	# does funny things (not like a clown tho) also -lintl is needed
	LIBS="$LIBS -lintl -L/usr/$host/sys-root/mingw/lib -lmingw32 -lSDL2main -lSDL2"
else
	CXXFLAGS="$CXXFLAGS $(sdl2-config --cflags)"
	LIBS="$LIBS $LIBINTL $(sdl2-config --libs)"
fi

# Makefiles
AC_CONFIG_FILES([Makefile 
po/Makefile.in
libgame/Makefile 
src/Makefile 
src/levels/Makefile 
src/themes/Makefile 
src/themes/Standard/Makefile 
])
AC_OUTPUT

