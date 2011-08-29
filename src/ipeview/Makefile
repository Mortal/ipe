# --------------------------------------------------------------------
# Makefile for Ipeview
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipeview
include ../common.mak

TARGET = $(call exe_target,ipeview)

CPPFLAGS += -I../include -I../ipecairo -I../ipecanvas \
	$(UI_CFLAGS) $(CAIRO_CFLAGS)
LIBS += -L$(buildlib) $(UI_LIBS) $(CAIRO_LIBS) -lipecanvas -lipecairo -lipe

all: $(TARGET)

win_sources = ipeview_win.cpp
qt_sources = ipeview_qt.cpp
moc_headers = ipeview_qt.h
gtk_sources = ipeview_gtk.cpp

ifdef WIN32
CXXFLAGS += -mwindows
LDFLAGS  += -mwindows

resource = $(OBJDIR)/res.o

$(resource): ipeview.rc
	$(WINDRES) -i ipeview.rc -o $@ --include-dir=../ipe

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
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPEBINDIR)

# --------------------------------------------------------------------
