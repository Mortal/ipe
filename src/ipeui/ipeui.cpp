// --------------------------------------------------------------------
// Lua bindings for Qt dialogs
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

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "ipeui.h"

#include <cstdio>
#include <cstdlib>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QFileDialog>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSyntaxHighlighter>
#include <QTextEdit>
#include <QThread>
#include <QTimer>

#if defined(WIN32)
#include <windows.h>
#endif

using namespace ipeui;

// --------------------------------------------------------------------

inline Dialog **check_dialog(lua_State *L, int i)
{
  return (Dialog **) luaL_checkudata(L, i, "Ipe.dialog");
}

inline QMenu **check_menu(lua_State *L, int i)
{
  return (QMenu **) luaL_checkudata(L, i, "Ipe.menu");
}

inline Timer **check_timer(lua_State *L, int i)
{
  return (Timer **) luaL_checkudata(L, i, "Ipe.timer");
}

// --------------------------------------------------------------------

static void push_string(lua_State *L, const QString &str)
{
  lua_pushstring(L, str.toUtf8());
}

inline QString tostring(lua_State *L, int i)
{
  return QString::fromUtf8(lua_tostring(L, i));
}

inline QString checkstring(lua_State *L, int i)
{
  return QString::fromUtf8(luaL_checkstring(L, i));
}

static QWidget *check_parent(lua_State *L, int i)
{
  if (lua_isnil(L, i))
    return 0;
  QWidget **p = (QWidget **) luaL_checkudata(L, i, "Ipe.appui");
  return *p;
}

// --------------------------------------------------------------------

class XmlHighlighter : public QSyntaxHighlighter
{
public:
  XmlHighlighter(QTextEdit *textEdit);
protected:
  void applyFormat(const QString &text, QRegExp &exp,
		   const QTextCharFormat &format);
  virtual void highlightBlock(const QString &text);
};

void XmlHighlighter::applyFormat(const QString &text, QRegExp &exp,
				 const QTextCharFormat &format)
{
  int index = text.indexOf(exp);
  while (index >= 0) {
    int length = exp.matchedLength();
    setFormat(index, length, format);
    index = text.indexOf(exp, index + length);
  }
}

void XmlHighlighter::highlightBlock(const QString &text)
{
  QTextCharFormat tagFormat, stringFormat, numberFormat;
  tagFormat.setFontWeight(QFont::Bold);
  tagFormat.setForeground(Qt::blue);
  stringFormat.setForeground(Qt::darkMagenta);
  numberFormat.setForeground(Qt::red);

  QRegExp tagExp( "<.*>" );
  QRegExp stringExp( "\"[a-zA-Z]*\"" );
  QRegExp numberExp( "[+|-]*[0-9]*.[0-9][0-9]*" );
  applyFormat(text, tagExp, tagFormat);
  applyFormat(text, stringExp, stringFormat);
  applyFormat(text, numberExp, numberFormat);
}

XmlHighlighter::XmlHighlighter(QTextEdit *textEdit)
  : QSyntaxHighlighter(textEdit)
{
  // nothing
}

// --------------------------------------------------------------------

class LatexHighlighter : public QSyntaxHighlighter
{
public:
  LatexHighlighter(QTextEdit *textEdit);
protected:
  void applyFormat(const QString &text, QRegExp &exp,
		   const QTextCharFormat &format);
  virtual void highlightBlock(const QString &text);
};

void LatexHighlighter::applyFormat(const QString &text, QRegExp &exp,
				   const QTextCharFormat &format)
{
  int index = text.indexOf(exp);
  while (index >= 0) {
    int length = exp.matchedLength();
    setFormat(index, length, format);
    index = text.indexOf(exp, index + length);
  }
}

void LatexHighlighter::highlightBlock(const QString &text)
{
  QTextCharFormat mathFormat, tagFormat;
  mathFormat.setForeground(Qt::red);
  tagFormat.setFontWeight(QFont::Bold);
  tagFormat.setForeground(Qt::blue);

  QRegExp mathExp( "\\$[^$]+\\$" );
  QRegExp tagExp( "\\\\[a-zA-Z]+" );
  applyFormat(text, mathExp, mathFormat);
  applyFormat(text, tagExp, tagFormat);
}

LatexHighlighter::LatexHighlighter(QTextEdit *textEdit)
  : QSyntaxHighlighter(textEdit)
{
  // nothing
}

// --------------------------------------------------------------------

Dialog::Dialog(lua_State *L0, QWidget *parent) :QDialog(parent)
{
  L = L0;
  iLuaDialog = LUA_NOREF;
  QGridLayout *lo = new QGridLayout;
  setLayout(lo);
}

