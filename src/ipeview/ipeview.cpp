// --------------------------------------------------------------------
// Main function
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

#include "ipeview.h"
#include "ipetool.h"

#include <cstdio>
#include <cstdlib>
#include <locale.h>

#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QMenuBar>
#include <QKeySequence>

using namespace ipe;
using namespace ipeqt;

// --------------------------------------------------------------------

IpeAction::IpeAction(int cmd, const QString &text, const char *shortcut,
		     QObject *parent)
  : QAction(text, parent)
{
  iCommand = cmd;
  connect(this, SIGNAL(triggered()), SLOT(forwardTrigger()));
  connect(this, SIGNAL(triggered(int)), parent, SLOT(cmd(int)));
  if (shortcut)
    setShortcut(QKeySequence(shortcut));
}

void IpeAction::forwardTrigger()
{
  emit triggered(iCommand);
}

// --------------------------------------------------------------------

AppUi::AppUi(Qt::WFlags f)
  : QMainWindow(0, f)
{
  iDoc = 0;

  iFileMenu = menuBar()->addMenu(tr("&File"));
  iViewMenu = menuBar()->addMenu(tr("&View"));
  iMoveMenu = menuBar()->addMenu(tr("&Move"));

  iFileMenu->addAction(new IpeAction(EOpen, "Open", "Ctrl+O", this));
  iFileMenu->addAction(new IpeAction(EQuit, "Quit", "Ctrl+Q", this));
  iGridVisible = new IpeAction(EGridVisible, "Grid visible", "F12", this);
  iGridVisible->setCheckable(true);
  iViewMenu->addAction(iGridVisible);
  iViewMenu->addAction(new IpeAction(EFullScreen, "Full screen",
				     "F11", this));
  iViewMenu->addAction(new IpeAction(EFitPage, "Fit page", ".", this));
  iViewMenu->addAction(new IpeAction(EZoomIn, "Zoom in", "=", this));
  iViewMenu->addAction(new IpeAction(EZoomOut, "Zoom out", "-", this));
  iMoveMenu->addAction(new IpeAction(ENextView, "Next view",
				     "PgDown", this));
  iMoveMenu->addAction(new IpeAction(EPreviousView, "Previous view",
				     "PgUp", this));

  iCanvas = new Canvas(this);
  setCentralWidget(iCanvas);

  Canvas::Style style = iCanvas->canvasStyle();
  style.pretty = true;
  style.paperClip = true;
  iCanvas->setCanvasStyle(style);

  iSnap.iSnap = 0; // Snap::ESnapGrid;
  iSnap.iGridVisible = false;
  iSnap.iGridSize = 8;
  iSnap.iAngleSize = M_PI / 6.0;
  iSnap.iSnapDistance = 10;
  iSnap.iWithAxes = false;
  iSnap.iOrigin = Vector::ZERO;
  iSnap.iDir = 0;
  iCanvas->setSnap(iSnap);
  // iCanvas->setFifiVisible(true);

  connect(iCanvas, SIGNAL(wheelMoved(int)), this, SLOT(wheelZoom(int)));
  connect(iCanvas, SIGNAL(mouseAction(int)), this, SLOT(startPan(int)));
}

void AppUi::cmd(int cmd)
{
  ipeDebug("Command %d", cmd);
  switch (cmd) {
  case EOpen:
    break;
  case EQuit:
    QApplication::exit();
    break;
  case EGridVisible: {
    Snap snap = iCanvas->snap();
    snap.iGridVisible = !snap.iGridVisible;
    iCanvas->setSnap(snap);
    iGridVisible->setChecked(snap.iGridVisible);
    iCanvas->update();
    break; }
  case EFullScreen: {
    setWindowState(windowState() ^ Qt::WindowFullScreen);
    break; }
  case EFitPage:
    fitBox(iDoc->cascade()->findLayout()->paper());
    break;
  case EZoomIn:
   zoom(+1);
    break;
  case EZoomOut:
    zoom(-1);
    break;
  case ENextView:
    nextView(+1);
    break;
  case EPreviousView:
    nextView(-1);
    break;
  default:
    // unknown action
    return;
  }
}

bool AppUi::load(const char *fname)
{
  Document *doc = Document::loadWithErrorReport(fname);
  if (!doc)
    return false;

  iFileName = String(fname);
  doc->runLatex();

  delete iDoc;
  iDoc = doc;
  iPageNo = 0;
  iViewNo = 0;

  // iDoc->page(iPageNo)->setSelect(0, EPrimarySelected);
  iCanvas->setFontPool(iDoc->fontPool());
  iCanvas->setPage(iDoc->page(iPageNo), iViewNo, iDoc->cascade());
  iCanvas->setPan(Vector(300, 400));
  iCanvas->update();

  updateLabel();
  return true;
}

void AppUi::updateLabel()
{
  String s = iFileName;
  if (iFileName.rfind('/') >= 0)
    s = iFileName.substr(iFileName.rfind('/') + 1);
  if (iDoc->countTotalViews() > 1) {
    ipe::StringStream ss(s);
    ss << " (" << iPageNo + 1 << "-" << iViewNo + 1 << ")";
  }
  setWindowTitle(QIpe(s));
}

void AppUi::fitBox(const Rect &box)
{
  if (box.isEmpty())
    return;
  ipeDebug("canvas: %d x %d", iCanvas->width(), iCanvas->height());
  double xfactor = box.width() > 0.0 ?
    (iCanvas->width() / box.width()) : 20.0;
  double yfactor = box.height() > 0.0 ?
    (iCanvas->height() / box.height()) : 20.0;
  double zoom = (xfactor > yfactor) ? yfactor : xfactor;
  iCanvas->setPan(0.5 * (box.bottomLeft() + box.topRight()));
  iCanvas->setZoom(zoom);
  iCanvas->update();
}

void AppUi::zoom(int delta)
{
  double zoom = iCanvas->zoom();
  while (delta > 0) {zoom *= 1.3; --delta;}
  while (delta < 0) {zoom /= 1.3; ++delta;}
  iCanvas->setZoom(zoom);
  iCanvas->update();
}

void AppUi::wheelZoom(int degrees)
{
  if (degrees > 0)
    zoom(+1);
  else
    zoom(-1);
}

void AppUi::startPan(int button)
{
  Tool *tool = new PanTool(iCanvas, iDoc->page(iPageNo), iViewNo);
  iCanvas->setTool(tool);
}

void AppUi::nextView(int delta)
{
  const Page *page = iDoc->page(iPageNo);
  if (0 <= iViewNo + delta && iViewNo + delta < page->countViews()) {
    iViewNo += delta;
  } else if (0 <= iPageNo + delta && iPageNo + delta < iDoc->countPages()) {
    iPageNo += delta;
    if (delta > 0)
      iViewNo = 0;
    else
      iViewNo = iDoc->page(iPageNo)->countViews() - 1;
  } else
    // at beginning or end of sequence
    return;
  iCanvas->setPage(iDoc->page(iPageNo), iViewNo, iDoc->cascade());
  iCanvas->update();
  updateLabel();
}

// --------------------------------------------------------------------

static void usage()
{
  fprintf(stderr, "Usage: ipeview <filename>\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  Platform::initLib(IPELIB_VERSION);
  QApplication a(argc, argv);
  setlocale(LC_NUMERIC, "C");

  if (argc != 2)
    usage();

  const char *load = argv[1];

  AppUi *ui = new AppUi();

  if (!ui->load(load))
    exit(2);

  ui->show();

  QObject::connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
  return a.exec();
}

// --------------------------------------------------------------------
