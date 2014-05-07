// -*- C++ -*-
// --------------------------------------------------------------------
// Appui for QT
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

#ifndef APPUI_QT_H
#define APPUI_QT_H

#include "appui.h"

#include <QApplication>
#include <QMainWindow>
#include <QAction>
#include <QCheckBox>
#include <QToolBar>
#include <QDockWidget>
#include <QActionGroup>
#include <QListWidget>
#include <QLabel>
#include <QComboBox>
#include <QToolButton>
#include <QTextEdit>

using namespace ipe;

// --------------------------------------------------------------------

class PathView;
class LayerBox;
class QSignalMapper;

// --------------------------------------------------------------------

class IpeApp : public QApplication {
  Q_OBJECT
public:
  IpeApp(lua_State *L0, int &argc, char **argv);
protected:
  virtual bool event(QEvent *);
private:
  void loadFile(const QString &fileName);
  lua_State *L;
};

// --------------------------------------------------------------------

class AppUi : public QMainWindow, public AppUiBase {
  Q_OBJECT

public:
  AppUi(lua_State *L0, int model, Qt::WFlags f=0);
  ~AppUi();

  QAction *findAction(const char *name) const;

  virtual void setLayers(const Page *page, int view);

  virtual void setZoom(double zoom);
  virtual void setActionsEnabled(bool mode);
  virtual void setNumbers(String vno, bool vm, String pno, bool pm);
  virtual void setNotes(String notes);

  virtual int actionId(const char *name) const;
  virtual WINID windowId();
  virtual void closeWindow();
  virtual bool actionState(const char *name);
  virtual void setActionState(const char *name, bool value);
  virtual void setWindowCaption(bool mod, const char *s);
  virtual void explain(const char *s, int t);
  virtual void showWindow(int width, int height);
  virtual void setBookmarks(int no, const String *s);
  virtual void setToolVisible(int m, bool vis);

public slots:
  void action(String name);
  void qAction(const QString &name);
  void selectLayerAction(QAction *a);
  void moveToLayerAction(QAction *a);
  void textStyleAction(QAction *a);

  void layerAction(String name, String layer);
  void toolbarModifiersChanged();
  void aboutIpe();

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

private:
  virtual void addRootMenu(int id, const char *name);
  virtual void addItem(int id, const char *title, const char *name);
  virtual void startSubMenu(int id, const char *name);
  virtual void addSubItem(const char *title, const char *name);
  virtual MENUHANDLE endSubMenu();
  virtual void setMouseIndicator(const char *s);
  virtual void addCombo(int sel, String s);
  virtual void resetCombos();
  virtual void addComboColor(int sel, String s, Color color);
  virtual void setComboCurrent(int sel, int idx);
  virtual void setCheckMark(String name, Attribute a);
  virtual void setPathView(const AllAttributes &all, Cascade *sheet);
  virtual void setButtonColor(int sel, Color color);

protected:
  void closeEvent(QCloseEvent *ev);

private:
  PathView *iPathView;

  QMenu *iMenu[ENumMenu];

  QToolButton *iButton[EUiGridSize];
  QComboBox *iSelector[EUiView];

  QToolButton *iViewNumber;
  QToolButton *iPageNumber;

  QCheckBox *iViewMarked;
  QCheckBox *iPageMarked;

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

  QLabel *iMouse;
  QLabel *iResolution;

  QSignalMapper *iActionMap;
  std::map<String, QAction *> iActions;
};

// --------------------------------------------------------------------
#endif
