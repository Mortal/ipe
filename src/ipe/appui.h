// -*- C++ -*-
// --------------------------------------------------------------------
// Appui
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

#ifndef APPUI_H
#define APPUI_H

#include "ipeqtcanvas.h"

#include <QMainWindow>
#include <QAction>
#include <QToolBar>
#include <QDockWidget>
#include <QActionGroup>
#include <QListWidget>
#include <QLabel>
#include <QComboBox>
#include <QToolButton>
#include <QTextEdit>

using namespace ipe;
using namespace ipeqt;

// avoid including Lua headers here
typedef struct lua_State lua_State;

// --------------------------------------------------------------------

class Prefs {
public:
  static Prefs *get();
  QIcon icon(String name);
  static QIcon colorIcon(Color color);
  QPixmap pixmap(String name);
  String iconDir() const { return iIconDir; }

private:
  Prefs();

private:
  static Prefs *singleton;
  String iIconDir;
};

// --------------------------------------------------------------------

class LayerBox : public QListWidget {
  Q_OBJECT
public:
  LayerBox(QWidget *parent = 0);

  void set(const Page *page, int view);

signals:
  void activated(String name, String layer);
  void showLayerBoxPopup(Vector v, String layer);

public slots:
  void layerChanged(QListWidgetItem *item);

protected:
  virtual void mouseReleaseEvent(QMouseEvent *e);
  virtual void mousePressEvent(QMouseEvent *e);

  void addAction(QMenu *m, QListWidgetItem *item, String name,
		 const QString &text);

private:
  bool iInSet;
};

// --------------------------------------------------------------------

class PathView : public QWidget {
  Q_OBJECT

public:
  PathView(QWidget* parent = 0, Qt::WFlags f = 0);

  void set(const AllAttributes &all, Cascade *sheet);

  virtual QSize sizeHint() const;

signals:
  void activated(String name);
  void showPathStylePopup(Vector v);

protected:
  virtual void paintEvent(QPaintEvent *ev);
  virtual void mouseReleaseEvent(QMouseEvent *e);
  virtual void mousePressEvent(QMouseEvent *e);
  virtual bool event(QEvent *ev);

private:
  Cascade *iCascade;
  AllAttributes iAll;
};

// --------------------------------------------------------------------

class QSignalMapper;

class AppUi : public QMainWindow {
  Q_OBJECT

public:
  enum { EFileMenu, EEditMenu, EPropertiesMenu, ESnapMenu,
	 EModeMenu, EZoomMenu, ELayerMenu, EViewMenu,
	 EPageMenu, EIpeletMenu, EHelpMenu, ENumMenu };

  // change list in front of AppUi::selector if enum changes
  enum { EUiStroke, EUiFill, EUiPen, EUiTextSize, EUiMarkShape,
	 EUiSymbolSize, EUiGridSize, EUiAngleSize, EUiNum,
	 EUiView = EUiNum, EUiPage };

public:
  AppUi(lua_State *L0, int model, Qt::WFlags f=0);
  ~AppUi();

  Canvas *canvas() { return iCanvas; }
  void setupSymbolicNames(const Cascade *sheet);
  void setAttributes(const AllAttributes &all, Cascade *sheet);
  void setGridAngleSize(Attribute abs_grid, Attribute abs_angle);
  void setLayers(const Page *page, int view);
  void setZoom(double zoom);
  void setActionsEnabled(bool mode);
  QAction *findAction(const char *name) const;
  void setNumbers(String vno, String pno);
  void setNotes(String notes);

  // direct Lua methods
  int setBookmarks(lua_State *L);
  int showTool(lua_State *L);

public slots:
  void action(String name);
  void qAction(const QString &name);
  void selectLayerAction(QAction *a);
  void moveToLayerAction(QAction *a);
  void textStyleAction(QAction *a);

  void layerAction(String name, String layer);
  void mouseAction(int button);
  void positionChanged();
  void toolChanged(bool hasTool);
  void toolbarModifiersChanged();
  void aboutIpe();

  void wheelZoom(int degrees);
  void absoluteButton(int id);
  void selector(int id, String value);
  void comboSelector(int id);

  void bookmarkSelected(QListWidgetItem *item);

  void aboutToShowSelectLayerMenu();
  void aboutToShowMoveToLayerMenu();
  void aboutToShowTextStyleMenu();
  void showPathStylePopup(Vector v);
  void showLayerBoxPopup(Vector v, String layer);

private:
  void addItem(QMenu *m, const QString &title, const char *name);
  void addItem(int m, const QString &title, const char *name);
  void addSnap(const char *name);
  void addEdit(const char *name);
  void buildMenus();
  void callSelector(String name, String value);
  void callSelector(const char *name, Color color);
  void callSelector(const char *name, double scalar);
  void setCheckMark(String name, Attribute a);

protected:
  void closeEvent(QCloseEvent *ev);

private:
  lua_State *L;
  int iModel;  // reference to Lua model

  Cascade *iCascade;
  AllAttributes iAll;  // current settings in UI

  Canvas *iCanvas;
  PathView *iPathView;

  QMenu *iMenu[ENumMenu];
  QMenu *iSelectLayerMenu;
  QMenu *iMoveToLayerMenu;
  QMenu *iTextStyleMenu;

  QToolButton *iButton[EUiGridSize];
  QComboBox *iSelector[EUiNum];

  QToolButton *iViewNumber;
  QToolButton *iPageNumber;

  QToolBar *iSnapTools;
  QToolBar *iEditTools;
  QToolBar *iObjectTools;

  QDockWidget *iPropertiesTools;
  QDockWidget *iLayerTools;
  QDockWidget *iBookmarkTools;
  QDockWidget *iNotesTools;

  QActionGroup *iModeActionGroup;

  QAction *iShiftKey;

  QListWidget *iBookmarks;
  LayerBox *iLayerList;
  QTextEdit *iPageNotes;

  String iCoordinatesFormat;
  int iMouseIn;
  QLabel *iMouse;
  QLabel *iResolution;

  QSignalMapper *iActionMap;
  std::map<String, QAction *> iActions;
};

// --------------------------------------------------------------------
#endif
