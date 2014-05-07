// --------------------------------------------------------------------
// Interface with Pdflatex
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2014  Otfried Cheong

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

#include "ipestyle.h"
#include "ipegroup.h"
#include "ipereference.h"

#include "ipefontpool.h"
#include "ipelatex.h"

using namespace ipe;

/*! \class ipe::Latex
  \brief Object that converts latex source to PDF format.

  This object is responsible for creating the PDF representation of
  text objects.

*/

//! Create a converter object.
Latex::Latex(const Cascade *sheet)
{
  iCascade = sheet;
  iFontPool = 0;
}

//! Destructor.
Latex::~Latex()
{
  for (XFormList::iterator it = iXForms.begin(); it != iXForms.end(); ++it)
    delete *it;
  delete iFontPool;
}

// --------------------------------------------------------------------

//! Return the newly created font pool and pass ownership of pool to caller.
FontPool *Latex::takeFontPool()
{
  FontPool *pool = iFontPool;
  iFontPool = 0;
  return pool;
}

// --------------------------------------------------------------------

class ipe::TextCollectingVisitor : public Visitor {
public:
  TextCollectingVisitor(Latex::TextList *list);
  virtual void visitText(const Text *obj);
  virtual void visitGroup(const Group *obj);
  virtual void visitReference(const Reference *obj);
public:
  bool iTextFound;
private:
  Latex::TextList *iList;
};

TextCollectingVisitor::TextCollectingVisitor(Latex::TextList *list)
  : iList(list)
{
  // nothing
}

void TextCollectingVisitor::visitText(const Text *obj)
{
  Latex::SText s;
  s.iText = obj;
  s.iSize = obj->size();
  iList->push_back(s);
  iTextFound = true;
}

void TextCollectingVisitor::visitGroup(const Group *obj)
{
  for (Group::const_iterator it = obj->begin(); it != obj->end(); ++it)
    (*it)->accept(*this);
}

void TextCollectingVisitor::visitReference(const Reference *)
{
  // need to figure out what to do for symbols
}

// --------------------------------------------------------------------

/*! Scan an object and insert all text objects into Latex's list.
  Returns total number of text objects found so far. */
int Latex::scanObject(const Object *obj)
{
  TextCollectingVisitor visitor(&iTextObjects);
  obj->accept(visitor);
  return iTextObjects.size();
}

/*! Scan a page and insert all text objects into Latex's list.
  Returns total number of text objects found so far. */
int Latex::scanPage(Page *page)
{
  page->applyTitleStyle(iCascade);
  TextCollectingVisitor visitor(&iTextObjects);
  const Text *title = page->titleText();
  if (title)
    title->accept(visitor);
  for (int i = 0; i < page->count(); ++i) {
    visitor.iTextFound = false;
    page->object(i)->accept(visitor);
    if (visitor.iTextFound)
      page->invalidateBBox(i);
  }
  return iTextObjects.size();
}

