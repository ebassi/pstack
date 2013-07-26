#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([PStack], [2], [philip.chimento@gmail.com])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_SRCDIR([pstack.c])
AC_CONFIG_HEADERS([config.h])
AM_SILENT_RULES([yes])
AM_PROG_AR  dnl must call before LT_INIT
LT_INIT([disable-static])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
PKG_PROG_PKG_CONFIG

# Checks for libraries.
PSTACK_PRIVATE_REQUIREMENTS="gtk+-3.0 >= 3.8.0"
AC_SUBST([PSTACK_PRIVATE_REQUIREMENTS])
PKG_CHECK_MODULES([PSTACK], $PSTACK_PRIVATE_REQUIREMENTS)

# Checks for header files.
AC_CHECK_HEADERS([string.h])

GOBJECT_INTROSPECTION_REQUIRE([0.9.7])

AC_CONFIG_FILES([Makefile pstack.pc])
AC_OUTPUT
