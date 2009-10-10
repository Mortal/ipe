// -*- C++ -*-
// --------------------------------------------------------------------
// Snapping.
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

#ifndef IPESNAP_H
#define IPESNAP_H

#include "ipegeo.h"

// --------------------------------------------------------------------

namespace ipe {

  class Page;

  class Snap {
  public:
    //! The different snap modes as bitmasks.
    enum TSnapModes { ESnapNone = 0,
		      ESnapVtx = 1, ESnapBd = 2, ESnapInt = 4,
		      ESnapGrid = 8, ESnapAngle = 0x10,
		      ESnapAuto = 0x20 };

    int iSnap;           //!< Activated snapping modes (TSnapModes)
    bool iGridVisible;   //!< Is the grid visible?
    int iGridSize;       //!< Snap grid spacing.
    double iAngleSize;   //!< Angle for angular snapping.
    int iSnapDistance;   //!< Snap distance (in pixels).
    bool iWithAxes;      //!< Show coordinate system?
    Vector iOrigin;      //!< Origin of coordinate system
    Angle iDir;          //!< Direction of x-axis

    bool intersectionSnap(Vector &pos, const Page *page,
			  double snapDist) const;
    bool snapAngularIntersection(Vector &pos, const Line &l,
				 const Page *page,
				 double snapDist) const;
    bool simpleSnap(Vector &pos, const Page *page,
		    double snapDist) const;
    bool snap(Vector &pos, const Page *page, double snapDist,
	      Vector *autoOrg = 0) const;
    Line getLine(const Vector &mouse, const Vector &base) const;
    bool setEdge(const Vector &pos, const Page *page);
  };

} // namespace

// --------------------------------------------------------------------
#endif
