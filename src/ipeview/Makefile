# --------------------------------------------------------------------
# Makefile for Ipeview
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipeview
include ../common.mak

TARGET = $(call exe_target,ipeview)

CPPFLAGS += -I../include -I../ipeqtcanvas $(QT_CFLAGS)
LIBS += -L$(buildlib) -lipeqtcanvas -lipecairo -lipe $(QT_LIBS)

all: $(TARGET)

sources	= ipeview.cpp
moc_headers = ipeview.h

ifdef WIN32
CXXFLAGS += -mwindows
LDFLAGS  += -mwindows

resource = $(OBJDIR)/res.o

$(resource): ipeview.rc
	windres -i ipeview.rc -o $@ --include-dir=../ipe

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
	$(INSTALL_PROGRAMS) $(INSTALL_ROOT)$(IPEBINDIR) $(TARGET)

# --------------------------------------------------------------------
