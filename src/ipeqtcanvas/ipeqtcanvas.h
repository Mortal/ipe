// -*- C++ -*-
// --------------------------------------------------------------------
// ipeqt::Canvas
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2012  Otfried Cheong

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

#ifndef IPEQTCANVAS_H
#define IPEQTCANVAS_H

#include "ipelib.h"

#include <QWidget>

// --------------------------------------------------------------------

namespace ipe {
  class Fonts;
}

// Avoid including cairo.h
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;

// --------------------------------------------------------------------

using namespace ipe;

namespace ipeqt {

  class Tool;

  inline QPointF QPt(const Vector &v)
  {
    return QPointF(v.x, v.y);
  }

  inline String IpeQ(const QString &str)
  {
    return String(str.toUtf8());
  }

  inline QString QIpe(const String &str)
  {
    return QString::fromUtf8(str.z());
  }

  inline QColor QIpe(Color color)
  {
    return QColor(int(color.iRed.toDouble() * 255 + 0.5),
		  int(color.iGreen.toDouble() * 255 + 0.5),
		  int(color.iBlue.toDouble() * 255 + 0.5));
  }

  inline Color IpeQ(QColor color) {
    return Color(color.red() * 1000 / 255,
		 color.green() * 1000 / 255,
		 color.blue() * 1000 / 255);
  }

  // --------------------------------------------------------------------

  class Canvas : public QWidget {
    Q_OBJECT

  public:
    /*! In pretty display, no dashed lines are drawn around text
      objects, and if Latex font data is not available, no text is
      drawn at all. */
    struct Style {
      Color paperColor;
      bool pretty;
      bool classicGrid;
      double thinLine;
      double thickLine;
      bool paperClip;
    };

    Canvas(QWidget* parent, Qt::WFlags f=0);
    ~Canvas();

    void setPage(const Page *page, int view, const Cascade *sheet);
    void setFontPool(const FontPool *fontPool);

    //! Return current pan.
    inline Vector pan() const { return iPan; }
    //! Return current zoom.
    inline double zoom() const { return iZoom; }
    //! Return current style sheet cascade.
    inline const Cascade *cascade() const { return iCascade; }
    //! Return center of canvas.
    inline Vector center() const {return 0.5 * Vector(iWidth, iHeight); }
    //! Return last mouse position (snapped!) in user coordinates.
    inline Vector pos() const { return iMousePos; }
    //! Return last unsnapped mouse position in user coordinates.
    inline Vector unsnappedPos() const { return iUnsnappedMousePos; }
    //! Return global mouse position of last mouse press/release.
    inline Vector globalPos() const { return iGlobalPos; }
    Vector simpleSnapPos() const;
    //! Return current snapping information.
    inline const Snap &snap() const { return iSnap; }

    //! Return current additional modifiers.
    inline int additionalModifiers() const { return iAdditionalModifiers; }
    void setAdditionalModifiers(int mod);

    Vector devToUser(const Vector &arg) const;
    Vector userToDev(const Vector &arg) const;

    void setCanvasStyle(const Style &style);
    //! Return canvas style
    Style canvasStyle() const { return iStyle; }
    void setPan(const Vector &v);
    void setZoom(double zoom);
    void setSnap(const Snap &s);
    void setDimmed(bool dimmed);
    void setAutoOrigin(const Vector &v);

    Matrix canvasTfm() const;

    void computeFifi(double x, double y);
    void setFifiVisible(bool visible);
    void update();
    void updateTool();

    void setTool(Tool *tool);
    void finishTool();

  signals:
    void wheelMoved(int degrees);
    void mouseAction(int button);
    void positionChanged();
    void toolChanged(bool hasTool);

  protected:
    virtual void paintEvent(QPaintEvent *ev);
    virtual void mousePressEvent(QMouseEvent *ev) ;
    virtual void mouseReleaseEvent(QMouseEvent *ev);
    virtual void mouseMoveEvent(QMouseEvent *ev);
    virtual void tabletEvent(QTabletEvent *ev);
    virtual void wheelEvent(QWheelEvent *ev);
    virtual void keyPressEvent(QKeyEvent *ev);
    virtual QSize sizeHint() const;

  protected:
    void drawPaper(cairo_t *cc);
    void drawFrame(cairo_t *cc);
    void drawAxes(cairo_t *cc);
    void drawGrid(cairo_t *cc);
    void drawObjects(cairo_t *cc);
    void drawTool(Painter &painter);
    bool snapToPaperAndFrame();

  protected:
    Tool *iTool;
    const Page *iPage;
    int iView;
    const Cascade *iCascade;

    Style iStyle;

    Vector iPan;
    double iZoom;
    Snap iSnap;
    bool iDimmed;
    bool iAutoSnap;
    Vector iAutoOrigin;
    int iAdditionalModifiers;

    bool iRepaintObjects;
    int iWidth, iHeight;
    cairo_surface_t *iSurface;

    Vector iUnsnappedMousePos;
    Vector iMousePos;
    Vector iGlobalPos;
    Vector iOldFifi;  // last fifi position that has been drawn
    bool iFifiVisible;

    Fonts *iFonts;
  };

} // namespace

// --------------------------------------------------------------------
#endif
