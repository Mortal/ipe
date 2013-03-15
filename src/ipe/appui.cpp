// --------------------------------------------------------------------
// AppUi
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

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "appui.h"
#include "tools.h"

#include "ipelua.h"

#include "ipecairopainter.h"

// for version info only
#include "ipefonts.h"
#include <zlib.h>

#include <cstdio>
#include <cstdlib>

#include <QMenuBar>
#include <QKeySequence>
#include <QMouseEvent>
#include <QStatusBar>
#include <QGridLayout>
#include <QPainter>
#include <QButtonGroup>
#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSignalMapper>
#include <QToolTip>
#include <QHelpEvent>

using namespace ipe;
using namespace ipeqt;
using namespace ipelua;

// --------------------------------------------------------------------

#define SIZE 16

Prefs *Prefs::singleton = 0;

Prefs *Prefs::get()
{
  if (singleton == 0)
    singleton = new Prefs();
  assert(singleton);
  return singleton;
}

Prefs::Prefs()
{
  const char *p = getenv("IPEICONDIR");
#ifdef WIN32
  String s(p ? p : Platform::ipeDir("icons"));
#else
  String s(p ? p : IPEICONDIR);
#endif
  if (s.right(1) != "/")
    s += "/";
  iIconDir = s;
}

QIcon Prefs::icon(String name)
{
  if (Platform::fileExists(iIconDir + name + ".png"))
    return QIcon(QIpe(iIconDir + name + ".png"));
  else
    return QIcon();
}

QPixmap Prefs::pixmap(String name)
{
  return QPixmap(QIpe(iIconDir + name + ".png"));
}

QIcon Prefs::colorIcon(Color color)
{
  QPixmap pixmap(SIZE, SIZE);
  pixmap.fill(QIpe(color));
  return QIcon(pixmap);
}

// --------------------------------------------------------------------

LayerBox::LayerBox(QWidget *parent)
  : QListWidget(parent)
{
  setFocusPolicy(Qt::NoFocus);
  setSelectionMode(NoSelection);
  iInSet = false;
  connect(this, SIGNAL(itemChanged(QListWidgetItem *)),
	  SLOT(layerChanged(QListWidgetItem *)));
}

void LayerBox::layerChanged(QListWidgetItem *item)
{
  if (iInSet)
    return;
  if (item->checkState() == Qt::Checked)
    emit activated("selecton", IpeQ(item->text()));
  else
    emit activated("selectoff", IpeQ(item->text()));
}

void LayerBox::set(const Page *page, int view)
{
  iInSet = true;
  // find ordering of layers <first_view, layer>
  std::vector<std::pair<int,int> > idx(page->countLayers());
  for (int i = 0; i < page->countLayers(); ++i) {
    int first = page->countViews();
    for (int j = 0; j < page->countViews(); ++j) {
      if (page->visible(j, i)) {
	first = j;
	break;
      }
    }
    idx[i] = std::pair<int,int>(first, i);
  }
  std::sort(idx.begin(), idx.end());

  clear();
  for (int j = 0; j < page->countLayers(); ++j) {
    int i = idx[j].second;
    QListWidgetItem *item = new QListWidgetItem(QIpe(page->layer(i)), this);
    item->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    item->setCheckState(page->visible(view, i) ? Qt::Checked : Qt::Unchecked);
    if (page->layer(i) == page->active(view))
      item->setBackgroundColor(Qt::yellow);
    if (page->isLocked(i))
      item->setBackground(QColor(255, 220, 220));
    if (!page->hasSnapping(i))
      item->setForeground(Qt::gray);
  }
  iInSet = false;
}

void LayerBox::mousePressEvent(QMouseEvent *ev)
{
  QListWidgetItem *item = itemAt(ev->pos());
  if (item && ev->button()== Qt::LeftButton && ev->x() > 30) {
    emit activated("active", IpeQ(item->text()));
  }
  QListWidget::mousePressEvent(ev);
}

void LayerBox::mouseReleaseEvent(QMouseEvent *ev)
{
  QListWidgetItem *item = itemAt(ev->pos());
  if (item && ev->button() == Qt::RightButton) {
    // make popup menu
    emit showLayerBoxPopup(Vector(ev->globalPos().x(), ev->globalPos().y()),
			   IpeQ(item->text()));
  } else
    QListWidget::mouseReleaseEvent(ev);
}

// --------------------------------------------------------------------

PathView::PathView(QWidget* parent, Qt::WFlags f)
  : QWidget(parent, f)
{
  iCascade = 0;
}

void PathView::set(const AllAttributes &all, Cascade *sheet)
{
  iCascade = sheet;
  iAll = all;
  update();
}

void PathView::paintEvent(QPaintEvent *ev)
{
  QSize s = size();
  int w = s.width();
  int h = s.height();

  cairo_surface_t *sf = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w, h);
  cairo_t *cc = cairo_create(sf);
  cairo_set_source_rgb(cc, 1, 1, 0.8);
  cairo_rectangle(cc, 0, 0, w, h);
  cairo_fill(cc);

  if (iCascade) {
    cairo_translate(cc, 0, h);
    double zoom = 2.0;
    cairo_scale(cc, zoom, -zoom);
    Vector v0 = (1.0/zoom) * Vector(0.1 * w, 0.5 * h);
    Vector v1 = (1.0/zoom) * Vector(0.7 * w, 0.5 * h);
    Vector u1 = (1.0/zoom) * Vector(0.88 * w, 0.8 * h);
    Vector u2 = (1.0/zoom) * Vector(0.80 * w, 0.5 * h);
    Vector u3 = (1.0/zoom) * Vector(0.88 * w, 0.2 * h);
    Vector u4 = (1.0/zoom) * Vector(0.96 * w, 0.5 * h);

    CairoPainter painter(iCascade, 0, cc, 3.0, false);
    painter.setPen(iAll.iPen);
    painter.setDashStyle(iAll.iDashStyle);
    painter.setStroke(iAll.iStroke);
    painter.setFill(iAll.iFill);
    painter.pushMatrix();
    painter.newPath();
    painter.moveTo(v0);
    painter.lineTo(v1);
    painter.drawPath(EStrokedOnly);
    if (iAll.iFArrow)
      Path::drawArrow(painter, v1, Angle(0),
		      iAll.iFArrowShape, iAll.iFArrowSize, 100.0);
    if (iAll.iRArrow)
      Path::drawArrow(painter, v0, Angle(IpePi),
		      iAll.iRArrowShape, iAll.iRArrowSize, 100.0);
    painter.setDashStyle(Attribute::NORMAL());
    painter.setTiling(iAll.iTiling);
    painter.newPath();
    painter.moveTo(u1);
    painter.lineTo(u2);
    painter.lineTo(u3);
    painter.lineTo(u4);
    painter.closePath();
    painter.drawPath(iAll.iPathMode);
    painter.popMatrix();
  }

  cairo_surface_flush(sf);
  cairo_destroy(cc);

  QPainter qPainter;
  qPainter.begin(this);
  QRect r = ev->rect();
  QImage bits(cairo_image_surface_get_data(sf), w, h, QImage::Format_RGB32);
  qPainter.drawImage(r.left(), r.top(), bits,
		     r.left(), r.top(), r.width(), r.height());
  qPainter.end();
  cairo_surface_destroy(sf);
}

