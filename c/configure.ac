#!/bin/bash

# Process this file with autoconf to produce a configure script.
AC_INIT(encaptunnel, 1.0.0, [])
AC_CONFIG_SRCDIR([src])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Disable static libraries by default
#AC_DISABLE_STATIC

# Checks for programs.
AC_PROG_CC
#AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdio.h stdlib.h])
AC_SUBST(AM_LDFLAGS, "$AM_LDFLAGS -lpthread")

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_CHECK_FUNCS([])

# Checks for GSE library
PKG_CHECK_MODULES([LIBGSE], [gse >= 1.2.0], [])
#LIBS="$LIBGSE $LIBS"
#AC_SUBST(AM_CFLAGS, "$LIBGSE_CFLAGS")

# check for pkg-config
#PKG_PROG_PKG_CONFIG
#CXXFLAGS="$CXXFLAGS -std=c++11"

# check if -Werror must be appended to CFLAGS
WERROR=""
AC_ARG_ENABLE(fail_on_warning,
              AS_HELP_STRING([--enable-fail-on-warning],
                             [build fails on warnings if enabled [[default=no]]]),
              fail_on_warning=$enableval,
              fail_on_warning=no)
if test "x$fail_on_warning" != "xno"; then
	WERROR="-Werror"
fi

AC_SUBST(AM_CFLAGS, "-g -Wall -W ${WERROR} -DUTI_DEBUG_ON $AM_CFLAGS")

AM_DEP_TRACK

AC_CONFIG_FILES([Makefile \
                 src/Makefile
                 src/common/Makefile
                 src/tap_udp/Makefile
                 ])

AC_OUTPUT