Dialog::~Dialog()
{
  // dereference lua methods
  for (uint i = 0; i < iElements.size(); ++i)
    luaL_unref(L, LUA_REGISTRYINDEX, iElements[i].lua_method);
  luaL_unref(L, LUA_REGISTRYINDEX, iLuaDialog);
}

void Dialog::callLua()
{
  // only call back to Lua during execute()
  if (iLuaDialog == LUA_NOREF)
    return;

  QObject *s = sender();
  for (uint i = 0; i < iElements.size(); ++i) {
    if (s == iElements[i].widget) {
      lua_rawgeti(L, LUA_REGISTRYINDEX, iElements[i].lua_method);
      lua_rawgeti(L, LUA_REGISTRYINDEX, iLuaDialog);
      lua_call(L, 1, 0);
      return;
    }
  }
}

QGridLayout *Dialog::gridlayout()
{
  return qobject_cast<QGridLayout *>(layout());
}

int Dialog::add(lua_State *L)
{
  static const char * const typenames[] =
    { "button", "text", "list", "label", "combo", "checkbox", "input", 0 };

  QString name = checkstring(L, 2);
  int type = luaL_checkoption(L, 3, 0, typenames);
  luaL_checktype(L, 4, LUA_TTABLE);
  int row = luaL_checkint(L, 5) - 1;
  int col = luaL_checkint(L, 6) - 1;
  int rowstretch = 1;
  int colstretch = 1;
  if (!lua_isnoneornil(L, 7))
    rowstretch = luaL_checkint(L, 7);
  if (!lua_isnoneornil(L, 8))
    colstretch = luaL_checkint(L, 8);

  SElement m;
  m.name = name;
  m.widget = 0;
  m.lua_method = LUA_NOREF;
  m.flags = 0;

  switch (type) {
  case 0:
    addButton(L, m);
    break;
  case 1:
    addTextEdit(L, m);
    break;
  case 2:
    addList(L, m);
    break;
  case 3:
    addLabel(L, m);
    break;
  case 4:
    addCombo(L, m);
    break;
  case 5:
    addCheckbox(L, m);
    break;
  case 6:
    addInput(L, m);
    break;
  default:
    break;
  }

  QGridLayout *lo = gridlayout();
  lo->addWidget(m.widget, row, col, rowstretch, colstretch);
  iElements.push_back(m);
  return 0;
}

void Dialog::addButton(lua_State *L, SElement &m)
{
  lua_getfield(L, 4, "label");
  luaL_argcheck(L, lua_isstring(L, -1), 4, "no button label");
  QString label = tostring(L, -1);
  QPushButton *b = new QPushButton(label, this);
  m.widget = b;
  lua_getfield(L, 4, "action");
  if (lua_isstring(L, -1)) {
    QString s = tostring(L, -1);
    if (s == "accept")
      connect(b, SIGNAL(clicked()), SLOT(accept()));
    else if (s == "reject")
      connect(b, SIGNAL(clicked()), SLOT(reject()));
    else
      luaL_argerror(L, 4, "unknown action");
  } else if (lua_isfunction(L, -1)) {
    lua_pushvalue(L, -1);
    m.lua_method = luaL_ref(L, LUA_REGISTRYINDEX);
    connect(b, SIGNAL(clicked()), SLOT(callLua()));
  } else if (!lua_isnil(L, -1))
    luaL_argerror(L, 4, "unknown action type");
  lua_pop(L, 2); // action, label
}

void Dialog::addTextEdit(lua_State *L, SElement &m)
{
  QTextEdit *t = new QTextEdit(this);
  t->setAcceptRichText(false);
  m.widget = t;
  lua_getfield(L, 4, "read_only");
  if (lua_toboolean(L, -1))
    t->setReadOnly(true);
  lua_getfield(L, 4, "syntax");
  if (!lua_isnil(L, -1)) {
    QString s = tostring(L, -1);
    if (s == "logfile") {
      m.flags |= ELogFile;
    } else if (s == "xml") {
      m.flags |= EXml;
      (void) new XmlHighlighter(t);
    } else if (s == "latex") {
      (void) new LatexHighlighter(t);
    } else
      luaL_argerror(L, 4, "unknown syntax");
  }
  lua_pop(L, 2); // syntax, read_only
}

static void setListItems(lua_State *L, int index, QListWidget *l = 0,
			 QComboBox *b = 0)
{
  int no = lua_objlen(L, index);
  for (int i = 1; i <= no; ++i) {
    lua_rawgeti(L, index, i);
    luaL_argcheck(L, lua_isstring(L, -1), index, "items must be strings");
    QString item = tostring(L, -1);
    if (l) l->addItem(item);
    if (b) b->addItem(item);
    lua_pop(L, 1); // item
  }
}

void Dialog::addList(lua_State *L, SElement &m)
{
  QListWidget *l = new QListWidget(this);
  m.widget = l;
  setListItems(L, 4, l, 0);
}

