// --------------------------------------------------------------------
// ipe::Canvas
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

#include "ipecanvas.h"
#include "ipetool.h"

#include "ipecairopainter.h"

using namespace ipe;

// --------------------------------------------------------------------

/*! \defgroup canvas Ipe canvas
  \brief A widget (control) that displays an Ipe document page.

  This module contains the classes needed to display and edit Ipe
  objects using the selected toolkit.

  These classes are not in Ipelib, but in a separate library
  libipecanvas.
*/

// --------------------------------------------------------------------

void CanvasObserver::canvasObserverWheelMoved(int degrees) { /* nothing */ }
void CanvasObserver::canvasObserverMouseAction(int button) { /* nothing */ }
void CanvasObserver::canvasObserverPositionChanged() { /* nothing */ }
void CanvasObserver::canvasObserverToolChanged(bool hasTool) { /* nothing */ }

/*! \class ipe::Canvas
  \ingroup canvas
  \brief A widget (control) that displays an Ipe document page.
*/

//! Construct a new canvas.
CanvasBase::CanvasBase()
{
  iObserver = 0;
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
  iSelectionVisible = true;

  iAdditionalModifiers = 0;

  iStyle.paperColor = Color(1000,1000,1000);
  iStyle.pretty = false;
  iStyle.classicGrid = false;
  iStyle.thinLine = 0.2;
  iStyle.thickLine = 0.9;
  iStyle.thinStep = 1;
  iStyle.thickStep = 4;
  iStyle.paperClip = false;
  iStyle.numberPages = false;

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
CanvasBase::~CanvasBase()
{
  if (iSurface)
    cairo_surface_destroy(iSurface);
  delete iFonts;
  delete iTool;
  ipeDebug("CanvasBase::~CanvasBase");
}

// --------------------------------------------------------------------

//! set information about Latex fonts (from ipe::Document)
void CanvasBase::setFontPool(const FontPool *fontPool)
{
  delete iFonts;
  iFonts = Fonts::New(fontPool);
}

// --------------------------------------------------------------------

//! Set the page to be displayed.
/*! Doesn't take ownership of any argument.
  The page number \a pno is only needed if page numbering is turned on.
*/
void CanvasBase::setPage(const Page *page, int pno, int view,
			 const Cascade *sheet)
{
  iPage = page;
  iPageNumber = pno;
  iView = view;
  iCascade = sheet;
}

//! Set style of canvas drawing.
/*! Includes paper color, pretty text, and grid. */
void CanvasBase::setCanvasStyle(const Style &style)
{
  iStyle = style;
}

//! Set current pan position.
/*! The pan position is the user coordinate that is displayed at
  the very center of the canvas. */
void CanvasBase::setPan(const Vector &v)
{
  iPan = v;
}

//! Set current zoom factor.
/*! The zoom factor maps user coordinates to screen pixel coordinates. */
void CanvasBase::setZoom(double zoom)
{
  iZoom = zoom;
}

//! Set the snapping information.
void CanvasBase::setSnap(const Snap &s)
{
  iSnap = s;
}

//! Dim whole canvas, except for the Tool.
/*! This mode will be reset when the Tool finishes. */
void CanvasBase::setDimmed(bool dimmed)
{
  iDimmed = dimmed;
}

//! Set additional modifiers.
/*! These modifier bits are passed to the Tool when a key is pressed
    or a drawing action is performed in addition to the actual
    keyboard modifiers. */
void CanvasBase::setAdditionalModifiers(int mod)
{
  iAdditionalModifiers = mod;
}

//! Enable automatic angular snapping with this origin.
void CanvasBase::setAutoOrigin(const Vector &v)
{
  iAutoOrigin = v;
  iAutoSnap = true;
}

//! Convert canvas (device) coordinates to user coordinates.
Vector CanvasBase::devToUser(const Vector &arg) const
{
  Vector v = arg - center();
  v.x /= iZoom;
  v.y /= -iZoom;
  v += iPan;
  return v;
}

//! Convert user coordinates to canvas (device) coordinates.
Vector CanvasBase::userToDev(const Vector &arg) const
{
  Vector v = arg - iPan;
  v.x *= iZoom;
  v.y *= -iZoom;
  v += center();
  return v;
}

//! Matrix mapping user coordinates to canvas coordinates
Matrix CanvasBase::canvasTfm() const
{
  return Matrix(center()) *
    Linear(iZoom, 0, 0, -iZoom) *
    Matrix(-iPan);
}

// --------------------------------------------------------------------

void CanvasBase::drawAxes(cairo_t *cc)
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

void CanvasBase::drawGrid(cairo_t *cc)
{
  int step = iSnap.iGridSize * iStyle.thinStep;
  double pixstep = step * iZoom;
  if (pixstep < 3.0)
    return;

  // Rect paper = iCascade->findLayout()->paper();
  // Vector ll = paper.bottomLeft();
  // Vector ur = paper.topRight();
  Vector ll = Vector::ZERO;
  Vector ur = iCascade->findLayout()->iFrameSize;

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

  if (iStyle.classicGrid) {
    double lw = iStyle.thinLine / iZoom;
    cairo_set_line_width(cc, lw);
    for (int y = bottom; y < ur.y; y += step) {
      if (screenLR.y <= y && y <= screenUL.y) {
	for (int x = left; x < ur.x; x += step) {
	  if (screenUL.x <= x && x <= screenLR.x) {
	    cairo_move_to(cc, x, y - 0.5 * lw);
	    cairo_line_to(cc, x, y + 0.5 * lw);
	    cairo_stroke(cc);
	  }
	}
      }
    }
  } else {
    double thinLine = iStyle.thinLine / iZoom;
    double thickLine = iStyle.thickLine / iZoom;
    int thickStep = iStyle.thickStep * step;

    // draw horizontal lines
    for (int y = bottom; y < ur.y; y += step) {
      if (screenLR.y <= y && y <= screenUL.y) {
	cairo_set_line_width(cc, (y % thickStep) ? thinLine : thickLine);
	cairo_move_to(cc, ll.x, y);
	cairo_line_to(cc, ur.x, y);
	cairo_stroke(cc);
      }
    }

    // draw vertical lines
    for (int x = left; x < ur.x; x += step) {
      if (screenUL.x <= x && x <= screenLR.x) {
	cairo_set_line_width(cc, (x % thickStep) ? thinLine : thickLine);
	cairo_move_to(cc, x, ll.y);
	cairo_line_to(cc, x, ur.y);
	cairo_stroke(cc);
      }
    }
  }

  cairo_restore(cc);
}

void CanvasBase::drawPaper(cairo_t *cc)
{
  const Layout *l = iCascade->findLayout();
  cairo_rectangle(cc, -l->iOrigin.x, -l->iOrigin.y,
		  l->iPaperSize.x, l->iPaperSize.y);
  cairo_set_source_rgb(cc, iStyle.paperColor.iRed.toDouble(),
		       iStyle.paperColor.iGreen.toDouble(),
		       iStyle.paperColor.iBlue.toDouble());
  cairo_fill(cc);
}

void CanvasBase::drawFrame(cairo_t *cc)
{
  const Layout *l = iCascade->findLayout();
  cairo_set_source_rgb(cc, 0.5, 0.5, 0.5);
  cairo_save(cc);
  double dashes[2] = {3.0 / iZoom, 7.0 / iZoom};
  cairo_set_dash(cc, dashes, 2, 0.0);
  cairo_set_line_width(cc, 2.5 / iZoom);
  cairo_move_to(cc, 0.0, 0.0);
  cairo_line_to(cc, 0.0, l->iFrameSize.y);
  cairo_line_to(cc, l->iFrameSize.x, l->iFrameSize.y);
  cairo_line_to(cc, l->iFrameSize.x, 0);
  cairo_close_path(cc);
  cairo_stroke(cc);
  cairo_restore(cc);
}

void CanvasBase::drawObjects(cairo_t *cc)
{
  if (!iPage)
    return;

  if (iStyle.paperClip) {
    const Layout *l = iCascade->findLayout();
    cairo_rectangle(cc, -l->iOrigin.x, -l->iOrigin.y,
		    l->iPaperSize.x, l->iPaperSize.y);
    cairo_clip(cc);
  }

  CairoPainter painter(iCascade, iFonts, cc, iZoom, iStyle.pretty);
  painter.setDimmed(iDimmed);
  // painter.Transform(CanvasTfm());
  painter.pushMatrix();

  const Symbol *background =
    iCascade->findSymbol(Attribute::BACKGROUND());
  if (background && iPage->findLayer("BACKGROUND") < 0)
    background->iObject->draw(painter);

  if (iStyle.numberPages) {
    const StyleSheet::PageNumberStyle *pns =
      iCascade->findPageNumberStyle();
    cairo_save(cc);
    cairo_set_source_rgb(cc, pns->iColor.iRed.toDouble(),
			 pns->iColor.iGreen.toDouble(),
			 pns->iColor.iBlue.toDouble());
    cairo_select_font_face(cc, "sans-serif",
			   CAIRO_FONT_SLANT_NORMAL,
			   CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cc, pns->iFontSize);
    cairo_move_to(cc, pns->iPos.x, pns->iPos.y);
    cairo_scale(cc, 1.0, -1.0);
    char buf[20];
    if (iPage->countViews() > 0)
      sprintf(buf, "%d-%d", iPageNumber + 1, iView + 1);
    else
      sprintf(buf, "%d", iPageNumber + 1);
    cairo_show_text(cc, buf);
    cairo_restore(cc);
  }

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
void CanvasBase::drawTool(Painter &painter)
{
  if (iTool) {
    iTool->draw(painter);
  } else if (iSelectionVisible) {
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

//! Set an observer.
/*! Use 0 to delete current observer. */
void CanvasBase::setObserver(CanvasObserver *observer)
{
  iObserver = observer;
}

//! Set a new tool.
/*! Calls canvasObserverToolChanged(). */
void CanvasBase::setTool(Tool *tool)
{
  assert(tool);
  iTool = tool;
  updateTool();
  if (iObserver)
    iObserver->canvasObserverToolChanged(true);
}

// Current tool has done its job.
/* Tool is deleted, canvas fully updated, and cursor reset.
   Calls canvasObserverToolChanged(). */
void CanvasBase::finishTool()
{
  delete iTool;
  iTool = 0;
  iDimmed = false;
  iAutoSnap = false;
  update();
  if (iSelectionVisible)
    setCursor(EStandardCursor);
  if (iObserver)
    iObserver->canvasObserverToolChanged(false);
}

// --------------------------------------------------------------------

bool CanvasBase::snapToPaperAndFrame()
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

//! Set whether Fifi should be shown.
/*! Fifi will only be shown if a snapping mode is active. */
void CanvasBase::setFifiVisible(bool visible)
{
  iFifiVisible = visible;
  if (!visible)
    updateTool(); // when making visible, wait for position update
}

//! Set whether selection should be shown when there is no tool.
void CanvasBase::setSelectionVisible(bool visible)
{
  iSelectionVisible = visible;
  updateTool();
}

//! Return snapped mouse position without angular snapping.
Vector CanvasBase::simpleSnapPos() const
{
  Vector pos = iUnsnappedMousePos;
  iSnap.simpleSnap(pos, iPage, iSnap.iSnapDistance / iZoom);
  return pos;
}

// --------------------------------------------------------------------

//! Mark for update with redrawing of objects.
void CanvasBase::update()
{
  iRepaintObjects = true;
  invalidate();
}

//! Mark for update with redrawing of tool only.
void CanvasBase::updateTool()
{
  invalidate();
}

// --------------------------------------------------------------------

/*! Stores the mouse position in iUnsnappedMousePos, computes Fifi if
  snapping is enabled, and stores snapped position in iMousePos. */
void CanvasBase::computeFifi(double x, double y)
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
      invalidate(int(iOldFifi.x - 10), int(iOldFifi.y - 10), 21, 21);
      invalidate(int(fifi.x - 10), int(fifi.y - 10), 21, 21);
    }
  } else if (iFifiVisible) {
    // remove old fifi
    invalidate(int(iOldFifi.x - 10), int(iOldFifi.y - 10), 21, 21);
    iFifiVisible = false;
  }
}

// --------------------------------------------------------------------

void CanvasBase::refreshSurface()
{
  if (!iSurface
      || iWidth != cairo_image_surface_get_width(iSurface)
      || iHeight != cairo_image_surface_get_height(iSurface)) {
    // size has changed
    ipeDebug("size has changed to %d x %d", iWidth, iHeight);
    if (iSurface)
      cairo_surface_destroy(iSurface);
    iSurface = 0;
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
      if (!iStyle.pretty)
	drawFrame(cc);
      if (iSnap.iGridVisible)
	drawGrid(cc);
      drawObjects(cc);
      if (iSnap.iWithAxes)
	drawAxes(cc);
    }
    cairo_surface_flush(iSurface);
    cairo_destroy(cc);
  }
}

// --------------------------------------------------------------------
