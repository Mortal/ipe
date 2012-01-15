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

#include "ipeqtcanvas.h"
#include "ipetool.h"

#include "ipepainter.h"

using namespace ipe;
using namespace ipeqt;

// --------------------------------------------------------------------

/*! \class ipeqt::Tool
  \ingroup qtcanvas
  \brief Abstract base class for various canvas tools.

  The Canvas doesn't know about the various modes for object creation,
  editing, and moving, but delegates the handling to a subclass of
  Tool.
*/

//! Constructor.
Tool::Tool(Canvas *canvas) : iCanvas(canvas)
{
  // nothing
}

//! Virtual destructor.
Tool::~Tool()
{
  // nothing
}

void Tool::mouseButton(int button, bool press)
{
  // ignore it
}

void Tool::mouseMove(int button)
{
  // ignore it
}

bool Tool::key(int code, int modifiers, String text)
{
  return false; // not handled
}

// --------------------------------------------------------------------

/*! \class ipeqt::PanTool
  \ingroup qtcanvas
  \brief A tool for panning the canvas.
*/

PanTool::PanTool(Canvas *canvas, const Page *page, int view)
  : Tool(canvas), iPage(page), iView(view)
{
  iPan = Vector::ZERO;
  iMouseDown = iCanvas->unsnappedPos();
  iCanvas->setCursor(QCursor(Qt::PointingHandCursor));
}

void PanTool::draw(Painter &painter) const
{
  painter.translate(iPan);
  painter.setStroke(Attribute(Color(0, 0, 1000)));

  // Doing this triggers a Qt bug
  painter.newPath();
  const Layout *l = iCanvas->cascade()->findLayout();
  painter.rect(Rect(-l->iOrigin, -l->iOrigin + l->iPaperSize));
  painter.drawPath(EStrokedOnly);

  for (int i = 0; i < iPage->count(); ++i) {
    if (iPage->objectVisible(iView, i))
      iPage->object(i)->drawSimple(painter);
  }
}

void PanTool::mouseButton(int button, bool press)
{
  if (!press) {
    Vector dpan = iCanvas->unsnappedPos() - iMouseDown;
    iCanvas->setPan(iCanvas->pan() - dpan);
  }
  iCanvas->finishTool();
}

void PanTool::mouseMove(int button)
{
  iPan = iCanvas->unsnappedPos() - iMouseDown;
  iCanvas->updateTool();
}

// --------------------------------------------------------------------

/*! \class ipeqt::SelectTool
  \ingroup qtcanvas
  \brief A tool for selecting objects.
*/

class SelectCompare {
public:
  int operator()(const SelectTool::SObj &lhs,
		 const SelectTool::SObj &rhs) const
  {
    return (lhs.distance < rhs.distance);
  }
};

//! Constructor starts selection.
SelectTool::SelectTool(Canvas *canvas, Page *page, int view,
		       double selectDistance, bool nonDestructive)
  : Tool(canvas)
{
  iPage = page;
  iView = view;
  iNonDestructive = nonDestructive;
  iSelectDistance = selectDistance;

  // coordinates in user space
  Vector v = iCanvas->unsnappedPos();

  iMouseDown = v;
  iDragging = false;

  double bound = iSelectDistance / iCanvas->zoom();

  // Collect objects close enough
  double d;
  for (int i = iPage->count() - 1; i >= 0; --i) {
    if (iPage->objectVisible(iView, i) &&
	!iPage->isLocked(iPage->layerOf(i))) {
      if ((d = iPage->distance(i, v, bound)) < bound) {
	SObj obj;
	obj.index = i;
	obj.distance = d;
	iObjs.push_back(obj);
      }
    }
  }
  iCur = 0;
  std::stable_sort(iObjs.begin(), iObjs.end(), SelectCompare());
  iCanvas->setCursor(QCursor(Qt::CrossCursor));
}

