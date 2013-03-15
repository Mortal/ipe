// --------------------------------------------------------------------
// ipetoipe
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

#include "ipedoc.h"

using ipe::Document;
using ipe::String;

static int topdforps(Document *doc, String src, String dst,
		     Document::TFormat fm, uint flags,
		     int fromPage = -1, int toPage = -1, int viewNo = -1)
{
  if (fm == Document::EEps) {
    if (viewNo < 0 && doc->countTotalViews() > 1) {
      fprintf(stderr, "The document contains %d views, "
	      "cannot convert to EPS format.\n", doc->countTotalViews());
      return 1;
    }

    bool trans = doc->hasTransparency();
    bool grad = doc->hasGradients();

    if (!trans && !grad) {
      int res = doc->runLatex();
      if (res)
	return res;
    }

    bool ttf = doc->hasTrueTypeFonts();

    if (trans || grad || ttf) {
      fprintf(stderr, "The document uses the following features:\n");
      if (trans)
	fprintf(stderr, " - transparency\n");
      if (grad)
	fprintf(stderr, " - gradients\n");
      if (ttf)
	fprintf(stderr, " - Truetype fonts\n");
      fprintf(stderr,
	      "Ipe does not support these features for Postscript output.\n"
	      "You can save as PDF, and use pdftops to create a "
	      "Postscript file.\n");
      return 1;
    }
  } else {
    int res = doc->runLatex();
    if (res) return res;
  }

  bool result = false;
  if (viewNo >= 0) {
    result = doc->exportView(dst.z(), fm, flags, fromPage, viewNo);
  } else if (toPage >= 0) {
    result = doc->exportPages(dst.z(), flags, fromPage, toPage);
  } else {
    result = doc->save(dst.z(), fm, flags);
  }
  if (!result) {
    fprintf(stderr, "Failed to save or export document!\n");
    return 1;
  }

  if (flags & Document::EExport)
    fprintf(stderr, "Warning: the exported file contains no Ipe markup.\n"
	    "It cannot be read by Ipe - make sure you keep the original!\n");
  return 0;
}

static void usage()
{
  fprintf(stderr,
	  "Usage: ipetoipe ( -xml | -eps | -pdf ) <options> "
	  "infile [ outfile ]\n"
	  "Ipetoipe converts between the different Ipe file formats.\n"
	  " -export      : output contains no Ipe markup.\n"
	  " -pages <n-m> : export only these pages (implies -export).\n"
	  " -view <p-v>  : export only this view (implies -export).\n"
	  " -markedview  "
	  ": export only marked views on marked pages (implies -export).\n"
	  " -runlatex    : run Latex even for XML output.\n"
	  " -nocolor     : avoid any color commands in EPS output.\n"
	  " -nozip:      : do not compress PDF streams.\n"
	  " -styleupdate : update style sheets.\n"
	  );
  exit(1);
}