/*! Create a Latex source file with all the text objects collected
  before.  The client should have prepared a directory for the
  Pdflatex run, and pass the name of the Latex source file to be
  written by Latex.

  Returns the number of text objects that did not yet have an XForm,
  or a negative error code.
*/
int Latex::createLatexSource(Stream &stream, String preamble)
{
  bool ancient = (getenv("IPEANCIENTPDFTEX") != 0);
  int count = 0;
  stream << "\\pdfcompresslevel0\n"
	 << "\\nonstopmode\n"
	 << "\\expandafter\\ifx\\csname pdfobjcompresslevel\\endcsname"
	 << "\\relax\\else\\pdfobjcompresslevel0\\fi\n";
  if (!ancient) {
    stream << "\\ifnum\\the\\pdftexversion<140"
	   << "\\errmessage{Pdftex is too old. "
	   << "Set IPEANCIENTPDFTEX environment variable!}\\fi\n";
  }
  stream << "\\documentclass{article}\n"
    // << "\\newcommand{\\Ipechar}[1]{\\unichar{#1}}\n"
	 << "\\newcommand{\\PageTitle}[1]{#1}\n"
	 << "\\newdimen\\ipefs\n"
	 << "\\newcommand{\\ipesymbol}[4]{\\ipefs 1ex\\pdfliteral"
	 << "{(#1) (\\the\\ipefs) (#2) (#3) (#4) sym}}\n"
	 << "\\usepackage{color}\n";
  AttributeSeq colors;
  iCascade->allNames(EColor, colors);
  for (AttributeSeq::const_iterator it = colors.begin();
       it != colors.end(); ++it) {
    // only symbolic names (not black, white, void)
    String name = it->string();
    Color value = iCascade->find(EColor, *it).color();
    if (value.isGray())
      stream << "\\definecolor{" << name << "}{gray}{"
	     << value.iRed << "}\n";
    else
      stream << "\\definecolor{" << name << "}{rgb}{"
	     << value.iRed << "," << value.iGreen << ","
	     << value.iBlue << "}\n";
  }
  if (!ancient) {
    stream << "\\def\\ipesetcolor{\\pdfcolorstack0 push{0 0 0 0 k 0 0 0 0 K}}\n"
	   << "\\def\\iperesetcolor{\\pdfcolorstack0 pop}\n";
  } else {
    stream << "\\def\\ipesetcolor{\\color[cmyk]{0,0,0,0}}\n"
	   << "\\def\\iperesetcolor{}\n";
  }
  stream << iCascade->findPreamble() << "\n"
	 << preamble << "\n"
	 << "\\pagestyle{empty}\n"
	 << "\\newcount\\bigpoint\\dimen0=0.01bp\\bigpoint=\\dimen0\n"
	 << "\\begin{document}\n"
	 << "\\begin{picture}(500,500)\n";
  for (TextList::iterator it = iTextObjects.begin();
       it != iTextObjects.end(); ++it) {
    const Text *text = it->iText;

    if (!text->getXForm())
      count++;

    Attribute fsAttr = iCascade->find(ETextSize, it->iSize);

    // compute x-stretch factor from textstretch
    Fixed stretch(1);
    if (it->iSize.isSymbolic())
      stretch = iCascade->find(ETextStretch, it->iSize).number();

#if 0
    Attribute abs = iCascade->Find(text->Stroke());
    if (abs.IsNull())
      abs = Attribute::Black();
    Color col = iCascade->Repository()->ToColor(abs);
#endif

    stream << "\\setbox0=\\hbox{";
    char ipeid[20];  // on 64-bit systems, pointers are 64 bit
    std::sprintf(ipeid, "/%08lx", (unsigned long int)(text));
    if (text->isMinipage()) {
      stream << "\\begin{minipage}{" <<
	text->width()/stretch.toDouble() << "bp}";
    }

    if (fsAttr.isNumber()) {
      Fixed fs = fsAttr.number();
      stream << "\\fontsize{" << fs << "}"
	     << "{" << fs.mult(6, 5) << "bp}\\selectfont\n";
    } else
      stream << fsAttr.string() << "\n";
#if 1
    stream << "\\ipesetcolor\n";
#else
    stream << "\\color[cmyk]{0,0,0,0}%\n";
#endif

    Attribute absStyle = iCascade->find(ETextStyle, text->style());
    String style = absStyle.string();
    int sp = 0;
    while (sp < style.size() && style[sp] != '\0')
      ++sp;
    if (text->isMinipage())
      stream << style.substr(0, sp);

    String txt = text->text();
#if 0
    for (int i = 0; i < txt.size(); ) {
      int uc = txt.unicode(i); // advances i
      if (uc < 0x80)
	stream << char(uc);
      else
	stream << "\\Ipechar{" << uc << "}";
    }
#endif
    stream << txt;

    if (text->isMinipage()) {
      if (txt[txt.size() - 1] != '\n')
	stream << "\n";
      stream << style.substr(sp + 1);
      stream << "\\end{minipage}";
    } else
      stream << "%\n";

    stream << "\\iperesetcolor}\n"
	   << "\\count0=\\dp0\\divide\\count0 by \\bigpoint\n"
	   << "\\pdfxform attr{/IpeId " << ipeid
	   << " /IpeStretch " << stretch.toDouble()
      	   << " /IpeDepth \\the\\count0}"
	   << "0\\put(0,0){\\pdfrefxform\\pdflastxform}\n";
  }
  stream << "\\end{picture}\n\\end{document}\n";
  return count;
}

