AC_INIT([cursetag], [3])
AM_INIT_AUTOMAKE([foreign -Wall])
AC_CONFIG_HEADERS([config.h])
AC_LANG([C++])
AC_PROG_CXXCPP
AC_PROG_CXX
AC_CONFIG_FILES([Makefile])

AC_CHECK_LIB(ncursesw,get_wch,,AC_MSG_ERROR([required library 'ncursesw' not found]))
AC_CHECK_HEADERS([taglib/fileref.h],,AC_MSG_ERROR([required library 'taglib' not found]))

AC_OUTPUT