QSize PathView::sizeHint() const
{
  return QSize(120, 40);
}

void PathView::mousePressEvent(QMouseEvent *ev)
{
  QSize s = size();
  if (ev->button()== Qt::LeftButton && ev->x() < s.width() * 3 / 10) {
    emit activated(iAll.iRArrow ? "rarrow|false" : "rarrow|true");
  } else if (ev->button()== Qt::LeftButton &&
	     ev->x() > s.width() * 4 / 10 &&
	     ev->x() < s.width() * 72 / 100) {
    emit activated(iAll.iFArrow ? "farrow|false" : "farrow|true");
    update();
  } else if (ev->button()== Qt::LeftButton && ev->x() > s.width() * 78 / 100) {
    switch (iAll.iPathMode) {
    case EStrokedOnly:
      emit activated("pathmode|strokedfilled");
      break;
    case EStrokedAndFilled:
      emit activated("pathmode|filled");
      break;
    case EFilledOnly:
      emit activated("pathmode|stroked");
      break;
    }
    update();
  }
}

void PathView::mouseReleaseEvent(QMouseEvent *ev)
{
  if (ev->button()== Qt::RightButton)
    emit showPathStylePopup(Vector(ev->globalPos().x(), ev->globalPos().y()));
}

bool PathView::event(QEvent *ev)
{
  if (ev->type() != QEvent::ToolTip)
    return QWidget::event(ev);

  QHelpEvent *hev = (QHelpEvent *) ev;
  QSize s = size();

  QString tip;
  if (hev->x() < s.width() * 3 / 10)
    tip = "Toggle reverse arrow";
  else if (hev->x() > s.width() * 4 / 10 &&
	   hev->x() < s.width() * 72 / 100)
    tip = "Toggle forward arrow";
  else if (hev->x() > s.width() * 78 / 100)
    tip = "Toggle stroked/stroked & filled/filled";

  if (!tip.isNull())
    QToolTip::showText(hev->globalPos(), tip, this);
  return true;
}

// --------------------------------------------------------------------

QAction *AppUi::findAction(const char *name) const
{
  std::map<String, QAction*>::const_iterator it = iActions.find(name);
  if (it != iActions.end())
    return it->second;
  else
    return 0;
}

void AppUi::addItem(QMenu *m, const QString &title, const char *name)
{
  QAction *a = new QAction(title, this);
  lua_getglobal(L, "shortcuts");
  lua_getfield(L, -1, name);
  if (lua_isstring(L, -1)) {
    QString s = lua_tostring(L, -1);
    a->setShortcut(QKeySequence(s));
    QString tt = title + " [" + s + "]";
    a->setToolTip(tt);
  } else if (lua_istable(L, -1)) {
    int no = lua_objlen(L, -1);
    QList<QKeySequence> kl;
    QString prim;
    for (int i = 1; i <= no; ++i) {
      lua_rawgeti(L, -1, i);
      if (lua_isstring(L, -1)) {
	QString s = lua_tostring(L, -1);
	kl.append(QKeySequence(s));
	if (prim.isEmpty())
	  prim = s;
      }
      lua_pop(L, 1); // pop string
    }
    a->setShortcuts(kl);
    QString tt = title + " [" + prim + "]";
    a->setToolTip(tt);
  }
  a->setIcon(Prefs::get()->icon(name));
  lua_pop(L, 2);
  m->addAction(a);
  if (m == iMenu[EModeMenu]) {
    a->setCheckable(true);
    a->setActionGroup(iModeActionGroup);
    iObjectTools->addAction(a);
  }
  if (String(name).find('|') >= 0) {
    // selector item
    a->setCheckable(true);
  }
  connect(a, SIGNAL(triggered()), iActionMap, SLOT(map()));
  iActionMap->setMapping(a, name);
  iActions[name] = a;
}

void AppUi::addItem(int m, const QString &title, const char *name)
{
  addItem(iMenu[m], title, name);
}

void AppUi::addSnap(const char *name)
{
  QAction *a = findAction(name);
  assert(a);
  a->setCheckable(true);
  iSnapTools->addAction(a);
}

void AppUi::addEdit(const char *name)
{
  QAction *a = findAction(name);
  assert(a);
  // a->setCheckable(true);
  iEditTools->addAction(a);
}

