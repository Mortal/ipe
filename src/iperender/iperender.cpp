// --------------------------------------------------------------------
// iperender
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

#include "ipeutils.h"
#include "ipedoc.h"
#include "ipefonts.h"

#include "ipecairopainter.h"

#include <cstdio>
#include <cstdlib>

#include <cairo.h>
#include <cairo-svg.h>
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif

enum TargetFormat { ESVG, EPNG, EPS, EPDF };

using ipe::Document;
using ipe::Page;

// --------------------------------------------------------------------

static void render(TargetFormat fm, const char *dst, const Document *doc,
		   const Page *page, int view, double zoom)
{
  ipe::Rect bbox = page->pageBBox(doc->cascade());
  int wid = int(bbox.width() * zoom + 1);
  int ht = int(bbox.height() * zoom + 1);

  unsigned long *data = 0;
  cairo_surface_t* surface = 0;

  if (fm == EPNG) {
    data = new unsigned long[wid * ht];
    for (unsigned long *p = data; p < data + wid * ht; )
      *p++ = 0xffffffff;
    surface = cairo_image_surface_create_for_data((uchar *) data,
						  CAIRO_FORMAT_ARGB32,
						  wid, ht, wid * 4);
  } else if (fm == ESVG) {
    surface = cairo_svg_surface_create(dst, wid, ht);
#ifdef CAIRO_HAS_PS_SURFACE
  } else if (fm == EPS) {
    surface = cairo_ps_surface_create(dst, wid, ht);
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  } else if (fm == EPDF) {
    surface = cairo_pdf_surface_create(dst, wid, ht);
#endif
  }

  cairo_t *cc = cairo_create(surface);
  cairo_scale(cc, zoom, -zoom);
  cairo_translate(cc, -bbox.topLeft().x, -bbox.topLeft().y);

  IpeAutoPtr<ipe::Fonts> fonts(ipe::Fonts::New(doc->fontPool()));
  ipe::CairoPainter painter(doc->cascade(), fonts.ptr(), cc, zoom, true);
  // painter.Transform(IpeLinear(zoom, 0, 0, -zoom));
  // painter.Translate(-bbox.TopLeft());
  painter.pushMatrix();
  for (int i = 0; i < page->count(); ++i) {
    if (page->objectVisible(view, i))
      page->object(i)->draw(painter);
  }
  painter.popMatrix();
  cairo_surface_flush(surface);
  cairo_show_page(cc);

  if (fm == EPNG)
    cairo_surface_write_to_png(surface, dst);

  cairo_destroy(cc);
  cairo_surface_destroy(surface);
  delete [] data;
}

// --------------------------------------------------------------------

static int renderPage(TargetFormat fm, const char *src, const char *dst,
		      int pageNum, int viewNum, double zoom)
{
  Document *doc = Document::loadWithErrorReport(src);

  if (!doc)
    return 1;

  if (pageNum < 1 || pageNum > doc->countPages()) {
    fprintf(stderr,
	    "The document contains %d pages, cannot convert page %d.\n",
	    doc->countPages(), pageNum);
    delete doc;
    return 1;
  }

  if (doc->runLatex()) {
    delete doc;
    return 1;
  }

  const Page *page = doc->page(pageNum - 1);
  render(fm, dst, doc, page, viewNum - 1, zoom);
  delete doc;
  return 0;
}

// --------------------------------------------------------------------

static void usage()
{
  fprintf(stderr,
	  "Usage: iperender [ -svg | -png ] "
	  "[ -page <page> ] [ -view <view> ] [ -resolution <dpi> ] "
	  "infile outfile\n"
	  "Iperender saves a single page of the Ipe document in some formats.\n"
	  " -page       : page to save (default 1).\n"
	  " -view       : view to save (default 1).\n"
	  " -resolution : resolution for png format (default 72.0 ppi).\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  ipe::Platform::initLib(ipe::IPELIB_VERSION);

  // ensure at least three arguments (handles -help as well :-)
  if (argc < 4)
    usage();

  TargetFormat fm = EPNG;
  if (!strcmp(argv[1], "-png"))
    fm = EPNG;
#ifdef CAIRO_HAS_PS_SURFACE
  else if (!strcmp(argv[1], "-ps"))
    fm = EPS;
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  else if (!strcmp(argv[1], "-pdf"))
    fm = EPDF;
#endif
  else if (!strcmp(argv[1], "-svg"))
    fm = ESVG;
  else
    usage();

  int page = 1;
  int view = 1;
  double dpi = 72.0;
  int i = 2;

  if (!strcmp(argv[i], "-page")) {
    page = ipe::Lex(ipe::String(argv[i+1])).getInt();
    i += 2;
  }

  if (!strcmp(argv[i], "-view")) {
    view = ipe::Lex(ipe::String(argv[i+1])).getInt();
    i += 2;
  }

  if (!strcmp(argv[i], "-resolution")) {
    dpi = ipe::Lex(ipe::String(argv[i+1])).getDouble();
    i += 2;
  }

  // remaining arguments must be two filenames
  if (argc != i + 2)
    usage();

  const char *src = argv[i];
  const char *dst = argv[i+1];

  return renderPage(fm, src, dst, page, view, dpi / 72.0);
}

// --------------------------------------------------------------------