bool Latex::getXForm(const PdfObj *xform)
{
  const PdfDict *dict = xform->dict();
  if (!dict || dict->stream().size() == 0)
    return false;
  Text::XForm *xf = new Text::XForm;
  iXForms.push_back(xf);
  assert(!dict->deflated());
  xf->iStream = dict->stream();
  /* Should we check /Matrix and /FormType?
     /Type /XObject
     /Subtype /Form
     /Id /abcd1234
     /Depth 246
     /Stretch [ 3 3 ]
     /BBox [0 0 4.639 4.289]
     /FormType 1
     /Matrix [1 0 0 1 0 0]
     /Resources 11 0 R
  */
  // Get  id
  const PdfObj *id = dict->get("IpeId", &iPdf);
  if (!id || !id->name())
    return false;
  Lex lex(id->name()->value());
  xf->iRefCount = lex.getHexNumber(); // abusing refcount field
  const PdfObj *depth = dict->get("IpeDepth", &iPdf);
  if (!depth || !depth->number())
    return false;
  xf->iDepth = int(depth->number()->value());
  const PdfObj *stretch = dict->get("IpeStretch", &iPdf);
  if (!stretch || !stretch->number())
    return false;
  xf->iStretch = stretch->number()->value();
  // Get BBox
  const PdfObj *bbox = dict->get("BBox", &iPdf);
  if (!bbox || !bbox->array())
    return false;
  const PdfObj *a[4];
  for (int i = 0; i < 4; i++) {
    a[i] = bbox->array()->obj(i, &iPdf);
    if (!a[i] || !a[i]->number())
      return false;
  }
  Vector bl(a[0]->number()->value(), a[1]->number()->value());
  Vector sz(a[2]->number()->value(), a[3]->number()->value());
  xf->iBBox.addPoint(bl);
  xf->iBBox.addPoint(bl + sz);
  if (xf->iBBox.bottomLeft() != Vector::ZERO) {
    ipeDebug("PDF bounding box is not zero-aligned: (%g, %g)",
	     xf->iBBox.bottomLeft().x, xf->iBBox.bottomLeft().y);
    return false;
  }
  const PdfObj *res = dict->get("Resources", &iPdf);
  if (!res || !res->dict()) {
    warn("No /Resources in XForm.");
    return false;
  }
  /*
    /Font << /F8 9 0 R /F10 18 0 R >>
    /ProcSet [ /PDF /Text ]
  */
  const PdfObj *fontDict = res->dict()->get("Font", &iPdf);
  if (fontDict && fontDict->dict()) {
    int numFonts = fontDict->dict()->count();
    xf->iFonts.resize(numFonts);
    for (int i = 0; i < numFonts; i++) {
      String fontName(fontDict->dict()->key(i));
      if (fontName[0] != 'F') {
	warn(String("Weird font name: ") + fontName);
	return false;
      }
      int fontNumber = Lex(fontName.substr(1)).getInt();
      xf->iFonts[i] = fontNumber;
      const PdfObj *fontRef = fontDict->dict()->get(fontName, 0);
      if (!fontRef || !fontRef->ref()) {
	warn("Font is not indirect");
	return false;
      }
      iFontObjects[fontNumber] = fontRef->ref()->value();
    }
  }
  return true;
}