void Dialog::addLabel(lua_State *L, SElement &m)
{
  lua_getfield(L, 4, "label");
  luaL_argcheck(L, lua_isstring(L, -1), 4, "no label");
  QString label = tostring(L, -1);
  QLabel *l = new QLabel(label, this);
  m.widget = l;
  lua_pop(L, 1); // label
}

void Dialog::addCombo(lua_State *L, SElement &m)
{
  QComboBox *b = new QComboBox(this);
  m.widget = b;
  setListItems(L, 4, 0, b);
}

void Dialog::addCheckbox(lua_State *L, SElement &m)
{
  lua_getfield(L, 4, "label");
  luaL_argcheck(L, lua_isstring(L, -1), 4, "no label");
  QString label = tostring(L, -1);
  QCheckBox *b = new QCheckBox(label, this);
  m.widget = b;
  lua_getfield(L, 4, "action");
  if (!lua_isnil(L, -1)) {
    luaL_argcheck(L, lua_isfunction(L, -1), 4, "unknown action type");
    lua_pushvalue(L, -1);
    m.lua_method = luaL_ref(L, LUA_REGISTRYINDEX);
    connect(b, SIGNAL(stateChanged(int)), SLOT(callLua()));
  }
  lua_pop(L, 2); // action, label
}

void Dialog::addInput(lua_State *L, SElement &m)
{
  QLineEdit *e = new QLineEdit(this);
  m.widget = e;
}

int Dialog::findElement(lua_State *L, int index)
{
  QString name = checkstring(L, index);
  for (uint i = 0; i < iElements.size(); ++i) {
    if (iElements[i].name == name)
      return i;
  }
  return luaL_argerror(L, index, "no such element in dialog");
}

static void markupLog(QTextEdit *t, const QString &text)
{
  QTextDocument *doc = new QTextDocument(t);
  doc->setPlainText(text);
  QTextCursor cursor(doc);
  int curPos = 0;
  int errNo = 0;
  for (;;) {
    int nextErr = text.indexOf(QLatin1String("\n!"), curPos);
    if (nextErr < 0)
      break;

    int lines = 0;
    while (curPos < nextErr + 1) {
      if (text[curPos++] == QLatin1Char('\n'))
	++lines;
    }
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, lines);
    int pos = cursor.position();
    cursor.movePosition(QTextCursor::Down);
    cursor.setPosition(pos, QTextCursor::KeepAnchor);
    ++errNo;
    QString s;
    s.sprintf("err%d", errNo);
    QTextCharFormat format;
    format.setBackground(Qt::yellow);
    format.setAnchorName(s);
    format.setAnchor(true);
    cursor.setCharFormat(format);
  }
  t->setDocument(doc);
  t->scrollToAnchor(QLatin1String("err1"));
}

#if 0
static void xmlIndent(QString &text)
{
  QStringList l = text.split('\n');

  QRegExp begExp( "<(?!/).*>" );
  QRegExp endExp( "</.*>" );
  int tabLevel = 0;

  for (QStringList::Iterator it = l.begin(); it != l.end(); ++it) {
    if (it->contains(endExp))
      tabLevel--;
    for (int i = 0; i < tabLevel; i++)
      it->insert(0, "  ");
    if (it->contains(begExp))
      tabLevel++;
  }
  text = l.join("\n");
}
#endif

int Dialog::set(lua_State *L)
{
  int idx = findElement(L, 2);

  QLabel *l = qobject_cast<QLabel *>(iElements[idx].widget);
  if (l) {
    l->setText(checkstring(L, 3));
    return 0;
  }
  QTextEdit *t = qobject_cast<QTextEdit *>(iElements[idx].widget);
  if (t) {
    QString text = checkstring(L, 3);
    if (iElements[idx].flags & ELogFile) {
      markupLog(t, text);
    } else
      t->setPlainText(text);
    return 0;
  }
  QListWidget *list = qobject_cast<QListWidget *>(iElements[idx].widget);
  if (list) {
    if (lua_isnumber(L, 3)) {
      int m = luaL_checkint(L, 3);
      luaL_argcheck(L, 1 <= m && m <= list->count(), 3,
		    "list index out of bounds");
      list->setCurrentRow(m - 1);
    } else {
      luaL_checktype(L, 3, LUA_TTABLE);
      list->clear();
      setListItems(L, 3, list, 0);
    }
    return 0;
  }

  QComboBox *b = qobject_cast<QComboBox *>(iElements[idx].widget);
  if (b) {
    if (lua_isnumber(L, 3)) {
      int m = luaL_checkint(L, 3);
      luaL_argcheck(L, 1 <= m && m <= b->count(), 3,
		    "list index out of bounds");
      b->setCurrentIndex(m - 1);
    } else {
      luaL_checktype(L, 3, LUA_TTABLE);
      b->clear();
      setListItems(L, 3, 0, b);
    }
    return 0;
  }

  QCheckBox *ch = qobject_cast<QCheckBox *>(iElements[idx].widget);
  if (ch) {
    bool value = lua_toboolean(L, 3);
    ch->setChecked(value);
    return 0;
  }

  QLineEdit *le = qobject_cast<QLineEdit *>(iElements[idx].widget);
  if (le) {
    QString text = checkstring(L, 3);
    le->setText(text);
    return 0;
  }

  return luaL_argerror(L, 2, "no suitable element");
}

