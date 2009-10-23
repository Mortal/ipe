# --------------------------------------------------------------------
# Makefile for Ipelib
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipelib
include ../common.mak

TARGET = $(call dll_target,ipe)

CPPFLAGS += -I../include  
ifndef WIN32
CPPFLAGS += -DIPEFONTMAP=\"$(IPEFONTMAP)\"
endif
CPPFLAGS += $(ZLIB_CFLAGS)
CXXFLAGS += $(DLL_CFLAGS)
LIBS += $(ZLIB_LIBS)

all: $(TARGET)

sources	= \
	ipebase.cpp \
	ipeplatform.cpp \
	ipegeo.cpp \
	ipexml.cpp \
	ipeattributes.cpp \
	ipebitmap.cpp \
	ipedct.cpp \
	ipeshape.cpp \
	ipegroup.cpp \
	ipeimage.cpp \
	ipetext.cpp \
	ipepath.cpp \
	ipereference.cpp \
	ipeobject.cpp \
	ipefactory.cpp \
	ipestdstyles.cpp \
	ipeiml.cpp \
	ipepage.cpp \
	ipepainter.cpp \
	ipepdfparser.cpp \
	ipepdfwriter.cpp \
	ipepswriter.cpp \
	ipestyle.cpp \
	ipesnap.cpp \
	ipeutils.cpp \
	ipelatex.cpp \
	ipedoc.cpp

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
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEHEADERDIR)
	$(INSTALL_PROGRAMS) $(INSTALL_ROOT)$(IPELIBDIR) $(TARGET)
	$(INSTALL_FILES) $(INSTALL_ROOT)$(IPEHEADERDIR) ../include/*.h

# --------------------------------------------------------------------