bool Latex::getEmbeddedFont(int fno, int objno)
{
  const PdfObj *fontObj = iPdf.object(objno);
  /*
    /Type /Font
    /Subtype /Type1
    /Encoding 24 0 R
    /FirstChar 6
    /LastChar 49
    /Widths 25 0 R
    /BaseFont /YEHLEP+CMR10
    /FontDescriptor 7 0 R
  */
  if (!fontObj || !fontObj->dict())
    return false;
  const PdfObj *type = fontObj->dict()->get("Type", 0);
  if (!type || !type->name() || type->name()->value() != "Font")
    return false;

  // Check whether the stream data is already there.
  FontPool::iterator it = iFontPool->begin();
  while (it != iFontPool->end() && it->iLatexNumber != fno)
    ++it;
  if (it == iFontPool->end()) {
    iFontPool->push_back(Font());
    it = iFontPool->end() - 1;
  }
  Font &font = *it;
  font.iLatexNumber = fno;

  // get font dictionary
  for (int i = 0; i < fontObj->dict()->count(); i++) {
    String key(fontObj->dict()->key(i));
    if (key != "FontDescriptor") {
      const PdfObj *data = fontObj->dict()->get(key, &iPdf);
      font.iFontDict += String("/") + key + " " + data->repr() + "\n";
    }
  }

  // Get type
  const PdfObj *subtype = fontObj->dict()->get("Subtype", &iPdf);
  if (!subtype || !subtype->name())
    return false;
  if (subtype->name()->value() == "Type1")
    font.iType = Font::EType1;
  else if (subtype->name()->value() == "TrueType")
    font.iType = Font::ETrueType;
  else {
    warn("Pdflatex has embedded a font of a type not supported by Ipe.");
    return false;
  }

  // Get encoding vector
  if (font.iType == Font::EType1) {
    const PdfObj *enc = fontObj->dict()->get("Encoding", &iPdf);
    if (!enc) {
      font.iHasEncoding = false;
    } else {
      if (!enc->dict())
	return 0;
      const PdfObj *diff = enc->dict()->get("Differences", &iPdf);
      if (!diff || !diff->array())
	return 0;
      for (int i = 0; i < 0x100; ++i)
	font.iEncoding[i] = ".notdef";
      int idx = 0;
      for (int i = 0; i < diff->array()->count(); ++i) {
	const PdfObj *obj = diff->array()->obj(i, 0);
	if (obj->number())
	  idx = int(obj->number()->value());
	else if (obj->name() && idx < 0x100)
	  font.iEncoding[idx++] = obj->name()->value();
      }
      font.iHasEncoding = true;
    }
  }

  // Get name
  const PdfObj *name = fontObj->dict()->get("BaseFont", 0);
  if (!name || !name->name())
    return false;
  font.iName = name->name()->value();

  // Get widths
  const PdfObj *fc = fontObj->dict()->get("FirstChar", 0);
  const PdfObj *wid = fontObj->dict()->get("Widths", &iPdf);
  if (font.iType == Font::EType1 && fc == 0 && wid == 0) {
    font.iStandardFont = true;
    return true;
  }
  font.iStandardFont = false;
  if (!fc || !fc->number() || !wid || !wid->array())
    return false;
  int firstChar = int(fc->number()->value());
  for (int i = 0; i < wid->array()->count(); ++i) {
    const PdfObj *obj = wid->array()->obj(i, 0);
    if (!obj->number())
      return false;
    font.iWidth[firstChar + i] = int(obj->number()->value());
  }

#if 0
  ipeDebug("Font resource /F%d = %s", fno, font.iFontDict.z());
#endif
  // get font descriptor
  /*
    /Ascent 694
    /CapHeight 683
    /Descent -194
    /FontName /YEHLEP+CMR10
    /ItalicAngle 0
    /StemV 69
    /XHeight 431
    /FontBBox [-251 -250 1009 969]
    /Flags 4
    /CharSet (/Sigma/one)
    /FontFile 8 0 R
  */
  const PdfObj *fontDescriptor =
    fontObj->dict()->get("FontDescriptor", &iPdf);
  if (!fontDescriptor && font.iStandardFont)
    return true;  // it's one of the 14 base fonts, no more data needed
  if (!fontDescriptor->dict())
    return false;
  const PdfObj *fontFile = 0;
  String fontFileKey;
  for (int i = 0; i < fontDescriptor->dict()->count(); i++) {
    String key(fontDescriptor->dict()->key(i));
    if (key.size() >= 8 && key.substr(0, 8) == "FontFile") {
      fontFileKey = key;
      fontFile = fontDescriptor->dict()->get(key, &iPdf);
    } else {
      const PdfObj *data = fontDescriptor->dict()->get(key, &iPdf);
      font.iFontDescriptor += String("/") + key + " " + data->repr() + "\n";
    }
  }
  font.iFontDescriptor += String("/") + fontFileKey + " ";
#if 0
  ipeDebug("Font descriptor /F%d = %s", fno,
	   font.iFontDescriptor.z());
#endif
  // get embedded font file
  if (!fontFile || !fontFile->dict() || fontFile->dict()->stream().size() == 0)
    return false;
  assert(font.iStreamData.size() == 0);
  assert(!fontFile->dict()->deflated());
  font.iStreamData = fontFile->dict()->stream();
  font.iLength1 = font.iLength2 = font.iLength3 = -1;
  for (int i = 0; i < fontFile->dict()->count(); i++) {
    String key = fontFile->dict()->key(i);
    const PdfObj *data = fontFile->dict()->get(key, &iPdf);
    if (key != "Length")
      font.iStreamDict += String("/") + key + " " + data->repr() + "\n";
    if (key == "Length1" && data->number())
      font.iLength1 = int(data->number()->value());
    if (key == "Length2" && data->number())
      font.iLength2 = int(data->number()->value());
    if (key == "Length3" && data->number())
      font.iLength3 = int(data->number()->value());
  }
  if (font.iType == Font::EType1 &&
      (font.iLength1 < 0 || font.iLength2 < 0 || font.iLength3 < 0))
    return false;
  return true;
}