int Dialog::get(lua_State *L)
{
  int idx = findElement(L, 2);

  QTextEdit *t = qobject_cast<QTextEdit *>(iElements[idx].widget);
  if (t) {
    QString s = t->toPlainText();
    push_string(L, s);
    return 1;
  }

  QListWidget *list = qobject_cast<QListWidget *>(iElements[idx].widget);
  if (list) {
    lua_pushnumber(L, list->currentRow() + 1);
    return 1;
  }

  QComboBox *b = qobject_cast<QComboBox *>(iElements[idx].widget);
  if (b) {
    lua_pushnumber(L, b->currentIndex() + 1);
    return 1;
  }

  QCheckBox *ch = qobject_cast<QCheckBox *>(iElements[idx].widget);
  if (ch) {
    lua_pushboolean(L, ch->isChecked());
    return 1;
  }

  QLineEdit *le = qobject_cast<QLineEdit *>(iElements[idx].widget);
  if (le) {
    push_string(L, le->text());
    return 1;
  }

  return luaL_argerror(L, 2, "no suitable element");
}

int Dialog::execute(lua_State *L)
{
  // remember Lua object for dialog for callbacks
  lua_pushvalue(L, 1);
  iLuaDialog = luaL_ref(L, LUA_REGISTRYINDEX);
  int result = exec();
  // discard reference to dialog object
  // (otherwise the circular reference will stop Lua gc)
  luaL_unref(L, LUA_REGISTRYINDEX, iLuaDialog);
  iLuaDialog = LUA_NOREF;
  return result;
}

int Dialog::setEnabled(lua_State *L)
{
  int idx = findElement(L, 2);
  bool value = lua_toboolean(L, 3);
  iElements[idx].widget->setEnabled(value);
  return 0;
}

// --------------------------------------------------------------------

static int dialog_constructor(lua_State *L)
{
  QWidget *parent = check_parent(L, 1);
  QString s = checkstring(L, 2);

  Dialog **dlg = (Dialog **) lua_newuserdata(L, sizeof(Dialog *));
  *dlg = 0;
  luaL_getmetatable(L, "Ipe.dialog");
  lua_setmetatable(L, -2);
  *dlg = new Dialog(L, parent);
  (*dlg)->setWindowTitle(s);
  return 1;
}

static int dialog_tostring(lua_State *L)
{
  check_dialog(L, 1);
  lua_pushfstring(L, "Dialog@%p", lua_topointer(L, 1));
  return 1;
}

static int dialog_destructor(lua_State *L)
{
  // fprintf(stderr, "Dialog destructor\n");
  Dialog **dlg = check_dialog(L, 1);
  delete (*dlg);
  *dlg = 0;
  return 0;
}

static int dialog_execute(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  lua_pushboolean(L, (*dlg)->execute(L));
  return 1;
}

static int dialog_setStretch(lua_State *L)
{
  static const char * const typenames[] = { "row", "column", 0 };
  Dialog **dlg = check_dialog(L, 1);
  int type = luaL_checkoption(L, 2, 0, typenames);
  int rowcol = luaL_checkint(L, 3) - 1;
  int stretch = luaL_checkint(L, 4);

  QGridLayout *lo = (*dlg)->gridlayout();

  if (type == 0)
    lo->setRowStretch(rowcol, stretch);
  else
    lo->setColumnStretch(rowcol, stretch);
  return 0;
}

static int dialog_add(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->add(L);
}

static int dialog_set(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->set(L);
}

static int dialog_get(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->get(L);
}

static int dialog_setEnabled(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->setEnabled(L);
}

// --------------------------------------------------------------------

static const struct luaL_Reg dialog_methods[] = {
  { "__tostring", dialog_tostring },
  { "__gc", dialog_destructor },
  { "execute", dialog_execute },
  { "setStretch", dialog_setStretch },
  { "add", dialog_add },
  { "set", dialog_set },
  { "get", dialog_get },
  { "setEnabled", dialog_setEnabled },
  { 0, 0 }
};

// --------------------------------------------------------------------

