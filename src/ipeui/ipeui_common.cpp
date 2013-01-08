// --------------------------------------------------------------------
// Lua bindings for UI, platform-neutral common part
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

#include "ipeui_common.h"

// --------------------------------------------------------------------

WINID check_winid(lua_State *L, int i)
{
  if (lua_isnil(L, i))
    return 0;
  WINID *w = (WINID *) luaL_checkudata(L, i, "Ipe.winid");
  return *w;
}

void push_winid(lua_State *L, WINID win)
{
  WINID *w = (WINID *) lua_newuserdata(L, sizeof(WINID));
  *w = win;
  luaL_getmetatable(L, "Ipe.winid");
  lua_setmetatable(L, -2);
}

static int winid_tostring(lua_State *L)
{
  check_winid(L, 1);
  lua_pushfstring(L, "GtkWidget@%p", lua_topointer(L, 1));
  return 1;
}

static const struct luaL_Reg winid_methods[] = {
  { "__tostring", winid_tostring },
  { 0, 0 }
};

// --------------------------------------------------------------------

String::String()
{
  iImp = 0;
}

String::String(const String &rhs)
{
  iImp = rhs.iImp;
  if (iImp)
    iImp->iRefCount += 1;
}

String::String(const char *str)
{
  if (str) {
    iImp = new Imp;
    iImp->iRefCount = 1;
    iImp->iData = strdup(str);
  } else
    iImp = 0;
}

String &String::operator=(const String &rhs)
{
  if (this != &rhs) {
    if (iImp) {
      iImp->iRefCount -= 1;
      if (iImp->iRefCount == 0) {
	free(iImp->iData);
	delete iImp;
      }
    }
    iImp = rhs.iImp;
    if (iImp)
      iImp->iRefCount += 1;
  }
  return *this;
}

String::~String()
{
  if (iImp) {
    iImp->iRefCount -= 1;
    if (iImp->iRefCount == 0) {
      free(iImp->iData);
      delete iImp;
    }
  }
}

// --------------------------------------------------------------------

Dialog::Dialog(lua_State *L0, WINID parent, const char *caption)
  : iCaption(caption)
{
  L = L0;
  iParent = parent;
  iLuaDialog = LUA_NOREF;
  hDialog = NULL;
  iIgnoreEscape = false;
  iNoRows = 1;
  iNoCols = 1;
  // TODO: handle "Ctrl+Return"
}

Dialog::~Dialog()
{
  // dereference lua methods
  for (int i = 0; i < int(iElements.size()); ++i)
    luaL_unref(L, LUA_REGISTRYINDEX, iElements[i].lua_method);
  luaL_unref(L, LUA_REGISTRYINDEX, iLuaDialog);
}

void Dialog::callLua(int luaMethod)
{
  // only call back to Lua during execute()
  if (iLuaDialog == LUA_NOREF)
    return;

  lua_rawgeti(L, LUA_REGISTRYINDEX, luaMethod);
  lua_rawgeti(L, LUA_REGISTRYINDEX, iLuaDialog);
  lua_call(L, 1, 0);
}

// name, label, action
int Dialog::addButton(lua_State *L)
{
  SElement m;
  m.name = String(luaL_checkstring(L, 2));
  m.type = EButton;
  m.lua_method = LUA_NOREF;
  m.flags = 0;
  m.row = -1;
  m.col = -1;
  m.rowspan = 1;
  m.colspan = 1;
  m.text = String(luaL_checkstring(L, 3));
  if (lua_isstring(L, 4)) {
    const char *s = lua_tostring(L, 4);
    if (!strcmp(s, "accept"))
      m.flags |= EAccept;
    else if (!strcmp(s, "reject"))
      m.flags |= EReject;
    else
      luaL_argerror(L, 4, "unknown action");
  } else {
    luaL_argcheck(L, lua_isfunction(L, 4), 4, "unknown action");
    lua_pushvalue(L, 4);
    m.lua_method = luaL_ref(L, LUA_REGISTRYINDEX);
  }
  m.minHeight = 16;
  m.minWidth = 4 * m.text.len() + 8;
  if (m.minWidth < 64)
    m.minWidth = 64;
  iElements.push_back(m);
  return 0;
}

