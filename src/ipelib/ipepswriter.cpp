// --------------------------------------------------------------------
// Creating Postscript output
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2010  Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "ipeimage.h"
#include "ipetext.h"
#include "ipepainter.h"
#include "ipegroup.h"
#include "ipereference.h"
#include "ipeutils.h"
#include "ipedoc.h"

#include "ipepswriter.h"
#include "ipefontpool.h"

using namespace ipe;

static const char hexChar[17] = "0123456789abcdef";

// --------------------------------------------------------------------

PsPainter::PsPainter(const Cascade *style, Stream &stream)
  : PdfPainter(style, stream)
{
  // Postscript has only one current color: we use iStroke for that
  iImageNumber = 1;
}

PsPainter::~PsPainter()
{
  //
}

void PsPainter::doNewPath()
{
  // nothing (override PDF implementation)
}

void PsPainter::strokePath()
{
  State &s = iState.back();
  State &sa = iActiveState.back();
  if (s.iDashStyle != sa.iDashStyle) {
    sa.iDashStyle = s.iDashStyle;
    if (s.iDashStyle.empty())
      iStream << "[] 0 d ";
    else
      iStream << s.iDashStyle << " d ";
  }
  if (s.iPen != sa.iPen) {
    sa.iPen = s.iPen;
    iStream << s.iPen << " w ";
  }
  if (s.iLineCap != sa.iLineCap) {
    sa.iLineCap = s.iLineCap;
    iStream << s.iLineCap << " J\n";
  }
  if (s.iLineJoin != sa.iLineJoin) {
    sa.iLineJoin = s.iLineJoin;
    iStream << s.iLineJoin << " j\n";
  }
  if (s.iStroke != sa.iStroke) {
    sa.iStroke = s.iStroke;
    drawColor(iStream, s.iStroke, "g", "rg");
  }
  iStream << "S";
}

void PsPainter::fillPath(bool eoFill, bool preservePath)
{
  if (tiling().isNormal()) {
    State &s = iState.back();
    State &sa = iActiveState.back();
    if (s.iFill != sa.iStroke) {
      sa.iStroke = s.iFill;
      drawColor(iStream, s.iFill, "g", "rg");
    }
  } else {
    State &s = iState.back();
    if (s.iFill.isGray())
      iStream << s.iFill.iRed << " Pat" << s.iTiling.index() << " patg\n";
    else
      iStream << s.iFill << " Pat" << s.iTiling.index() << " patrg\n";
    // set an impossible color
    iActiveState.back().iStroke.iRed = Fixed(2);
  }
  if (preservePath)
    iStream << "q ";
  iStream << (eoFill ? "f*" : "f");
  if (preservePath)
    iStream << " Q ";
}

void PsPainter::doDrawPath(TPathMode mode)
{
  bool eofill = (fillRule() == EEvenOddRule);
  // if (!mode)
  // iStream << "np";  // flush path
  if (mode == EFilledOnly)
    fillPath(eofill, false);
  else if (mode == EStrokedOnly)
    strokePath();
  else {
    fillPath(eofill, true);
    strokePath();
  }
  iStream << "\n";
}

void PsPainter::doAddClipPath()
{
  iStream << "eoclip np\n";
}

