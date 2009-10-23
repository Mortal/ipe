// -*- C++ -*-
// --------------------------------------------------------------------
// ipeqt::Tool
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2009  Otfried Cheong

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

#ifndef IPETOOL_H
#define IPETOOL_H

#include "ipelib.h"

using namespace ipe;

namespace ipeqt {

  class Canvas;

  class Tool {
  public:
    virtual ~Tool();

  public:
    virtual void draw(Painter &painter) const = 0;
    virtual void mouseButton(int button, bool press);
    virtual void mouseMove(int button);
    virtual bool key(int code, int modifiers, String text);

  protected:
    Tool(Canvas *canvas);

  protected:
    Canvas *iCanvas;
  };

  // --------------------------------------------------------------------

  class PanTool : public Tool {
  public:
    PanTool(Canvas *canvas, const Page *page, int view);
    virtual void draw(Painter &painter) const;
    virtual void mouseButton(int button, bool press);
    virtual void mouseMove(int button);
  private:
    const Page *iPage;
    int iView;
    Vector iPan;
    Vector iMouseDown;
  };

  // --------------------------------------------------------------------

  class SelectTool : public Tool {
  public:
    SelectTool(Canvas *canvas, Page *page, int view,
	       double selectDistance, bool nonDestructive);

    virtual void draw(Painter &painter) const;
    virtual void mouseButton(int button, bool press);
    virtual void mouseMove(int button);
    virtual bool key(int code, int modifiers, String text);

  private:
    void ensurePrimary();

  public:
    struct SObj {
      int index;
      double distance;
    };

  private:
    Page *iPage;
    int iView;

    bool iNonDestructive;
    int iSelectDistance;
    Vector iMouseDown;
    std::vector<SObj> iObjs;
    int iCur;

    bool iDragging;
    Vector iCorner;
  };

  // --------------------------------------------------------------------

  class TransformTool : public Tool {
  public:
    enum TType { ETranslate, EScale, EStretch, ERotate };

    TransformTool(Canvas *canvas, Page *page, int view, TType type,
		  bool withShift);

    bool isValid() const;

    virtual void draw(Painter &painter) const;
    virtual void mouseButton(int button, bool press);
    virtual void mouseMove(int button);

    virtual void report();

  protected:
    void compute(const Vector &v);

  protected:
    Page *iPage;
    int iView;

    TType iType;
    bool iWithShift;
    bool iOnlyHorizontal;
    bool iOnlyVertical;
    Vector iMouseDown;
    Matrix iTransform;
    Vector iOrigin;

    bool iValid;
  };

} // namespace

// --------------------------------------------------------------------
#endif