void AppUi::buildMenus()
{
  iMenu[EFileMenu] = menuBar()->addMenu("&File");
  iMenu[EEditMenu] = menuBar()->addMenu("&Edit");
  iMenu[EPropertiesMenu] = menuBar()->addMenu("P&roperties");
  iMenu[ESnapMenu] = menuBar()->addMenu("&Snap");
  iMenu[EModeMenu] = menuBar()->addMenu("&Mode");
  iMenu[EZoomMenu] = menuBar()->addMenu("&Zoom");
  iMenu[ELayerMenu] = menuBar()->addMenu("&Layer");
  iMenu[EViewMenu] = menuBar()->addMenu("&View");
  iMenu[EPageMenu] = menuBar()->addMenu("&Page");
  iMenu[EIpeletMenu] = menuBar()->addMenu("&Ipelets");
  menuBar()->addSeparator();
  iMenu[EHelpMenu] = menuBar()->addMenu("&Help");

  addItem(EFileMenu, "New Window", "new_window");
  addItem(EFileMenu, "New", "new");
  addItem(EFileMenu, "Open", "open");
  addItem(EFileMenu, "Save", "save");
  addItem(EFileMenu, "Save as", "save_as");
  iMenu[EFileMenu]->addSeparator();
  addItem(EFileMenu, "Run Latex", "run_latex");
  iMenu[EFileMenu]->addSeparator();
  addItem(EFileMenu, "Close", "close");
  addItem(EFileMenu, "Exit", "quit");

  addItem(EEditMenu, "Undo", "undo");
  addItem(EEditMenu, "Redo", "redo");
  iMenu[EEditMenu]->addSeparator();
  addItem(EEditMenu, "Cut", "cut");
  addItem(EEditMenu, "Copy", "copy");
  addItem(EEditMenu, "Paste", "paste");
  addItem(EEditMenu, "Paste at cursor", "paste_at_cursor");
  addItem(EEditMenu, "Delete", "delete");
  iMenu[EEditMenu]->addSeparator();
  addItem(EEditMenu, "Group", "group");
  addItem(EEditMenu, "Ungroup", "ungroup");
  addItem(EEditMenu, "Front", "front");
  addItem(EEditMenu, "Back", "back");
  addItem(EEditMenu, "Forward", "forward");
  addItem(EEditMenu, "Backward", "backward");
  addItem(EEditMenu, "Just before", "before");
  addItem(EEditMenu, "Just behind", "behind");
  addItem(EEditMenu, "Duplicate", "duplicate");
  addItem(EEditMenu, "Select all", "select_all");
  iMenu[EEditMenu]->addSeparator();
  addItem(EEditMenu, "Insert text box", "insert_text_box");
  addItem(EEditMenu, "Edit object", "edit");
  addItem(EEditMenu, "Edit object as XML", "edit_as_xml");
  addItem(EEditMenu, "Change text width", "change_width");
  iMenu[EEditMenu]->addSeparator();
  addItem(EEditMenu, "Document properties", "document_properties");
  addItem(EEditMenu, "Style sheets", "style_sheets");
  addItem(EEditMenu, "Update style sheets", "update_style_sheets");
  addItem(EEditMenu, "Check symbolic attributes", "check_style");

  QMenu *m;

  m = new QMenu("Pinned");
  addItem(m, "none", "pinned|none");
  addItem(m, "horizontal", "pinned|horizontal");
  addItem(m, "vertical", "pinned|vertical");
  addItem(m, "fixed", "pinned|fixed");
  iMenu[EPropertiesMenu]->addMenu(m);

  m = new QMenu("Transformations");
  addItem(m, "translations", "transformations|translations");
  addItem(m, "rigid motions", "transformations|rigid");
  addItem(m, "affine", "transformations|affine");
  iMenu[EPropertiesMenu]->addMenu(m);

  iMenu[EPropertiesMenu]->addSeparator();

  iTextStyleMenu = new QMenu("Text style");
  iMenu[EPropertiesMenu]->addMenu(iTextStyleMenu);

  m = new QMenu("Horizontal alignment");
  addItem(m, "left", "horizontalalignment|left");
  addItem(m, "center", "horizontalalignment|hcenter");
  addItem(m, "right", "horizontalalignment|right");
  iMenu[EPropertiesMenu]->addMenu(m);

  m = new QMenu("Vertical alignment");
  addItem(m, "bottom", "verticalalignment|bottom");
  addItem(m, "baseline", "verticalalignment|baseline");
  addItem(m, "center", "verticalalignment|vcenter");
  addItem(m, "top", "verticalalignment|top");
  iMenu[EPropertiesMenu]->addMenu(m);

  m = new QMenu("Transformable text");
  addItem(m, "Yes", "transformabletext|true");
  addItem(m, "No", "transformabletext|false");
  iMenu[EPropertiesMenu]->addMenu(m);

  iModeActionGroup = new QActionGroup(this);
  iModeActionGroup->setExclusive(true);

  addItem(EModeMenu, "Select objects (with Shift: non-destructive)",
	  "mode_select");
  addItem(EModeMenu, "Translate objects (with Shift: horizontal/vertical)",
	  "mode_translate");
  addItem(EModeMenu, "Rotate objects", "mode_rotate");
  addItem(EModeMenu, "Stretch objects (with Shift: scale objects)",
	  "mode_stretch");
  addItem(EModeMenu, "Pan the canvas", "mode_pan");
  iMenu[EModeMenu]->addSeparator();
  iObjectTools->addSeparator();
  addItem(EModeMenu, "Text labels", "mode_label");
  addItem(EModeMenu, "Mathematical symbols", "mode_math");
  addItem(EModeMenu, "Paragraphs", "mode_paragraph");
  addItem(EModeMenu, "Marks", "mode_marks");
  addItem(EModeMenu, "Rectangles (with Shift: squares)", "mode_rectangles");
  addItem(EModeMenu, "Lines and polylines", "mode_lines");
  addItem(EModeMenu, "Polygons", "mode_polygons");
  addItem(EModeMenu, "Splines", "mode_splines");
  addItem(EModeMenu, "Splinegons", "mode_splinegons");
  addItem(EModeMenu, "Circular arcs (by center, right and left point)",
    "mode_arc1");
  addItem(EModeMenu, "Circular arcs (by center, left and right point)",
    "mode_arc2");
  addItem(EModeMenu, "Circular arcs (by 3 points)", "mode_arc3");
  addItem(EModeMenu, "Circles (by center and radius)", "mode_circle1");
  addItem(EModeMenu, "Circles (by diameter)", "mode_circle2");
  addItem(EModeMenu, "Circles (by 3 points)", "mode_circle3");
  addItem(EModeMenu, "Ink", "mode_ink");

  addItem(ESnapMenu, "Snap to vertex", "snapvtx");
  addItem(ESnapMenu, "Snap to boundary", "snapbd");
  addItem(ESnapMenu, "Snap to intersection", "snapint");
  addItem(ESnapMenu, "Snap to grid", "snapgrid");
  addItem(ESnapMenu, "Angular snap", "snapangle");
  addItem(ESnapMenu, "Automatic snap", "snapauto");
  iMenu[ESnapMenu]->addSeparator();
  addItem(ESnapMenu, "Set origin && snap", "set_origin_snap");
  addItem(ESnapMenu, "Hide axes", "hide_axes");
  addItem(ESnapMenu, "Set direction", "set_direction");
  addItem(ESnapMenu, "Reset direction", "reset_direction");
  addItem(ESnapMenu, "Set line", "set_line");
  addItem(ESnapMenu, "Set line && snap", "set_line_snap");

  addItem(EZoomMenu, "Fullscreen", "fullscreen");
  addItem(EZoomMenu, "Grid visible", "grid_visible");

  m = new QMenu("Coordinates");
  addItem(m, "points", "coordinates|points");
  addItem(m, "mm", "coordinates|mm");
  addItem(m, "inch", "coordinates|inch");
  iMenu[EZoomMenu]->addMenu(m);

  QActionGroup *cg = new QActionGroup(this);
  cg->setExclusive(true);
  findAction("coordinates|points")->setActionGroup(cg);
  findAction("coordinates|mm")->setActionGroup(cg);
  findAction("coordinates|inch")->setActionGroup(cg);

  iMenu[EZoomMenu]->addSeparator();
  addItem(EZoomMenu, "Zoom in", "zoom_in");
  addItem(EZoomMenu, "Zoom out", "zoom_out");
  addItem(EZoomMenu, "Normal size", "normal_size");
  addItem(EZoomMenu, "Fit page", "fit_page");
  addItem(EZoomMenu, "Fit width", "fit_width");
  addItem(EZoomMenu, "Fit objects", "fit_objects");
  addItem(EZoomMenu, "Fit selection", "fit_selection");
  iMenu[EZoomMenu]->addSeparator();
  addItem(EZoomMenu, "Pan here", "pan_here");

  addItem(ELayerMenu, "New layer", "new_layer");
  iMenu[ELayerMenu]->addSeparator();
  addItem(ELayerMenu, "Select all in active  layer",
    "select_in_active_layer");
  iSelectLayerMenu = new QMenu("Select all in layer");
  iMenu[ELayerMenu]->addMenu(iSelectLayerMenu);
  addItem(ELayerMenu, "Move to active layer", "move_to_active_layer");
  iMoveToLayerMenu = new QMenu("Move to layer");
  iMenu[ELayerMenu]->addMenu(iMoveToLayerMenu);

  addItem(EViewMenu, "Next view", "next_view");
  addItem(EViewMenu, "Previous view", "previous_view");
  addItem(EViewMenu, "First view", "first_view");
  addItem(EViewMenu, "Last view", "last_view");
  iMenu[EViewMenu]->addSeparator();
  addItem(EViewMenu, "New layer, new view", "new_layer_view");
  addItem(EViewMenu, "New view", "new_view");
  addItem(EViewMenu, "Delete view", "delete_view");
  iMenu[EViewMenu]->addSeparator();
  addItem(EViewMenu, "Edit effects", "edit_effects");

  addItem(EPageMenu, "Next page", "next_page");
  addItem(EPageMenu, "Previous page", "previous_page");
  addItem(EPageMenu, "First page", "first_page");
  addItem(EPageMenu, "Last page", "last_page");
  iMenu[EPageMenu]->addSeparator();
  addItem(EPageMenu, "New page", "new_page");
  addItem(EPageMenu, "Cut page", "cut_page");
  addItem(EPageMenu, "Copy page", "copy_page");
  addItem(EPageMenu, "Paste page", "paste_page");
  addItem(EPageMenu, "Delete page", "delete_page");
  iMenu[EPageMenu]->addSeparator();
  addItem(EPageMenu, "Edit title && sections", "edit_title");
  addItem(EPageMenu, "Edit notes", "edit_notes");
  addItem(EPageMenu, "Page sorter", "page_sorter");
  iMenu[EPageMenu]->addSeparator();

  addItem(EHelpMenu, "Ipe &manual", "manual");
  addItem(EHelpMenu, "Onscreen keyboard", "keyboard");
  addItem(EHelpMenu, "Show &configuration", "show_configuration");
  addItem(EHelpMenu, "About the &ipelets", "about_ipelets");
  addItem(EHelpMenu, "&About Ipe", "about");

  // build Ipelet menu
  lua_getglobal(L, "ipelets");
  int n = lua_objlen(L, -1);
  for (int i = 1; i <= n; ++i) {
    lua_rawgeti(L, -1, i);
    lua_getfield(L, -1, "label");
    String label(lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "name");
    String name(lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "methods");
    char buf[20];
    if (lua_isnil(L, -1)) {
      String action("ipelet_1_");
      action += name;
      addItem(EIpeletMenu, label.z(), action.z());
    } else {
      int m = lua_objlen(L, -1);
      QMenu *menu = new QMenu(QIpe(label));
      for (int j = 1; j <= m; ++j) {
	lua_rawgeti(L, -1, j);
	lua_getfield(L, -1, "label");
	sprintf(buf, "ipelet_%d_", j);
	String action(buf);
	action += name;
	addItem(menu, lua_tostring(L, -1), action.z());
	lua_pop(L, 2); // sublabel, method
      }
      iMenu[EIpeletMenu]->addMenu(menu);
    }
    lua_pop(L, 2); // methods, ipelet
  }
  lua_pop(L, 1);
}

