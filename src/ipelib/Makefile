# --------------------------------------------------------------------
# Makefile for Ipelib
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipelib
include ../common.mak

TARGET = $(call dll_target,ipe)
MAKE_SYMLINKS = $(call dll_symlinks,ipe)
SONAME = $(call soname,ipe)
INSTALL_SYMLINKS = $(call install_symlinks,ipe)

CPPFLAGS += -I../include  
ifndef WIN32
CPPFLAGS += -DIPEFONTMAP=\"$(IPEFONTMAP)\"
endif
CPPFLAGS += $(IPE_USE_ICONV) $(ZLIB_CFLAGS) $(ICONV_CFLAGS) $(JPEG_CFLAGS)
CXXFLAGS += $(DLL_CFLAGS)
LIBS += $(JPEG_LIBS) $(ZLIB_LIBS) $(ICONV_LIBS)

all: $(TARGET)

sources	= \
	ipebase.cpp \
	ipeplatform.cpp \
	ipegeo.cpp \
	ipexml.cpp \
	ipeattributes.cpp \
	ipebitmap.cpp \
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
	$(CXX) $(LDFLAGS) $(DLL_LDFLAGS) $(SONAME) -o $@ $^ $(LIBS)
	$(MAKE_SYMLINKS)

clean:
	@-rm -f $(objects) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

install: $(TARGET)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPELIBDIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEHEADERDIR)
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPELIBDIR)
	$(INSTALL_FILES) ../include/*.h $(INSTALL_ROOT)$(IPEHEADERDIR)
	$(INSTALL_SYMLINKS)

# --------------------------------------------------------------------