void SelectTool::draw(Painter &painter) const
{
  if (iDragging) {
    Rect r(iMouseDown, iCorner);
    painter.setStroke(Attribute(Color(1000, 0, 1000)));
    painter.newPath();
    painter.rect(r);
    painter.drawPath(EStrokedOnly);
  } else {
    painter.setStroke(Attribute(Color(1000, 0, 1000)));
    painter.newPath();
    double d = iSelectDistance / iCanvas->zoom();
    painter.drawArc(Arc(Matrix(d, 0, 0, d, iMouseDown.x, iMouseDown.y)));
    painter.closePath();
    painter.drawPath(EStrokedOnly);

    if (iObjs.size() > 0) {
      // display current object
      painter.setStroke(Attribute(Color(1000, 0, 0)));
      iPage->object(iObjs[iCur].index)->drawSimple(painter);
    }
  }
}

void SelectTool::mouseButton(int button, bool press)
{
  if (press) {
    iCanvas->finishTool();
    return;
  }

  bool changed = false;
  if (iDragging) {
    Rect r(iMouseDown, iCorner);
    if (iNonDestructive) {
      // xor selection status of objects in range
      for (int i = 0; i < iPage->count(); ++i) {
	Matrix id;
	if (iPage->objectVisible(iView, i)) {
	  Rect s;
	  iPage->object(i)->addToBBox(s, id, false);
	  if (r.contains(s)) {
	    changed = true;
	    // in range
	    if (iPage->select(i))
	      iPage->setSelect(i, ENotSelected);
	    else
	      iPage->setSelect(i, ESecondarySelected);
	  }
	}
      }
    } else {
      // deselect all objects outside range,
      // secondary select all objects in range,
      for (int i = 0; i < iPage->count(); ++i) {
	iPage->setSelect(i, ENotSelected);
	Matrix id;
	if (iPage->objectVisible(iView, i)) {
	  Rect s;
	  iPage->object(i)->addToBBox(s, id, false);
	  if (r.contains(s))
	    iPage->setSelect(i, ESecondarySelected);
	}
      }
      changed = true;  // XXX this isn't accurate
    }
    iPage->ensurePrimarySelection();
  } else if (iObjs.size() > 0) {
    int index = iObjs[iCur].index;
    if (iNonDestructive) {
      if (!iPage->select(index))
	// selecting unselected object
	iPage->setSelect(index, ESecondarySelected);
      else
	// deselecting selected object
	iPage->setSelect(index, ENotSelected);
      changed = true;
      iPage->ensurePrimarySelection();
    } else {
      // destructive: unselect all
      for (int i = 0; i < iPage->count(); ++i) {
	if (i != index && iPage->select(i)) {
	  changed = true;
	  iPage->setSelect(i, ENotSelected);
	}
      }
      if (iPage->select(index) != EPrimarySelected)
	changed = true;
      iPage->setSelect(index, EPrimarySelected);
    }
  } else {
    // no object in range, deselect all
    if (!iNonDestructive) {
      changed = iPage->hasSelection();
      iPage->deselectAll();
    }
  }
  iCanvas->finishTool();
}

void SelectTool::mouseMove(int button)
{
  iCorner = iCanvas->unsnappedPos();
  if ((iCorner - iMouseDown).sqLen() > 9.0)
    iDragging = true;
  iCanvas->updateTool();
}

bool SelectTool::key(int code, int modifiers, String text)
{
  if (!iDragging && code == ' ' && iObjs.size() > 0) {
    iCur++;
    if (iCur >= int(iObjs.size()))
      iCur = 0;
    iCanvas->updateTool();
    return true;
  } else if (text == "\e") {
    iCanvas->finishTool();
    return true;
  } else
    return false;
}

// --------------------------------------------------------------------

/*! \class ipeqt::TransformTool
  \ingroup qtcanvas
  \brief A tool for transforming the selected objects on the canvas.

  Supports moving, rotating, scaling, and stretching.
*/

//! Constructor starts transformation.
/*! After constructing a TransformTool, you must call isValid() to
    ensure that the transformation can be performed.

    A transformation can fail because the selection contains pinned
    objects, or because the initial mouse position is identical to the
    transformation origin. */
