// -*- C++ -*-
// --------------------------------------------------------------------
// Appui
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

#ifndef APPUI_H
#define APPUI_H

#include "ipecanvas.h"

using namespace ipe;

// avoid including Lua headers here
typedef struct lua_State lua_State;

#ifdef IPEUI_GTK
#include <gtk/gtk.h>
typedef GtkWidget *WINID;
typedef GtkMenu *MENUHANDLE;
#endif
#ifdef IPEUI_WIN32
#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0501
#include <windows.h>
#include <commctrl.h>
typedef HWND WINID;
typedef HMENU MENUHANDLE;
#endif
#ifdef IPEUI_QT
#include <QWidget>
#include <QMenu>
typedef QWidget *WINID;
typedef QMenu *MENUHANDLE;
#endif

class AppUiBase;
extern WINID check_winid(lua_State *L, int i);
extern void push_winid(lua_State *L, WINID win);
extern String ipeIconDirectory();
extern AppUiBase *createAppUi(lua_State *L0, int model);

// --------------------------------------------------------------------

class AppUiBase : public CanvasObserver {
public:
  enum { EFileMenu, EEditMenu, EPropertiesMenu, ESnapMenu,
	 EModeMenu, EZoomMenu, ELayerMenu, EViewMenu,
	 EPageMenu, EIpeletMenu, EHelpMenu, ENumMenu };

  // change list selectorNames if enum changes
  enum { EUiStroke, EUiFill, EUiPen, EUiTextSize, EUiMarkShape,
	 EUiSymbolSize, EUiGridSize, EUiAngleSize,
	 EUiView, EUiPage, EUiViewMarked, EUiPageMarked };

public:
  virtual ~AppUiBase();

  CanvasBase *canvas() { return iCanvas; }

  void setupSymbolicNames(const Cascade *sheet);
  void setGridAngleSize(Attribute abs_grid, Attribute abs_angle);
  void setAttributes(const AllAttributes &all, Cascade *sheet);

public:  // What platforms must implement:
  virtual WINID windowId() = 0;
  virtual int actionId(const char *name) const = 0;
  virtual void closeWindow() = 0;
  virtual bool actionState(const char *name) = 0;
  virtual void setActionState(const char *name, bool value) = 0;
  virtual void setNumbers(String vno, bool vm, String pno, bool pm) = 0;
  virtual void setLayers(const Page *page, int view) = 0;
  virtual void setZoom(double zoom) = 0;
  virtual void setWindowCaption(bool mod, const char *s) = 0;
  virtual void setNotes(String notes) = 0;
  virtual void explain(const char *s, int t) = 0;
  virtual void showWindow(int width, int height) = 0;
  virtual void action(String name) = 0;
  virtual void setActionsEnabled(bool mode) = 0;
  virtual void setMouseIndicator(const char *s) = 0;
  virtual void setBookmarks(int no, const String *s) = 0;
  virtual void setToolVisible(int m, bool vis) = 0;

protected:   // What platforms must implement:
  virtual void addRootMenu(int id, const char *name) = 0;
  // if title == 0, add separator
  virtual void addItem(int id, const char *title = 0,
		       const char *name = 0) = 0;
  virtual void startSubMenu(int id, const char *name) = 0;
  virtual void addSubItem(const char *title, const char *name) = 0;
  virtual MENUHANDLE endSubMenu() = 0;
  virtual void addCombo(int sel, String s) = 0;
  virtual void resetCombos() = 0;
  virtual void addComboColor(int sel, String s, Color color) = 0;
  virtual void setComboCurrent(int sel, int idx) = 0;
  virtual void setPathView(const AllAttributes &all, Cascade *sheet) = 0;
  virtual void setCheckMark(String name, Attribute a) = 0;
  virtual void setButtonColor(int sel, Color color) = 0;

protected:
  virtual void canvasObserverWheelMoved(int degrees);
  virtual void canvasObserverMouseAction(int button);
  virtual void canvasObserverPositionChanged();
  virtual void canvasObserverToolChanged(bool hasTool);
  virtual void canvasObserverSizeChanged();

protected:
  AppUiBase(lua_State *L0, int model);
  static const char * const selectorNames[];

  void luaSelector(String name, String value);
  void luaAction(String name);
  void luaShowPathStylePopup(Vector v);
  void luaShowLayerBoxPopup(Vector v, String layer);
  void luaLayerAction(String name, String layer);
  void luaAbsoluteButton(const char *s);
  void luaBookmarkSelected(int index);

  void buildMenus();
  void showInCombo(const Cascade *sheet, Kind kind,
		   int sel, const char *deflt = 0);
  void showMarksInCombo(const Cascade *sheet);
  void setAttribute(int sel, Attribute a);

protected:
  lua_State *L;
  int iModel;  // reference to Lua model

  MENUHANDLE iSelectLayerMenu;
  MENUHANDLE iMoveToLayerMenu;
  MENUHANDLE iTextStyleMenu;

  Cascade *iCascade;
  AllAttributes iAll;  // current settings in UI
  std::vector<String> iComboContents[EUiView];

  CanvasBase *iCanvas;

  std::vector<int> iScalings;
  String iCoordinatesFormat;
  int iMouseIn;
  double iMouseFactor;
};

// --------------------------------------------------------------------
#endif
