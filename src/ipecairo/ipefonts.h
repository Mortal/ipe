// -*- C++ -*-
// --------------------------------------------------------------------
// CanvasFonts maintains the Freetype fonts for the canvas
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

#ifndef IPEFONTS_H
#define IPEFONTS_H

#include "ipebase.h"
#include "ipegeo.h"
#include "ipefontpool.h"

#include <cairo.h>

//------------------------------------------------------------------------

struct FT_FaceRec_;

namespace ipe {

  class Face {
  public:
    Face(int id, const Font &font);
    ~Face();
    inline bool matches(int id) const { return id == iId; }
    inline Font::TType type() const { return iType; }
    inline int width(int ch) const { return iWidth[ch]; }
    inline cairo_font_face_t *cairoFont() { return iCairoFont; }
    int getGlyph(int ch);

  private:
    int iId;
    Font::TType iType;
    int iGlyphIndex[0x100];
    int iWidth[0x100];
    cairo_font_face_t *iCairoFont;
  };

  class Fonts {
  public:
    static Fonts *New(const FontPool *fontPool);
    ~Fonts();
    Face *getFace(int id);
    static cairo_font_face_t *screenFont();
    static String freetypeVersion();

  private:
    Fonts(const FontPool *fontPool);

  private:
    const FontPool *iFontPool;

    typedef std::list<Face *> FaceSeq;
    FaceSeq iFaces;
  };

} // namespace

//------------------------------------------------------------------------
#endif
