# --------------------------------------------------------------------
# Makefile for Ipe
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipe
include ../common.mak

TARGET = $(call exe_target,ipe)

CPPFLAGS += -I../include -I../ipecanvas -I../ipecairo -I../ipelua \
	$(LUA_CFLAGS) $(UI_CFLAGS) $(CAIRO_CFLAGS) $(ZLIB_CFLAGS)
LIBS += -L$(buildlib) -lipecanvas -lipecairo -lipelua -lipe -lipeui \
	$(LUA_LIBS) $(UI_LIBS) $(CAIRO_LIBS) $(ZLIB_LIBS)

CPPFLAGS += -DIPELETPATH=\"$(IPELETPATH)\"
CPPFLAGS += -DIPELUAPATH=\"$(IPELUAPATH)\"
CPPFLAGS += -DIPEICONDIR=\"$(IPEICONDIR)\"
CPPFLAGS += -DIPEDOCDIR=\"$(IPEDOCDIR)\"
CPPFLAGS += -DIPESTYLEDIR=\"$(IPESTYLEDIR)\"

all: $(TARGET)

sources	= main.cpp tools.cpp lfs.cpp appui.cpp uilua.cpp 
qt_sources = appui_qt.cpp controls_qt.cpp 
moc_headers = appui_qt.h controls_qt.h
win_sources = appui_win.cpp pathview_win.cpp
gtk_sources = appui_gtk.cpp

ifdef WIN32
CXXFLAGS += -mwindows
LDFLAGS  += -mwindows

resource = $(OBJDIR)/res.o

$(resource): ipe.rc
	$(WINDRES) -i ipe.rc -o $@ --include-dir=.

else
resource = 
endif

$(TARGET): $(objects) $(resource)
	$(MAKE_BINDIR)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	@-rm -f $(objects) $(resource) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

install: $(TARGET)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEBINDIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEMANDIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEDOCDIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPELUADIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPESTYLEDIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPESCRIPTDIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEICONDIR)
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPEBINDIR)
	$(INSTALL_SCRIPTS) ../../scripts/* $(INSTALL_ROOT)$(IPESCRIPTDIR)
	$(INSTALL_FILES) ../../man/*.1 $(INSTALL_ROOT)$(IPEMANDIR) 
	$(INSTALL_FILES) ../../build/doc/* $(INSTALL_ROOT)$(IPEDOCDIR)
	$(INSTALL_FILES) lua/*.lua $(INSTALL_ROOT)$(IPELUADIR)
	$(INSTALL_FILES) ../../styles/basic.isy $(INSTALL_ROOT)$(IPESTYLEDIR)
	$(INSTALL_FILES) ../../styles/colors.isy $(INSTALL_ROOT)$(IPESTYLEDIR)
	$(INSTALL_FILES) ../../styles/beamer.isy $(INSTALL_ROOT)$(IPESTYLEDIR)
	$(INSTALL_FILES) ../../styles/presentation.isy \
			 $(INSTALL_ROOT)$(IPESTYLEDIR)
	$(INSTALL_FILES) icons/*.png $(INSTALL_ROOT)$(IPEICONDIR)

# --------------------------------------------------------------------