// --------------------------------------------------------------------

AppUi::AppUi(lua_State *L0, int model, Qt::WFlags f)
  : QMainWindow(0, f)
{
  L = L0;
  iModel = model;

  Prefs *prefs = Prefs::get();

  setWindowIcon(prefs->icon("ipe"));
  setAttribute(Qt::WA_DeleteOnClose);
  setDockOptions(AnimatedDocks); // do not allow stacking properties and layers

  iCanvas = new Canvas(this);
  setCentralWidget(iCanvas);

  iSnapTools = addToolBar("Snap");
  iEditTools = addToolBar("Edit");
  addToolBarBreak();
  iObjectTools = addToolBar("Objects");

  iActionMap = new QSignalMapper(this);
  connect(iActionMap, SIGNAL(mapped(const QString &)),
	  SLOT(qAction(const QString &)));

  buildMenus();

  connect(iSelectLayerMenu, SIGNAL(triggered(QAction *)),
	  SLOT(selectLayerAction(QAction *)));
  connect(iMoveToLayerMenu, SIGNAL(triggered(QAction *)),
	  SLOT(moveToLayerAction(QAction *)));
  connect(iTextStyleMenu, SIGNAL(triggered(QAction *)),
	  SLOT(textStyleAction(QAction *)));

  QSignalMapper *comboMap = new QSignalMapper(this);

  iSelector[EUiGridSize] = new QComboBox();
  iSelector[EUiAngleSize] = new QComboBox();
  iSelector[EUiGridSize]->setFocusPolicy(Qt::NoFocus);
  iSelector[EUiAngleSize]->setFocusPolicy(Qt::NoFocus);
  connect(iSelector[EUiGridSize], SIGNAL(activated(int)),
	  comboMap, SLOT(map()));
  connect(iSelector[EUiAngleSize], SIGNAL(activated(int)),
	  comboMap, SLOT(map()));
  comboMap->setMapping(iSelector[EUiGridSize], EUiGridSize);
  comboMap->setMapping(iSelector[EUiAngleSize], EUiAngleSize);

  addSnap("snapvtx");
  addSnap("snapbd");
  addSnap("snapint");
  addSnap("snapgrid");
  iSnapTools->addWidget(iSelector[EUiGridSize]);
  addSnap("snapangle");
  iSnapTools->addWidget(iSelector[EUiAngleSize]);
  addSnap("snapauto");

  addEdit("copy");
  addEdit("cut");
  addEdit("paste");
  addEdit("delete");
  addEdit("undo");
  addEdit("redo");
  addEdit("zoom_in");
  addEdit("zoom_out");
  addEdit("fit_objects");
  addEdit("fit_page");
  addEdit("fit_width");
  addEdit("keyboard");
  iShiftKey = new QAction("shift_key", this);
  iShiftKey->setCheckable(true);
  iShiftKey->setIcon(Prefs::get()->icon("shift_key"));
  iEditTools->addAction(iShiftKey);
  connect(iShiftKey, SIGNAL(triggered()), SLOT(toolbarModifiersChanged()));

  findAction("fullscreen")->setCheckable(true);
  findAction("grid_visible")->setCheckable(true);

  iPropertiesTools = new QDockWidget("Properties", this);
  addDockWidget(Qt::LeftDockWidgetArea, iPropertiesTools);
  iPropertiesTools->setAllowedAreas(Qt::LeftDockWidgetArea|
				    Qt::RightDockWidgetArea);

  iLayerTools = new QDockWidget("Layers", this);
  addDockWidget(Qt::LeftDockWidgetArea, iLayerTools);
  iLayerTools->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);

  iNotesTools = new QDockWidget("Notes", this);
  addDockWidget(Qt::RightDockWidgetArea, iNotesTools);
  iNotesTools->setAllowedAreas(Qt::LeftDockWidgetArea|
			       Qt::RightDockWidgetArea);
  iMenu[EPageMenu]->addAction(iNotesTools->toggleViewAction());


  iBookmarkTools = new QDockWidget("Bookmarks", this);
  addDockWidget(Qt::RightDockWidgetArea, iBookmarkTools);
  iBookmarkTools->setAllowedAreas(Qt::LeftDockWidgetArea|
				  Qt::RightDockWidgetArea);
  iMenu[EPageMenu]->addAction(iBookmarkTools->toggleViewAction());

  // object names are used for saving toolbar state
  iSnapTools->setObjectName(QLatin1String("SnapTools"));
  iObjectTools->setObjectName(QLatin1String("ObjectTools"));
  iPropertiesTools->setObjectName(QLatin1String("PropertiesTools"));
  iLayerTools->setObjectName(QLatin1String("LayerTools"));
  iNotesTools->setObjectName(QLatin1String("NotesTools"));
  iBookmarkTools->setObjectName(QLatin1String("BookmarkTools"));

  QWidget *wg = new QFrame();
  QGridLayout *lo = new QGridLayout();
  wg->setLayout(lo);
  lo->setSpacing(0);
  lo->setContentsMargins(0, 0, 0, 0);
  // wg->setMaximumWidth(150);
  lo->setSizeConstraint(QLayout::SetFixedSize);
  QButtonGroup *bg = new QButtonGroup(wg);
  bg->setExclusive(false);
  connect(bg, SIGNAL(buttonClicked(int)), this, SLOT(absoluteButton(int)));
  iButton[EUiMarkShape] = 0; // no such button
  for (int i = 0; i <= EUiSymbolSize; ++i) {
    if (i != EUiMarkShape) {
      iButton[i] = new QToolButton();
      iButton[i]->setFocusPolicy(Qt::NoFocus);
      bg->addButton(iButton[i], i);
      lo->addWidget(iButton[i], i > 2 ? i+1 : i, 0);
    }
    iSelector[i] = new QComboBox();
    iSelector[i]->setFocusPolicy(Qt::NoFocus);
    lo->addWidget(iSelector[i], i > 2 ? i+1 : i, 1);
    connect(iSelector[i], SIGNAL(activated(int)), comboMap, SLOT(map()));
    comboMap->setMapping(iSelector[i], i);
  }
  iButton[EUiStroke]->setIcon(Prefs::colorIcon(Color(1000, 0, 0)));
  iButton[EUiFill]->setIcon(Prefs::colorIcon(Color(1000, 1000, 0)));
  iButton[EUiPen]->setIcon(prefs->icon("pen"));
  iButton[EUiTextSize]->setIcon(prefs->icon("mode_label"));
  iButton[EUiSymbolSize]->setIcon(prefs->icon("mode_marks"));

  iButton[EUiStroke]->setToolTip("Absolute stroke color");
  iButton[EUiFill]->setToolTip("Absolute fill color");
  iButton[EUiPen]->setToolTip("Absolute pen width");
  iButton[EUiTextSize]->setToolTip("Absolute text size");
  iButton[EUiSymbolSize]->setToolTip("Absolute symbol size");

  iSelector[EUiStroke]->setToolTip("Symbolic stroke color");
  iSelector[EUiFill]->setToolTip("Symbolic fill color");
  iSelector[EUiPen]->setToolTip("Symbolic pen width");
  iSelector[EUiTextSize]->setToolTip("Symbolic text size");
  iSelector[EUiMarkShape]->setToolTip("Mark shape");
  iSelector[EUiSymbolSize]->setToolTip("Symbolic symbol size");

  iSelector[EUiGridSize]->setToolTip("Grid size");
  iSelector[EUiAngleSize]->setToolTip("Angle for angular snap");

  connect(comboMap, SIGNAL(mapped(int)), this, SLOT(comboSelector(int)));

  QLabel *msl = new QLabel();
  msl->setPixmap(prefs->pixmap("mode_marks"));
  lo->addWidget(msl, EUiMarkShape + 1, 0);

  iPathView = new PathView();
  connect(iPathView, SIGNAL(activated(String)), SLOT(action(String)));
  connect(iPathView, SIGNAL(showPathStylePopup(Vector)),
	  SLOT(showPathStylePopup(Vector)));
  lo->addWidget(iPathView, 3, 0, 1, 2);
  // lo->setRowStretch(3, 1);
  iPropertiesTools->setWidget(wg);

  QHBoxLayout *hol = new QHBoxLayout();
  iViewNumber = new QToolButton();
  iPageNumber = new QToolButton();
  iViewNumber->setFocusPolicy(Qt::NoFocus);
  iPageNumber->setFocusPolicy(Qt::NoFocus);
  iViewNumber->setText("View 1/1");
  iViewNumber->setToolTip("Current view number");
  iPageNumber->setText("Page 1/1");
  iPageNumber->setToolTip("Current page number");
  bg->addButton(iViewNumber, EUiView);
  bg->addButton(iPageNumber, EUiPage);
  hol->setSpacing(0);
  hol->addWidget(iViewNumber);
  hol->addStretch(1);
  hol->addWidget(iPageNumber);
  lo->addLayout(hol, EUiSymbolSize + 2, 0, 1, -1);

  iPageNotes = new QTextEdit();
  iPageNotes->setAcceptRichText(false);
  iPageNotes->setReadOnly(true);
  iPageNotes->setFocusPolicy(Qt::NoFocus);
  iNotesTools->setWidget(iPageNotes);

  iBookmarks = new QListWidget();
  iBookmarks->setToolTip("Bookmarks of this document");
  iBookmarks->setFocusPolicy(Qt::NoFocus);
  connect(iBookmarks, SIGNAL(itemActivated(QListWidgetItem *)),
	  this, SLOT(bookmarkSelected(QListWidgetItem *)));
  iBookmarkTools->setWidget(iBookmarks);

  iLayerList = new LayerBox();
  iLayerList->setToolTip("Layers of this page");
  // iLayerList->setWhatsThis(layerListText));
  iLayerTools->setWidget(iLayerList);
  connect(iLayerList, SIGNAL(activated(String, String)),
	  this, SLOT(layerAction(String, String)));
  connect(iLayerList, SIGNAL(showLayerBoxPopup(Vector, String)),
	  SLOT(showLayerBoxPopup(Vector, String)));

  connect(iSelectLayerMenu, SIGNAL(aboutToShow()),
	  this, SLOT(aboutToShowSelectLayerMenu()));
  connect(iMoveToLayerMenu, SIGNAL(aboutToShow()),
	  this, SLOT(aboutToShowMoveToLayerMenu()));
  connect(iTextStyleMenu, SIGNAL(aboutToShow()),
	  this, SLOT(aboutToShowTextStyleMenu()));

  iMouse = new QLabel(statusBar());
  iMouseIn = 0; // points
  findAction("coordinates|points")->setChecked(true);
  statusBar()->addPermanentWidget(iMouse, 0);

  iCoordinatesFormat = "%g%s, %g%s";
  lua_getglobal(L, "prefs");
  lua_getfield(L, -1, "coordinates_format");
  if (lua_isstring(L, -1)) {
    iCoordinatesFormat = lua_tostring(L, -1);
  }
  lua_pop(L, 2);

  iResolution = new QLabel(statusBar());
  statusBar()->addPermanentWidget(iResolution, 0);

  connect(iCanvas, SIGNAL(wheelMoved(int)),   SLOT(wheelZoom(int)));
  connect(iCanvas, SIGNAL(mouseAction(int)),  SLOT(mouseAction(int)));
  connect(iCanvas, SIGNAL(positionChanged()), SLOT(positionChanged()));
  connect(iCanvas, SIGNAL(toolChanged(bool)), SLOT(toolChanged(bool)));
}