TransformTool::TransformTool(Canvas *canvas, Page *page, int view,
			     TType type, bool withShift)
  : Tool(canvas)
{
  iPage = page;
  iView = view;
  iType = type;
  iWithShift = withShift;
  iMouseDown = iCanvas->pos();
  if (iType == ETranslate)
    iCanvas->setAutoOrigin(iMouseDown);
  iOnlyHorizontal = false;
  iOnlyVertical = false;
  iValid = true;

  // check if objects are pinned
  TPinned pin = ENoPin;
  for (int i = 0; i < page->count(); ++i) {
    if (page->select(i))
      pin = TPinned(pin | page->object(i)->pinned());
  }

  // rotating, scaling, and stretching are not allowed on pinned objects
  if (pin == EFixedPin || (pin && iType != ETranslate)) {
    iValid = false;
    return;
  }

  if (pin) {
    if (pin == EVerticalPin)
      iOnlyHorizontal = true;
    else
      iOnlyVertical = true;
    iWithShift = false;  // ignore this and follow pinning restriction
  }

  // compute origin
  const Snap &sd = iCanvas->snap();
  if (sd.iWithAxes) {
    iOrigin = sd.iOrigin;
  } else {
    // find bounding box of selected objects
    Rect bbox;
    for (int i = 0; i < iPage->count(); ++i) {
      if (iPage->select(i))
	bbox.addRect(iPage->bbox(i));
    }
    iOrigin = 0.5 * (bbox.bottomLeft() + bbox.topRight());
    if (iType == EStretch || iType == EScale) {
      if (iMouseDown.x > iOrigin.x)
	iOrigin.x = bbox.bottomLeft().x;
      else
	iOrigin.x = bbox.topRight().x;
      if (iMouseDown.y > iOrigin.y)
	iOrigin.y = bbox.bottomLeft().y;
      else
	iOrigin.y = bbox.topRight().y;
    }
  }

  if (iType != ETranslate && iMouseDown == iOrigin)
    iValid = false;
  else
    iCanvas->setCursor(Qt::PointingHandCursor);
}

//! Check that the transformation can be performed.
bool TransformTool::isValid() const
{
  return iValid;
}

void TransformTool::draw(Painter &painter) const
{
  painter.setStroke(Attribute(Color(0, 600, 0)));
  painter.transform(iTransform);
  for (int i = 0; i < iPage->count(); ++i) {
    if (iPage->select(i))
      iPage->object(i)->drawSimple(painter);
  }
}

// compute iTransform
void TransformTool::compute(const Vector &v1)
{
  Vector u0 = iMouseDown - iOrigin;
  Vector u1 = v1 - iOrigin;

  switch (iType) {
  case ETranslate: {
    Vector d = v1 - iMouseDown;
    if (iOnlyHorizontal || (iWithShift && ipe::abs(d.x) > ipe::abs(d.y)))
      d.y = 0.0;
    else if (iOnlyVertical || iWithShift)
      d.x = 0.0;
    iTransform = Matrix(d);
    break; }
  case ERotate:
    iTransform = (Matrix(iOrigin) *
		  Linear(Angle(u1.angle() - u0.angle())) *
		  Matrix(-iOrigin));
    break;
  case EScale: {
    double factor = sqrt(u1.sqLen() / u0.sqLen());
    iTransform = (Matrix(iOrigin) *
		  Linear(factor, 0, 0, factor) *
		  Matrix(-iOrigin));
    break; }
  case EStretch: {
    double xfactor = u0.x == 0.0 ? 1.0 : u1.x / u0.x;
    double yfactor = u0.y == 0.0 ? 1.0 : u1.y / u0.y;
    iTransform = (Matrix(iOrigin) *
		  Linear(xfactor, 0, 0, yfactor) *
		  Matrix(-iOrigin));
    break; }
  }
}

void TransformTool::mouseButton(int button, bool press)
{
  if (!press) {
    compute(iCanvas->pos());
    report();
  }
  iCanvas->finishTool();
}

//! Report the final transformation chosen.
/*! The implementation in TransformTool does nothing.
  Derived classes should reimplement report(). */
void TransformTool::report()
{
  // nothing yet
}

void TransformTool::mouseMove(int button)
{
  compute(iCanvas->pos());
  iCanvas->updateTool();
}

// --------------------------------------------------------------------
