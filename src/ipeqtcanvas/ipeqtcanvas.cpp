// --------------------------------------------------------------------
// ipeqt::Canvas
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

#include "ipeqtcanvas.h"
#include "ipetool.h"

#include "ipecairopainter.h"

#include <QPainter>
#include <QPaintEvent>

using namespace ipe;
using namespace ipeqt;

// --------------------------------------------------------------------

/*! \namespace ipeqt
  \brief Ipe canvas library namespace

  This namespace is used for the components of Ipe that are dependent
  on the Qt toolkit.
*/

/*! \defgroup qtcanvas Ipe Qt canvas
  \brief A Qt widget that displays an Ipe document page.

  This module contains the classes needed to display and edit Ipe
  objects using the Qt toolkit.

  These classes are not in Ipelib, but in a separate library
  libipeqtcanvas.
*/

// --------------------------------------------------------------------

class IpeQtPainter : public Painter {
public:
  IpeQtPainter(const Cascade *sheet, QPainter *painter);
  virtual ~IpeQtPainter();

protected:
  virtual void doNewPath();
  virtual void doMoveTo(const Vector &v);
  virtual void doLineTo(const Vector &v);
  virtual void doCurveTo(const Vector &v1, const Vector &v2,
			 const Vector &v3);
  virtual void doClosePath();

  virtual void doDrawPath(TPathMode mode);

private:
  QPainter *iQP;
  QPainterPath iPP;
};

IpeQtPainter::IpeQtPainter(const Cascade *sheet, QPainter *painter)
  : Painter(sheet)
{
  iQP = painter;
}

IpeQtPainter::~IpeQtPainter()
{
  // nothing
}

void IpeQtPainter::doNewPath()
{
  iPP = QPainterPath();
}

void IpeQtPainter::doMoveTo(const Vector &v)
{
  iPP.moveTo(QPt(v));
}

void IpeQtPainter::doLineTo(const Vector &v)
{
  iPP.lineTo(QPt(v));
}

void IpeQtPainter::doCurveTo(const Vector &v1, const Vector &v2,
	       const Vector &v3)
{
  iPP.cubicTo(QPt(v1), QPt(v2), QPt(v3));
}

void IpeQtPainter::doClosePath()
{
  iPP.closeSubpath();
}

void IpeQtPainter::doDrawPath(TPathMode mode)
{
  if (mode >= EStrokedAndFilled) {
    QBrush qbrush(QIpe(fill()));
    iQP->fillPath(iPP, qbrush);
  }
  if (mode <= EStrokedAndFilled) {
    QPen qpen(QIpe(stroke()));
    qpen.setWidthF(pen().toDouble());
    iQP->strokePath(iPP, qpen);
  }
}

// --------------------------------------------------------------------

/*! \class ipeqt::Canvas
  \ingroup qtcanvas
  \brief A Qt widget that displays an Ipe document page.
*/

//! Construct a new canvas.
Canvas::Canvas(QWidget* parent, Qt::WFlags f)
  : QWidget(parent, f)
{
  setAttribute(Qt::WA_NoBackground);
  setMouseTracking(true);
  setFocusPolicy(Qt::ClickFocus);

  iTool = 0;
  iPage = 0;
  iCascade = 0;
  iSurface = 0;
  iPan = Vector::ZERO;
  iZoom = 1.0;
  iDimmed = false;
  iWidth = 0;   // not yet known (canvas is not yet mapped)
  iHeight = 0;
  iRepaintObjects = false;
  iFonts = 0;
  iAutoSnap = false;
  iFifiVisible = false;
  iPretty = false;

  iSnap.iSnap = 0;
  iSnap.iGridVisible = false;
  iSnap.iGridSize = 8;
  iSnap.iAngleSize = M_PI / 6.0;
  iSnap.iSnapDistance = 10;
  iSnap.iWithAxes = false;
  iSnap.iOrigin = Vector::ZERO;
  iSnap.iDir = 0;
}

//! destructor.
Canvas::~Canvas()
{
  if (iSurface)
    cairo_surface_destroy(iSurface);
  delete iFonts;
  delete iTool;
}

QSize Canvas::sizeHint() const
{
  return QSize(640, 480);
}

// --------------------------------------------------------------------

//! set information about Latex fonts (from ipe::Document)
void Canvas::setFontPool(const FontPool *fontPool)
{
  delete iFonts;
  iFonts = Fonts::New(fontPool);
}