AppUi::~AppUi()
{
  ipeDebug("AppUi C++ destructor");
  luaL_unref(L, LUA_REGISTRYINDEX, iModel);
  // collect this model
  lua_gc(L, LUA_GCCOLLECT, 0);
}

// --------------------------------------------------------------------

void AppUi::aboutToShowSelectLayerMenu()
{
  iSelectLayerMenu->clear();
  for (int i = 0; i < iLayerList->count(); ++i) {
    QAction *a = new QAction(iLayerList->item(i)->text(), iSelectLayerMenu);
    iSelectLayerMenu->addAction(a);
  }
}

void AppUi::selectLayerAction(QAction *a)
{
  action(String("selectinlayer-") + IpeQ(a->text()));
}

void AppUi::aboutToShowMoveToLayerMenu()
{
  iMoveToLayerMenu->clear();
  for (int i = 0; i < iLayerList->count(); ++i) {
    QAction *a = new QAction(iLayerList->item(i)->text(), iMoveToLayerMenu);
    iMoveToLayerMenu->addAction(a);
  }
}

void AppUi::moveToLayerAction(QAction *a)
{
  action(String("movetolayer-") + IpeQ(a->text()));
}

void AppUi::aboutToShowTextStyleMenu()
{
  AttributeSeq seq;
  iCascade->allNames(ETextStyle, seq);
  iTextStyleMenu->clear();
  for (uint i = 0; i < seq.size(); ++i) {
    String s = seq[i].string();
    QAction *a = new QAction(QIpe(s), iTextStyleMenu);
    a->setCheckable(true);
    if (s == iAll.iTextStyle.string())
      a->setChecked(true);
    iTextStyleMenu->addAction(a);
  }
}

