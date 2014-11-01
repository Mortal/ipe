// --------------------------------------------------------------------
// Ipeview for GTK
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2013  Otfried Cheong

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

#include "ipecanvas_gtk.h"
#include "ipetool.h"

#include <gtk/gtk.h>

using namespace ipe;

class AppUi : public CanvasObserver {
public:
  enum TAction { EOpen = 9001, EQuit, EGridVisible, EFullScreen, EFitPage,
		 EZoomIn, EZoomOut, ENextView, EPreviousView,
		 ENumActions = EPreviousView - 9000 };

  AppUi();
  virtual ~AppUi();

  void show();

  bool load(const char* fn);

  void zoom(int delta);
  void nextView(int delta);
  void fitBox(const Rect &box);
  void updateLabel();

  void cmd(int action);

  GtkWidget *windowId() const { return iWindow; }

protected: // from CanvasObserver
  virtual void canvasObserverWheelMoved(int degrees);
  virtual void canvasObserverMouseAction(int button);

private:
  void addItem(int m, int action, const char *label,
	       const char *shortcut = 0, bool checkable=false);

  CanvasBase *iCanvas;
  Snap iSnap;

  bool iGridVisible;
  Document *iDoc;
  String iFileName;
  int iPageNo;
  int iViewNo;

  GtkWidget *iWindow;
  GtkWidget *iRootMenu[3];
  GtkWidget *iSubMenu[3];
  GtkWidget *vbox;
  GtkAccelGroup *iAccelGroup;
};

static void menuitem_response(GtkWidget *item, AppUi::TAction a)
{
  AppUi *ui = (AppUi *) g_object_get_data(G_OBJECT(item), "appui");
  ui->cmd(a);
}

void AppUi::addItem(int m, int action, const char *label,
		    const char *shortcut, bool checkable)
{
  GtkMenuShell *menu = GTK_MENU_SHELL(iSubMenu[m]);

  GtkWidget *item = checkable ?
    gtk_check_menu_item_new_with_label(label) :
    gtk_menu_item_new_with_label(label);
  g_object_set_data(G_OBJECT(item), "appui", this);
  gtk_menu_shell_append(menu, item);
  g_signal_connect(item, "activate", G_CALLBACK(menuitem_response),
		   GINT_TO_POINTER(action));

  if (shortcut) {
    guint accel_key;
    GdkModifierType accel_mods;
    gtk_accelerator_parse(shortcut, &accel_key, &accel_mods);
    if (accel_key > 0)
      gtk_widget_add_accelerator(item, "activate", iAccelGroup,
				 accel_key, accel_mods, GTK_ACCEL_VISIBLE);
  }
  gtk_widget_show(item);

  /*
  gtk_accel_group_connect(iAccelGroup, accel_key, accel_mods, GtkAccelFlags(0),
			  g_cclosure_new(G_CALLBACK(shortcutResponse),
			  this, NULL));
  */
}

AppUi::AppUi()
{
  iWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (GTK_WIDGET(iWindow), 600, 400);
  gtk_window_set_title(GTK_WINDOW(iWindow), "Ipeview");
  g_signal_connect(iWindow, "delete-event", G_CALLBACK(gtk_main_quit), NULL);

  iAccelGroup = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(iWindow), iAccelGroup);

  for (int i = 0; i < 3; ++i)
    iSubMenu[i] = gtk_menu_new();

  addItem(0, EOpen, "Open", "<Control>O");
  addItem(0, EQuit, "Quit", "<Ctrl>Q");
  addItem(1, EGridVisible, "Grid visible", "F12", true);
  addItem(1, EFullScreen, "Full screen", 0, true);
  addItem(1, EFitPage, "Fit page", "<Ctrl>F");
  addItem(1, EZoomIn, "Zoom in", "<Ctrl>plus");
  addItem(1, EZoomOut, "Zoom out", "<Ctrl>minus");
  addItem(2, ENextView, "Next view", "Next");
  addItem(2, EPreviousView, "Previous view", "Prior");

  iRootMenu[0] = gtk_menu_item_new_with_mnemonic("_File");
  iRootMenu[1] = gtk_menu_item_new_with_mnemonic("_View");
  iRootMenu[2] = gtk_menu_item_new_with_mnemonic("_Move");

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(iWindow), vbox);
  gtk_widget_show(vbox);

  GtkWidget *menu_bar = gtk_menu_bar_new();

  gtk_box_pack_start(GTK_BOX (vbox), menu_bar, FALSE, FALSE, 2);
  gtk_widget_show(menu_bar);

  for (int i = 0; i < 3; ++i) {
    gtk_widget_show(iRootMenu[i]);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(iRootMenu[i]), iSubMenu[i]);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), iRootMenu[i]);
  }

  Canvas *canvas = new Canvas(iWindow);
  gtk_box_pack_end(GTK_BOX(vbox), canvas->window(), TRUE, TRUE, 2);
  gtk_widget_show(canvas->window());

  iDoc = 0;
  iGridVisible = false;
  iPageNo = iViewNo = 0;

  iSnap.iSnap = Snap::ESnapGrid | Snap::ESnapVtx;
  iSnap.iGridVisible = false;
  iSnap.iGridSize = 8;
  iSnap.iAngleSize = M_PI / 6.0;
  iSnap.iSnapDistance = 10;
  iSnap.iWithAxes = false;
  iSnap.iOrigin = Vector::ZERO;
  iSnap.iDir = 0;

  iCanvas = canvas;
  iCanvas->setObserver(this);
  iCanvas->setSnap(iSnap);
  iCanvas->setFifiVisible(true);
}

