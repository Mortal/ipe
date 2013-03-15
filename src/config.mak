# -*- makefile -*-
# --------------------------------------------------------------------
#
# Ipe configuration
#
# --------------------------------------------------------------------
#
# Which user interface library shall we use?
#
# Currently, the choice is between the Win32 API and Qt 4.
# Choose one of "WIN32", or "QT".  Unless you compile for Microsoft
# Windows, you will want "QT" (so the default will be fine).
# (In fact, WIN32 doesn't work yet.)
#
IPEUI 	     ?= QT
#
# Do you wish to enable the use of alternative text encodings
# for Latex conversion?  (Useful for Japanese, Korean, Russian, etc.)
# If so, uncomment the following line:
#
#IPE_USE_ICONV = -DIPE_USE_ICONV
#
# If you enabled this feature, Ipe will need the functions
# iconv_open, iconv, and iconv_close.
# On Linux, these are simply in libc, and you need to do nothing extra.
# On other systems, install libiconv and uncomment the following lines:
#
#ICONV_CFLAGS =
#ICONV_LIBS   = -liconv
#
# ------------------------------------------------------------------
# Include and linking options for libraries
# ------------------------------------------------------------------
#
ifndef MACOS
#
# We just query "pkg-config" for the correct flags.  If this doesn't
# work on your system, enter the correct linker flags and directories
# directly.  You don't have to worry about the UI libraries you
# haven't selected above.
#
# The name of the Lua package (it could be "lua", "lua52", or "lua5.2")
#
LUA_PACKAGE   ?= lua5.2
#
ZLIB_CFLAGS   ?=
ZLIB_LIBS     ?= -lz
FREETYPE_CFLAGS ?= $(shell pkg-config --cflags freetype2)
FREETYPE_LIBS ?= $(shell pkg-config --libs freetype2)
CAIRO_CFLAGS  ?= $(shell pkg-config --cflags cairo)
CAIRO_LIBS    ?= $(shell pkg-config --libs cairo)
LUA_CFLAGS    ?= $(shell pkg-config --cflags $(LUA_PACKAGE))
LUA_LIBS      ?= $(shell pkg-config --libs $(LUA_PACKAGE))
GTK_CFLAGS    ?= $(shell pkg-config --cflags gtk+-2.0)
GTK_LIBS      ?= $(shell pkg-config --libs gtk+-2.0)
QT_CFLAGS     ?= $(shell pkg-config --cflags QtGui QtCore)
QT_LIBS	      ?= $(shell pkg-config --libs QtGui QtCore)
#
# MOC is the Qt meta-object compiler.
# If you have different Qt versions installed, you may have to change
# this to "moc-qt4" if the default is Qt3's "moc".
#
MOC	      ?= moc
#
else
#
# Settings for Mac OS 10.6
#
CONFIG     += x86_64
ZLIB_CFLAGS   ?=
ZLIB_LIBS     ?= -lz
FREETYPE_CFLAGS ?= -I/usr/X11/include/freetype2 -I/usr/X11/include
FREETYPE_LIBS ?= -L/usr/X11/lib -lfreetype
CAIRO_CFLAGS  ?= -I/usr/X11/include/cairo -I/usr/X11/include/pixman-1 \
	 -I/usr/X11/include/freetype2 -I/usr/X11/include \
	 -I/usr/X11/include/libpng12
CAIRO_LIBS ?= -L/usr/X11/lib -lcairo
LUA_CFLAGS ?= -I/usr/local/include
LUA_LIBS   ?= -L/usr/local/lib -llua52 -lm
QT_CFLAGS  ?= -I/Library/Frameworks/QtCore.framework/Versions/4/Headers \
	      -I/Library/Frameworks/QtGui.framework/Versions/4/Headers
QT_LIBS    ?= -F/Library/Frameworks -L/Library/Frameworks \
	      -framework QtCore -framework ApplicationServices \
	      -framework QtGui -framework AppKit -framework Cocoa -lz -lm
MOC	   ?= moc
endif
#
# --------------------------------------------------------------------
#
# The C++ compiler (only g++ is properly tested)
#
CXX = g++
#
# Special compilation flags for compiling shared libraries
# 64-bit Linux requires shared libraries to be compiled as
# position independent code
# (it doesn't hurt much on 32bit Linux, although you could comment it
# out for slightly faster code)
DLL_CFLAGS = -fpic
#
# --------------------------------------------------------------------
#
# Installing Ipe:
#
IPEVERS = 7.1.4
#
# IPEPREFIX is the global prefix for the Ipe directory structure, which
# you can override individually for any of the specific directories.
# You could choose "/usr/local" or "/opt/ipe7", or
# even "/usr", or "$(HOME)/ipe7" if you have to install in your home
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
# Where Lua code will be installed
# (This is the part of the Ipe program written in the Lua language)
IPELUADIR = $(IPEPREFIX)/share/ipe/$(IPEVERS)/lua
#
# Directory where Ipe will look for scripts
# (standard scripts will also be installed here)
IPESCRIPTDIR = $(IPEPREFIX)/share/ipe/$(IPEVERS)/scripts
#
# Directory where Ipe will look for style files
# (standard Ipe styles will also be installed here)
IPESTYLEDIR = $(IPEPREFIX)/share/ipe/$(IPEVERS)/styles
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