void AppUi::textStyleAction(QAction *a)
{
  action(String("textstyle|") + IpeQ(a->text()));
}

void AppUi::wheelZoom(int degrees)
{
  if (degrees > 0)
    action("wheel_zoom_in");
  else
    action("wheel_zoom_out");
}

static void adjust(double &x, int mode)
{
  if (ipe::abs(x) < 1e-12)
    x = 0.0;
  switch (mode) {
  case 1: // mm
    x = (x / 72.0) * 25.4;
    break;
  case 2: // in
    x /= 72;
    break;
  default:
    break;
  }
}

static const char * const mouse_units[] = { "", " mm", " in" };

void AppUi::positionChanged()
{
  Vector v = iCanvas->pos();
  const Snap &snap = iCanvas->snap();
  if (snap.iWithAxes) {
    v = v - snap.iOrigin;
    v = Linear(-snap.iDir) * v;
  }
  adjust(v.x, iMouseIn);
  adjust(v.y, iMouseIn);
  const char *units = mouse_units[iMouseIn];
  QString s;
  s.sprintf(iCoordinatesFormat.z(), v.x, units, v.y, units);
  /* TODO: if tool active, show pos relative to origin
  if (!iFileTools->isEnabled()) {
    IpeVector u = pos - iMouseBase;
    if (iSnapData.iWithAxes)
      u = IpeLinear(-iSnapData.iDir) * u;
      Adjust(u.iX, iMouseIn);
      Adjust(u.iY, iMouseIn);
      QString r;
      r.sprintf(" (%+g%s,%+g%s)", u.iX, units, u.iY, units);
      s += r;
    }
  */
  iMouse->setText(s);
}

void AppUi::toolChanged(bool hasTool)
{
  setActionsEnabled(!hasTool);
}

void AppUi::toolbarModifiersChanged()
{
  if (iCanvas) {
    int mod = 0;
    if (iShiftKey->isChecked())
      mod |= Qt::ShiftModifier;
    iCanvas->setAdditionalModifiers(mod);
  }
}

// --------------------------------------------------------------------

static String stripMark(Attribute mark)
{
  String s = mark.string();
  if (s.left(5) == "mark/") {
    int i = s.rfind('(');
    return s.substr(5, i > 0 ? i-5 : -1);
  } else
    return String();
}

