// -*- C++ -*-
// --------------------------------------------------------------------
// Appui for GTK
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

#ifndef APPUI_GTK_H
#define APPUI_GTK_H

#include "appui.h"

using namespace ipe;

// --------------------------------------------------------------------

class AppUi : public AppUiBase {
public:
  AppUi(lua_State *L0, int model);
  ~AppUi();

  virtual void action(String name);
  virtual void setupSymbolicNames(const Cascade *sheet);
  virtual void setAttributes(const AllAttributes &all, Cascade *sheet);
  virtual void setGridAngleSize(Attribute abs_grid, Attribute abs_angle);
  virtual void setLayers(const Page *page, int view);

  virtual void setZoom(double zoom);
  virtual void setActionsEnabled(bool mode);
  virtual void setNumbers(String vno, bool vm, String pno, bool pm);
  virtual void setNotes(String notes);
  virtual bool checkbox(lua_State *L);

  virtual WINID windowId();
  virtual int actionId(const char *name);
  virtual bool closeWindow();
  virtual bool actionState(const char *name);
  virtual void setActionState(const char *name, bool value);
  virtual void setWindowCaption(const char *s);
  virtual void explain(const char *s, int t);
  virtual void showWindow(int width, int height);

  // direct Lua methods
  virtual int setBookmarks(lua_State *L);
  virtual int showTool(lua_State *L);

private:
  virtual void addRootMenu(int id, const char *name);
  void addItem(GtkMenuShell *shell, const char *title, const char *name);
  virtual void addItem(int id, const char *title, const char *name);
  virtual void startSubMenu(int id, const char *name);
  virtual void addSubItem(const char *title, const char *name);
  virtual MENUHANDLE endSubMenu();
  virtual void setMouseIndicator(const char *s);

  static void menuitem_cb(GtkWidget *item, gpointer data);
private:
  struct SAction {
    String name;
    GtkWidget *menuItem;
  };
  std::vector<SAction> iActions;
  GtkWidget *iWindow;
  GtkWidget *iRootMenu[ENumMenu];
  GtkWidget *iSubMenu[ENumMenu];
  GtkWidget *iStatusBar;
  int iStatusBarContextid;
  GtkWidget *iMousePosition;
  GtkWidget *iResolution;
  GtkAccelGroup *iAccelGroup;
};

// --------------------------------------------------------------------
#endif