int Dialog::add(lua_State *L)
{
  static const char * const typenames[] =
    { "button", "text", "list", "label", "combo", "checkbox", "input", 0 };

  SElement m;
  m.name = String(luaL_checkstring(L, 2));
  m.type = TType(luaL_checkoption(L, 3, 0, typenames));
  luaL_checktype(L, 4, LUA_TTABLE);
  m.lua_method = LUA_NOREF;
  m.flags = 0;
  m.row = luaL_checkint(L, 5) - 1;
  m.col = luaL_checkint(L, 6) - 1;
  m.rowspan = 1;
  m.colspan = 1;
  if (!lua_isnoneornil(L, 7))
    m.rowspan = luaL_checkint(L, 7);
  if (!lua_isnoneornil(L, 8))
    m.colspan = luaL_checkint(L, 8);
  if (m.row + m.rowspan > iNoRows)
    iNoRows = m.row + m.rowspan;
  if (m.col + m.colspan > iNoCols)
    iNoCols = m.col + m.colspan;

  switch (m.type) {
  case EButton:
    addButtonItem(L, m);
    break;
  case ETextEdit:
    addTextEdit(L, m);
    break;
  case EList:
    addList(L, m);
    break;
  case ELabel:
    addLabel(L, m);
    break;
  case ECombo:
    addCombo(L, m);
    break;
  case ECheckBox:
    addCheckbox(L, m);
    break;
  case EInput:
    addInput(L, m);
    break;
  default:
    break;
  }
  iElements.push_back(m);
  return 0;
}

void Dialog::addLabel(lua_State *L, SElement &m)
{
  lua_getfield(L, 4, "label");
  luaL_argcheck(L, lua_isstring(L, -1), 4, "no label");
  m.text = String(lua_tostring(L, -1));
  lua_pop(L, 1); // label
  m.minHeight = 16;
  m.minWidth = 4 * m.text.len();
}

void Dialog::addButtonItem(lua_State *L, SElement &m)
{
  lua_getfield(L, 4, "label");
  luaL_argcheck(L, lua_isstring(L, -1), 4, "no button label");
  const char *label = lua_tostring(L, -1);
  m.text = String(label);
  lua_getfield(L, 4, "action");
  if (lua_isstring(L, -1)) {
    const char *s = lua_tostring(L, -1);
    if (!strcmp(s, "accept"))
      m.flags |= EAccept;
    else if (!strcmp(s, "reject"))
      m.flags |= EReject;
    else
      luaL_argerror(L, 4, "unknown action");
  } else if (lua_isfunction(L, -1)) {
    lua_pushvalue(L, -1);
    m.lua_method = luaL_ref(L, LUA_REGISTRYINDEX);
  } else if (!lua_isnil(L, -1))
    luaL_argerror(L, 4, "unknown action type");
  lua_pop(L, 2); // action, label
  m.minHeight = 16;
  m.minWidth = 4 * m.text.len() + 8;
  if (m.minWidth < 64)
    m.minWidth = 64;
}

void Dialog::addCheckbox(lua_State *L, SElement &m)
{
  lua_getfield(L, 4, "label");
  luaL_argcheck(L, lua_isstring(L, -1), 4, "no label");
  m.text = String(lua_tostring(L, -1));
  lua_getfield(L, 4, "action");
  if (!lua_isnil(L, -1)) {
    luaL_argcheck(L, lua_isfunction(L, -1), 4, "unknown action type");
    lua_pushvalue(L, -1);
    m.lua_method = luaL_ref(L, LUA_REGISTRYINDEX);
  }
  lua_pop(L, 2); // action, label
  m.value = 0;
  m.minHeight = 16;
  m.minWidth = 4 * m.text.len() + 32;
}

void Dialog::addInput(lua_State *L, SElement &m)
{
  m.minHeight = 12;
  m.minWidth = 100;
}

void Dialog::addTextEdit(lua_State *L, SElement &m)
{
  lua_getfield(L, 4, "read_only");
  if (lua_toboolean(L, -1))
    m.flags |= EReadOnly;
  lua_getfield(L, 4, "syntax");
  if (!lua_isnil(L, -1)) {
    const char *s = lua_tostring(L, -1);
    if (!strcmp(s, "logfile")) {
      m.flags |= ELogFile;
    } else if (!strcmp(s, "xml")) {
      m.flags |= EXml;
    } else if (!strcmp(s, "latex")) {
    } else
      luaL_argerror(L, 4, "unknown syntax");
  }
  lua_pop(L, 2); // syntax, read_only
  m.minHeight = 48;
  m.minWidth = 100;
}