void AppUi::setupSymbolicNames(const Cascade *sheet)
{
  for (int i = 0; i < EUiNum; ++i)
    iSelector[i]->clear();
  AttributeSeq seq;
  sheet->allNames(EColor, seq);
  for (uint i = 0; i < seq.size(); ++i) {
    Attribute abs = sheet->find(EColor, seq[i]);
    QIcon icon = Prefs::colorIcon(abs.color());
    iSelector[EUiStroke]->addItem(icon, QIpe(seq[i].string()));
    iSelector[EUiFill]->addItem(icon, QIpe(seq[i].string()));
  }
  seq.clear();
  sheet->allNames(EPen, seq);
  for (uint i = 0; i < seq.size(); ++i)
    iSelector[EUiPen]->addItem(QIpe(seq[i].string()));
  seq.clear();
  sheet->allNames(ETextSize, seq);
  for (uint i = 0; i < seq.size(); ++i)
    iSelector[EUiTextSize]->addItem(QIpe(seq[i].string()));
  seq.clear();
  sheet->allNames(ESymbolSize, seq);
  for (uint i = 0; i < seq.size(); ++i)
    iSelector[EUiSymbolSize]->addItem(QIpe(seq[i].string()));
  seq.clear();
  sheet->allNames(ESymbol, seq);
  for (uint i = 0; i < seq.size(); ++i) {
    String s = stripMark(seq[i]);
    if (!s.isEmpty())
      iSelector[EUiMarkShape]->addItem(QIpe(s));
  }
  seq.clear();
  sheet->allNames(EGridSize, seq);
  if (!seq.size())
    iSelector[EUiGridSize]->addItem(QString("16pt"));
  for (uint i = 0; i < seq.size(); ++i)
    iSelector[EUiGridSize]->addItem(QIpe(seq[i].string()));
  seq.clear();
  sheet->allNames(EAngleSize, seq);
  if (!seq.size())
    iSelector[EUiAngleSize]->addItem(QString("45 deg"));
  for (uint i = 0; i < seq.size(); ++i)
    iSelector[EUiAngleSize]->addItem(QIpe(seq[i].string()));
}

static void setAtt(QComboBox *b, Attribute a)
{
  String s = a.string();
  for (int i = 0; i < b->count(); ++i) {
    if (IpeQ(b->itemText(i)) == s) {
      b->setCurrentIndex(i);
      return;
    }
  }
}

void AppUi::setCheckMark(String name, Attribute a)
{
  String sa = name + "|";
  int na = sa.size();
  String sb = sa + a.string();
  for (std::map<String, QAction *>::iterator it = iActions.begin();
       it != iActions.end(); ++it) {
    if (it->first.left(na) == sa)
      it->second->setChecked(it->first == sb);
  }
}

void AppUi::setAttributes(const AllAttributes &all, Cascade *sheet)
{
  iAll = all;
  iCascade = sheet;
  iPathView->set(all, sheet);

  setAtt(iSelector[EUiStroke], iAll.iStroke);
  setAtt(iSelector[EUiFill], iAll.iFill);
  Color stroke = iCascade->find(EColor, iAll.iStroke).color();
  Color fill = iCascade->find(EColor, iAll.iFill).color();
  iButton[EUiStroke]->setIcon(Prefs::colorIcon(stroke));
  iButton[EUiFill]->setIcon(Prefs::colorIcon(fill));
  setAtt(iSelector[EUiPen], iAll.iPen);
  setAtt(iSelector[EUiTextSize], iAll.iTextSize);
  setAtt(iSelector[EUiSymbolSize], iAll.iSymbolSize);

  String s = stripMark(iAll.iMarkShape);
  for (int i = 0; i < iSelector[EUiMarkShape]->count(); ++i) {
    if (IpeQ(iSelector[EUiMarkShape]->itemText(i)) == s) {
      iSelector[EUiMarkShape]->setCurrentIndex(i);
      break;
    }
  }

  setCheckMark("horizontalalignment", Attribute(iAll.iHorizontalAlignment));
  setCheckMark("verticalalignment", Attribute(iAll.iVerticalAlignment));
  setCheckMark("pinned", Attribute(iAll.iPinned));
  setCheckMark("transformabletext",
	       Attribute::Boolean(iAll.iTransformableText));
  setCheckMark("transformations", Attribute(iAll.iTransformations));
  setCheckMark("linejoin", Attribute(iAll.iLineJoin));
  setCheckMark("linecap", Attribute(iAll.iLineCap));
  setCheckMark("fillrule", Attribute(iAll.iFillRule));
}

void AppUi::setGridAngleSize(Attribute abs_grid, Attribute abs_angle)
{
  AttributeSeq seq;
  iCascade->allNames(EGridSize, seq);
  for (uint i = 0; i < seq.size(); ++i) {
    if (iCascade->find(EGridSize, seq[i]) == abs_grid) {
      iSelector[EUiGridSize]->setCurrentIndex(i);
      break;
    }
  }
  seq.clear();
  iCascade->allNames(EAngleSize, seq);
  for (uint i = 0; i < seq.size(); ++i) {
    if (iCascade->find(EAngleSize, seq[i]) == abs_angle) {
      iSelector[EUiAngleSize]->setCurrentIndex(i);
      break;
    }
  }
}

void AppUi::setNumbers(String vno, String pno)
{
  if (vno.isEmpty()) {
    iViewNumber->hide();
  } else {
    iViewNumber->setText(QIpe(vno));
    iViewNumber->show();
  }
  if (pno.isEmpty()) {
    iPageNumber->hide();
  } else {
    iPageNumber->show();
    iPageNumber->setText(QIpe(pno));
  }
}

void AppUi::setNotes(String notes)
{
  iPageNotes->setPlainText(QIpe(notes));
}

void AppUi::setLayers(const Page *page, int view)
{
  iLayerList->set(page, view);
}

int AppUi::setBookmarks(lua_State *L)
{
  luaL_argcheck(L, lua_istable(L, 2), 2, "argument is not a table");
  int no = lua_objlen(L, 2);
  iBookmarks->clear();
  for (int i = 1; i <= no; ++i) {
    lua_rawgeti(L, 2, i);
    luaL_argcheck(L, lua_isstring(L, -1), 2, "item is not a string");
    QString s = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);
    QListWidgetItem *item = new QListWidgetItem(s);
    if (s[0] == ' ')
      item->setTextColor(Qt::blue);
    iBookmarks->addItem(item);
  }
  return 0;
}

void AppUi::bookmarkSelected(QListWidgetItem *item)
{
  int index = iBookmarks->row(item);
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "bookmark");
  lua_insert(L, -2); // method, model
  lua_pushnumber(L, index + 1);
  lua_call(L, 2, 0);
}

int AppUi::showTool(lua_State *L)
{
  static const char * const option_names[] = {
    "properties", "bookmarks", "notes", "layers", 0 };
  int m = luaL_checkoption(L, 2, 0, option_names);
  bool s = lua_toboolean(L, 3);
  QWidget *tool = 0;
  switch (m) {
  case 0: tool = iPropertiesTools; break;
  case 1: tool = iBookmarkTools; break;
  case 2: tool = iNotesTools; break;
  case 3: tool = iLayerTools; break;
  default: break;
  }
  if (s)
    tool->show();
  else
    tool->hide();
  return 0;
}

void AppUi::setZoom(double zoom)
{
  QString s;
  s.sprintf("(%dppi)", int(72.0 * zoom));
  iResolution->setText(s);
  iCanvas->setZoom(zoom);
}

// --------------------------------------------------------------------