// --------------------------------------------------------------------

//! Set the page to be displayed.
/*! Doesn't take ownership of any argument. */
void Canvas::setPage(const Page *page, int view, const Cascade *sheet)
{
  iPage = page;
  iView = view;
  iCascade = sheet;
}

//! Determine whether pretty display should be used.
/*! In pretty display, no dashed lines are drawn around text objects,
  and if Latex font data is not available, no text is drawn at all. */
void Canvas::setPretty(bool pretty)
{
  iPretty = pretty;
}

//! Set current pan position.
/*! The pan position is the user coordinate that is displayed at
  the very center of the canvas. */
void Canvas::setPan(const Vector &v)
{
  iPan = v;
}

//! Set current zoom factor.
/*! The zoom factor maps user coordinates to screen pixel coordinates. */
void Canvas::setZoom(double zoom)
{
  iZoom = zoom;
}

//! Set the snapping information.
void Canvas::setSnap(const Snap &s)
{
  iSnap = s;
}

//! Dim whole canvas, except for the Tool.
/*! This mode will be reset when the Tool finishes. */
void Canvas::setDimmed(bool dimmed)
{
  iDimmed = dimmed;
}

//! Enable automatic angular snapping with this origin.
void Canvas::setAutoOrigin(const Vector &v)
{
  iAutoOrigin = v;
  iAutoSnap = true;
}

//! Mark for update with redrawing of objects.
void Canvas::update()
{
  iRepaintObjects = true;
  QWidget::update();
}

//! Mark for update with redrawing of tool only.
void Canvas::updateTool()
{
  QWidget::update();
}

//! Convert canvas (device) coordinates to user coordinates.
Vector Canvas::devToUser(const Vector &arg) const
{
  Vector v = arg - center();
  v.x /= iZoom;
  v.y /= -iZoom;
  v += iPan;
  return v;
}

//! Convert user coordinates to canvas (device) coordinates.
Vector Canvas::userToDev(const Vector &arg) const
{
  Vector v = arg - iPan;
  v.x *= iZoom;
  v.y *= -iZoom;
  v += center();
  return v;
}

//! Matrix mapping user coordinates to canvas coordinates
Matrix Canvas::canvasTfm() const
{
  return Matrix(center()) *
    Linear(iZoom, 0, 0, -iZoom) *
    Matrix(-iPan);
}

// --------------------------------------------------------------------

void Canvas::drawAxes(cairo_t *cc)
{
  double alpha = 0.0;
  double ep = (iWidth + iHeight) / iZoom;

  cairo_save(cc);
  cairo_set_source_rgb(cc, 0.0, 1.0, 0.0);
  cairo_set_line_width(cc, 2.0 / iZoom);
  while (alpha < IpeTwoPi) {
    double beta = iSnap.iDir + alpha;
    cairo_move_to(cc, iSnap.iOrigin.x, iSnap.iOrigin.y);
    Vector dir(beta);
    cairo_rel_line_to(cc, ep * dir.x, ep * dir.y);
    if (alpha == 0.0) {
      cairo_stroke(cc);
      cairo_set_line_width(cc, 1.0 / iZoom);
    }
    alpha += iSnap.iAngleSize;
  }
  cairo_stroke(cc);
  cairo_restore(cc);
}

void Canvas::drawGrid(cairo_t *cc)
{
  int step = iSnap.iGridSize;
  double pixstep = step * iZoom;
  if (pixstep < 3.0)
    return;

  int vfactor = 1;
  int vstep = step * vfactor;

  Rect paper = iCascade->findLayout()->paper();
  Vector ll = paper.bottomLeft();
  Vector ur = paper.topRight();

  int left = step * int(ll.x / step);
  if (left < ll.x)
    ++left;
  int bottom = step * int(ll.y / step);
  if (bottom < ll.y)
    ++bottom;

  // only draw lines that intersect canvas
  Vector screenUL = devToUser(Vector::ZERO);
  Vector screenLR = devToUser(Vector(iWidth, iHeight));

  cairo_save(cc);
  cairo_set_source_rgb(cc, 0.3, 0.3, 0.3);
  double thinLine = 0.2 / iZoom;
  double thickLine = 0.9 / iZoom;
  int thickStep = 4 * step;

  // draw horizontal lines
  for (int y = bottom; y < ur.y; y += vstep) {
    if (screenLR.y <= y && y <= screenUL.y) {
      cairo_set_line_width(cc, (y % thickStep) ? thinLine : thickLine);
      cairo_move_to(cc, ll.x, y);
      cairo_line_to(cc, ur.x, y);
      cairo_stroke(cc);
    }
  }

  // draw vertical lines
  for (int x = left; x < ur.x; x += vstep) {
    if (screenUL.x <= x && x <= screenLR.x) {
      cairo_set_line_width(cc, (x % thickStep) ? thinLine : thickLine);
      cairo_move_to(cc, x, ll.y);
      cairo_line_to(cc, x, ur.y);
      cairo_stroke(cc);
    }
  }
  cairo_restore(cc);
}

