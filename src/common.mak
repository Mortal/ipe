# -*- makefile -*-
# --------------------------------------------------------------------
#
# Building Ipe --- common definitions
#
# --------------------------------------------------------------------
# Are we compiling for Windows?  For Mac OS X?
ifdef COMSPEC
  WIN32	=  1   
else ifdef IPEMXE
  WIN32 = 1
else ifdef MINGWCROSS
  WIN32 = 1
else
  UNAME = $(shell uname)
  ifeq "$(UNAME)" "Darwin"
    MACOS = 1
  endif
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

ifdef WIN32
  IPEUI ?= QT
  IPE_USE_ICONV :=
  BUILDDIR = $(IPESRCDIR)/../mingw
else
  BUILDDIR = $(IPESRCDIR)/../build
endif

# set variables for UI choice
ifeq ($(IPEUI),QT)
CPPFLAGS += -DIPEUI_QT
IPEUI_QT := 1
UI_CFLAGS = $(QT_CFLAGS)
UI_LIBS = $(QT_LIBS)
moc_sources = $(addprefix moc_, $(subst .h,.cpp,$(moc_headers)))
all_sources = $(sources) $(qt_sources)
objects = $(addprefix $(OBJDIR)/, $(subst .cpp,.o,$(all_sources) \
	$(moc_sources)))
else
ifeq ($(IPEUI), WIN32)
CPPFLAGS += -DIPEUI_WIN32
IPEUI_WIN32 := 1
UI_CFLAGS := 
UI_LIBS := -lcomctl32 -lcomdlg32 -lgdi32
all_sources = $(sources) $(win_sources)
objects = $(addprefix $(OBJDIR)/, $(subst .cpp,.o,$(all_sources)))
else
ifeq ($(IPEUI), GTK)
CPPFLAGS += -DIPEUI_GTK
IPEUI_GTK := 1
UI_CFLAGS = $(GTK_CFLAGS)
UI_LIBS = $(GTK_LIBS)
all_sources = $(sources) $(gtk_sources)
BUILDDIR = $(IPESRCDIR)/../build-gtk
objects = $(addprefix $(OBJDIR)/, $(subst .cpp,.o,$(all_sources)))
else
error("Unknown IPEUI selected")
endif
endif
endif

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
  soname        = 
  dll_symlinks  = 
  install_symlinks = 
  ipelet_target = $(BUILDDIR)/ipelets/$1.dll


ifdef IPEMXE
  # --------------- Cross compiling with MXE ---------------
  CXXFLAGS	+= -g -O2
  CXX = i686-pc-mingw32-g++
  CC = i686-pc-mingw32-gcc
  STRIP_TARGET  = i686-pc-mingw32-strip $(TARGET)
  WINDRES	= i686-pc-mingw32-windres

  IPEDEPS	:= /sw/mingw

  QT_CFLAGS := -I/sw/mxe/usr/i686-pc-mingw32/include/QtGui \
	-I/sw/mxe/usr/i686-pc-mingw32/include/QtCore \
	-I/sw/mxe/usr/i686-pc-mingw32/include/Qt
  QT_LIBS := /sw/mxe/usr/i686-pc-mingw32/lib/QtCore4.dll \
	/sw/mxe/usr/i686-pc-mingw32/lib/QtGui4.dll
  MOC	      	?= i686-pc-mingw32-moc
else ifdef MINGWCROSS
  # --------------- Cross compiling with MinGW32 ---------------
  CXXFLAGS	+= -g -O2
  CXX = i586-mingw32msvc-g++
  CC = i586-mingw32msvc-gcc
  STRIP_TARGET  = i586-mingw32msvc-strip $(TARGET)
  WINDRES	= i586-mingw32msvc-windres

  IPEDEPS	:= /sw/mingw
else
  # --------------- Compiling with MinGW32 under Windows ---------------
  WINDRES	= C:/MinGW/bin/windres.exe
  CXXFLAGS	+= -g -O2

  QTHOME	:= E:/Qt/4.8.4
  IPEDEPS	:= E:/IpeDeps

  QT_CFLAGS := -I$(QTHOME)/include/QtGui \
	-I$(QTHOME)/include/QtCore \
	-I$(QTHOME)/include
  QT_LIBS := $(QTHOME)/bin/QtCore4.dll $(QTHOME)/bin/QtGui4.dll
  MOC	      	?= $(QTHOME)/bin/moc.exe
endif

  ZLIB_CFLAGS   := -I$(IPEDEPS)/include
  ZLIB_LIBS     := $(IPEDEPS)/bin/zlib1.dll
  FREETYPE_CFLAGS := -I$(IPEDEPS)/include/freetype2 \
       -I$(IPEDEPS)/include
  FREETYPE_LIBS := $(IPEDEPS)/bin/freetype6.dll
  CAIRO_CFLAGS  := -I$(IPEDEPS)/include/cairo
  CAIRO_LIBS    := $(IPEDEPS)/bin/libcairo-2.dll
  LUA_CFLAGS    := -I$(IPEDEPS)/lua52/include
  LUA_LIBS      := $(IPEDEPS)/lua52/lua52.dll

else
  # -------------------- Unix --------------------
  CXXFLAGS	+= -g -O2
  ifdef MACOS
    DLL_LDFLAGS	+= -dynamiclib 
    # IPELIBDIRINFO can be overridden as @executable_path/../lib
    # on the command line or by an environment variable
    IPELIBDIRINFO ?= $(IPELIBDIR)
    soname      = -Wl,-dylib_install_name,$(IPELIBDIRINFO)/lib$1.so.$(IPEVERS)
  else	
    DLL_LDFLAGS	+= -shared 
    soname      = -Wl,-soname,lib$1.so.$(IPEVERS)
  endif
  buildlib	= $(BUILDDIR)/lib
  dll_target    = $(buildlib)/lib$1.so.$(IPEVERS)
  dll_symlinks  = ln -sf lib$1.so.$(IPEVERS) $(buildlib)/lib$1.so
  install_symlinks = ln -sf lib$1.so.$(IPEVERS) \
		$(INSTALL_ROOT)$(IPELIBDIR)/lib$1.so
  ipelet_target = $(BUILDDIR)/ipelets/$1.so
  exe_target	= $(BUILDDIR)/bin/$1
endif

# Macros

INSTALL_DIR = install -d
INSTALL_FILES = install -m 0644
INSTALL_SCRIPTS = install -m 0755
ifdef MACOS
# stripping is faulty on Mac?
INSTALL_PROGRAMS = install -m 0755
else
INSTALL_PROGRAMS = install -s -m 0755
endif

MAKE_BINDIR = mkdir -p $(BUILDDIR)/bin

MAKE_LIBDIR = mkdir -p $(buildlib)

MAKE_IPELETDIR = mkdir -p $(BUILDDIR)/ipelets

MAKE_DEPEND = \
	mkdir -p $(OBJDIR); \
	echo "" > $@; \
	for f in $(all_sources); do \
	$(CXX) -MM -MT $(OBJDIR)/$${f%%.cpp}.o $(CPPFLAGS) $$f >> $@; done

# The rules

$(OBJDIR)/%.o:  %.cpp
	@echo Compiling $(<F)...
	$(COMPILE.cc) -o $@ $<

ifdef IPEUI_QT
moc_%.cpp:  %.h
	@echo Running moc on $(<F)...
	$(MOC) $(CPPFLAGS )-o $@ $<
endif

# --------------------------------------------------------------------
