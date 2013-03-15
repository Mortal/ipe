// --------------------------------------------------------------------
// Making thumbnails of Ipe pages
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

#include "ipethumbs.h"

#include "ipecairopainter.h"
#include <cairo.h>

using namespace ipe;

// --------------------------------------------------------------------

Thumbnail::Thumbnail(const Document *doc, int width)
{
  iDoc = doc;
  iWidth = width;

  iLayout = iDoc->cascade()->findLayout();
  Rect paper = iLayout->paper();
  iHeight = int(iWidth * paper.height() / paper.width());
  iZoom = iWidth / paper.width();
  // ipeDebug("%g %g -> %d %d", paper.width(), paper.height(), iWidth, iHeight);

  iFonts = Fonts::New(doc->fontPool());
}

Thumbnail::~Thumbnail()
{
  delete iFonts;
}

Buffer Thumbnail::render(const Page *page, int view)
{
  Buffer buffer(iWidth * iHeight * 4);
  memset(buffer.data(), 0xff, iWidth * iHeight * 4);

  cairo_surface_t* surface =
    cairo_image_surface_create_for_data((uchar *) buffer.data(),
					CAIRO_FORMAT_ARGB32,
					iWidth, iHeight, iWidth * 4);
  cairo_t *cc = cairo_create(surface);
  cairo_scale(cc, iZoom, -iZoom);
  Vector offset = iLayout->iOrigin - iLayout->paper().topLeft();
  cairo_translate(cc, offset.x, offset.y);

  CairoPainter painter(iDoc->cascade(), iFonts, cc, iZoom, true);
  painter.pushMatrix();
  for (int i = 0; i < page->count(); ++i) {
    if (page->objectVisible(view, i))
      page->object(i)->draw(painter);
  }
  painter.popMatrix();
  cairo_surface_flush(surface);
  cairo_show_page(cc);
  cairo_destroy(cc);
  cairo_surface_destroy(surface);

  return buffer;
}

// --------------------------------------------------------------------
