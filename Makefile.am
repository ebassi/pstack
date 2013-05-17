## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = @PSTACK_CFLAGS@

# Libraries
lib_LTLIBRARIES = libpstack.la
libpstack_la_SOURCES = pstack.c pstack.h
libpstack_la_LIBADD = @PSTACK_LIBS@
include_HEADERS = pstack.h

# Pkg-config file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pstack.pc
DISTCLEANFILES = pstack.pc
EXTRA_DIST = pstack.pc.in