int main(int argc, char *argv[])
{
  ipe::Platform::initLib(ipe::IPELIB_VERSION);

  // ensure at least two arguments (handles -help as well :-)
  if (argc < 3)
    usage();

  Document::TFormat frm = Document::EUnknown;
  if (!strcmp(argv[1], "-xml"))
    frm = Document::EXml;
  else if (!strcmp(argv[1], "-eps"))
    frm = Document::EEps;
  else if (!strcmp(argv[1], "-pdf"))
    frm = Document::EPdf;

  if (frm == Document::EUnknown)
    usage();

  uint flags = Document::ESaveNormal;
  bool runLatex = false;
  bool styleUpdate = false;
  int fromPage = -1;
  int toPage = -1;
  int viewNo = -1;
  int i = 2;

  String infile;
  String outfile;

  while (i < argc) {

    if (!strcmp(argv[i], "-export")) {
      flags |= Document::EExport;
      ++i;
    } else if (!strcmp(argv[i], "-view")) {
      if (sscanf(argv[i+1], "%d-%d", &fromPage, &viewNo) != 2)
	usage();
      flags |= Document::EExport;
      i += 2;
    } else if (!strcmp(argv[i], "-pages")) {
      if (sscanf(argv[i+1], "%d-%d", &fromPage, &toPage) != 2)
	usage();
      flags |= Document::EExport;
      i += 2;
    } else if (!strcmp(argv[i], "-markedview")) {
      flags |= Document::EMarkedView;
      flags |= Document::EExport;
      ++i;
    } else if (!strcmp(argv[i], "-runlatex")) {
      runLatex = true;
      ++i;
    } else if (!strcmp(argv[i], "-nocolor")) {
      flags |= Document::ENoColor;
      ++i;
    } else if (!strcmp(argv[i], "-nozip")) {
      flags |= Document::ENoZip;
      ++i;
    } else if (!strcmp(argv[i], "-styleupdate")) {
      styleUpdate = true;
      ++i;
    } else {
      // last one or two arguments must be filenames
      infile = argv[i];
      ++i;
      if (i < argc) {
	outfile = argv[i];
	++i;
      }
      if (i != argc)
	usage();
    }
  }

  if (infile.empty())
    usage();

  if ((flags & Document::EExport) && frm == Document::EXml) {
    fprintf(stderr, "-export only available with -pdf and -eps.\n");
    exit(1);
  }

  if (toPage >= 0 && frm != Document::EPdf) {
    fprintf(stderr, "-pages only available with -pdf.\n");
    exit(1);
  }

  if (toPage >= 0 && viewNo >= 0) {
    fprintf(stderr, "cannot specify both -pages and -view.\n");
    exit(1);
  }

  if (outfile.empty()) {
    outfile = infile;
    String ext = infile.right(4);
    if (ext == ".ipe" || ext == ".pdf" || ext == ".xml" || ext == ".eps")
      outfile = infile.left(infile.size() - 4);
    switch (frm) {
    case Document::EXml:
      outfile += ".ipe";
      break;
    case Document::EPdf:
      outfile += ".pdf";
      break;
    case Document::EEps:
      outfile += ".eps";
    default:
      break;
    }
    if (outfile == infile) {
      fprintf(stderr, "Cannot guess output filename.\n");
      exit(1);
    }
  }

  IpeAutoPtr<Document> doc(Document::loadWithErrorReport(infile.z()));

  if (!doc.ptr())
    return 1;

  if (styleUpdate) {
    String curdir = ipe::Platform::currentDirectory();
    fprintf(stderr, "%s", doc.ptr()->cascade()->update(curdir).z());
  }

  fprintf(stderr, "Document %s has %d pages (%d views)\n",
	  infile.z(), doc->countPages(), doc->countTotalViews());

  // check frompage, topage, viewno
  if (toPage >= 0) {
    if (fromPage <= 0 || fromPage > toPage || toPage > doc->countPages()) {
      fprintf(stderr, "incorrect -pages specification.\n");
      exit(1);
    }
    --fromPage;
    --toPage;
  } else if (fromPage >= 0) {
    if (fromPage <= 0 || fromPage > doc->countPages() ||
	viewNo <= 0 || viewNo > doc->page(fromPage - 1)->countViews()) {
      fprintf(stderr, "incorrect -view specification.\n");
      exit(1);
    }
    --fromPage;
    --viewNo;
  }

  char buf[64];
  sprintf(buf, "ipetoipe %d.%d.%d",
	  ipe::IPELIB_VERSION / 10000,
	  (ipe::IPELIB_VERSION / 100) % 100,
	  ipe::IPELIB_VERSION % 100);
  Document::SProperties props = doc->properties();
  props.iCreator = buf;
  doc->setProperties(props);

  switch (frm) {
  case Document::EXml:
    if (runLatex)
      return topdforps(doc.ptr(), infile, outfile, Document::EXml, flags);
    else
      doc->save(outfile.z(), Document::EXml, Document::ESaveNormal);
  default:
    return 0;

  case Document::EPdf:
    return topdforps(doc.ptr(), infile, outfile, Document::EPdf,
		     flags, fromPage, toPage, viewNo);

  case Document::EEps:
    return topdforps(doc.ptr(), infile, outfile, Document::EEps,
		     flags, fromPage, toPage, viewNo);
  }
}

// --------------------------------------------------------------------