void Canvas::drawPaper(cairo_t *cc)
{
  const Layout *l = iCascade->findLayout();
  cairo_rectangle(cc, -l->iOrigin.x, -l->iOrigin.y,
		  l->iPaperSize.x, l->iPaperSize.y);
  cairo_set_source_rgb(cc, 1.0, 1.0, 1.0);
  cairo_fill(cc);
}

void Canvas::drawFrame(cairo_t *cc)
{
  const Layout *l = iCascade->findLayout();
  cairo_set_source_rgb(cc, 0.5, 0.5, 0.5);
  cairo_save(cc);
  double dashes[2] = {1.0, 3.0};
  cairo_set_dash(cc, dashes, 2, 0.0);
  cairo_move_to(cc, 0.0, 0.0);
  cairo_line_to(cc, 0.0, l->iFrameSize.y);
  cairo_line_to(cc, l->iFrameSize.x, l->iFrameSize.y);
  cairo_line_to(cc, l->iFrameSize.x, 0);
  cairo_close_path(cc);
  cairo_stroke(cc);
  cairo_restore(cc);
}

void Canvas::drawObjects(cairo_t *cc)
{
  if (!iPage)
    return;

  CairoPainter painter(iCascade, iFonts, cc, iZoom, iPretty);
  painter.setDimmed(iDimmed);
  // painter.Transform(CanvasTfm());
  painter.pushMatrix();

  const Symbol *background =
    iCascade->findSymbol(Attribute::BACKGROUND());
  if (background && iPage->findLayer("BACKGROUND") < 0)
    background->iObject->draw(painter);

  const Text *title = iPage->titleText();
  if (title)
    title->draw(painter);

  for (int i = 0; i < iPage->count(); ++i) {
    if (iPage->objectVisible(iView, i))
      iPage->object(i)->draw(painter);
  }
  painter.popMatrix();
}

// --------------------------------------------------------------------

//! Draw the current canvas tool.
/*! If no tool is set, it draws the selected objects. */
void Canvas::drawTool(Painter &painter)
{
  if (iTool) {
    iTool->draw(painter);
  } else {
    for (int i = 0; i < iPage->count(); ++i) {
      if (iPage->objectVisible(iView, i)) {
	if (iPage->select(i) == EPrimarySelected) {
	  painter.setStroke(Attribute(Color(1000, 0, 0)));
	  painter.setPen(Attribute(Fixed(2)));
	  iPage->object(i)->drawSimple(painter);
	} else if (iPage->select(i) == ESecondarySelected) {
	  painter.setStroke(Attribute(Color(1000, 0, 1000)));
	  painter.setPen(Attribute(Fixed(1)));
	  iPage->object(i)->drawSimple(painter);
	}
      }
    }
  }
}

//! Set a new tool.
/*! Emits toolChanged() signal. */
void Canvas::setTool(Tool *tool)
{
  assert(tool);
  iTool = tool;
  updateTool();
  emit toolChanged(true);
}

// Current tool has done its job.
/* Tool is deleted, canvas fully updated, and cursor reset.
   Emits toolChanged() signal. */
void Canvas::finishTool()
{
  delete iTool;
  iTool = 0;
  iDimmed = false;
  iAutoSnap = false;
  update();
  unsetCursor();
  emit toolChanged(false);
}

// --------------------------------------------------------------------

