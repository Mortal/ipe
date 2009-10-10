# --------------------------------------------------------------------
# Makefile for Ipecairo
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipecairo
include ../common.mak

TARGET = $(call dll_target,ipecairo)

CPPFLAGS += -I../include $(CAIRO_CFLAGS) $(FREETYPE_CFLAGS)
LIBS += -L$(buildlib) -lipe $(CAIRO_LIBS) $(FREETYPE_LIBS)
CXXFLAGS += $(DLL_CFLAGS)

all: $(TARGET)

sources	= \
	ipecairopainter.cpp \
	ipefonts.cpp	\
	ipestdfonts.cpp	   

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
