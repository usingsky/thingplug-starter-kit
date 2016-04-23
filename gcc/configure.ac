#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
#AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_INIT(thingplugteset, 1.0, usingsky@gmail.com)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/thingplugtest.cpp])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_LANG_PUSH([C++])

# Checks for libraries.
#PKG_CHECK_MODULES(libcurl, libcurl)
PKG_CHECK_MODULES(libcurl, libcurl, , AC_MSG_ERROR('Unable to find libcurl.  Please make sure library and header files are installed.'))
PKG_CHECK_MODULES(jsonc, json-c)

#AC_CHECK_LIB([mqttpp], [client::client])
#AC_CHECK_LIB([paho-mqtt3cs], [main]) 
AC_SUBST(common_CFLAGS, [-I$srcdir/include])
#AC_SUBST(libcurl_LIBS)


# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
