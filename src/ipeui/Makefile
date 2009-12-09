# --------------------------------------------------------------------
# Makefile for ipeui
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipeui
include ../common.mak

TARGET = $(call dll_target,ipeui)
MAKE_SYMLINKS = $(call dll_symlinks,ipeui)
SONAME = $(call soname,ipeui)
INSTALL_SYMLINKS = $(call install_symlinks,ipeui)

CPPFLAGS += $(QT_CFLAGS) $(LUA_CFLAGS) 
LIBS += $(QT_LIBS) $(LUA_LIBS)
CXXFLAGS += $(DLL_CFLAGS)

all: $(TARGET)

sources	= ipeui.cpp
moc_headers = ipeui.h

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