static void enableActions(QMenu *menu, bool mode)
{
  menu->setEnabled(mode);
  QListIterator<QAction *> it(menu->actions());
  while (it.hasNext())
    it.next()->setEnabled(mode);
}

void AppUi::setActionsEnabled(bool mode)
{
  enableActions(iMenu[EFileMenu], mode);
  enableActions(iMenu[EEditMenu], mode);
  enableActions(iMenu[EModeMenu], mode);
  enableActions(iMenu[EPropertiesMenu], mode);
  enableActions(iMenu[ELayerMenu], mode);
  enableActions(iMenu[EViewMenu], mode);
  enableActions(iMenu[EPageMenu], mode);
  enableActions(iMenu[EIpeletMenu], mode);

  iModeActionGroup->setEnabled(mode);
  iPropertiesTools->setEnabled(mode);
  iLayerTools->setEnabled(mode);
  iBookmarkTools->setEnabled(mode);
}

// --------------------------------------------------------------------

//! Window has been closed
void AppUi::closeEvent(QCloseEvent* ce)
{
  // calls model
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "closeEvent");
  lua_pushvalue(L, -2); // model
  lua_remove(L, -3);
  lua_call(L, 1, 1);
  bool result = lua_toboolean(L, -1);
  if (result)
    ce->accept();
  else
    ce->ignore();
}

static void call_selector(lua_State *L, int model, String name)
{
  // calls model selector
  lua_rawgeti(L, LUA_REGISTRYINDEX, model);
  lua_getfield(L, -1, "selector");
  lua_pushvalue(L, -2); // model
  lua_remove(L, -3);
  push_string(L, name);
}

static void call_absolute(lua_State *L, int model, String name)
{
  // calls model selector
  lua_rawgeti(L, LUA_REGISTRYINDEX, model);
  lua_getfield(L, -1, "set_absolute");
  lua_pushvalue(L, -2); // model
  lua_remove(L, -3);
  push_string(L, name);
}

void AppUi::callSelector(String name, String value)
{
  call_selector(L, iModel, name);
  if (value == "true")
    lua_pushboolean(L, true);
  else if (value == "false")
    lua_pushboolean(L, false);
  else
    push_string(L, value);
  lua_call(L, 3, 0);
}

void AppUi::callSelector(const char *name, Color color)
{
  call_absolute(L, iModel, name);
  push_color(L, color);
  lua_call(L, 3, 0);
}

void AppUi::callSelector(const char *name, double scalar)
{
  call_absolute(L, iModel, name);
  lua_pushnumber(L, scalar);
  lua_call(L, 3, 0);
}

static const char * const selector_name[] =
  { "stroke", "fill", "pen", "textsize", "markshape",
    "symbolsize", "gridsize", "anglesize", "view", "page" };

void AppUi::absoluteButton(int id)
{
  // calls model selector
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "absoluteButton");
  lua_insert(L, -2); // method, model
  lua_pushstring(L, selector_name[id]);
  lua_call(L, 2, 0);
}

void AppUi::selector(int id, String value)
{
  callSelector(String(selector_name[id]), value);
}

void AppUi::comboSelector(int id)
{
  callSelector(String(selector_name[id]), IpeQ(iSelector[id]->currentText()));
}

void AppUi::layerAction(String name, String layer)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "layerAction");
  lua_insert(L, -2); // before model
  push_string(L, name);
  push_string(L, layer);
  lua_call(L, 3, 0);
}

// --------------------------------------------------------------------

static const char * const aboutText =
"<qt><h2>Ipe %d.%d.%d</h2>"
"<p>Copyright (c) 1993-2010 Otfried Cheong</p>"
"<p>The extensible drawing editor Ipe creates figures "
"in Postscript and PDF format, "
"using LaTeX to format the text in the figures.</p>"
"<p>Ipe relies on the following fine pieces of software:"
"<ul>"
"<li> Pdflatex"
"<li> %s (%d kB used)" // Lua
"<li> The font rendering library %s"
"<li> The rendering library Cairo %s / %s"
"<li> The GUI toolkit Qt %s / %s"
"<li> The compression library zlib %s"
"<li> Some code from Xpdf"
"</ul>"
"<p>Ipe is released under the GNU Public License.</p>"
"<p>See <a href=\"http://ipe7.sourceforge.net\">ipe7.sourceforge.net</a>"
" for details.</p>"
"</qt>";

void AppUi::aboutIpe()
{
  int luaSize = lua_gc(L, LUA_GCCOUNT, 0);
  std::vector<char> buf(strlen(aboutText) + 100);
  sprintf(&buf[0], aboutText,
	  IPELIB_VERSION / 10000,
	  (IPELIB_VERSION / 100) % 100,
	  IPELIB_VERSION % 100,
	  LUA_RELEASE, luaSize,
	  Fonts::freetypeVersion().z(),
	  CAIRO_VERSION_STRING, cairo_version_string(),
	  QT_VERSION_STR, qVersion(),
	  ZLIB_VERSION);

  QMessageBox msgBox(this);
  msgBox.setWindowTitle("About Ipe");
  msgBox.setWindowIcon(Prefs::get()->icon("ipe"));
  msgBox.setInformativeText(&buf[0]);
  msgBox.setIconPixmap(Prefs::get()->pixmap("ipe"));
  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.exec();
}

void AppUi::qAction(const QString &name)
{
  action(IpeQ(name));
}

void AppUi::action(String name)
{
  if (name == "fullscreen") {
    setWindowState(windowState() ^ Qt::WindowFullScreen);
  } else if (name == "about") {
    aboutIpe();
  } else if (name.left(12) == "coordinates|") {
    if (name.right(2) == "mm")
      iMouseIn = 1;
    else if (name.right(4) == "inch")
      iMouseIn = 2;
    else
      iMouseIn = 0;
  } else if (name.find('|') >= 0) {
    // calls model selector
    int i = name.find('|');
    callSelector(name.left(i), name.substr(i+1));
  } else {
    // calls model action
    lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
    lua_getfield(L, -1, "action");
    lua_insert(L, -2); // before model
    push_string(L, name);
    lua_call(L, 2, 0);
  }
}

// --------------------------------------------------------------------

void AppUi::mouseAction(int button)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "mouseAction");
  lua_insert(L, -2); // before model
  push_button(L, button);
  lua_call(L, 3, 0);
}

// --------------------------------------------------------------------

void AppUi::showPathStylePopup(Vector v)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "showPathStylePopup");
  lua_insert(L, -2); // before model
  push_vector(L, v);
  lua_call(L, 2, 0);
}

void AppUi::showLayerBoxPopup(Vector v, String layer)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "showLayerBoxPopup");
  lua_insert(L, -2); // before model
  push_vector(L, v);
  push_string(L, layer);
  lua_call(L, 3, 0);
}

// --------------------------------------------------------------------