MenuAction::MenuAction(const QString &name, int number,
		       const QString &item, const QString &text,
		       QWidget *parent)
  : QAction(text, parent)
{
  iName = name;
  iItemName = item;
  iNumber = number;
}

static int menu_constructor(lua_State *L)
{
  QMenu **m = (QMenu **) lua_newuserdata(L, sizeof(QMenu *));
  *m = 0;
  luaL_getmetatable(L, "Ipe.menu");
  lua_setmetatable(L, -2);

  *m = new QMenu();
  return 1;
}

static int menu_tostring(lua_State *L)
{
  check_menu(L, 1);
  lua_pushfstring(L, "Menu@%p", lua_topointer(L, 1));
  return 1;
}

static int menu_destructor(lua_State *L)
{
  QMenu **m = check_menu(L, 1);
  delete *m;
  *m = 0;
  return 0;
}

static int menu_execute(lua_State *L)
{
  QMenu **m = check_menu(L, 1);
  int vx = luaL_checkint(L, 2);
  int vy = luaL_checkint(L, 3);
  QAction *a = (*m)->exec(QPoint(vx, vy));
  MenuAction *ma = qobject_cast<MenuAction *>(a);
  if (ma) {
    push_string(L, ma->name());
    lua_pushnumber(L, ma->number());
    push_string(L, ma->itemName());
    return 3;
  }
  return 0;
}

#define SIZE 16

static QIcon colorIcon(double red, double green, double blue)
{
  QPixmap pixmap(SIZE, SIZE);
  pixmap.fill(QColor(int(red * 255.0 + 0.5),
		     int(green * 255.0 + 0.5),
		     int(blue * 255.0 + 0.5)));
  return QIcon(pixmap);
}

static int menu_add(lua_State *L)
{
  QMenu **m = check_menu(L, 1);
  QString name = checkstring(L, 2);
  QString title = checkstring(L, 3);
  if (lua_gettop(L) == 3) {
    (*m)->addAction(new MenuAction(name, 0, QString(), title, *m));
  } else {
    luaL_argcheck(L, lua_istable(L, 4), 4, "argument is not a table");
    bool hasmap = !lua_isnoneornil(L, 5) && lua_isfunction(L, 5);
    bool hastable = !hasmap && !lua_isnoneornil(L, 5);
    bool hascolor = !lua_isnoneornil(L, 6);
    if (hastable)
      luaL_argcheck(L, lua_istable(L, 5), 5,
		    "argument is not a function or table");
    if (hascolor)
      luaL_argcheck(L, lua_isfunction(L, 6), 6, "argument is not a function");
    int no = lua_objlen(L, 4);
    QMenu *sm = new QMenu(title, *m);
    for (int i = 1; i <= no; ++i) {
      lua_rawgeti(L, 4, i);
      luaL_argcheck(L, lua_isstring(L, -1), 4, "items must be strings");
      QString item = tostring(L, -1);
      QString text = item;
      if (hastable) {
	lua_rawgeti(L, 5, i);
	luaL_argcheck(L, lua_isstring(L, -1), 5, "labels must be strings");
	text = tostring(L, -1);
	lua_pop(L, 1);
      }
      if (hasmap) {
	lua_pushvalue(L, 5);   // function
	lua_pushnumber(L, i);  // index
 	lua_pushvalue(L, -3);  // name
	lua_call(L, 2, 1);     // function returns label
	luaL_argcheck(L, lua_isstring(L, -1), 5,
		      "function does not return string");
	text = tostring(L, -1);
	lua_pop(L, 1);         // pop result
      }
      MenuAction *ma = new MenuAction(name, i, item, text, sm);
      if (hascolor) {
	lua_pushvalue(L, 6);   // function
	lua_pushnumber(L, i);  // index
 	lua_pushvalue(L, -3);  // name
	lua_call(L, 2, 3);     // function returns red, green, blue
	double red = luaL_checknumber(L, -3);
	double green = luaL_checknumber(L, -2);
	double blue = luaL_checknumber(L, -1);
	lua_pop(L, 3);         // pop result
	ma->setIcon(colorIcon(red, green, blue));
      }
      lua_pop(L, 1); // item
      sm->addAction(ma);
    }
    (*m)->addMenu(sm);
  }
  return 0;
}

// --------------------------------------------------------------------

static const struct luaL_Reg menu_methods[] = {
  { "__tostring", menu_tostring },
  { "__gc", menu_destructor },
  { "execute", menu_execute },
  { "add", menu_add },
  { 0, 0 }
};

// --------------------------------------------------------------------

static int ipeui_getString(lua_State *L)
{
  QWidget *parent = check_parent(L, 1);
  QString s = checkstring(L, 2);
  bool ok;
  QString str = QInputDialog::getText(parent, QString("Ipe"), s,
				      QLineEdit::Normal,
				      QString(),
				      &ok);
  if (ok && !str.isEmpty()) {
    push_string(L, str);
    return 1;
  } else
    return 0;
}

