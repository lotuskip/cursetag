
AC_PREREQ([2.65])
AC_INIT([CurseTag], [1])
AM_INIT_AUTOMAKE([foreign -Wall])
AC_OUTPUT(Makefile src/Makefile)
AC_CONFIG_SRCDIR([src/cursetag.cpp])
AC_LANG([C++])

AC_PROG_CXXCPP
AC_PROG_CXX

AC_CONFIG_HEADER(config.h)
AC_CHECK_LIB(ncursesw,get_wch,,AC_MSG_ERROR(required library 'ncursesw' not found))
AC_CHECK_LIB(vorbisfile,ov_open,[AC_DEFINE(ENABLE_OGG,1,"def to 1 if you have libvorbisfile")
		LIBS=$LIBS"-lvorbisfile "])
AC_CHECK_HEADER(FLAC++/all.h,[AC_DEFINE(ENABLE_FLAC,1,"def to 1 if you have libFLAC++")
		LIBS=$LIBS"-lFLAC++ "])
AC_CHECK_HEADER(id3/tag.h,[AC_DEFINE(ENABLE_MP3,1,"def to 1 if you have id3lib")
		LIBS=$LIBS"-lid3 "])

AC_OUTPUT
