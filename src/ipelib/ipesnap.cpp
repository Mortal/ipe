// --------------------------------------------------------------------
// Snapping
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

#include "ipesnap.h"
#include "ipepage.h"
#include "ipegroup.h"
#include "ipereference.h"
#include "ipepath.h"

using namespace ipe;

/*! \defgroup high Ipe Utilities
  \brief Classes to manage Ipe documents and objects.

  This module contains classes used in the implementation of the Ipe
  program itself.  The only classes from this module you may be
  interested in are Visitor (which is essential to traverse an Ipe
  object structure), and perhaps Snap (if you are writing an Ipelet
  whose behavior depends on the current snap setting in the Ipe
  program).
*/

/*! \class ipe::Snap
  \ingroup high
  \brief Performs snapping operations, and stores snapping state.

*/

// --------------------------------------------------------------------

class CollectSegs : public Visitor {
public:
  CollectSegs(const Vector &mouse, double snapDist, const Page *page);

  virtual void visitGroup(const Group *obj);
  virtual void visitPath(const Path *obj);

public:
  std::vector<Segment> iSegs;
  std::vector<Bezier> iBeziers;
  std::vector<Arc> iArcs;

private:
  std::vector<Matrix> iMatrices;
  Vector iMouse;
  double iDist;
};

CollectSegs::CollectSegs(const Vector &mouse, double snapDist,
			 const Page *page)
  : iMouse(mouse), iDist(snapDist)
{
  iMatrices.push_back(Matrix()); // identity matrix
  for (int i = 0; i < page->count(); ++i) {
    if (page->hasSnapping(page->layerOf(i)))
      page->object(i)->accept(*this);
  }
}

void CollectSegs::visitGroup(const Group *obj)
{
  iMatrices.push_back(iMatrices.back() * obj->matrix());
  for (Group::const_iterator it = obj->begin(); it != obj->end(); ++it)
    (*it)->accept(*this);
  iMatrices.pop_back();
}

// TODO: use bounding boxes for subsegs/beziers to speed up ?
void CollectSegs::visitPath(const Path *obj)
{
  Bezier b;
  Arc arc;
  Matrix m = iMatrices.back() * obj->matrix();
  for (int i = 0; i < obj->shape().countSubPaths(); ++i) {
    const SubPath *sp = obj->shape().subPath(i);
    switch (sp->type()) {
    case SubPath::EEllipse:
      if (sp->distance(iMouse, m, iDist) < iDist)
	iArcs.push_back(m * Arc(sp->asEllipse()->matrix()));
      break;
    case SubPath::EClosedSpline: {
      std::vector<Bezier> bez;
      sp->asClosedSpline()->beziers(bez);
      for (uint i = 0; i < bez.size(); ++i) {
	b = m * bez[i];
	if (b.distance(iMouse, iDist) < iDist)
	  iBeziers.push_back(b);
      }
      break; }
    case SubPath::ECurve: {
      const Curve *ssp = sp->asCurve();
      int ns = ssp->closed() ? -1 : 0;
      Vector u[2];
      for (int j = ns; j < ssp->countSegments(); ++j) {
	CurveSegment seg = j < 0 ? ssp->closingSegment(u) : ssp->segment(j);
	switch (seg.type()) {
	case CurveSegment::ESegment:
	  if (seg.distance(iMouse, m, iDist) < iDist)
	    iSegs.push_back(Segment(m * seg.cp(0), m * seg.cp(1)));
	  break;
	case CurveSegment::EBezier:
	case CurveSegment::EQuad:
	  b = m * seg.bezier();
	  if (b.distance(iMouse, iDist) < iDist)
	    iBeziers.push_back(b);
	  break;
	case CurveSegment::EArc:
	  arc = m * seg.arc();
	  if (arc.distance(iMouse, iDist) < iDist)
	    iArcs.push_back(arc);
	  break;
	case CurveSegment::ESpline: {
	  std::vector<Bezier> bez;
	  seg.beziers(bez);
	  for (uint i = 0; i < bez.size(); ++i) {
	    b = m * bez[i];
	    if (b.distance(iMouse, iDist) < iDist)
	      iBeziers.push_back(b);
	  }
	  break; }
	}
      }
      break; }
    }
  }
}

// --------------------------------------------------------------------

/*! Find line through \a base with slope determined by angular snap
  size and direction. */
Line Snap::getLine(const Vector &mouse, const Vector &base) const
{
  Angle alpha = iDir;
  Vector d = mouse - base;

  if (d.len() > 2.0) {
    alpha = d.angle() - iDir;
    alpha.normalize(0.0);
    alpha = iAngleSize * int(alpha / iAngleSize + 0.5) + iDir;
  }
  return Line(base, Vector(alpha));
}