static int ipeui_getDouble(lua_State *L)
{
  QWidget *parent = check_parent(L, 1);
  QString title = checkstring(L, 2);
  QString detail = checkstring(L, 3);
  double value = luaL_checknumber(L, 4);
  double minv = luaL_checknumber(L, 5);
  double maxv = luaL_checknumber(L, 6);
  int decimals = luaL_checkint(L, 7);
  bool ok;
  double d = QInputDialog::getDouble(parent, title, detail,
				     value, minv, maxv, decimals, &ok);
  if (ok) {
    lua_pushnumber(L, d);
    return 1;
  } else
    return 0;
}

static int ipeui_getInt(lua_State *L)
{
  QWidget *parent = check_parent(L, 1);
  QString title = checkstring(L, 2);
  QString detail = checkstring(L, 3);
  int value = luaL_checkint(L, 4);
  int minv = luaL_checkint(L, 5);
  int maxv = luaL_checkint(L, 6);
  int step = luaL_checkint(L, 7);
  bool ok;
#if QT_VERSION >= 0x040500
  int d = QInputDialog::getInt(parent, title, detail,
			       value, minv, maxv, step, &ok);
#else
  (void) step;
  int d = int(QInputDialog::getDouble(parent, title, detail,
				      value, minv, maxv, 0, &ok));
#endif
  if (ok) {
    lua_pushnumber(L, d);
    return 1;
  } else
    return 0;
}

static int ipeui_getColor(lua_State *L)
{
  QWidget *parent = check_parent(L, 1);
  QString title = checkstring(L, 2);
  QColor initial = QColor::fromRgbF(luaL_checknumber(L, 3),
				    luaL_checknumber(L, 4),
				    luaL_checknumber(L, 5));
#if QT_VERSION >= 0x040500
  QColor changed = QColorDialog::getColor(initial, parent, title);
#else
  QColor changed = QColorDialog::getColor(initial, parent);
#endif
  if (changed.isValid()) {
    lua_pushnumber(L, changed.redF());
    lua_pushnumber(L, changed.greenF());
    lua_pushnumber(L, changed.blueF());
    return 3;
  } else
    return 0;
}

// --------------------------------------------------------------------

static int ipeui_fileDialog(lua_State *L)
{
  static const char * const typenames[] = { "open", "save", 0 };

  QWidget *parent = check_parent(L, 1);
  int type = luaL_checkoption(L, 2, 0, typenames);
  QString caption = checkstring(L, 3);
  QString filter = checkstring(L, 4);

  QString dir;
  if (!lua_isnoneornil(L, 5))
    dir = checkstring(L, 5);
  QString selected;
  if (!lua_isnoneornil(L, 6))
    selected = checkstring(L, 6);

  QFileDialog dialog(parent);
  dialog.setWindowTitle(caption);
  dialog.setNameFilter(filter);
  dialog.setConfirmOverwrite(false);

  if (type == 0) {
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
  } else {
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
  }

  if (!selected.isNull())
    dialog.selectFilter(selected);
  if (!dir.isNull())
    dialog.setDirectory(dir);

  if (dialog.exec() == QDialog::Accepted) {
    QStringList fns = dialog.selectedFiles();
    if (!fns.isEmpty()) {
      push_string(L, fns[0]);
      push_string(L, dialog.selectedNameFilter());
      return 2;
    }
  }
  return 0;
}

// --------------------------------------------------------------------

