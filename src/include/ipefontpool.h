// -*- C++ -*-
// --------------------------------------------------------------------
// The pool of embedded fonts.
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

#ifndef IPEFONTPOOL_H
#define IPEFONTPOOL_H

#include "ipebase.h"
#include "ipegeo.h"

// --------------------------------------------------------------------

namespace ipe {

  //! A text font.
  struct Font {
    //! Ipe supports two types of fonts.
    enum TType { EType1, ETrueType };
    //! The font type.
    TType iType;
    //! The name of this font (e.g. "Times-Roman").
    String iName;
    //! The font id in the Pdflatex output: /Fxx
    int iLatexNumber;
    //! The font dictionary in the PDF file.
    String iFontDict;
    //! The font descriptor in the PDF file.
    String iFontDescriptor;
    //! The stream dictionary for the font stream in the PDF file.
    String iStreamDict;
    //! The values of LengthX in the font stream in the PDF file.
    int iLength1, iLength2, iLength3;
    //! The stream data for the font stream in the PDF file.
    Buffer iStreamData;
    //! Is there an explicit encoding for this font?
    bool iHasEncoding;
    //! The encoding of this font.
    String iEncoding[0x100];
    //! Is this one of the 14 standard fonts?
    bool iStandardFont;
    //! The width of each character in font units.
    int iWidth[0x100];
  };

  //! A list of fonts used by a Document. \relates Font
  typedef std::vector<Font> FontPool;

} // namespace

// --------------------------------------------------------------------
#endif
