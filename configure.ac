AC_INIT([cursetag], [1])
AM_INIT_AUTOMAKE([foreign -Wall])
AC_CONFIG_HEADERS([config.h])
AC_LANG([C++])
AC_PROG_CXXCPP
AC_PROG_CXX
AC_CONFIG_FILES([Makefile])

AC_CHECK_LIB(ncursesw,get_wch,,AC_MSG_ERROR([required library 'ncursesw' not found]))
AC_CHECK_LIB(vorbisfile,ov_open,[AC_DEFINE([ENABLE_OGG],1,["def to 1 if you have libvorbisfile"])
		[LIBS=$LIBS"-lvorbisfile "]])
AC_CHECK_HEADERS([FLAC++/all.h],[AC_DEFINE([ENABLE_FLAC],1,["def to 1 if you have libFLAC++"])
		[LIBS=$LIBS"-lFLAC++ "]])
AC_CHECK_HEADERS([id3/tag.h],[AC_DEFINE([ENABLE_MP3],1,["def to 1 if you have id3lib"])
		[LIBS=$LIBS"-lid3 "]])

AC_OUTPUT