void PsPainter::doDrawBitmap(Bitmap bitmap)
{
  switch (bitmap.colorSpace()) {
  case Bitmap::EDeviceGray:
    iStream << "/DeviceGray setcolorspace ";
    break;
  case Bitmap::EDeviceRGB:
    iStream << "/DeviceRGB setcolorspace ";
    break;
  case Bitmap::EDeviceCMYK:
    iStream << "/DeviceCMYK setcolorspace ";
    break;
  }
  iStream << matrix() << " cm\n";
  iStream << "<< /ImageType 1\n";
  iStream << "   /Width " << bitmap.width() << "\n";
  iStream << "   /Height " << bitmap.height() << "\n";
  iStream << "   /BitsPerComponent " << bitmap.bitsPerComponent() << "\n";
  iStream << "   /Decode [ ";
  for (int i = 0; i < bitmap.components(); ++i)
    iStream << "0 1 ";
  iStream << "]\n";
  iStream << "   /ImageMatrix [ " << bitmap.width() << " 0 0 "
	  << -bitmap.height() << " 0 " << bitmap.height() << " ]\n";
  iStream << "   /DataSource currentfile /ASCII85Decode filter";
  if (bitmap.filter() == Bitmap::EFlateDecode)
    iStream << " /FlateDecode filter\n";
  else if (bitmap.filter() == Bitmap::EDCTDecode)
    iStream << " /DCTDecode filter\n";
  else
    iStream << "\n";
  iStream << ">>\n";
  iStream << "%%BeginIpeImage: " << iImageNumber
	  << " " << bitmap.size() << "\n";
  bitmap.setObjNum(iImageNumber++);
  iStream << "image\n";
  const char *p = bitmap.data();
  const char *p1 = p + bitmap.size();
  A85Stream a85(iStream);
  while (p < p1)
    a85.putChar(*p++);
  a85.close();
  iStream << "%%EndIpeImage\n";
}

// --------------------------------------------------------------------

/*! \class ipe::PsWriter
  \brief Create Postscript file.

  This class is responsible for the creation of a Postscript file from the
   data. You have to create an PsWriter first, providing a stream
  that has been opened for (binary) writing and is empty.

*/

//! Create Postscript writer operating on this (open and empty) file.
PsWriter::PsWriter(TellStream &stream, const Document *doc, bool noColor)
  : iStream(stream), iDoc(doc), iNoColor(noColor)
{
  // nothing
}

// --------------------------------------------------------------------

//! Create the document header and prolog (the resources).
/*! Embeds no fonts if \c pool is 0.
  Returns false if a TrueType font is present. */
