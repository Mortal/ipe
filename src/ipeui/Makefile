# --------------------------------------------------------------------
# Makefile for ipeui
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipeui
include ../common.mak

TARGET = $(call dll_target,ipeui)
MAKE_SYMLINKS = $(call dll_symlinks,ipeui)
SONAME = $(call soname,ipeui)
INSTALL_SYMLINKS = $(call install_symlinks,ipeui)

CPPFLAGS += $(UI_CFLAGS) $(LUA_CFLAGS) 
LIBS += $(UI_LIBS) $(LUA_LIBS)
CXXFLAGS += $(DLL_CFLAGS)

all: $(TARGET)

sources = ipeui_common.cpp 
qt_sources = ipeui_qt.cpp
moc_headers = ipeui_qt.h
win_sources = ipeui_win.cpp
gtk_sources = ipeui_gtk.cpp

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
