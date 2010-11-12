AC_INIT([irccmd], [1.0], [corpeters@@incas3.eu])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_PROG_CC

AC_CHECK_LIB( [ lua5.1],    [ luaL_newstate],      [ ], [ echo "Error! liblua5.1 is required."    ; exit -1 ])
AC_CHECK_LIB( [ argtable2], [ arg_parse],          [ ], [ echo "Error! libargtable2 is required." ; exit -1 ])
AC_CHECK_LIB( [ ircclient], [ irc_create_session], [ ], [ echo "Error! libircclient is required." ; exit -1 ])
AC_CHECK_LIB( [ lua5.1],    [ luaL_newstate] )
AC_CHECK_LIB( [ argtable2], [ arg_parse] )
AC_CHECK_LIB( [ ircclient], [ irc_create_session] )

AC_CHECK_HEADERS([stdbool.h])
AC_CHECK_HEADERS([signal.h])
AC_CHECK_HEADERS([errno.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([sys/select.h])
AC_CHECK_HEADERS([sys/time.h])

AC_CHECK_HEADERS([lua5.1/lua.h])
AC_CHECK_HEADERS([lua5.1/lualib.h])
AC_CHECK_HEADERS([lua5.1/luaxlib.h])
AC_CHECK_HEADERS([argtable2.h])
AC_CHECK_HEADERS([libircclient/libircclient.h])

AC_CONFIG_SRCDIR(src/main.c)
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_HEADERS([src/def.h])

AC_OUTPUT

