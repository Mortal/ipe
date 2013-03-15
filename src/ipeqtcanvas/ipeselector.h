// -*- C++ -*-
// --------------------------------------------------------------------
// ipeqt::PageSelector
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

#ifndef IPESELECTOR_H
#define IPESELECTOR_H

#include "ipedoc.h"

#include <QListWidget>

using namespace ipe;

// --------------------------------------------------------------------

namespace ipeqt {

  class PageSelector : public QListWidget {
    Q_OBJECT

  public:
    PageSelector(Document *doc, int page, int width, QWidget *parent = 0);

    //! Return index of selected view or page.
    int selectedIndex() const { return iSelectedPage; }

    static int selectPageOrView(Document *doc, int page = -1,
				int pageWidth = 240,
				int width = 600, int height = 480);

  signals:
    void selectionMade();

  private slots:
    void pageSelected(const QModelIndex &index);

  private:
    Document *iDoc;
    int iSelectedPage;
  };

} // namespace

// --------------------------------------------------------------------
#endif