bool PsWriter::createHeader(int pno, int view)
{
  const FontPool *pool = iDoc->fontPool();
  const Document::SProperties &props = iDoc->properties();

  // Check needed and supplied fonts, and reject Truetype fonts
  String needed = "";
  String neededSep = "%%DocumentNeededResources: font ";
  String supplied = "";
  String suppliedSep = "%%DocumentSuppliedResources: font ";
  if (pool) {
    for (FontPool::const_iterator font = pool->begin();
	 font != pool->end(); ++font) {
      if (font->iType == Font::ETrueType)
	return false;
      if (font->iStandardFont) {
	needed += neededSep;
	needed += font->iName;
	needed += "\n";
	neededSep = "%%+ font ";
      } else {
	supplied += suppliedSep;
	supplied += font->iName;
	supplied += "\n";
	suppliedSep = "%%+ font ";
      }
    }
  }

  iStream << "%!PS-Adobe-3.0 EPSF-3.0\n";

  iStream << "%%Creator: Ipelib " << IPELIB_VERSION;
  if (!props.iCreator.empty())
    iStream << " (" << props.iCreator << ")";
  iStream << "\n";
  // CreationDate is informational, no fixed format
  iStream << "%%CreationDate: " << props.iModified << "\n";
  // Should use level 1 if no images or patterns present?
  iStream << "%%LanguageLevel: 2\n";

  const Page *page = iDoc->page(pno);

  int viewBBoxLayer = page->findLayer("VIEWBBOX");
  Rect r;
  if (viewBBoxLayer >= 0 && page->visible(view, viewBBoxLayer))
    r = page->viewBBox(iDoc->cascade(), view);
  else
    r = page->pageBBox(iDoc->cascade());

  iStream << "%%BoundingBox: " // (x0, y0) (x1, y1)
	  << int(r.bottomLeft().x) << " " << int(r.bottomLeft().y) << " "
	  << int(r.topRight().x + 1) << " " << int(r.topRight().y + 1)
	  << "\n";
  iStream << "%%HiResBoundingBox: "
	  << r.bottomLeft() << " " << r.topRight() << "\n";

  iStream << needed;
  iStream << supplied;
  if (props.iAuthor.size())
    iStream << "%%Author: " << props.iAuthor << "\n";
  if (props.iTitle.size())
    iStream << "%%Title: " << props.iTitle << "\n";
  iStream << "%%EndComments\n";
  iStream << "%%BeginProlog\n";
  // procset <name> <version (a real)> <revision (an integer)>
  // <revision> is upwards compatible, <version> not
  iStream << "%%BeginResource: procset ipe 7.0 " << IPELIB_VERSION << "\n";
  iStream << "/ipe 40 dict def ipe begin\n";
  iStream << "/np { newpath } def\n";
  iStream << "/m { moveto } def\n";
  iStream << "/l { lineto } def\n";
  iStream << "/c { curveto } def\n";
  iStream << "/h { closepath } def\n";
  iStream << "/re { 4 2 roll moveto 1 index 0 rlineto 0 exch rlineto\n";
  iStream << "      neg 0 rlineto closepath } def\n";
  iStream << "/d { setdash } def\n";
  iStream << "/w { setlinewidth } def\n";
  iStream << "/J { setlinecap } def\n";
  iStream << "/j { setlinejoin } def\n";
  iStream << "/cm { [ 7 1 roll ] concat } def\n";
  iStream << "/q { gsave } def\n";
  iStream << "/Q { grestore } def\n";
  iStream << "/g { setgray } def\n";
  iStream << "/G { setgray } def\n";  // used inside text objects
  if (!iNoColor) {
    iStream << "/rg { setrgbcolor } def\n";
    iStream << "/RG { setrgbcolor } def\n"; // used inside text objects
  }
  iStream << "/S { stroke } def\n";
  iStream << "/f* { eofill } def\n";
  iStream << "/f { fill } def\n";
  iStream << "/ipeMakeFont {\n";
  iStream << "  exch findfont\n";
  iStream << "  dup length dict begin\n";
  iStream << "    { 1 index /FID ne { def } { pop pop } ifelse } forall\n";
  iStream << "    /Encoding exch def\n";
  iStream << "    currentdict\n";
  iStream << "  end\n";
  iStream << "  definefont pop\n";
  iStream << "} def\n";
  iStream << "/ipeFontSize 0 def\n";
  iStream << "/Tf { dup /ipeFontSize exch store selectfont } def\n";
  iStream << "/Td { translate } def\n";
  iStream << "/BT { gsave } def\n";
  iStream << "/ET { grestore } def\n";
  iStream << "/TJ { 0 0 moveto { dup type /stringtype eq\n";
  iStream << " { show } { ipeFontSize mul -0.001 mul 0 rmoveto } ifelse\n";
  iStream << "} forall } def\n";

  // embed tiling patterns
  AttributeSeq ts;
  iDoc->cascade()->allNames(ETiling, ts);
  for (uint i = 0; i < ts.size(); ++i) {
    const Tiling *t = iDoc->cascade()->findTiling(ts[i]);
    Linear m(t->iAngle);
    iStream << "<<\n"
	    << "/PatternType 1\n"  // tiling pattern
	    << "/PaintType 2\n"    // uncolored pattern
	    << "/TilingType 2\n"   // faster
	    << "/BBox [ 0 0 100 " << t->iStep << " ]\n"
	    << "/XStep 99\n"
	    << "/YStep " << t->iStep << "\n"
	    << "/PaintProc { pop 0 0 100 " << t->iWidth << " re fill} bind\n"
	    << ">>\n"
	    << "[ " << m << " 0 0 ]\n"
	    << "makepattern\n"
	    << "/Pat" << ts[i].index() << " exch def\n";
  }
  if (ts.size() > 0) {
    iStream << "/patg { [/Pattern /DeviceGray ] setcolorspace "
	    << "setcolor } def\n";
    if (!iNoColor)
      iStream << "/patrg { [/Pattern /DeviceRGB ] setcolorspace "
	      << "setcolor } def\n";
  }

  iStream << "end\n";
  iStream << "%%EndResource\n";

  iStream << "%%EndProlog\n";
  iStream << "%%BeginSetup\n";
  iStream << "ipe begin\n";

  if (pool) {
    for (FontPool::const_iterator font = pool->begin();
	 font != pool->end(); ++font) {

      if (font->iStandardFont) {
	iStream << "%%IncludeResource: font " << font->iName << "\n";
      } else {
	iStream << "%%BeginResource: font " << font->iName << "\n";
	embedFont(*font);
	iStream << "%%EndResource\n";
      }

      // Create font dictionary
      iStream << "/F" << font->iLatexNumber
	      << " /" << font->iName;
      if (font->iHasEncoding) {
	String sep = "\n[ ";
	for (int i = 0; i < 0x100; ++i) {
	  if (i % 8 == 0) {
	    iStream << sep;
	    sep = "\n  ";
	  }
	  iStream << "/" << font->iEncoding[i];
	}
	iStream << " ]\nipeMakeFont\n";
      } else {
	iStream << " findfont definefont pop\n";
      }
    }
  }

  iStream << "%%EndSetup\n";
  return true;
}

