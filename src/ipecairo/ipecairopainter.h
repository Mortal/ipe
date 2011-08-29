// -*- C++ -*-
// --------------------------------------------------------------------
// ipe::CairoPainter
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2011  Otfried Cheong

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

#ifndef IPECAIROPAINTER_H
#define IPECAIROPAINTER_H

#include "ipeattributes.h"
#include "ipepainter.h"
#include "ipefonts.h"

#include <cairo.h>

// --------------------------------------------------------------------

namespace ipe {

  class Cascade;
  class PdfObj;

  class CairoPainter : public Painter {
  public:
    CairoPainter(const Cascade *sheet, Fonts *fonts, cairo_t *cc,
		 double zoom, bool pretty);
    virtual ~CairoPainter() { }

    void setDimmed(bool dim) { iDimmed = dim; }

  protected:
    virtual void doPush();
    virtual void doPop();
    virtual void doMoveTo(const Vector &v);
    virtual void doLineTo(const Vector &v);
    virtual void doCurveTo(const Vector &v1, const Vector &v2,
			   const Vector &v3);
    virtual void doClosePath();
    virtual void doDrawArc(const Arc &arc);

    virtual void doAddClipPath();
    virtual void doDrawPath(TPathMode mode);
    virtual void doDrawBitmap(Bitmap bitmap);
    virtual void doDrawText(const Text *text);

  private:
    // void DimColor(QColor &col);
    void drawGlyphs(std::vector<cairo_glyph_t> &glyphs);
    void execute(const Buffer &buffer);
    void clearArgs();
    void opcm();
    void opBT();
    void opTf();
    void opTd();
    void opTJ();
    void opk(bool stroke);
    void opg(bool stroke);
    void oprg(bool stroke);
    void opw();
    void opm();
    void opl();
    void opq();
    void opQ();
    void opre();
    void opsym();

  private:
    Fonts *iFonts;
    cairo_t *iCairo;

    double iZoom;
    bool iPretty;

    bool iDimmed;
    bool iAfterMoveTo;

    // PDF operator drawing
    std::vector<const PdfObj *> iArgs;
    double iTextRgb[3];
    double iStrokeRgb[3];
    double iFillRgb[3];
    double iLineWid;
    Vector iMoveTo;
    // text state
    Face *iFont;  // not owned
    Linear iFontMatrix;
    double iFontSize;
    Vector iTextLinePos;
    Vector iTextPos;
  };

} // namespace

// --------------------------------------------------------------------
#endif
