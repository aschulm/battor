AC_INIT([BattOr Reader], [1.0], [aschulm@stanford.edu], [battor-reader], [http://www.stanford.edu/~schulman/battor.html])

AM_INIT_AUTOMAKE([1.10 no-define])

AC_SEARCH_LIBS([pow], [m], [], [
	AC_MSG_ERROR([unable to find math library])
])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_PROG_CC

AC_OUTPUT
