// --------------------------------------------------------------------
// AppUi for QT
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2014  Otfried Cheong

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

#include "appui_qt.h"
#include "ipecanvas_qt.h"
#include "controls_qt.h"

#include "ipelua.h"

// for version info only
#include "ipefonts.h"
#include <zlib.h>

#include <cstdio>
#include <cstdlib>

#include <QMenuBar>
#include <QKeySequence>
#include <QCloseEvent>
#include <QStatusBar>
#include <QGridLayout>
#include <QButtonGroup>
#include <QMessageBox>
#include <QSignalMapper>
#include <QToolTip>

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

#define SIZE 16

QIcon prefsIcon(String name)
{
  String dir = ipeIconDirectory();
  if (Platform::fileExists(dir + name + ".png"))
    return QIcon(QIpe(dir + name + ".png"));
  else
    return QIcon();
}

QPixmap prefsPixmap(String name)
{
  return QPixmap(QIpe(ipeIconDirectory() + name + ".png"));
}

QIcon prefsColorIcon(Color color)
{
  QPixmap pixmap(SIZE, SIZE);
  pixmap.fill(QIpe(color));
  return QIcon(pixmap);
}

// --------------------------------------------------------------------

IpeApp::IpeApp(lua_State *L0, int &argc, char **argv)
  : QApplication(argc, argv)
{
  L = L0;
}

bool IpeApp::event(QEvent *event)
{
  switch (event->type()) {
  case QEvent::FileOpen:
    loadFile(static_cast<QFileOpenEvent *>(event)->file());
    return true;
  default:
    return QApplication::event(event);
  }
}

