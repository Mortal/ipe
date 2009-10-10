# -*- makefile -*-
# --------------------------------------------------------------------
#
# Ipe configuration
#
# --------------------------------------------------------------------
#
# Include and linking options for libraries
#
# We just query "pkg-config" for the correct flags.  If this doesn't
# work on your system, enter the correct linker flags and directories
# directly.
#
ZLIB_CFLAGS   ?=
ZLIB_LIBS     ?= -lz
FREETYPE_CFLAGS ?= $(shell pkg-config --cflags freetype2)
FREETYPE_LIBS ?= $(shell pkg-config --libs freetype2)
CAIRO_CFLAGS  ?= $(shell pkg-config --cflags cairo)
CAIRO_LIBS    ?= $(shell pkg-config --libs cairo)
LUA_CFLAGS    ?= $(shell pkg-config --cflags lua5.1)
LUA_LIBS      ?= $(shell pkg-config --libs lua5.1)
QT_CFLAGS     ?= $(shell pkg-config --cflags QtGui QtCore)
QT_LIBS	      ?= $(shell pkg-config --libs QtGui QtCore)
MOC	      ?= moc
#
# --------------------------------------------------------------------
#
# The C++ compiler (only g++ is properly tested)
#
CXX = g++
#
# Special compilation flags for compiling shared libraries
# 64-bit Linux requires shared libraries to be compiled with -fPIC
# (and it doesn't hurt on 32bit Linux)
DLL_CFLAGS = -fPIC
#
# --------------------------------------------------------------------
#
# Installing Ipe:
#
IPEVERS = 7.0.3
#
# IPEPREFIX is the global prefix for the Ipe directory structure, which
# you can override individually for any of the specific directories.
# You could choose "/usr/local" or "/opt/ipe-7.0", or
# even "/usr", or "$(HOME)/ipe-7.0" if you have to install in your home
# directory.
#
# If you are installing Ipe in a networked environment, keep in mind
# that executables, ipelets, and Ipe library are machine-dependent,
# while the documentation and fonts can be shared.
#
#IPEPREFIX  := /usr/local
#IPEPREFIX  := /usr
#IPEPREFIX  := /opt/ipe7
#
ifeq "$(IPEPREFIX)" ""
$(error You need to specify IPEPREFIX!)
endif
#
# Where Ipe executables will be installed ('ipe', 'ipetoipe' etc)
IPEBINDIR  = $(IPEPREFIX)/bin
#
# Where the Ipe libraries will be installed ('libipe.so' etc.)
IPELIBDIR  = $(IPEPREFIX)/lib
#
# Where the header files for Ipelib will be installed:
IPEHEADERDIR = $(IPEPREFIX)/include
#
# Where Ipelets will be installed:
IPELETDIR = $(IPEPREFIX)/lib/ipe/$(IPEVERS)/ipelets
#
# List of paths where Ipe will search for Ipelets:
# (Individual paths are separated by ";" on both Windows and Unix!)
IPELETPATH = $(IPELETDIR)
#
# Where Lua code will be installed
# (This is the part of the Ipe program written in the Lua language)
IPELUADIR = $(IPEPREFIX)/share/ipe/$(IPEVERS)/lua
#
# List of patterns where Ipe will search for Lua code:
# (Individual paths are separated by ";" on both Windows and Unix!)
IPELUAPATH = $(IPELUADIR)/?.lua
#
# Directory where Ipe will look for style files
# (standard Ipe styles will also be installed here)
#
IPESTYLES = $(IPEPREFIX)/share/ipe/$(IPEVERS)/styles
#
# IPEICONDIR contains the icons used in the Ipe user interface
#
IPEICONDIR = $(IPEPREFIX)/share/ipe/$(IPEVERS)/icons
#
# IPEDOCDIR contains the Ipe documentation (mostly html files)
#
IPEDOCDIR = $(IPEPREFIX)/share/ipe/$(IPEVERS)/doc
#
# The Ipe manual pages are installed into IPEMANDIR
#
IPEMANDIR = $(IPEPREFIX)/share/man/man1
#
# The full path to the Ipe fontmap
#
IPEFONTMAP = $(IPEPREFIX)/share/ipe/$(IPEVERS)/fontmap.xml
#
# --------------------------------------------------------------------
