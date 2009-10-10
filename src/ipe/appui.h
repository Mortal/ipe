// -*- C++ -*-
// --------------------------------------------------------------------
// Appui
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

class IpeAction : public QAction {
  Q_OBJECT
public:
  IpeAction(String name, const QString &text, QObject *parent);
  String name() const { return iName; }

signals:
  void triggered(String name);
private slots:
  void forwardTrigger();
private:
  String iName;
};

// --------------------------------------------------------------------

class IpeComboBox : public QComboBox {
  Q_OBJECT

public:
  IpeComboBox(int id, QWidget *parent = 0);

signals:
  void activated(int id, String name);
private slots:
  void forwardActivated(const QString &text);

private:
  int iD;
};

// --------------------------------------------------------------------

class LayerBox : public QListWidget {
  Q_OBJECT
public:
  LayerBox(QWidget *parent = 0);

  void set(const Page *page, int view);

signals:
  void activated(String name, String layer);

public slots:
  void action(String name);
  void layerChanged(QListWidgetItem *item);

protected:
  virtual void mouseReleaseEvent(QMouseEvent *e);
  virtual void mousePressEvent(QMouseEvent *e);

  void addAction(QMenu *m, QListWidgetItem *item, String name,
		 const QString &text);

private:
  bool iInSet;
  enum { EIsLocked = 0x01, EHasNoSnapping = 0x02,
	 ECanDelete = 0x04, EIsActive = 0x08 };
  std::vector<uint> iFlags;
};

// --------------------------------------------------------------------

class AppUi;

class PathView : public QWidget {
  Q_OBJECT

public:
  PathView(QWidget* parent = 0, Qt::WFlags f = 0);

  void set(const AllAttributes &all, Cascade *sheet);

  virtual QSize sizeHint() const;

signals:
  void activated(String name);
  void showPathStylePopup(Vector v);

public slots:
  void action(String name);

protected:
  virtual void paintEvent(QPaintEvent *ev);
  virtual void mouseReleaseEvent(QMouseEvent *e);
  virtual void mousePressEvent(QMouseEvent *e);

private:
  Cascade *iCascade;
  AllAttributes iAll;
};

// --------------------------------------------------------------------

class AppUi : public QMainWindow {
  Q_OBJECT

public:
  enum { EFileMenu, EEditMenu, EPropertiesMenu, ESnapMenu,
	 EModeMenu, EZoomMenu, ELayerMenu, EViewMenu,
	 EPageMenu, EIpeletMenu, EHelpMenu, ENumMenu };

  // change list in front of AppUi::selector if enum changes
  enum { EUiStroke, EUiFill, EUiPen, EUiTextSize, EUiMarkShape,
	 EUiSymbolSize, EUiGridSize, EUiAngleSize, EUiNum };

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
  IpeAction *findAction(const char *name) const;

  // direct Lua methods
  int setBookmarks(lua_State *L);

public slots:
  void action(String name);
  void layerAction(String name, String layer);
  void mouseAction(int button);
  void positionChanged();
  void toolChanged(bool hasTool);
  void aboutIpe();

  void wheelZoom(int degrees);
  void absoluteButton(int id);
  void selector(int id, String value);
  void bookmarkSelected(QListWidgetItem *item);

  void aboutToShowSelectLayerMenu();
  void aboutToShowMoveToLayerMenu();
  void aboutToShowTextStyleMenu();
  void showPathStylePopup(Vector v);

private:
  void addItem(QMenu *m, const QString &title, const char *name);
  void addItem(int m, const QString &title, const char *name);
  void addSnap(const char *name);
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
  IpeComboBox *iSelector[EUiNum];

  QToolBar *iSnapTools;
  QToolBar *iObjectTools;

  QDockWidget *iPropertiesTools;
  QDockWidget *iLayerTools;
  QDockWidget *iBookmarkTools;

  QActionGroup *iModeActionGroup;

  QListWidget *iBookmarks;
  LayerBox *iLayerList;

  int iMouseIn;
  QLabel *iMouse;
  QLabel *iResolution;

  std::vector<IpeAction *> iActions;
};

// --------------------------------------------------------------------
#endif
