// -*- C++ -*-
// --------------------------------------------------------------------
// Appui for Win32
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

#ifndef APPUI_WIN_H
#define APPUI_WIN_H

#include "appui.h"

class PathView;

using namespace ipe;

// --------------------------------------------------------------------

class AppUi : public AppUiBase {
public:
  static void init(HINSTANCE hInstance);
  AppUi(lua_State *L0, int model);
  ~AppUi();

  virtual void action(String name);
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

private:
  void initUi();
  void layoutChildren();
  void createAction(String name, String tooltip, bool canWhileDrawing = false);
  int findAction(const char *name) const;
  void addItem(HMENU menu, const char *title, const char *name);
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

  void populateTextStyleMenu();
  void populateLayerMenus();
  void cmd(int id, int notification);
  void setCheckMark(String name, String value);
  void aboutIpe();
  void closeRequested();
  int iconId(const char *name) const;
  void addTButton(HWND tb, const char *name = 0, int flags = 0);
  void setTooltip(HWND h, String tip, bool isComboBoxEx = false);
  void toggleVisibility(String action, HWND h);
  HWND createButton(HINSTANCE hInst, int id,
		    int flags = BS_BITMAP|BS_PUSHBUTTON);

private:
  static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message,
				  WPARAM wParam, LPARAM lParam);
  static BOOL CALLBACK enumThreadWndProc(HWND hwnd, LPARAM lParam);
  static const char className[];
private:
  struct SAction {
    String name;
    String tooltip;
    int icon;
    bool alwaysOn;
  };
  std::vector<SAction> iActions;
  HMENU hMenuBar;
  HMENU hRootMenu[ENumMenu];
  HIMAGELIST hIcons;
  HWND hwnd;

  HWND hwndCanvas;

  HWND hTip;
  HWND hStatusBar;
  HWND hToolBar;
  HWND hNotes;
  HWND hBookmarks;

  HWND hProperties;

  HWND hButton[EUiGridSize];
  HWND hSelector[EUiView];
  HWND hViewNumber;
  HWND hPageNumber;
  HWND hViewMarked;
  HWND hPageMarked;
  PathView *iPathView;
};

// --------------------------------------------------------------------
#endif