void Dialog::setListItems(lua_State *L, int index, SElement &m)
{
  int no = lua_objlen(L, index);
  m.minWidth = 48;
  for (int i = 1; i <= no; ++i) {
    lua_rawgeti(L, index, i);
    luaL_argcheck(L, lua_isstring(L, -1), index, "items must be strings");
    const char *item = lua_tostring(L, -1);
    m.items.push_back(String(item));
    int w = 4 * strlen(item) + 16;
    if (w > m.minWidth)
      m.minWidth = w;
    lua_pop(L, 1); // item
  }
}

void Dialog::addList(lua_State *L, SElement &m)
{
  setListItems(L, 4, m);
  m.value = 0;
  m.minHeight = 48;
}

void Dialog::addCombo(lua_State *L, SElement &m)
{
  setListItems(L, 4, m);
  m.value = 0;
  m.minHeight = 16;
}

int Dialog::findElement(lua_State *L, int index)
{
  const char *name = luaL_checkstring(L, index);
  for (int i = 0; i < int(iElements.size()); ++i) {
    if (!strcmp(iElements[i].name.z(), name))
      return i;
  }
  return luaL_argerror(L, index, "no such element in dialog");
}

int Dialog::set(lua_State *L)
{
  const char *s = luaL_checkstring(L, 2);
  if (!strcmp(s, "ignore-cancel")) {
    iIgnoreEscape = lua_toboolean(L, 3);
    return 0;
  }

  int idx = findElement(L, 2);

  // set internal representation
  setUnmapped(L, idx);

  // if dialog is on screen, also set there
  if (iLuaDialog != LUA_NOREF)
    setMapped(L, idx);
  return 0;
}

void Dialog::setUnmapped(lua_State *L, int idx)
{
  SElement &m = iElements[idx];
  switch (m.type) {
  case ELabel:
  case ETextEdit:
  case EInput:
    m.text = String(luaL_checkstring(L, 3));
    break;
  case EList:
  case ECombo:
    if (lua_isnumber(L, 3)) {
      int n = luaL_checkint(L, 3);
      luaL_argcheck(L, 1 <= n && n <= int(m.items.size()), 3,
		    "list index out of bounds");
      m.value = n - 1;
    } else {
      luaL_checktype(L, 3, LUA_TTABLE);
      m.items.clear();
      setListItems(L, 3, m);
      m.value = 0;
    }
    break;
  case ECheckBox:
    m.value = lua_toboolean(L, 3);
    break;
  default:
    luaL_argerror(L, 2, "no suitable element");
  }
}

int Dialog::get(lua_State *L)
{
  if (iLuaDialog != LUA_NOREF)
    retrieveValues();

  int idx = findElement(L, 2);
  SElement &m = iElements[idx];

  switch (m.type) {
  case ETextEdit:
  case EInput:
    lua_pushstring(L, m.text.z());
    return 1;
  case EList:
  case ECombo:
    lua_pushinteger(L, m.value + 1);
    return 1;
  case ECheckBox:
    lua_pushboolean(L, m.value);
    return 1;
  default:
    return luaL_argerror(L, 2, "no suitable element");
  }
}

bool Dialog::execute(lua_State *L, int w, int h)
{
  // remember Lua object for dialog for callbacks
  lua_pushvalue(L, 1);
  iLuaDialog = luaL_ref(L, LUA_REGISTRYINDEX);
  bool result = buildAndRun(w, h); // execute dialog
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
  if (iLuaDialog != LUA_NOREF) // mapped
    enableItem(idx, value);
  else if (value)
    iElements[idx].flags &= ~EDisabled;
  else
    iElements[idx].flags |= EDisabled;
  return 0;
}

