# --------------------------------------------------------------------
# Makefile for image ipelet
# --------------------------------------------------------------------

IPESRCDIR = ../..
OBJDIR = $(BUILDDIR)/obj/ipelets
DEPEND = $(OBJDIR)/image.depend
include ../../common.mak

TARGET = $(call ipelet_target,image)
LUASCRIPT = image.lua

CPPFLAGS += -I../../include $(QT_CFLAGS)
LIBS += -L$(buildlib) -lipe $(QT_LIBS)
CXXFLAGS += $(DLL_CFLAGS)

all: $(TARGET)

sources	= image.cpp

$(TARGET): $(objects)
	$(MAKE_IPELETDIR)
	$(CXX) $(LDFLAGS) $(DLL_LDFLAGS) -o $@ $^ $(LIBS)

clean:
	@-rm -f $(objects) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

install: $(TARGET)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPELETDIR)
	$(INSTALL_PROGRAMS) $(INSTALL_ROOT)$(IPELETDIR) $(TARGET)
	$(INSTALL_FILES) $(INSTALL_ROOT)$(IPELETDIR) $(LUASCRIPT)

# --------------------------------------------------------------------