//! Perform intersection snapping.
bool Snap::intersectionSnap(Vector &pos, const Page *page,
			    double snapDist) const
{
  CollectSegs segs(pos, snapDist, page);

  Vector v;
  std::vector<Vector> pts;

  // 1. seg-seg intersections
  for (uint i = 0; i < segs.iSegs.size(); ++i) {
    for (uint j = i + 1; j < segs.iSegs.size(); ++j) {
      if (segs.iSegs[i].intersects(segs.iSegs[j], v))
	pts.push_back(v);
    }
  }

  // 2. bezier-bezier and bezier-seg intersections
  for (uint i = 0; i < segs.iBeziers.size(); ++i) {
    for (uint j = i + 1; j < segs.iBeziers.size(); ++j)
      segs.iBeziers[i].intersect(segs.iBeziers[j], pts);
    for (uint j = 0; j < segs.iSegs.size(); ++j)
      segs.iBeziers[i].intersect(segs.iSegs[j], pts);
  }

  // 3. arc-arc, arc-bezier, and arc-segment intersections
  for (uint i = 0; i < segs.iArcs.size(); ++i) {
    for (uint j = i+1; j < segs.iArcs.size(); ++j)
      segs.iArcs[i].intersect(segs.iArcs[j], pts);
    for (uint j = 0; j < segs.iBeziers.size(); ++j)
      segs.iArcs[i].intersect(segs.iBeziers[j], pts);
    for (uint j = 0; j < segs.iSegs.size(); ++j)
      segs.iArcs[i].intersect(segs.iSegs[j], pts);
  }

  double d = snapDist;
  Vector pos1 = pos;
  double d1;
  for (uint k = 0; k < pts.size(); ++k) {
    if ((d1 = (pos - pts[k]).len()) < d) {
      d = d1;
      pos1 = pts[k];
    }
  }

  if (d < snapDist) {
    pos = pos1;
    return true;
  }
  return false;
}

//! Perform snapping to intersection of angular line and pos.
bool Snap::snapAngularIntersection(Vector &pos, const Line &l,
				   const Page *page,
				   double snapDist) const
{
  CollectSegs segs(pos, snapDist, page);

  std::vector<Vector> pts;
  Vector v;

  for (std::vector<Segment>::const_iterator it = segs.iSegs.begin();
       it != segs.iSegs.end(); ++it) {
    if (it->intersects(l, v))
      pts.push_back(v);
  }
  for (std::vector<Arc>::const_iterator it = segs.iArcs.begin();
       it != segs.iArcs.end(); ++it) {
    it->intersect(l, pts);
  }
  for (std::vector<Bezier>::const_iterator it = segs.iBeziers.begin();
       it != segs.iBeziers.end(); ++it) {
    it->intersect(l, pts);
  }

  double d = snapDist;
  Vector pos1 = pos;
  double d1;

  for (uint i = 0; i < pts.size(); ++i) {
    if ((d1 = (pos - pts[i]).len()) < d) {
      d = d1;
      pos1 = pts[i];
    }
  }

  if (d < snapDist) {
    pos = pos1;
    return true;
  }
  return false;
}

//! Tries vertex, intersection, boundary, and grid snapping.
/*! If snapping occurred, \a pos is set to the new user space position. */
bool Snap::simpleSnap(Vector &pos, const Page *page, double snapDist) const
{
  double d = snapDist;
  Vector fifi = pos;

  // highest priority: vertex snapping
  if (iSnap & ESnapVtx) {
    for (int i = 0; i < page->count(); ++i) {
      if (page->hasSnapping(page->layerOf(i)))
	page->snapVtx(i, pos, fifi, d);
    }
  }
  if (iSnap & ESnapInt) {
    intersectionSnap(pos, page, d);
  }

  // Return if snapping has occurred
  if (d < snapDist) {
    pos = fifi;
    return true;
  }

  // boundary snapping
  if (iSnap & ESnapBd) {
    for (int i = 0; i < page->count(); ++i) {
      if (page->hasSnapping(page->layerOf(i)))
	page->snapBnd(i, pos, fifi, d);
    }
    if (d < snapDist) {
      pos = fifi;
      return true;
    }
  }

  // grid snapping: always occurs
  if (iSnap & ESnapGrid) {
    int grid = iGridSize;
    fifi.x = grid * int(pos.x / grid + 0.5);
    fifi.y = grid * int(pos.y / grid + 0.5);
    pos = fifi;
    return true;
  }
  return false;
}

//! Performs snapping of position \a pos.
/*! Returns \c true if snapping occurred. In that case \a pos is set
  to the new user space position.

  Automatic angular snapping occurs if \a autoOrg is not null --- the
  value is then used as the origin for automatic angular snapping.
*/
bool Snap::snap(Vector &pos, const Page *page, double snapDist,
		Vector *autoOrg) const
{
  // automatic angular snapping and angular snapping both on?
  if (autoOrg && (iSnap & ESnapAuto) && (iSnap & ESnapAngle)) {
    // only one possible point!
    Line angular = getLine(pos, iOrigin);
    Line automat = getLine(pos, *autoOrg);
    Vector v;
    if (angular.intersects(automat, v)) {
      pos = v;
      return true;
    }
    // if the two lines do not intersect, use following case
  }

  // case of only one angular snapping mode
  if ((iSnap & ESnapAngle) || (autoOrg && (iSnap & ESnapAuto))) {
    Vector org;
    if (iSnap & ESnapAngle)
      org = iOrigin;
    else
      org = *autoOrg;
    Line l = getLine(pos, org);
    pos = l.project(pos);
    if (iSnap & ESnapBd)
      snapAngularIntersection(pos, l, page, snapDist);
    return true;
  }

  // we are not in any angular snapping mode
  return simpleSnap(pos, page, snapDist);
}

//! Set axis origin and direction from edge near mouse.
/*! Returns \c true if successful. */
bool Snap::setEdge(const Vector &pos, const Page *page)
{
  CollectSegs segs(pos, 0.001, page);

  if (!segs.iSegs.empty()) {
    Segment seg = segs.iSegs.front();
    Line l = seg.line();
    iOrigin = l.project(pos);
    Vector dir = l.dir();
    if ((iOrigin - seg.iP).len() > (iOrigin - seg.iQ).len())
      dir = -dir;
    iDir = dir.angle();
    return true;
  }

  // TODO should also support Arc and Bezier
  return false;
}

// --------------------------------------------------------------------
