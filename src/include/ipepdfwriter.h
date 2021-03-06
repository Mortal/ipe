// -*- C++ -*-
// --------------------------------------------------------------------
// Creating PDF output
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

#ifndef IPEPDFWRITER_H
#define IPEPDFWRITER_H

#include "ipebase.h"
#include "ipepage.h"
#include "ipedoc.h"
#include "ipeimage.h"
#include "ipefontpool.h"

// --------------------------------------------------------------------

namespace ipe {

  class PdfPainter : public Painter {
  public:
    PdfPainter(const Cascade *style, Stream &stream);
    virtual ~PdfPainter() { }

    static void drawColor(Stream &stream, Color color,
			  const char *gray, const char *rgb);

  protected:
    virtual void doPush();
    virtual void doPop();
    virtual void doNewPath();
    virtual void doMoveTo(const Vector &v);
    virtual void doLineTo(const Vector &v);
    virtual void doCurveTo(const Vector &v1, const Vector &v2,
			   const Vector &v3);
    virtual void doClosePath();
    virtual void doDrawPath(TPathMode mode);
    virtual void doDrawBitmap(Bitmap bitmap);
    virtual void doDrawText(const Text *text);
    virtual void doAddClipPath();
    virtual void doDrawSymbol(Attribute symbol);

  protected:
    void drawAttributes();
    void drawOpacity();

  protected:
    Stream &iStream;
    // iActiveState records the attribute settings that have been
    // recorded in the PDF output, to avoid repeating them
    // over and over again.
    std::list<State> iActiveState;
  };

  // --------------------------------------------------------------------

  class PdfWriter {
  public:
    PdfWriter(TellStream &stream, const Document *doc,
	      const FontPool *pool,
	      bool markedView, int fromPage, int toPage, int compression);
    ~PdfWriter();

    void createPages();
    void createPageView(int page, int view);
    void createBookmarks();
    void createXmlStream(String xmldata, bool preCompressed);
    void createTrailer();

  private:
    int startObject(int objnum = -1);
    void createStream(const char *data, int size, bool preCompressed);
    void writeString(String text);
    void embedBitmap(Bitmap bitmap);
    void paintView(Stream &stream, int pno, int view);
    void embedBitmaps(const BitmapFinder &bm);
    void createResources(const BitmapFinder &bm);
    void embedFonts(const FontPool *pool);

  private:
    TellStream &iStream;
    const Document *iDoc;
    //! Show only last view of each page?
    bool iMarkedView;
    //! Obj id of XML stream.
    int iXmlStreamNum;
    //! Obj id of font resource dictionary.
    int iResourceNum;
    //! Obj id of outline dictionary.
    int iBookmarks;
    //! Compression level (0..9).
    int iCompressLevel;
    //! Obj id of font for page numbers.
    int iPageNumberFont;
    //! Obj id of GState with opacity definitions.
    int iExtGState;
    //! Obj id of dictionary with pattern definitions.
    int iPatternNum;
    // Export only those pages
    int iFromPage;
    int iToPage;

    std::vector<Bitmap> iBitmaps;
    //! Next unused PDF object number.
    int iObjNum;

    //! Object numbers of gradients, indexed by attribute name
    std::map<int, int> iGradients;
    //! Object numbers of symbols, indexed by attribute name
    std::map<int, int> iSymbols;
    //! List of pages, expressed as Pdf object numbers.
    std::vector<int> iPageObjectNumbers;
    //! List of file locations, in object number order (starting with 0).
    std::map<int, long> iXref;
  };

} // namespace

// --------------------------------------------------------------------
#endif