bool Canvas::snapToPaperAndFrame()
{
  double snapDist = iSnap.iSnapDistance / iZoom;
  double d = snapDist;
  Vector fifi = iMousePos;
  const Layout *layout = iCascade->findLayout();
  Rect paper = layout->paper();
  Rect frame(Vector::ZERO, layout->iFrameSize);

  // vertices
  if (iSnap.iSnap & Snap::ESnapVtx) {
    paper.bottomLeft().snap(iMousePos, fifi, d);
    paper.topRight().snap(iMousePos, fifi, d);
    paper.topLeft().snap(iMousePos, fifi, d);
    paper.bottomRight().snap(iMousePos, fifi, d);
    frame.bottomLeft().snap(iMousePos, fifi, d);
    frame.topRight().snap(iMousePos, fifi, d);
    frame.topLeft().snap(iMousePos, fifi, d);
    frame.bottomRight().snap(iMousePos, fifi, d);
  }

  // Return if snapping has occurred
  if (d < snapDist) {
    iMousePos = fifi;
    return true;
  }

  // boundary
  if (iSnap.iSnap & Snap::ESnapBd) {
    Segment(paper.bottomLeft(), paper.bottomRight()).snap(iMousePos, fifi, d);
    Segment(paper.bottomRight(), paper.topRight()).snap(iMousePos, fifi, d);
    Segment(paper.topRight(), paper.topLeft()).snap(iMousePos, fifi, d);
    Segment(paper.topLeft(), paper.bottomLeft()).snap(iMousePos, fifi, d);
    Segment(frame.bottomLeft(), frame.bottomRight()).snap(iMousePos, fifi, d);
    Segment(frame.bottomRight(), frame.topRight()).snap(iMousePos, fifi, d);
    Segment(frame.topRight(), frame.topLeft()).snap(iMousePos, fifi, d);
    Segment(frame.topLeft(), frame.bottomLeft()).snap(iMousePos, fifi, d);
  }

  if (d < snapDist) {
    iMousePos = fifi;
    return true;
  }

  return true;
}

// --------------------------------------------------------------------

/*! Stores the mouse position in iUnsnappedMousePos, computes Fifi if
  snapping is enabled, and stores snapped position in iMousePos. */
void Canvas::computeFifi(double x, double y)
{
  iUnsnappedMousePos = devToUser(Vector(x, y));
  iMousePos = iUnsnappedMousePos;

  if (!iPage)
    return;

  int mask = iAutoSnap ? 0 : Snap::ESnapAuto;
  if (iSnap.iSnap & ~mask) {
    if (!iSnap.snap(iMousePos, iPage, iSnap.iSnapDistance / iZoom,
		    (iAutoSnap ? &iAutoOrigin : 0)))
      snapToPaperAndFrame();

    // convert fifi coordinates back into device space
    Vector fifi = userToDev(iMousePos);
    if (iFifiVisible && fifi != iOldFifi) {
      QWidget::update(QRect(int(iOldFifi.x - 10), int(iOldFifi.y - 10),
			    21, 21));
      QWidget::update(QRect(int(fifi.x - 10), int(fifi.y - 10), 21, 21));
    }
  } else if (iFifiVisible) {
    // remove old fifi
    QWidget::update(QRect(int(iOldFifi.x - 10), int(iOldFifi.y - 10), 21, 21));
    iFifiVisible = false;
  }
}

//! Set whether Fifi should be shown.
/*! Fifi will only be shown if a snapping mode is active. */
void Canvas::setFifiVisible(bool visible)
{
  iFifiVisible = visible;
  if (!visible)
    updateTool(); // when making visible, wait for position update
}

//! Return snapped mouse position without angular snapping.
Vector Canvas::simpleSnapPos() const
{
  Vector pos = iUnsnappedMousePos;
  iSnap.simpleSnap(pos, iPage, iSnap.iSnapDistance / iZoom);
  return pos;
}

// --------------------------------------------------------------------

void Canvas::keyPressEvent(QKeyEvent *ev)
{
  String s = IpeQ(ev->text());
  if (iTool && iTool->key(ev->key(), ev->modifiers(), s))
    ev->accept();
  else
    ev->ignore();
}

void Canvas::mousePressEvent(QMouseEvent *ev)
{
  iGlobalPos = Vector(ev->globalPos().x(), ev->globalPos().y());
  computeFifi(ev->x(), ev->y());
  if (iTool)
    iTool->mouseButton(ev->button() | ev->modifiers(), true);
  else
    emit mouseAction(ev->button() | ev->modifiers());
}

