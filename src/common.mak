# -*- makefile -*-
# --------------------------------------------------------------------
#
# Building Ipe --- common definitions
#
# --------------------------------------------------------------------
# Are we compiling for Windows?
ifdef COMSPEC
  WIN32	=  1
  MINGLIBS ?= c:/Home/Devel/ming
  QTDIR ?= c:/Qt/4.5.2
endif
ifdef MINGWCROSS
  WIN32 = 1
  MINGLIBS ?= /windows/Home/Devel/ming
  QTDIR ?= /windows/Qt/4.5.2
endif
# --------------------------------------------------------------------
# IPESRCDIR is Ipe's top "src" directory
# if 'common.mak' is included on a different level than a subdirectory
# of "src", then IPESRCDIR must be set before including 'common.mak'.

IPESRCDIR ?= ..

# --------------------------------------------------------------------
# Read configuration options (not used on Win32)

ifndef WIN32
  include $(IPESRCDIR)/config.mak
endif

# --------------------------------------------------------------------

BUILDDIR = $(IPESRCDIR)/../build

moc_sources = $(addprefix moc_, $(subst .h,.cpp,$(moc_headers)))

objects = $(addprefix $(OBJDIR)/, $(subst .cpp,.o,$(sources) $(moc_sources)))

CXXFLAGS += -Wall

ifdef IPESTRICT
  CXXFLAGS += -Werror
endif

DEPEND ?= $(OBJDIR)/depend.mak

.PHONY: clean 
.PHONY: install
.PHONY: all

ifdef WIN32
  CPPFLAGS	+= -DWIN32      
  DLL_LDFLAGS	+= -shared 
  buildlib	= $(BUILDDIR)/bin
  exe_target	= $(BUILDDIR)/bin/$1.exe
  dll_target    = $(buildlib)/$1.dll
  ipelet_target = $(BUILDDIR)/ipelets/$1.dll

  ZLIB_CFLAGS   := -I$(MINGLIBS)/zlib/include
  ZLIB_LIBS     := $(MINGLIBS)/zlib/lib/zlib.dll
  FREETYPE_CFLAGS := -I$(MINGLIBS)/freetype/include
  FREETYPE_LIBS := $(MINGLIBS)/freetype/lib/freetype2.dll
  CAIRO_CFLAGS  := -I$(MINGLIBS)/cairo/include
  CAIRO_LIBS    := $(MINGLIBS)/cairo/lib/cairo.dll
  LUA_CFLAGS    := -I$(MINGLIBS)/lua/include
  LUA_LIBS      := $(MINGLIBS)/lua/lib/lua51.dll
  QT_CFLAGS     := -I$(QTDIR)/include -I$(QTDIR)/include/QtCore \
	           -I$(QTDIR)/include/QtGui
  QT_LIBS	:= -L$(MINGLIBS)/qt/lib -lQtGui4 -lQtCore4

ifdef MINGWCROSS
  # --------------- Cross compiling with MinGW32 ---------------
  CXXFLAGS	+= -g -O2 -fno-rtti -fno-exceptions
  CXX = i586-mingw32msvc-g++
  CC = i586-mingw32msvc-gcc
  STRIP_TARGET  = i586-mingw32msvc-strip $(TARGET)
  WINDRES	= i586-mingw32msvc-windres
else
  # --------------- Compiling with MinGW32 under Windows ---------------
  WINDRES	= windres
  CXXFLAGS	+= -g -O2 -fno-rtti -fno-exceptions
  LDFLAGS	+= -enable-stdcall-fixup \
	-Wl,--enable-runtime-pseudo-reloc -Wl,-enable-auto-import 
  MOC	        := $(QTDIR)/bin/moc
endif

else
  # -------------------- Unix --------------------
  CXXFLAGS	+= -g -O2
  DLL_LDFLAGS	+= -shared 
  buildlib	= $(BUILDDIR)/lib
  dll_target    = $(buildlib)/lib$1.so
  ipelet_target = $(BUILDDIR)/ipelets/$1.so
  exe_target	= $(BUILDDIR)/bin/$1
endif

# Macros

INSTALL_DIR = install -d
INSTALL_FILES = install -m 0644 -t
INSTALL_PROGRAMS = install -s -m 0755 -t

MAKE_BINDIR = mkdir -p $(BUILDDIR)/bin

MAKE_LIBDIR = mkdir -p $(buildlib)

MAKE_IPELETDIR = mkdir -p $(BUILDDIR)/ipelets

MAKE_DEPEND = \
	mkdir -p $(OBJDIR); \
	echo "" > $@; \
	for f in $(sources); do \
	$(CXX) -MM -MT $(OBJDIR)/$${f%%.cpp}.o $(CPPFLAGS) $$f >> $@; done

# The rules

$(OBJDIR)/%.o:  %.cpp
	@echo Compiling $(<F)...
	$(COMPILE.cc) -o $@ $<

moc_%.cpp:  %.h
	@echo Running moc on $(<F)...
	$(MOC) $(CPPFLAGS )-o $@ $<

# --------------------------------------------------------------------