void IpeApp::loadFile(const QString &fileName)
{
  lua_getglobal(L, "file_open_event");
  lua_pushstring(L, fileName.toUtf8());
  lua_call(L, 1, 0);
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

// Not used
int AppUi::actionId(const char *name) const
{
  return 0;
}

void AppUi::addItem(QMenu *m, const QString &title, const char *name)
{
  // bool canUseWhileDrawing = false;
  if (name[0] == '@') {
    // canUseWhileDrawing = true;
    name = name + 1;
  }
  bool checkable = (m == iMenu[EModeMenu]) ||
    (String(name).find('|') >= 0);
  if (name[0] == '*') {
    checkable = true;
    name = name + 1;
  }
  QAction *a = new QAction(title, this);
  if (checkable)
    a->setCheckable(true);
  lua_getglobal(L, "shortcuts");
  lua_getfield(L, -1, name);
  if (lua_isstring(L, -1)) {
    QString s = lua_tostring(L, -1);
    a->setShortcut(QKeySequence(s));
    QString tt = title + " [" + s + "]";
    a->setToolTip(tt);
  } else if (lua_istable(L, -1)) {
    int no = lua_rawlen(L, -1);
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
  a->setIcon(prefsIcon(name));
  lua_pop(L, 2);
  m->addAction(a);
  if (m == iMenu[EModeMenu]) {
    a->setActionGroup(iModeActionGroup);
    iObjectTools->addAction(a);
  }
  connect(a, SIGNAL(triggered()), iActionMap, SLOT(map()));
  iActionMap->setMapping(a, name);
  iActions[name] = a;
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

void AppUi::addRootMenu(int id, const char *name)
{
  iMenu[id] = menuBar()->addMenu(name);
}

// if title and name == 0, add separator (default)
void AppUi::addItem(int id, const char *title, const char *name)
{
  if (title == 0)
    iMenu[id]->addSeparator();
  else
    addItem(iMenu[id], QString::fromUtf8(title), name);
}

static QMenu *submenu = 0;
static int submenuId = 0;

void AppUi::startSubMenu(int id, const char *name)
{
  submenuId = id;
  submenu = new QMenu(name);
}

void AppUi::addSubItem(const char *title, const char *name)
{
  addItem(submenu, title, name);
}

MENUHANDLE AppUi::endSubMenu()
{
  iMenu[submenuId]->addMenu(submenu);
  return submenu;
}

Qt::DockWidgetArea getDockSide(lua_State *L, const char *name,
			       Qt::DockWidgetArea deflt)
{
  Qt::DockWidgetArea r = deflt;
  lua_getglobal(L, "prefs");
  lua_getfield(L, -1, "docks");
  if (lua_istable(L, -1)) {
    lua_getfield(L, -1, name);
    const char *s = lua_tolstring(L, -1, 0);
    if (!strcmp(s, "left"))
      r = Qt::LeftDockWidgetArea;
    else if (!strcmp(s, "right"))
      r = Qt::RightDockWidgetArea;
    lua_pop(L, 1); // left or right
  }
  lua_pop(L, 2); // prefs, docks
  return r;
}

// --------------------------------------------------------------------

extern void qt_set_sequence_auto_mnemonic(bool b);

AppUi::AppUi(lua_State *L0, int model, Qt::WFlags f)
  : QMainWindow(0, f), AppUiBase(L0, model)
{
  setWindowIcon(prefsIcon("ipe"));
  QMainWindow::setAttribute(Qt::WA_DeleteOnClose);
  setDockOptions(AnimatedDocks); // do not allow stacking properties and layers

  // enable automatic shortcuts on Mac OS X
  qt_set_sequence_auto_mnemonic(true);

  Canvas *canvas = new Canvas(this);
  iCanvas = canvas;
  setCentralWidget(canvas);

  iSnapTools = addToolBar("Snap");
  iEditTools = addToolBar("Edit");
  addToolBarBreak();
  iObjectTools = addToolBar("Objects");

  iActionMap = new QSignalMapper(this);
  connect(iActionMap, SIGNAL(mapped(const QString &)),
	  SLOT(qAction(const QString &)));

  iModeActionGroup = new QActionGroup(this);
  iModeActionGroup->setExclusive(true);
  QActionGroup *cg = new QActionGroup(this);
  cg->setExclusive(true);
  QActionGroup *cs = new QActionGroup(this);
  cs->setExclusive(true);

  buildMenus();

  findAction("coordinates|points")->setActionGroup(cg);
  findAction("coordinates|mm")->setActionGroup(cg);
  findAction("coordinates|m")->setActionGroup(cg);
  findAction("coordinates|inch")->setActionGroup(cg);

  for (uint i = 0; i < iScalings.size(); ++i) {
    char action[32];
    sprintf(action, "scaling|%d", iScalings[i]);
    findAction(action)->setActionGroup(cs);
  }

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
  iShiftKey->setIcon(prefsIcon("shift_key"));
  iEditTools->addAction(iShiftKey);
  iEditTools->addAction(findAction("grid_visible"));
  connect(iShiftKey, SIGNAL(triggered()), SLOT(toolbarModifiersChanged()));

  iPropertiesTools = new QDockWidget("Properties", this);
  addDockWidget(getDockSide(L, "properties", Qt::LeftDockWidgetArea),
		iPropertiesTools);
  iPropertiesTools->setAllowedAreas(Qt::LeftDockWidgetArea|
				    Qt::RightDockWidgetArea);

  iBookmarkTools = new QDockWidget("Bookmarks", this);
  addDockWidget(getDockSide(L, "bookmarks", Qt::RightDockWidgetArea),
		iBookmarkTools);
  iBookmarkTools->setAllowedAreas(Qt::LeftDockWidgetArea|
				  Qt::RightDockWidgetArea);
  iMenu[EPageMenu]->addAction(iBookmarkTools->toggleViewAction());

  iNotesTools = new QDockWidget("Notes", this);
  addDockWidget(getDockSide(L, "notes", Qt::RightDockWidgetArea),
		iNotesTools);
  iNotesTools->setAllowedAreas(Qt::LeftDockWidgetArea|
			       Qt::RightDockWidgetArea);
  iMenu[EPageMenu]->addAction(iNotesTools->toggleViewAction());

  iLayerTools = new QDockWidget("Layers", this);
  addDockWidget(getDockSide(L, "layers", Qt::LeftDockWidgetArea),
		iLayerTools);
  iLayerTools->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);

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
  iButton[EUiStroke]->setIcon(prefsColorIcon(Color(1000, 0, 0)));
  iButton[EUiFill]->setIcon(prefsColorIcon(Color(1000, 1000, 0)));
  iButton[EUiPen]->setIcon(prefsIcon("pen"));
  iButton[EUiTextSize]->setIcon(prefsIcon("mode_label"));
  iButton[EUiSymbolSize]->setIcon(prefsIcon("mode_marks"));

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
  msl->setPixmap(prefsPixmap("mode_marks"));
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
  iViewMarked = new QCheckBox();
  iPageMarked = new QCheckBox();
  iViewMarked->setFocusPolicy(Qt::NoFocus);
  iPageMarked->setFocusPolicy(Qt::NoFocus);
  bg->addButton(iViewNumber, EUiView);
  bg->addButton(iPageNumber, EUiPage);
  bg->addButton(iViewMarked, EUiViewMarked);
  bg->addButton(iPageMarked, EUiPageMarked);
  hol->setSpacing(0);
  hol->addWidget(iViewMarked);
  hol->addWidget(iViewNumber);
  hol->addStretch(1);
  hol->addWidget(iPageMarked);
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
  findAction("coordinates|points")->setChecked(true);
  statusBar()->addPermanentWidget(iMouse, 0);
  QFont font = iMouse->font();
  font.setFamily("Monospace");
  iMouse->setFont(font);

  iResolution = new QLabel(statusBar());
  statusBar()->addPermanentWidget(iResolution, 0);

  iCanvas->setObserver(this);
}

AppUi::~AppUi()
{
  ipeDebug("AppUi C++ destructor");
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

void AppUi::toolbarModifiersChanged()
{
  if (iCanvas) {
    int mod = 0;
    if (iShiftKey->isChecked())
      mod |= CanvasBase::EShift;
    iCanvas->setAdditionalModifiers(mod);
  }
}

// --------------------------------------------------------------------

void AppUi::resetCombos()
{
  for (int i = 0; i < EUiView; ++i)
    iSelector[i]->clear();
}

void AppUi::addComboColor(int sel, String s, Color color)
{
  QIcon icon = prefsColorIcon(color);
  iSelector[sel]->addItem(icon, QIpe(s));
}

void AppUi::addCombo(int sel, String s)
{
  iSelector[sel]->addItem(QIpe(s));
}

void AppUi::setComboCurrent(int sel, int idx)
{
  iSelector[sel]->setCurrentIndex(idx);
}

void AppUi::setButtonColor(int sel, Color color)
{
  iButton[sel]->setIcon(prefsColorIcon(color));
}

void AppUi::setPathView(const AllAttributes &all, Cascade *sheet)
{
  iPathView->set(all, sheet);
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

void AppUi::setNumbers(String vno, bool vm, String pno, bool pm)
{
  if (vno.isEmpty()) {
    iViewNumber->hide();
    iViewMarked->hide();
  } else {
    iViewNumber->setText(QIpe(vno));
    iViewNumber->show();
    iViewMarked->setCheckState(vm ? Qt::Checked : Qt::Unchecked);
    iViewMarked->show();
  }
  if (pno.isEmpty()) {
    iPageNumber->hide();
    iPageMarked->hide();
  } else {
    iPageNumber->show();
    iPageMarked->show();
    iPageNumber->setText(QIpe(pno));
    iPageMarked->setCheckState(pm ? Qt::Checked : Qt::Unchecked);
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

void AppUi::setBookmarks(int no, const String *s)
{
  iBookmarks->clear();
  for (int i = 0; i < no; ++i) {
    QListWidgetItem *item = new QListWidgetItem(QIpe(s[i]));
    if (s[i][0] == ' ')
      item->setTextColor(Qt::blue);
    iBookmarks->addItem(item);
  }
}

void AppUi::bookmarkSelected(QListWidgetItem *item)
{
  int index = iBookmarks->row(item);
  luaBookmarkSelected(index);
}

void AppUi::setToolVisible(int m, bool vis)
{
  QWidget *tool = 0;
  switch (m) {
  case 0: tool = iPropertiesTools; break;
  case 1: tool = iBookmarkTools; break;
  case 2: tool = iNotesTools; break;
  case 3: tool = iLayerTools; break;
  default: break;
  }
  if (vis)
    tool->show();
  else
    tool->hide();
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

// --------------------------------------------------------------------

// Determine if action is checked
// Used for snapXXX, grid_visible, viewmarked, and pagemarked
bool AppUi::actionState(const char *name)
{
  if (!strcmp(name, "viewmarked"))
    return (iViewMarked->checkState() == Qt::Checked);
  if (!strcmp(name, "pagemarked"))
    return (iPageMarked->checkState() == Qt::Checked);

  QAction *a = findAction(name);
  if (a)
    return a->isChecked();
  else
    return 0;
}

// Check/uncheck an action
// Used for snapXXX, grid_visible, to initialize mode_select
void AppUi::setActionState(const char *name, bool value)
{
  QAction *a = findAction(name);
  if (a) a->setChecked(value);
}

void AppUi::absoluteButton(int id)
{
  luaAbsoluteButton(selectorNames[id]);
}

void AppUi::selector(int id, String value)
{
  luaSelector(String(selectorNames[id]), value);
}

void AppUi::comboSelector(int id)
{
  luaSelector(String(selectorNames[id]), IpeQ(iSelector[id]->currentText()));
}

// --------------------------------------------------------------------

static const char * const aboutText =
"<qt><h2>Ipe %d.%d.%d</h2>"
"<p>Copyright (c) 1993-2014 Otfried Cheong</p>"
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
  msgBox.setWindowIcon(prefsIcon("ipe"));
  msgBox.setInformativeText(&buf[0]);
  msgBox.setIconPixmap(prefsPixmap("ipe"));
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
  } else
    luaAction(name);
}

// --------------------------------------------------------------------

void AppUi::showPathStylePopup(Vector v)
{
  luaShowPathStylePopup(v);
}

void AppUi::showLayerBoxPopup(Vector v, String layer)
{
  luaShowLayerBoxPopup(v, layer);
}

void AppUi::layerAction(String name, String layer)
{
  luaLayerAction(name, layer);
}

// --------------------------------------------------------------------

WINID AppUi::windowId()
{
  return this;
}

void AppUi::closeWindow()
{
  close();
}

void AppUi::setWindowCaption(bool mod, const char *s)
{
  setWindowModified(mod);
  setWindowTitle(QString::fromUtf8(s));
}

void AppUi::setMouseIndicator(const char *s)
{
  iMouse->setText(s);
}

void AppUi::explain(const char *s, int t)
{
  statusBar()->showMessage(QString::fromUtf8(s), t);
}

void AppUi::showWindow(int width, int height)
{
  if (width > 0 && height > 0)
    resize(width, height);
  show();
}

AppUiBase *createAppUi(lua_State *L0, int model)
{
  return new AppUi(L0, model);
}

// --------------------------------------------------------------------