//! Read the PDF file created by Pdflatex.
/*! Must have performed the call to Pdflatex, and pass the name of the
  resulting output file.
*/
bool Latex::readPdf(DataSource &source)
{
  if (!iPdf.parse(source)) {
    warn("Ipe cannot parse the PDF file produced by Pdflatex.");
    return false;
  }

  const PdfDict *page1 = iPdf.page();
  const PdfObj *res = page1->get("Resources", &iPdf);
  if (!res || !res->dict())
    return false;

  iFontPool = new FontPool;

  const PdfObj *obj = res->dict()->get("XObject", &iPdf);
  if (!obj || !obj->dict()) {
    warn("Page 1 has no XForms.");
    return false;
  }
  // collect list of XObject's and their fonts
  for (int i = 0; i < obj->dict()->count(); i++) {
    String key = obj->dict()->key(i);
    const PdfObj *xform = obj->dict()->get(key, &iPdf);
    if (!getXForm(xform))
      return false;
  }
  // collect all fonts
  std::map<int, int>::iterator it;
  for (it = iFontObjects.begin(); it != iFontObjects.end(); ++it) {
    int fno = it->first;
    int objno = it->second;
    if (!getEmbeddedFont(fno, objno))
      return false;
  }
  return true;
}

//! Notify all text objects about their updated PDF code.
/*! Returns true if successful. */
bool Latex::updateTextObjects()
{
  for (TextList::iterator it = iTextObjects.begin();
       it != iTextObjects.end(); ++it) {
    XFormList::iterator xf = iXForms.begin();
    unsigned long int ipeid = (unsigned long int) it->iText;
    while (xf != iXForms.end() && (*xf)->iRefCount != ipeid)
      ++xf;
    if (xf == iXForms.end())
      return false;
    Text::XForm *xform = *xf;
    iXForms.erase(xf);
    it->iText->setXForm(xform);
  }
  return true;
}

/*! Messages about the (mis)behaviour of Pdflatex, probably
  incomprehensible to the user. */
void Latex::warn(String msg)
{
  ipeDebug(msg.z());
}

// --------------------------------------------------------------------