int Dialog::setStretch(lua_State *L)
{
  static const char * const typenames[] = { "row", "column", 0 };

  while (int(iColStretch.size()) < iNoCols)
    iColStretch.push_back(0);
  while (int(iRowStretch.size()) < iNoRows)
    iRowStretch.push_back(0);

  int type = luaL_checkoption(L, 2, 0, typenames);
  int rowcol = luaL_checkint(L, 3) - 1;
  int stretch = luaL_checkint(L, 4);

  if (type == 0) {
    luaL_argcheck(L, 0 <= rowcol && rowcol < iNoRows, 3,
		  "Row index out of range");
    iRowStretch[rowcol] = stretch;
  } else {
    luaL_argcheck(L, 0 <= rowcol && rowcol < iNoCols, 3,
		  "Column index out of range");
    iColStretch[rowcol] = stretch;
  }
  return 0;
}

// --------------------------------------------------------------------

static int dialog_tostring(lua_State *L)
{
  check_dialog(L, 1);
  lua_pushfstring(L, "Dialog@%p", lua_topointer(L, 1));
  return 1;
}

static int dialog_destructor(lua_State *L)
{
  //fprintf(stderr, "Dialog::~Dialog()\n");
  Dialog **dlg = check_dialog(L, 1);
  delete (*dlg);
  *dlg = 0;
  return 0;
}

static int dialog_execute(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  int w = 0;
  int h = 0;
  if (!lua_isnoneornil(L, 2)) {
    luaL_argcheck(L, lua_istable(L, 2), 2, "argument is not a table");
    lua_rawgeti(L, 2, 1);
    luaL_argcheck(L, lua_isnumber(L, -1), 2, "width is not a number");
    lua_rawgeti(L, 2, 2);
    luaL_argcheck(L, lua_isnumber(L, -1), 2, "height is not a number");
    w = lua_tointeger(L, -2);
    h = lua_tointeger(L, -1);
    lua_pop(L, 2); // w & h
  }
  lua_pushboolean(L, (*dlg)->execute(L, w, h));
  return 1;
}

static int dialog_setStretch(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->setStretch(L);
}

static int dialog_add(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->add(L);
}

static int dialog_addButton(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->addButton(L);
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
  { "addButton", dialog_addButton },
  { "set", dialog_set },
  { "get", dialog_get },
  { "setEnabled", dialog_setEnabled },
  { 0, 0 }
};

// --------------------------------------------------------------------

Menu::~Menu()
{
  // empty
}

// --------------------------------------------------------------------

static int menu_tostring(lua_State *L)
{
  check_menu(L, 1);
  lua_pushfstring(L, "Menu@%p", lua_topointer(L, 1));
  return 1;
}

static int menu_destructor(lua_State *L)
{
  //fprintf(stderr, "Menu::~Menu()\n");
  Menu **m = check_menu(L, 1);
  delete *m;
  *m = 0;
  return 0;
}

static int menu_execute(lua_State *L)
{
  Menu **m = check_menu(L, 1);
  return (*m)->execute(L);
}

static int menu_add(lua_State *L)
{
  Menu **m = check_menu(L, 1);
  return (*m)->add(L);
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

Timer::Timer(lua_State *L0, int lua_object, const char *method)
  : iMethod(method)
{
  L = L0;
  iLuaObject = lua_object;
  iSingleShot = false;
}

Timer::~Timer()
{
  luaL_unref(L, LUA_REGISTRYINDEX, iLuaObject);
}

void Timer::callLua()
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iLuaObject);
  lua_rawgeti(L, -1, 1); // get Lua object
  if (lua_isnil(L, -1)) {
    lua_pop(L, 2); // pop weak table, nil
    return;
  }
  lua_getfield(L, -1, iMethod.z());
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
  iSingleShot = t;
  return 0;
}

// --------------------------------------------------------------------

static int timer_tostring(lua_State *L)
{
  check_timer(L, 1);
  lua_pushfstring(L, "Timer@%p", lua_topointer(L, 1));
  return 1;
}

static int timer_destructor(lua_State *L)
{
  Timer **t = check_timer(L, 1);
  //fprintf(stderr, "Timer::~Timer()\n");
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

int luaopen_ipeui_common(lua_State *L)
{
  make_metatable(L, "Ipe.winid", winid_methods);
  make_metatable(L, "Ipe.dialog", dialog_methods);
  make_metatable(L, "Ipe.menu", menu_methods);
  make_metatable(L, "Ipe.timer", timer_methods);
  return 0;
}

// --------------------------------------------------------------------