//! Destructor.
PsWriter::~PsWriter()
{
  // nothing
}

//! Write all fonts to the Postscript file.
void PsWriter::embedFont(const Font &font)
{
  // write unencrypted front matter
  iStream.putRaw(font.iStreamData.data(), font.iLength1);
  // write encrypted section
  const char *p = font.iStreamData.data() + font.iLength1;
  const char *p1 = font.iStreamData.data() + font.iLength1 + font.iLength2;
  int i = 0;
  while (p < p1) {
    iStream.putChar(hexChar[(*p >> 4) & 0x0f]);
    iStream.putChar(hexChar[*p & 0x0f]);
    ++p;
    if (++i % 32 == 0)
      iStream << "\n";
  }
  if (i % 32 > 0)
    iStream << "\n";
  // write tail matter
  if (font.iLength3 > 0)
    iStream.putRaw(p1, font.iLength3);
  else {
    for (i = 0; i < 512; ++i) {
      if ((i & 63) == 0)
	iStream.putChar('\n');
      iStream.putChar('0');
    }
    iStream << "\ncleartomark\n";
  }
}

// --------------------------------------------------------------------

//! Create contents and page stream for this view.
/*! Background is not shown. */
void PsWriter::createPageView(int pno, int vno)
{
  const Page *page = iDoc->page(pno);
  PsPainter painter(iDoc->cascade(), iStream);
  for (int i = 0; i < page->count(); ++i) {
    if (page->objectVisible(vno, i))
      page->object(i)->draw(painter);
  }
  iStream << "showpage\n";
}

//! Create the trailer of the Postscript file.
void PsWriter::createTrailer()
{
  iStream << "%%Trailer\n";
  iStream << "end\n"; // to end ipe dictionary
  iStream << "%%EOF\n";
}

// --------------------------------------------------------------------

class PercentStream : public Stream {
public:
  PercentStream(Stream &stream)
    : iStream(stream), iNeedPercent(true) { /* nothing */ }
  virtual void putChar(char ch);

private:
  Stream &iStream;
  bool iNeedPercent;
};

void PercentStream::putChar(char ch)
{
  if (iNeedPercent)
    iStream.putChar('%');
  iStream.putChar(ch);
  iNeedPercent = (ch == '\n');
}

//! Save  information in XML format.
void PsWriter::createXml(int compressLevel)
{
  PercentStream s1(iStream);
  if (compressLevel > 0) {
    iStream << "%%BeginIpeXml: /FlateDecode\n";
    A85Stream a85(s1);
    DeflateStream s2(a85, compressLevel);
    iDoc->saveAsXml(s2, true);
    s2.close();
  } else {
    iStream << "%%BeginIpeXml:\n";
    iDoc->saveAsXml(s1, true);
    s1.close();
  }
  iStream << "%%EndIpeXml\n";
}

// --------------------------------------------------------------------

