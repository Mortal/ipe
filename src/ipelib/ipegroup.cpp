// --------------------------------------------------------------------
// The group object
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

#include "ipegroup.h"
#include "ipepainter.h"
#include "ipetext.h"
#include "ipeshape.h"

using namespace ipe;

/*! \class ipe::Group
  \ingroup obj
  \brief The group object.

  Ipe objects can be grouped together, and the resulting composite can
  be used like any Ipe object.

  This is an application of the "Composite" pattern.
*/

//! Create empty group (objects can be added later).
Group::Group() : Object()
{
  iImp = new Imp;
  iImp->iRefCount = 1;
  iImp->iPinned = ENoPin;
}

//! Create empty group with these attributes (objects can be added later).
Group::Group(const XmlAttributes &attr)
  : Object(attr)
{
  iImp = new Imp;
  iImp->iRefCount = 1;
  iImp->iPinned = ENoPin;
  String str;
  if (attr.has("clip", str)) {
    Shape clip;
    if (clip.load(str))
      iClip = clip;
  }
}

//! Copy constructor. Constant time --- components are not copied!
Group::Group(const Group &rhs)
  : Object(rhs)
{
  iImp = rhs.iImp;
  iImp->iRefCount++;
  iClip = rhs.iClip;
}

//! Destructor.
Group::~Group()
{
  if (iImp->iRefCount == 1) {
    for (List::iterator it = iImp->iObjects.begin();
	 it != iImp->iObjects.end(); ++it) {
      delete *it;
      *it = 0;
    }
    delete iImp;
  } else
    iImp->iRefCount--;
}

//! Assignment operator (constant-time).
Group &Group::operator=(const Group &rhs)
{
  if (this != &rhs) {
    if (iImp->iRefCount == 1)
      delete iImp;
    else
      iImp->iRefCount--;
    iImp = rhs.iImp;
    iImp->iRefCount++;
    iClip = rhs.iClip;
    Object::operator=(rhs);
  }
  return *this;
}

//! Clone a group object (constant-time).
Object *Group::clone() const
{
  return new Group(*this);
}

//! Return pointer to this object.
Group *Group::asGroup()
{
  return this;
}

Object::Type Group::type() const
{
  return EGroup;
}

//! Add an object.
/*! Takes ownership of the object.
  This will panic if the object shares its implementation!
  The method is only useful right after construction of the group. */
void Group::push_back(Object *obj)
{
  assert(iImp->iRefCount == 1);
  iImp->iObjects.push_back(obj);
  iImp->iPinned = TPinned(iImp->iPinned | obj->pinned());
}

//! Save all the components, one by one, in XML format.
void Group::saveComponentsAsXml(Stream &stream) const
{
  for (const_iterator it = begin(); it != end(); ++it)
    (*it)->saveAsXml(stream, String());
}

//! Call visitGroup of visitor.
void Group::accept(Visitor &visitor) const
{
  visitor.visitGroup(this);
}

void Group::saveAsXml(Stream &stream, String layer) const
{
  stream << "<group";
  saveAttributesAsXml(stream, layer);
  if (iClip.countSubPaths()) {
    stream << " clip=\"";
    iClip.save(stream);
    stream << "\"";
  }
  stream << ">\n";
  saveComponentsAsXml(stream);
  stream << "</group>\n";
}

void Group::draw(Painter &painter) const
{
  painter.pushMatrix();
  painter.transform(matrix());
  painter.untransform(transformations());
  if (iClip.countSubPaths()) {
    painter.push();
    painter.newPath();
    iClip.draw(painter);
    painter.addClipPath();
  }
  for (const_iterator it = begin(); it != end(); ++it)
    (*it)->draw(painter);
  if (iClip.countSubPaths())
    painter.pop();
  painter.popMatrix();
}

void Group::drawSimple(Painter &painter) const
{
  painter.pushMatrix();
  painter.transform(matrix());
  painter.untransform(transformations());
  if (iClip.countSubPaths()) {
    painter.push();
    painter.newPath();
    iClip.draw(painter);
    painter.addClipPath();
  }
  for (const_iterator it = begin(); it != end(); ++it)
    (*it)->drawSimple(painter);
  if (iClip.countSubPaths())
    painter.pop();
  painter.popMatrix();
}

void Group::addToBBox(Rect &box, const Matrix &m, bool cp) const
{
  Matrix m1 = m * matrix();
  Rect tbox;
  for (const_iterator it = begin(); it != end(); ++it) {
    (*it)->addToBBox(tbox, m1, cp);
  }
  // now clip to clipping path
  if (iClip.countSubPaths()) {
    Rect cbox;
    iClip.addToBBox(cbox, m1, false);
    tbox.clipTo(cbox);
  }
  box.addRect(tbox);
}

//! Return total pinning status of group and its elements.
TPinned Group::pinned() const
{
  return TPinned(Object::pinned() | iImp->iPinned);
}

double Group::distance(const Vector &v, const Matrix &m, double bound) const
{
  double d = bound;
  double d1;
  Matrix m1 = m * matrix();
  for (const_iterator it = begin(); it != end(); ++it) {
    if ((d1 = (*it)->distance(v, m1, d)) < d)
      d = d1;
  }
  return d;
}

void Group::snapVtx(const Vector &mouse, const Matrix &m,
		    Vector &pos, double &bound) const
{
  Matrix m1 = m * matrix();
  for (const_iterator it = begin(); it != end(); ++it)
    (*it)->snapVtx(mouse, m1, pos, bound);
}

void Group::snapBnd(const Vector &mouse, const Matrix &m,
		    Vector &pos, double &bound) const
{
  Matrix m1 = m * matrix();
  for (const_iterator it = begin(); it != end(); ++it)
    (*it)->snapBnd(mouse, m1, pos, bound);
}

void Group::checkStyle(const Cascade *sheet,
		       AttributeSeq &seq) const
{
  for (const_iterator it = begin(); it != end(); ++it)
    (*it)->checkStyle(sheet, seq);
}

//! Set clip path for this group.
/*! Any previously set clip path is deleted. */
void Group::setClip(const Shape &clip)
{
  iClip = clip;
}

//! Create private implementation.
void Group::detach()
{
  Imp *old = iImp;
  iImp = new Imp;
  iImp->iRefCount = 1;
  for (const_iterator it = old->iObjects.begin();
       it != old->iObjects.end(); ++it)
    iImp->iObjects.push_back((*it)->clone());

}

//! Set attribute on all children.
bool Group::setAttribute(Property prop, Attribute value,
			 Attribute stroke, Attribute fill)
{
  if (prop == EPropPinned || prop == EPropTransformations)
    return Object::setAttribute(prop, value, stroke, fill);
  // all others are handled by elements themselves
  detach();
  bool result = false;
  for (List::iterator it = iImp->iObjects.begin();
       it != iImp->iObjects.end(); ++it)
    result |= (*it)->setAttribute(prop, value, stroke, fill);
  return result;
}

// --------------------------------------------------------------------
