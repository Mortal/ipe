// --------------------------------------------------------------------
// Classes common to several platforms
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

#ifndef IPEUI_COMMON_H
#define IPEUI_COMMON_H

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#ifdef IPEUI_GTK
#include <gtk/gtk.h>
typedef GtkWidget *WINID;
#endif
#ifdef IPEUI_WIN32
#include <windows.h>
typedef HWND WINID;
#endif
#ifdef IPEUI_QT
#include <QWidget>
typedef QWidget *WINID;
#endif

// --------------------------------------------------------------------

extern WINID check_winid(lua_State *L, int i);
extern void push_winid(lua_State *L, WINID win);

// --------------------------------------------------------------------

class String {
public:
  String();
  explicit String(const char *str);
  String(const String &rhs);
  String &operator=(const String &rhs);
  ~String();
  int len() const { return iImp ? strlen(iImp->iData) : 0; }
  const char *z() const { return iImp ? iImp->iData : ""; }
private:
  struct Imp {
    int iRefCount;
    char *iData;
  };
  Imp *iImp;
};

// --------------------------------------------------------------------

class Dialog {
public:
  Dialog(lua_State *L0, WINID parent, const char *caption);
  virtual ~Dialog();

  // Lua methods
  int add(lua_State *L);
  int addButton(lua_State *L);
  int set(lua_State *L);
  int get(lua_State *L);
  int setEnabled(lua_State *L);
  int setStretch(lua_State *L);

  bool execute(lua_State *L, int w, int h);
  virtual void accept(lua_State *L) = 0;

protected:
  enum TFlags { ELogFile = 0x001, EXml = 0x002, ELatex = 0x040,
		EAccept = 0x004, EReject = 0x008,
		EReadOnly = 0x010, EDisabled = 0x020,
		ESelectAll = 0x080,
  };
  enum TType { EButton = 0, ETextEdit, EList, ELabel, ECombo,
	       ECheckBox, EInput };

  struct SElement {
    String name;
    TType type;
    int row, col, rowspan, colspan;
    int minWidth, minHeight;
    int lua_method;
    int flags;
    std::vector<String> items;
    String text;
    int value;
  };

  void callLua(int luaMethod);

  int findElement(lua_State *L, int index);

  void setUnmapped(lua_State *L, int idx);

  virtual void setMapped(lua_State *L, int idx) = 0;
  virtual bool buildAndRun(int w, int h) = 0;
  virtual void retrieveValues() = 0;
  virtual void enableItem(int idx, bool value) = 0;

  void addButtonItem(lua_State *L, SElement &m);
  void addTextEdit(lua_State *L, SElement &m);
  void addList(lua_State *L, SElement &m);
  void addLabel(lua_State *L, SElement &m);
  void addCombo(lua_State *L, SElement &m);
  void addCheckbox(lua_State *L, SElement &m);
  void addInput(lua_State *L, SElement &m);

  void setListItems(lua_State *L, int index, SElement &m);

 protected:
  lua_State *L;
  WINID iParent;
  WINID hDialog;
  String iCaption;
  std::vector<SElement> iElements;
  int iLuaDialog;
  bool iIgnoreEscape;
  int iBaseX, iBaseY;
  int iNoRows, iNoCols;
  std::vector<int> iRowStretch;
  std::vector<int> iColStretch;
};

// --------------------------------------------------------------------

inline Dialog **check_dialog(lua_State *L, int i)
{
  return (Dialog **) luaL_checkudata(L, i, "Ipe.dialog");
}

// --------------------------------------------------------------------

class Menu {
public:
  virtual ~Menu();
  virtual int add(lua_State *L) = 0;
  virtual int execute(lua_State *L) = 0;
};

// --------------------------------------------------------------------

inline Menu **check_menu(lua_State *L, int i)
{
  return (Menu **) luaL_checkudata(L, i, "Ipe.menu");
}

// --------------------------------------------------------------------

class Timer {
public:
  Timer(lua_State *L0, int lua_object, const char *method);
  virtual ~Timer();

  int setSingleShot(lua_State *L);
  virtual int setInterval(lua_State *L) = 0;
  virtual int active(lua_State *L) = 0;
  virtual int start(lua_State *L) = 0;
  virtual int stop(lua_State *L) = 0;

protected:
  void callLua();

protected:
  lua_State *L;
  int iLuaObject;
  String iMethod;
  bool iSingleShot;
};

// --------------------------------------------------------------------

inline Timer **check_timer(lua_State *L, int i)
{
  return (Timer **) luaL_checkudata(L, i, "Ipe.timer");
}

// --------------------------------------------------------------------

extern int luaopen_ipeui_common(lua_State *L);

// --------------------------------------------------------------------
#endif
