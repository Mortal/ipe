# --------------------------------------------------------------------
# Makefile for Ipeqtcanvas
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipeqtcanvas
include ../common.mak

TARGET = $(call dll_target,ipeqtcanvas)

CPPFLAGS += -I../include -I../ipecairo $(CAIRO_CFLAGS) $(QT_CFLAGS)
LIBS += -L$(buildlib) -lipecairo -lipe $(CAIRO_LIBS) $(QT_LIBS)
CXXFLAGS += $(DLL_CFLAGS)

all: $(TARGET)

sources	= ipeqtcanvas.cpp ipetool.cpp
moc_headers = ipeqtcanvas.h

$(TARGET): $(objects)
	$(MAKE_LIBDIR)
	$(CXX) $(LDFLAGS) $(DLL_LDFLAGS) -o $@ $^ $(LIBS)

clean:
	@-rm -f $(objects) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

install: $(TARGET)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPELIBDIR)
	$(INSTALL_PROGRAMS) $(INSTALL_ROOT)$(IPELIBDIR) $(TARGET)

# --------------------------------------------------------------------
