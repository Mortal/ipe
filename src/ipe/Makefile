# --------------------------------------------------------------------
# Makefile for Ipe
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipe
include ../common.mak

TARGET = $(call exe_target,ipe)

CPPFLAGS += -I../include -I../ipeqtcanvas -I../ipecairo -I../ipelua \
	$(LUA_CFLAGS) $(QT_CFLAGS) $(CAIRO_CFLAGS) $(ZLIB_CFLAGS)
LIBS += -L$(buildlib) -lipeqtcanvas -lipecairo -lipelua -lipe -lipeui \
	$(LUA_LIBS) $(QT_LIBS) $(CAIRO_LIBS)

CPPFLAGS += -DIPELETPATH=\"$(IPELETPATH)\"
CPPFLAGS += -DIPELUAPATH=\"$(IPELUAPATH)\"
CPPFLAGS += -DIPEICONDIR=\"$(IPEICONDIR)\"
CPPFLAGS += -DIPEDOCDIR=\"$(IPEDOCDIR)\"
CPPFLAGS += -DIPESTYLES=\"$(IPESTYLES)\"

all: $(TARGET)

sources	= main.cpp appui.cpp tools.cpp lfs.cpp uilua.cpp
moc_headers = appui.h

ifdef WIN32
CXXFLAGS += -mwindows
LDFLAGS  += -mwindows

resource = $(OBJDIR)/res.o

$(resource): ipe.rc
	windres -i ipe.rc -o $@ --include-dir=.

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
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEMANDIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEDOCDIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPELUADIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPESTYLES) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEICONDIR)
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPEBINDIR)
	$(INSTALL_SCRIPTS) ../../scripts/ipe-update-master \
			    $(INSTALL_ROOT)$(IPEBINDIR)
	$(INSTALL_FILES) ../../man/*.1 $(INSTALL_ROOT)$(IPEMANDIR) 
	$(INSTALL_FILES) ../../build/doc/* $(INSTALL_ROOT)$(IPEDOCDIR)
	$(INSTALL_FILES) lua/*.lua $(INSTALL_ROOT)$(IPELUADIR)
	$(INSTALL_FILES) ../../styles/basic.isy $(INSTALL_ROOT)$(IPESTYLES)
	$(INSTALL_FILES) ../../styles/colors.isy $(INSTALL_ROOT)$(IPESTYLES)
	$(INSTALL_FILES) ../../styles/presentation.isy \
			 $(INSTALL_ROOT)$(IPESTYLES)
	$(INSTALL_FILES) icons/*.png $(INSTALL_ROOT)$(IPEICONDIR)

# --------------------------------------------------------------------
