# --------------------------------------------------------------------
# Makefile for qvoronoi ipelet
# Needs qhull library (www.qhull.org)
# On Ubuntu:  apt-get install libqhull-dev
# --------------------------------------------------------------------

IPESRCDIR = ../..
OBJDIR = $(BUILDDIR)/obj/ipelets
DEPEND = $(OBJDIR)/qvoronoi.depend
include ../../common.mak

ifdef WIN32
QHULL_CFLAGS = -I../../../../qhull2002.1/src
QHULL_LIBS = -L../../../../qhull2002.1 -lqhull
else
QHULL_CFLAGS = -I/usr/include/qhull
QHULL_LIBS = -lqhull
endif

TARGET = $(call ipelet_target,qvoronoi)
LUASCRIPT = qvoronoi.lua

CPPFLAGS += -I../../include $(QHULL_CFLAGS)
LIBS += -L$(buildlib) $(QHULL_LIBS) -lipe 
CXXFLAGS += $(DLL_CFLAGS)

all: $(TARGET)

sources	= qvoronoi.cpp

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
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPELETDIR)
	$(INSTALL_FILES) $(LUASCRIPT) $(INSTALL_ROOT)$(IPELETDIR)

# --------------------------------------------------------------------
