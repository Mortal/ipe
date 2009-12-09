# --------------------------------------------------------------------
# Makefile for Ipeqtcanvas
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipeqtcanvas
include ../common.mak

TARGET = $(call dll_target,ipeqtcanvas)
MAKE_SYMLINKS = $(call dll_symlinks,ipeqtcanvas)
SONAME = $(call soname,ipeqtcanvas)
INSTALL_SYMLINKS = $(call install_symlinks,ipeqtcanvas)

CPPFLAGS += -I../include -I../ipecairo $(CAIRO_CFLAGS) $(QT_CFLAGS)
LIBS += -L$(buildlib) -lipecairo -lipe $(CAIRO_LIBS) $(QT_LIBS)
CXXFLAGS += $(DLL_CFLAGS)

all: $(TARGET)

sources	= ipeqtcanvas.cpp ipetool.cpp
moc_headers = ipeqtcanvas.h

$(TARGET): $(objects)
	$(MAKE_LIBDIR)
	$(CXX) $(LDFLAGS) $(DLL_LDFLAGS) $(SONAME) -o $@ $^ $(LIBS)
	$(MAKE_SYMLINKS)

clean:
	@-rm -f $(objects) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

install: $(TARGET)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPELIBDIR)
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPELIBDIR)
	$(INSTALL_SYMLINKS)

# --------------------------------------------------------------------
