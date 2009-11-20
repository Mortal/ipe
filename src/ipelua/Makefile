# --------------------------------------------------------------------
# Makefile for Lua bindings for ipe
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipelua
include ../common.mak

TARGET = $(call dll_target,ipelua)

CPPFLAGS += -I../include $(LUA_CFLAGS) $(QT_CFLAGS)
LIBS += -L$(buildlib) -lipe $(LUA_LIBS) $(QT_LIBS)
CXXFLAGS += $(DLL_CFLAGS)

all: $(TARGET)

sources	= \
	ipelib.cpp \
	ipeluageo.cpp \
	ipeluaobj.cpp \
	ipeluastyle.cpp \
	ipeluapage.cpp \
	ipeluaipelet.cpp

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
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPELIBDIR)

# --------------------------------------------------------------------