static int ipeui_messageBox(lua_State *L)
{
  static const char * const options[] =  {
    "none", "warning", "information", "question", "critical", 0 };
  static const char * const buttontype[] = {
    "ok", "okcancel", "yesnocancel", "discardcancel",
    "savediscardcancel", 0 };

  QWidget *parent = check_parent(L, 1);
  int type = luaL_checkoption(L, 2, "none", options);
  QString text = checkstring(L, 3);
  const char *details = 0;
  if (!lua_isnoneornil(L, 4))
    details = luaL_checkstring(L, 4);
  int buttons = 0;
  if (lua_isnumber(L, 5))
    buttons = luaL_checkint(L, 5);
  else if (!lua_isnoneornil(L, 5))
    buttons = luaL_checkoption(L, 5, 0, buttontype);

  QMessageBox msgBox(parent);
  msgBox.setText(text);
  if (details)
    msgBox.setInformativeText(QString::fromUtf8(details));

  switch (type) {
  case 0:
  default:
    msgBox.setIcon(QMessageBox::NoIcon);
    break;
  case 1:
    msgBox.setIcon(QMessageBox::Warning);
    break;
  case 2:
    msgBox.setIcon(QMessageBox::Information);
    break;
  case 3:
    msgBox.setIcon(QMessageBox::Question);
    break;
  case 4:
    msgBox.setIcon(QMessageBox::Critical);
    break;
  }

  switch (buttons) {
  case 0:
  default:
    msgBox.setStandardButtons(QMessageBox::Ok);
    break;
  case 1:
    msgBox.setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);
    break;
  case 2:
    msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No|
			      QMessageBox::Cancel);
    break;
  case 3:
    msgBox.setStandardButtons(QMessageBox::Discard|QMessageBox::Cancel);
    break;
  case 4:
    msgBox.setStandardButtons(QMessageBox::Save|QMessageBox::Discard|
			      QMessageBox::Cancel);
    break;
  }

  int ret = msgBox.exec();

  switch (ret) {
  case QMessageBox::Ok:
  case QMessageBox::Yes:
  case QMessageBox::Save:
    lua_pushnumber(L, 1);
    break;
  case QMessageBox::No:
  case QMessageBox::Discard:
    lua_pushnumber(L, 0);
    break;
  case QMessageBox::Cancel:
  default:
    lua_pushnumber(L, -1);
    break;
  }
  return 1;
}

// --------------------------------------------------------------------

class EditorThread : public QThread
{
public:
  EditorThread(const QString &cmd);

protected:
  virtual void run();
private:
  QString iCommand;
};

EditorThread::EditorThread(const QString &cmd) : QThread()
{
  iCommand = cmd;
}

void EditorThread::run()
{
  int result = std::system(iCommand.toUtf8());
  (void) result;
}

class EditorDialog : public QDialog
{
public:
  EditorDialog(QWidget *parent = 0);
protected:
  void closeEvent(QCloseEvent *ev);
};

EditorDialog::EditorDialog(QWidget *parent)
  : QDialog(parent)
{
  QGridLayout *lo = new QGridLayout;
  setLayout(lo);
  setWindowTitle("Ipe: waiting");
  QLabel *l = new QLabel("Waiting for external editor", this);
  lo->addWidget(l, 0, 0);
}

void EditorDialog::closeEvent(QCloseEvent *ev)
{
  ev->ignore();
}

static int ipeui_wait(lua_State *L)
{
  QString cmd = checkstring(L, 1);

  EditorDialog *dialog = new EditorDialog();

  EditorThread *thread = new EditorThread(cmd);
  dialog->connect(thread, SIGNAL(finished()), SLOT(accept()));

  thread->start();
  dialog->exec();
  delete dialog;
  delete thread;
  return 0;
}

// --------------------------------------------------------------------

Timer::Timer(lua_State *L0, int lua_object,
	     const QString &method)
{
  L = L0;
  iLuaObject = lua_object;
  iMethod = method;
  iTimer = new QTimer();
  connect(iTimer, SIGNAL(timeout()), SLOT(callLua()));
}

Timer::~Timer()
{
  luaL_unref(L, LUA_REGISTRYINDEX, iLuaObject);
  delete iTimer;
}

void Timer::callLua()
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iLuaObject);
  lua_rawgeti(L, -1, 1); // get Lua object
  if (lua_isnil(L, -1)) {
    lua_pop(L, 2); // pop weak table, nil
    return;
  }
  lua_getfield(L, -1, iMethod.toUtf8());
  if (lua_isnil(L, -1)) {
    lua_pop(L, 3); // pop weak table, table, nil
    return;
  }
  lua_remove(L, -3); // remove weak table
  lua_insert(L, -2); // stack is now: method, table
  lua_call(L, 1, 0); // call method
}

int Timer::setSingleShot(lua_State *L)
{
  bool t = lua_toboolean(L, 2);
  iTimer->setSingleShot(t);
  return 0;
}

int Timer::setInterval(lua_State *L)
{
  int t = luaL_checkint(L, 2);
  iTimer->setInterval(t);
  return 0;
}

int Timer::active(lua_State *L)
{
  lua_pushboolean(L, iTimer->isActive());
  return 1;
}

int Timer::start(lua_State *L)
{
  iTimer->start();
  return 0;
}

int Timer::stop(lua_State *L)
{
  iTimer->stop();
  return 0;
}

// --------------------------------------------------------------------

static int timer_constructor(lua_State *L)
{
  luaL_argcheck(L, lua_istable(L, 1), 1, "argument is not a table");
  QString method = luaL_checkstring(L, 2);

  Timer **t = (Timer **) lua_newuserdata(L, sizeof(Timer *));
  *t = 0;
  luaL_getmetatable(L, "Ipe.timer");
  lua_setmetatable(L, -2);

  // create a table with weak reference to Lua object
  lua_createtable(L, 1, 1);
  lua_pushliteral(L, "v");
  lua_setfield(L, -2, "__mode");
  lua_pushvalue(L, -1);
  lua_setmetatable(L, -2);
  lua_pushvalue(L, 1);
  lua_rawseti(L, -2, 1);
  int lua_object = luaL_ref(L, LUA_REGISTRYINDEX);
  *t = new Timer(L, lua_object, method);
  return 1;
}

