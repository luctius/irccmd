AC_INIT([irccmd], [1.0], [corpeters@@incas3.eu])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_PROG_CC
AM_PROG_CC_C_O
AC_LANG_C

AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM

AC_MSG_CHECKING(whether compiler understands -Wall)
old_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall"
AC_TRY_COMPILE([],[], AC_MSG_RESULT(yes), AC_MSG_RESULT(no), CFLAGS="$old_CFLAGS")

PKG_CHECK_MODULES([lua], [lua], [], 
                  [PKG_CHECK_MODULES([lua], [lua5.1], [], [])])
AC_SEARCH_LIBS( [arg_parse], [argtable argtable2] )
AC_SEARCH_LIBS( [irc_create_session], [ircclient ircclient0] )

# Checks for library functions (for argtable2).
AC_FUNC_MALLOC
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_CHECK_FUNCS([bzero strchr strcspn strrchr strtol])
AC_CHECK_FUNC(getopt_long, SYS_GETOPTLONG=1, SYS_GETOPTLONG=0)
AC_CHECK_FUNC(regcomp,     SYS_REGEX=1,      SYS_REGEX=0)
AC_CHECK_FUNC(strptime,    SYS_STRPTIME=1,   SYS_STRPTIME=0)

# Define automake conditionals (for argtable2)
AM_CONDITIONAL(USE_SYS_GETOPTLONG, test "$SYS_GETOPTLONG" = "1")
AM_CONDITIONAL(USE_ARGREX,         test "$SYS_REGEX" = "1")
AM_CONDITIONAL(USE_ARGDATE,        test "$SYS_STRPTIME" = "1")

AC_CHECK_HEADERS([stdbool.h])
AC_CHECK_HEADERS([signal.h])
AC_CHECK_HEADERS([errno.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([sys/select.h])
AC_CHECK_HEADERS([sys/time.h])

AC_CHECK_HEADERS([argtable2.h])
AC_CHECK_HEADERS([libircclient.h])
AC_CHECK_HEADERS([libircclient/libircclient.h])

AC_CONFIG_SRCDIR(src/main.c)
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_HEADERS([src/def.h])


AC_OUTPUT

