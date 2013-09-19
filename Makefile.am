## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = @PSTACK_CFLAGS@

EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =

# Libraries
lib_LTLIBRARIES = libpstack.la
libpstack_la_SOURCES = \
	pstack.c pstack.h \
	plistbox.c plistbox.h \
	prevealer.c prevealer.h \
	pbubblewindow.c pbubblewindow.h
libpstack_la_LIBADD = @PSTACK_LIBS@
include_HEADERS = pstack.h plistbox.h \
	prevealer.h

# Pkg-config file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pstack.pc
DISTCLEANFILES += pstack.pc
EXTRA_DIST += pstack.pc.in

include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --warn-all
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
PLib-1.0.gir: libpstack.la Makefile
PLib_1_0_gir_VERSION = 1.0
PLib_1_0_gir_LIBS = libpstack.la
PLib_1_0_gir_CFLAGS = @PSTACK_CFLAGS@
PLib_1_0_gir_SCANNERFLAGS = \
       --warn-all \
       --identifier-prefix=P \
       --symbol-prefix=p
PLib_1_0_gir_INCLUDES = GObject-2.0 Gtk-3.0
PLib_1_0_gir_FILES = $(libpstack_la_SOURCES)
INTROSPECTION_GIRS += PLib-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = PLib-1.0.gir

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = PLib-1.0.typelib

CLEANFILES += $(gir_DATA) $(typelibs_DATA)
endif