AppUi::~AppUi()
{
  fprintf(stderr, "AppUi::~AppUi()\n");
}

void AppUi::show()
{
  gtk_widget_show(iWindow);
}

// --------------------------------------------------------------------

void AppUi::cmd(int cmd)
{
  ipeDebug("Command %d", cmd);
  switch (cmd) {
  case EOpen:
    // load(szFileName);
    break;
  case EQuit:
    gtk_main_quit();
    break;
  case EFullScreen:
    gtk_window_fullscreen(GTK_WINDOW(iWindow));
    break;
  case EGridVisible: {
    Snap snap = iCanvas->snap();
    snap.iGridVisible = !snap.iGridVisible;
    iCanvas->setSnap(snap);
    iCanvas->update();
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

void AppUi::canvasObserverWheelMoved(int degrees)
{
  if (degrees > 0)
    zoom(+1);
  else
    zoom(-1);
}

void AppUi::canvasObserverMouseAction(int button)
{
  Tool *tool = new PanTool(iCanvas, iDoc->page(iPageNo), iViewNo);
  iCanvas->setTool(tool);
}

// --------------------------------------------------------------------

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

  /*
  if (iDoc->countPages() > 1) {
    int p = PageSelector::selectPageOrView(iDoc);
    if (p >= 0)
      iPageNo = p;
    if (iDoc->page(iPageNo)->countViews() > 1) {
      int v = PageSelector::selectPageOrView(iDoc, iPageNo);
      if (v >= 0)
	iViewNo = v;
    }
  }
  */

  iDoc->page(iPageNo)->setSelect(0, EPrimarySelected);

  iCanvas->setFontPool(iDoc->fontPool());
  iCanvas->setPage(iDoc->page(iPageNo), iPageNo, iViewNo, iDoc->cascade());
  iCanvas->setPan(Vector(300, 400));
  iCanvas->update();
  iCanvas->setFifiVisible(true);

  updateLabel();
  return true;
}

// --------------------------------------------------------------------

void AppUi::updateLabel()
{
  String s = iFileName;
  if (iFileName.rfind('/') >= 0)
    s = iFileName.substr(iFileName.rfind('/') + 1);
  if (iDoc->countTotalViews() > 1) {
    ipe::StringStream ss(s);
    ss << " (" << iPageNo + 1 << "-" << iViewNo + 1 << ")";
  }
  gtk_window_set_title(GTK_WINDOW(iWindow), s.z());
}

void AppUi::fitBox(const Rect &box)
{
  if (box.isEmpty())
    return;
  double xfactor = box.width() > 0.0 ?
    (iCanvas->canvasWidth() / box.width()) : 20.0;
  double yfactor = box.height() > 0.0 ?
    (iCanvas->canvasHeight() / box.height()) : 20.0;
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
  iCanvas->setPage(iDoc->page(iPageNo), iPageNo, iViewNo, iDoc->cascade());
  iCanvas->update();
  updateLabel();
}

// --------------------------------------------------------------------

int main(int argc, char *argv[])
{
  Platform::initLib(IPELIB_VERSION);

  gtk_init(&argc, &argv);

  AppUi *ui = new AppUi();
  if (argc > 1 && !ui->load(argv[1]))
    exit(2);

  ui->show();

  gtk_main();
  delete ui;
  return 0;
}

// --------------------------------------------------------------------
