AC_INIT([irccmd], [1.0], [corpeters@incas3.eu])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_PROG_CC
AM_PROG_CC_C_O
AC_LANG_C

AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM

AC_MSG_CHECKING(whether compiler understands -Wall and -Wextra)
old_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall -Wextra"
AC_TRY_COMPILE([],[], AC_MSG_RESULT(yes), AC_MSG_RESULT(no), CFLAGS="$old_CFLAGS")

PKG_CHECK_MODULES([lua], [lua], [], [PKG_CHECK_MODULES([lua], [lua5.1] )])
AC_SEARCH_LIBS( [luaL_loadstring], [lua5.1], [], [AC_MSG_ERROR("liblua5.1 is missing")])
AC_SEARCH_LIBS( [arg_parse], [argtable argtable2], [], [AC_MSG_ERROR("Argtable2 is missing")])
AC_SEARCH_LIBS( [irc_create_session], [ircclient ircclient0], [], [AC_MSG_ERROR("libircclient is missing")])
AC_SEARCH_LIBS( [rl_callback_handler_install], [readline], [], [AC_MSG_ERROR("readline is missing")])

# Define automake conditionals (for argtable2)
AM_CONDITIONAL(USE_SYS_GETOPTLONG, test "$SYS_GETOPTLONG" = "1")
AM_CONDITIONAL(USE_ARGREX,         test "$SYS_REGEX" = "1")
AM_CONDITIONAL(USE_ARGDATE,        test "$SYS_STRPTIME" = "1")

AC_CHECK_HEADERS([stdbool.h], [], AC_MSG_ERROR("stdbool.h is missing"))
AC_CHECK_HEADERS([signal.h], [], AC_MSG_ERROR("signal.h is missing"))
AC_CHECK_HEADERS([errno.h], [], AC_MSG_ERROR("errno.h is missing"))
AC_CHECK_HEADERS([fcntl.h], [], AC_MSG_ERROR("fcntl.h is missing"))
AC_CHECK_HEADERS([sys/select.h], [], AC_MSG_ERROR("sys/select.h is missing"))
AC_CHECK_HEADERS([sys/time.h], [], AC_MSG_ERROR("sys/tim.h is missing"))

AC_HEADER_STDBOOL
AC_CHECK_HEADERS([argtable2.h], [], AC_MSG_ERROR("argtable2.h is missing"))
AC_CHECK_HEADERS([readline/readline.h], [], AC_MSG_ERROR("readline.h is missing"))
lic_h=false
AC_CHECK_HEADERS([libircclient.h], [], lic_h=true)
AC_CHECK_HEADERS([libircclient/libircclient.h], [], lic_h=true)
if test "x$lic_h" != xtrue
then
    AC_MSG_ERROR("libircclient.h is missing")
fi

AC_CONFIG_SRCDIR(src/main.c)
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_HEADERS([src/def.h])

# Check Functions
AC_FUNC_MALLOC
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([bzero strchr strcspn strrchr strtol], [], AC_MSG_ERROR("String functions missing or not available"))
AC_CHECK_FUNC([getopt_long], [], AC_MSG_ERROR("getopt_long function missing or not available"))
AC_CHECK_FUNC([regcomp], [], AC_MSG_ERROR("regcomp function missing or not available"))
AC_CHECK_FUNC([strptime], [], AC_MSG_ERROR("strptime function missing or not available"))
AC_CHECK_FUNCS([memmove], [], AC_MSG_ERROR("memmove function missing or not available"))
AC_CHECK_FUNCS([memset], [], AC_MSG_ERROR("memset function missing or not available"))
AC_CHECK_FUNCS([select], [], AC_MSG_ERROR("select function missing or not available"))
AC_CHECK_FUNCS([strdup], [], AC_MSG_ERROR("strdup function missing or not available"))
AC_CHECK_FUNCS([strstr], [], AC_MSG_ERROR("strstr function missing or not available"))

AC_OUTPUT