static int timer_tostring(lua_State *L)
{
  check_timer(L, 1);
  lua_pushfstring(L, "Timer@%p", lua_topointer(L, 1));
  return 1;
}

static int timer_destructor(lua_State *L)
{
  Timer **t = check_timer(L, 1);
  // fprintf(stderr, "Timer destructor\n");
  delete *t;
  *t = 0;
  return 0;
}

// --------------------------------------------------------------------

static int timer_start(lua_State *L)
{
  Timer **t = check_timer(L, 1);
  return (*t)->start(L);
}

static int timer_stop(lua_State *L)
{
  Timer **t = check_timer(L, 1);
  return (*t)->stop(L);
}

static int timer_active(lua_State *L)
{
  Timer **t = check_timer(L, 1);
  return (*t)->active(L);
}

static int timer_setinterval(lua_State *L)
{
  Timer **t = check_timer(L, 1);
  return (*t)->setInterval(L);
}

static int timer_setsingleshot(lua_State *L)
{
  Timer **t = check_timer(L, 1);
  return (*t)->setSingleShot(L);
}

// --------------------------------------------------------------------

static const struct luaL_Reg timer_methods[] = {
  { "__tostring", timer_tostring },
  { "__gc", timer_destructor },
  { "start", timer_start },
  { "stop", timer_stop },
  { "active", timer_active },
  { "setInterval", timer_setinterval },
  { "setSingleShot", timer_setsingleshot },
  { 0, 0 }
};

// --------------------------------------------------------------------

static int ipeui_mainloop(lua_State *L)
{
  QApplication::exec();
  return 0;
}

static int ipeui_closeAllWindows(lua_State *L)
{
  QApplication::closeAllWindows();
  return 0;
}

static int ipeui_setClipboard(lua_State *L)
{
  QString data = checkstring(L, 1);
  QClipboard *cb = QApplication::clipboard();
  cb->setText(data);
  return 0;
}

static int ipeui_clipboard(lua_State *L)
{
  QClipboard *cb = QApplication::clipboard();
  QString data = cb->text();
  push_string(L, data);
  return 1;
}

static int ipeui_currentDateTime(lua_State *L)
{
  QDateTime dt = QDateTime::currentDateTime();
  QString mod;
  mod.sprintf("%04d%02d%02d%02d%02d%02d",
	      dt.date().year(), dt.date().month(), dt.date().day(),
	      dt.time().hour(), dt.time().minute(), dt.time().second());
  push_string(L, mod);
  return 1;
}

#if defined(WIN32)
static int ipeui_startBrowser(lua_State *L)
{
  const char *url = luaL_checkstring(L, 1);
  ShellExecuteA(0, "open", url, 0, 0, 0);
  return 0;
}
#endif

// --------------------------------------------------------------------

static const struct luaL_Reg ipeui_functions[] = {
  { "Dialog", dialog_constructor },
  { "Menu", menu_constructor },
  { "Timer", timer_constructor },
  { "getString", ipeui_getString },
  { "getDouble", ipeui_getDouble },
  { "getInt", ipeui_getInt },
  { "getColor", ipeui_getColor },
  { "fileDialog", ipeui_fileDialog },
  { "messageBox", ipeui_messageBox },
  { "WaitDialog", ipeui_wait },
  { "mainLoop", ipeui_mainloop },
  { "closeAllWindows", ipeui_closeAllWindows },
  { "currentDateTime", ipeui_currentDateTime },
  { "setClipboard", ipeui_setClipboard },
  { "clipboard", ipeui_clipboard },
#ifdef WIN32
  { "startBrowser", ipeui_startBrowser },
#endif
  { 0, 0},
};

// --------------------------------------------------------------------

static void make_metatable(lua_State *L, const char *name,
			   const struct luaL_Reg *methods)
{
  luaL_newmetatable(L, name);
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);  /* pushes the metatable */
  lua_settable(L, -3);   /* metatable.__index = metatable */
  luaL_register(L, 0, methods);
  lua_pop(L, 1);
}

int luaopen_ipeui(lua_State *L)
{
  luaL_register(L, "ipeui", ipeui_functions);
  make_metatable(L, "Ipe.dialog", dialog_methods);
  make_metatable(L, "Ipe.menu", menu_methods);
  make_metatable(L, "Ipe.timer", timer_methods);
  return 0;
}

// --------------------------------------------------------------------
