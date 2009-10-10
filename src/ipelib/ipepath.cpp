// --------------------------------------------------------------------
// The path object
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

#include "ipepath.h"
#include "ipepainter.h"

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::Path
  \ingroup obj
  \brief The path object (polylines, polygons, and generalizations).

  This object represents any vector graphics.  The geometry is
  contained in a Shape.

  The filling algorithm is the <b>even-odd rule</b> of PDF: To
  determine whether a point lies inside the filled shape, draw a ray
  from that point in any direction, and count the number of path
  segments that cross the ray.  If this number is odd, the point is
  inside; if even, the point is outside. (Path objects can also render
  using the <b>winding fill rule</b> by setting the fillRule
  attribute.  This isn't really supported by the Ipe user interface,
  which doesn't show the orientation of paths.)
*/

//! Construct from XML data.
Path *Path::create(const XmlAttributes &attr, String data)
{
  IpeAutoPtr<Path> self(new Path(attr));
  if (!self->iShape.load(data))
    return 0;
  self->makeArrowData();
  return self.take();
}

//! Create empty path with attributes taken from XML
Path::Path(const XmlAttributes &attr)
  : Object(attr)
{
  bool stroked = false;
  bool filled = false;

  String str;
  if (attr.has("stroke", str)) {
    iStroke = Attribute::makeColor(str, Attribute::BLACK());
    stroked = true;
  }
  if (attr.has("fill", str)) {
    iFill = Attribute::makeColor(str, Attribute::BLACK());
    filled = true;
  }

  iPathMode = (stroked ? (filled ? EStrokedAndFilled : EStrokedOnly) :
	       EFilledOnly);

  iDashStyle = Attribute::makeDashStyle(attr["dash"]);

  iPen = Attribute::makeScalar(attr["pen"], Attribute::NORMAL());

  iOpacity = Attribute::makeScalar(attr["opacity"], Attribute::OPAQUE());

  if (attr.has("tiling", str))
    iTiling = Attribute(true, str);
  else
    iTiling = Attribute::NORMAL();

  iLineCap = EDefaultCap;
  iLineJoin = EDefaultJoin;
  iFillRule = EDefaultRule;
  if (attr.has("cap", str))
    iLineCap = TLineCap(Lex(str).getInt() + 1);
  if (attr.has("join", str))
    iLineJoin = TLineJoin(Lex(str).getInt() + 1);
  if (attr.has("fillrule", str)) {
    if (str == "eofill")
      iFillRule = EEvenOddRule;
    else if (str == "wind")
      iFillRule = EWindRule;
  }

  iHasFArrow = false;
  iHasRArrow = false;
  iFArrowShape = iRArrowShape = Attribute::ARROW_NORMAL();
  iFArrowSize = iRArrowSize = Attribute::NORMAL();

  if (attr.has("arrow", str)) {
    iHasFArrow = true;
    int i = str.find("/");
    if (i >= 0) {
      iFArrowShape = Attribute(true, String("arrow/") + str.left(i) + "(spx)");
      iFArrowSize = Attribute::makeScalar(str.substr(i+1), Attribute::NORMAL());
    } else
      iFArrowSize = Attribute::makeScalar(str, Attribute::NORMAL());
  }

  if (attr.has("rarrow", str)) {
    iHasRArrow = true;
    int i = str.find("/");
    if (i >= 0) {
      iRArrowShape = Attribute(true, String("arrow/") + str.left(i) + "(spx)");
      iRArrowSize = Attribute::makeScalar(str.substr(i+1), Attribute::NORMAL());
    } else
      iRArrowSize = Attribute::makeScalar(str, Attribute::NORMAL());
  }
}

void Path::init(const AllAttributes &attr, bool withArrows)
{
  iPathMode = attr.iPathMode;
  iStroke = attr.iStroke;
  iFill = attr.iFill;
  iDashStyle = attr.iDashStyle;
  iPen = attr.iPen;
  iOpacity = attr.iOpacity;
  iTiling = attr.iTiling;
  iLineCap = attr.iLineCap;
  iLineJoin = attr.iLineJoin;
  iFillRule = attr.iFillRule;
  iHasFArrow = false;
  iHasRArrow = false;
  iFArrowShape = iRArrowShape = Attribute::ARROW_NORMAL();
  iFArrowSize = iRArrowSize = Attribute::NORMAL();
  if (withArrows) {
    if (attr.iFArrow) {
      iHasFArrow = true;
      iFArrowShape = attr.iFArrowShape;
      iFArrowSize = attr.iFArrowSize;
    }
    if (attr.iRArrow) {
      iHasRArrow = true;
      iRArrowShape = attr.iRArrowShape;
      iRArrowSize = attr.iRArrowSize;
    }
  }
}

//! Create for given shape.
Path::Path(const AllAttributes &attr, const Shape &shape, bool withArrows)
  : Object(attr), iShape(shape)
{
  init(attr, withArrows);
  makeArrowData();
}

//! Return a clone (constant-time).
Object *Path::clone() const
{
  return new Path(*this);
}

//! Compute the arrow information.
void Path::makeArrowData()
{
  assert(iShape.countSubPaths() > 0);
  if (iShape.countSubPaths() > 1 || iShape.subPath(0)->closed()) {
    iFArrowOk = false;
    iRArrowOk = false;
  } else {
    CurveSegment seg = iShape.subPath(0)->asCurve()->segment(0);
    iRArrowOk = true;
    iRArrowPos = seg.cp(0);
    if (seg.type() == CurveSegment::EArc) {
      Angle alpha = (seg.matrix().inverse() * seg.cp(0)).angle();
      iRArrowDir = (seg.matrix().linear() *
		    Vector(Angle(alpha - IpeHalfPi))).angle();
    } else {
      if (seg.cp(1) == seg.cp(0))
	iRArrowOk = false;
      else
	iRArrowDir = (iRArrowPos - seg.cp(1)).angle();
    }

    seg = iShape.subPath(0)->asCurve()->segment(-1);
    iFArrowOk = true;
    iFArrowPos = seg.last();
    if (seg.type() == CurveSegment::EArc) {
      Angle alpha = (seg.matrix().inverse() * seg.cp(1)).angle();
      iFArrowDir = (seg.matrix().linear() *
		    Vector(Angle(alpha + IpeHalfPi))).angle();
    } else {
      if (seg.cp(seg.countCP() - 2) == seg.last())
	iFArrowOk = false;
      else
	iFArrowDir = (iFArrowPos - seg.cp(seg.countCP() - 2)).angle();
    }
  }
}

//! Return pointer to this object.
Path *Path::asPath()
{
  return this;
}

Object::Type Path::type() const
{
  return EPath;
}

//! Call visitPath of visitor.
void Path::accept(Visitor &visitor) const
{
  visitor.visitPath(this);
}

void Path::saveAsXml(Stream &stream, String layer) const
{
  stream << "<path";
  saveAttributesAsXml(stream, layer);
  if (iPathMode <= EStrokedAndFilled)
    stream << " stroke=\"" << iStroke.string() << "\"";
  if (iPathMode >= EStrokedAndFilled)
    stream << " fill=\"" << iFill.string() << "\"";
  if (!iDashStyle.isNormal())
    stream << " dash=\"" << iDashStyle.string() << "\"";
  if (!iPen.isNormal())
    stream << " pen=\"" << iPen.string() << "\"";
  if (iLineCap != EDefaultCap)
    stream << " cap=\"" << iLineCap << "\"";
  if (iLineJoin != EDefaultJoin)
    stream << " join=\"" << iLineJoin << "\"";
  if (iFillRule == EWindRule)
    stream << " fillrule=\"wind\"";
  else if (iFillRule == EEvenOddRule)
    stream << " fillrule=\"eofill\"";
  if (iHasFArrow && iFArrowOk) {
    String s = iFArrowShape.string();
    stream << " arrow=\"" << s.substr(6, s.size() - 11)
	   << "/" << iFArrowSize.string() << "\"";
  }
  if (iHasRArrow && iRArrowOk) {
    String s = iRArrowShape.string();
    stream << " rarrow=\"" << s.substr(6, s.size() - 11)
	   << "/" << iRArrowSize.string() << "\"";
  }
  if (iOpacity != Attribute::OPAQUE())
    stream << " opacity=\"" << iOpacity.string() << "\"";
  if (!iTiling.isNormal())
    stream << " tiling=\"" << iTiling.string() << "\"";
  stream << ">\n";
  iShape.save(stream);
  stream << "</path>\n";
}

//! Draw an arrow of \a size with tip at \a v1 directed from \a v0 to \a v1.
void Path::drawArrow(Painter &painter, Vector pos, Angle angle,
		     Attribute shape, Attribute size)
{
  const Symbol *symbol = painter.cascade()->findSymbol(shape);
  if (symbol) {
    double s =
      painter.cascade()->find(EArrowSize, size).number().toDouble();
    Color color = painter.stroke();

    painter.push();
    painter.pushMatrix();
    painter.translate(pos);
    painter.transform(Linear(angle));
    painter.untransform(ETransformationsRigidMotions);
    Matrix m(s, 0, 0, s, 0, 0);
    painter.transform(m);
    painter.setSymStroke(Attribute(color));
    painter.setSymFill(Attribute(color));
    painter.setSymPen(Attribute(painter.pen()));
    symbol->iObject->draw(painter);
    painter.popMatrix();
    painter.pop();
  }
}

void Path::setShape(const Shape &shape)
{
  iShape = shape;
  makeArrowData();
}

void Path::draw(Painter &painter) const
{
  painter.push();
  if (iPathMode <= EStrokedAndFilled) {
    painter.setStroke(iStroke);
    painter.setDashStyle(iDashStyle);
    painter.setPen(iPen);
    painter.setLineCap(lineCap());
    painter.setLineJoin(lineJoin());
  }
  if (iPathMode >= EStrokedAndFilled) {
    painter.setFill(iFill);
    painter.setFillRule(fillRule());
  }
  painter.setOpacity(iOpacity);
  painter.setTiling(iTiling);
  painter.pushMatrix();
  painter.transform(matrix());
  painter.untransform(transformations());
  painter.newPath();
  iShape.draw(painter);
  painter.drawPath(iPathMode);
  // Draw arrows
  if (iHasFArrow && iFArrowOk)
    drawArrow(painter, iFArrowPos, iFArrowDir, iFArrowShape, iFArrowSize);
  if (iHasRArrow && iRArrowOk)
    drawArrow(painter, iRArrowPos, iRArrowDir, iRArrowShape, iRArrowSize);
  painter.popMatrix();
  painter.pop();
}

void Path::drawSimple(Painter &painter) const
{
  painter.pushMatrix();
  painter.transform(matrix());
  painter.untransform(transformations());
  painter.newPath();
  iShape.draw(painter);
  painter.drawPath(EStrokedOnly);
  painter.popMatrix();
}

void Path::addToBBox(Rect &box, const Matrix &m, bool cp) const
{
  iShape.addToBBox(box, m * matrix(), cp);
}

double Path::distance(const Vector &v, const Matrix &m, double bound) const
{
  return iShape.distance(v, m * matrix(), bound);
}

void Path::snapVtx(const Vector &mouse, const Matrix &m,
		   Vector &pos, double &bound) const
{
  iShape.snapVtx(mouse, m * matrix(), pos, bound);
}

void Path::snapBnd(const Vector &mouse, const Matrix &m,
		   Vector &pos, double &bound) const
{
  iShape.snapBnd(mouse, m * matrix(), pos, bound);
}

//! Set whether object will be stroked and filled.
void Path::setPathMode(TPathMode pm)
{
  if (iPathMode == EStrokedOnly && pm != EStrokedOnly)
    iFill = Attribute::WHITE();
  if (iPathMode == EFilledOnly && pm != EFilledOnly)
    iStroke = Attribute::BLACK();
  iPathMode = pm;
}

//! Set stroke color.
void Path::setStroke(Attribute stroke)
{
  iStroke = stroke;
}

//! Set fill color.
void Path::setFill(Attribute fill)
{
  iFill = fill;
}

//! Set tiling pattern of the object.
void Path::setTiling(Attribute til)
{
  iTiling = til;
}

//! Set opacity of the object.
void Path::setOpacity(Attribute opaq)
{
  iOpacity = opaq;
}

//! Set pen.
void Path::setPen(Attribute pen)
{
  iPen = pen;
}

//! Set dash style.
void Path::setDashStyle(Attribute dash)
{
  iDashStyle = dash;
}

//! Set forward arrow.
void Path::setArrow(bool arrow, Attribute shape, Attribute size)
{
  iHasFArrow = arrow;
  iFArrowShape = shape;
  iFArrowSize = size;
}

//! Set backward arrow (if the object can take it).
void Path::setRarrow(bool arrow, Attribute shape, Attribute size)
{
  iHasRArrow = arrow;
  iRArrowShape = shape;
  iRArrowSize = size;
}

//! Set line cap style.
void Path::setLineCap(TLineCap s)
{
  iLineCap = s;
}

//! Set line join style.
void Path::setLineJoin(TLineJoin s)
{
  iLineJoin = s;
}

//! Set fill rule.
void Path::setFillRule(TFillRule s)
{
  iFillRule = s;
}

void Path::checkStyle(const Cascade *sheet, AttributeSeq &seq) const
{
  if (iPathMode <= EStrokedAndFilled)
    checkSymbol(EColor, iStroke, sheet, seq);
  if (iPathMode >= EStrokedAndFilled)
    checkSymbol(EColor, iFill, sheet, seq);
  checkSymbol(EDashStyle, iDashStyle, sheet, seq);
  checkSymbol(EPen, iPen, sheet, seq);
  checkSymbol(EArrowSize, iFArrowSize, sheet, seq);
  checkSymbol(EArrowSize, iRArrowSize, sheet, seq);
  checkSymbol(ESymbol, iFArrowShape, sheet, seq);
  checkSymbol(ESymbol, iRArrowShape, sheet, seq);
  checkSymbol(EOpacity, iOpacity, sheet, seq);
}

bool Path::setAttribute(Property prop, Attribute value,
			Attribute nStroke, Attribute nFill)
{
  switch (prop) {
  case EPropPathMode:
    if (value.pathMode() != pathMode()) {
      TPathMode old = pathMode();
      setPathMode(value.pathMode());
      if (old == EStrokedOnly)
	setFill(nFill);
      if (old == EFilledOnly)
	setStroke(nStroke);
      return true;
    }
    break;
  case EPropStrokeColor:
    if (value != stroke()) {
      setStroke(value);
      return true;
    }
    break;
  case EPropFillColor:
    if (value != fill()) {
      setFill(value);
      return true;
    }
    break;
  case EPropPen:
    if (value != pen()) {
      setPen(value);
      return true;
    }
    break;
  case EPropDashStyle:
    if (value != dashStyle()) {
      setDashStyle(value);
      return true;
    }
    break;
  case EPropTiling:
    if (value != tiling()) {
      setTiling(value);
      return true;
    }
    break;
  case EPropOpacity:
    if (value != opacity()) {
      setOpacity(value);
      return true;
    }
    break;
  case EPropFArrow:
    if (value.boolean() != iHasFArrow) {
      iHasFArrow = value.boolean();
      return true;
    }
    break;
  case EPropRArrow:
    if (value.boolean() != iHasRArrow) {
      iHasRArrow = value.boolean();
      return true;
    }
    break;
  case EPropFArrowSize:
    if (value != iFArrowSize) {
      iFArrowSize= value;
      return true;
    }
    break;
  case EPropRArrowSize:
    if (value != iRArrowSize) {
      iRArrowSize = value;
      return true;
    }
    break;
  case EPropFArrowShape:
    if (value != iFArrowShape) {
      iFArrowShape = value;
      return true;
    }
    break;
  case EPropRArrowShape:
    if (value != iRArrowShape) {
      iRArrowShape = value;
      return true;
    }
    break;
  case EPropLineJoin:
    assert(value.isEnum());
    if (value.lineJoin() != iLineJoin) {
      iLineJoin = value.lineJoin();
      return true;
    }
    break;
  case EPropLineCap:
    assert(value.isEnum());
    if (value.lineCap() != iLineCap) {
      iLineCap = value.lineCap();
      return true;
    }
    break;
  case EPropFillRule:
    assert(value.isEnum());
    if (value.fillRule() != iFillRule) {
      iFillRule = value.fillRule();
      return true;
    }
    break;
  default:
    return Object::setAttribute(prop, value, nStroke, nFill);
  }
  return false;
}

Attribute Path::getAttribute(Property prop)
{
  switch (prop) {
  case EPropPathMode:
    return Attribute(iPathMode);
  case EPropStrokeColor:
    if (iPathMode <= EStrokedAndFilled)
      return stroke();
    else
      return Attribute::UNDEFINED();
  case EPropFillColor:
    if (iPathMode >= EStrokedAndFilled)
      return fill();
    else
      return Attribute::UNDEFINED();
  case EPropPen:
    return pen();
  case EPropDashStyle:
    return dashStyle();
  case EPropOpacity:
    return opacity();
  case EPropTiling:
    return tiling();
  case EPropFArrow:
    return Attribute::Boolean(iHasFArrow);
  case EPropRArrow:
    return Attribute::Boolean(iHasRArrow);
  case EPropFArrowSize:
    return iFArrowSize;
  case EPropRArrowSize:
    return iRArrowSize;
  case EPropFArrowShape:
    return iFArrowShape;
  case EPropRArrowShape:
    return iRArrowShape;
  case EPropLineJoin:
    return Attribute(iLineJoin);
  case EPropLineCap:
    return Attribute(iLineCap);
  case EPropFillRule:
    return Attribute(iFillRule);
  default:
    return Object::getAttribute(prop);
  }
}

// --------------------------------------------------------------------