void Canvas::mouseReleaseEvent(QMouseEvent *ev)
{
  iGlobalPos = Vector(ev->globalPos().x(), ev->globalPos().y());
  computeFifi(ev->x(), ev->y());
  if (iTool)
    iTool->mouseButton(ev->button(), false);
}

void Canvas::mouseMoveEvent(QMouseEvent *ev)
{
  computeFifi(ev->x(), ev->y());
  if (iTool)
    iTool->mouseMove(ev->buttons());
  emit positionChanged();
}

void Canvas::tabletEvent(QTabletEvent *ev)
{
  Vector globalPos(ev->hiResGlobalPos().x(), ev->hiResGlobalPos().y());
  QPointF hiPos = ev->hiResGlobalPos() - (ev->globalPos() - ev->pos());
  ev->accept();

  switch (ev->type()) {
  case QEvent::TabletPress:
    if (ev->pointerType() == QTabletEvent::Eraser)
      return; // not yet implemented
    iGlobalPos = globalPos;
    computeFifi(hiPos.x(), hiPos.y());
    if (iTool)
      iTool->mouseButton(Qt::LeftButton, true);
    else
      emit mouseAction(Qt::LeftButton);
    break;
  case QEvent::TabletMove:
    if (ev->pressure() > 0.01) {
      computeFifi(hiPos.x(), hiPos.y());
      if (iTool)
	iTool->mouseMove(Qt::LeftButton);
      emit positionChanged();
      break;
    }
    // else fall through and consider it a release event
  case QEvent::TabletRelease:
    iGlobalPos = globalPos;
    computeFifi(hiPos.x(), hiPos.y());
    if (iTool)
      iTool->mouseButton(Qt::LeftButton, false);
    break;
  default:
    ipeDebug("Unknown tablet event");
    break;
  }
}

void Canvas::wheelEvent(QWheelEvent *ev)
{
  emit wheelMoved(ev->delta());
  ev->accept();
}

void Canvas::paintEvent(QPaintEvent * ev)
{
  QSize s = size();
  if (s.width() != iWidth || s.height() != iHeight) {
    // size has changed
    // ipeDebug("size has changed to %d x %d", s.width(), s.height());
    if (iSurface)
      cairo_surface_destroy(iSurface);
    iSurface = 0;
    iWidth = s.width();
    iHeight = s.height();
    iRepaintObjects = true;
  }
  if (iRepaintObjects) {
    iRepaintObjects = false;
    if (!iSurface)
      iSurface =
	cairo_image_surface_create(CAIRO_FORMAT_RGB24, iWidth, iHeight);
    cairo_t *cc = cairo_create(iSurface);
    // background
    cairo_set_source_rgb(cc, 0.4, 0.4, 0.4);
    cairo_rectangle(cc, 0, 0, iWidth, iHeight);
    cairo_fill(cc);

    cairo_translate(cc, 0.5 * iWidth, 0.5 * iHeight);
    cairo_scale(cc, iZoom, -iZoom);
    cairo_translate(cc, -iPan.x, -iPan.y);

    if (iPage) {
      drawPaper(cc);
      if (!iPretty)
	drawFrame(cc);
      if (iSnap.iGridVisible)
	drawGrid(cc);
      if (iSnap.iWithAxes)
	drawAxes(cc);
      drawObjects(cc);
    }
    cairo_surface_flush(iSurface);
    cairo_destroy(cc);
  }
  // -----------------------------------
  QPainter qPainter;
  qPainter.begin(this);
  QRect r = ev->rect();
  QImage bits(cairo_image_surface_get_data(iSurface),
	      iWidth, iHeight, QImage::Format_RGB32);
  qPainter.drawImage(r.left(), r.top(), bits,
		     r.left(), r.top(), r.width(), r.height());
  qPainter.translate(-1, -1);
  if (iFifiVisible) {
    Vector p = userToDev(iMousePos);
    qPainter.setPen(QColor(255, 0, 0, 255));
    qPainter.drawLine(QPt(p - Vector(8, 0)), QPt(p + Vector(8, 0)));
    qPainter.drawLine(QPt(p - Vector(0, 8)), QPt(p + Vector(0, 8)));
    iOldFifi = p;
  }
  // -----------------------------------
  if (iPage) {
    IpeQtPainter qp(iCascade, &qPainter);
    qp.transform(canvasTfm());
    qp.pushMatrix();
    drawTool(qp);
    qp.popMatrix();
  }
  qPainter.end();
}

// --------------------------------------------------------------------
