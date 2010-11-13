// -*- C++ -*-
// --------------------------------------------------------------------
// A UI toolkit for Lua
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

#ifndef IPEDIALOGS_H
#define IPEDIALOGS_H

#include <QDialog>
#include <QGridLayout>
#include <QAction>

class QTimer;

// avoid including Lua headers here
typedef struct lua_State lua_State;

namespace ipeui {

  // --------------------------------------------------------------------

  class MenuAction : public QAction {
    Q_OBJECT
  public:
    MenuAction(const QString &name, int number,
	       const QString &item, const QString &text,
	       QWidget *parent);
    QString name() const { return iName; }
    QString itemName() const { return iItemName; }
    int number() const { return iNumber; }

  private:
    QString iName;
    QString iItemName;
    int iNumber;
  };

  class Timer : public QObject {
    Q_OBJECT
  public:
    Timer(lua_State *L0, int lua_object, const QString &method);
    ~Timer();

    int setSingleShot(lua_State *L);
    int setInterval(lua_State *L);
    int active(lua_State *L);
    int start(lua_State *L);
    int stop(lua_State *L);

  private slots:
    void callLua();

  private:
    lua_State *L;
    QTimer *iTimer;
    int iLuaObject;
    QString iMethod;
  };

  // --------------------------------------------------------------------

  class Dialog : public QDialog {
    Q_OBJECT

    public:
    Dialog(lua_State *L0, QWidget *parent);
    ~Dialog();

    QGridLayout *gridlayout();

    // Lua methods
    int add(lua_State *L);
    int set(lua_State *L);
    int get(lua_State *L);
    int setEnabled(lua_State *L);
    int execute(lua_State *L);

  private slots:
    void callLua();

  private:
    enum { ELogFile = 0x001, EXml = 0x002 };

    struct SElement {
      QString name;
      QWidget *widget;
      int lua_method;
      uint flags;
    };

    int findElement(lua_State *L, int index);

    void addButton(lua_State *L, SElement &m);
    void addTextEdit(lua_State *L, SElement &m);
    void addList(lua_State *L, SElement &m);
    void addLabel(lua_State *L, SElement &m);
    void addCombo(lua_State *L, SElement &m);
    void addCheckbox(lua_State *L, SElement &m);
    void addInput(lua_State *L, SElement &m);

  private:
    lua_State *L;
    std::vector<SElement> iElements;
    int iLuaDialog;
  };

} // namespace

// --------------------------------------------------------------------
#endif
