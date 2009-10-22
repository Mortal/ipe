// --------------------------------------------------------------------
// Lua bindings for user interface
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
#include "lfs.h"
}

#include "appui.h"
#include "tools.h"

#include "ipelua.h"

#include <cstdio>
#include <cstdlib>

#include <QApplication>
#include <QStatusBar>

using namespace ipe;
using namespace ipeqt;
using namespace ipelua;

// --------------------------------------------------------------------

static Canvas *check_canvas(lua_State *L, int i)
{
  AppUi **ui = (AppUi **) luaL_checkudata(L, i, "Ipe.appui");
  return (*ui)->canvas();
}

inline AppUi **check_appui(lua_State *L, int i)
{
  return (AppUi **) luaL_checkudata(L, i, "Ipe.appui");
}

static int appui_tostring(lua_State *L)
{
  check_appui(L, 1);
  lua_pushfstring(L, "AppUi@%p", lua_topointer(L, 1));
  return 1;
}

/* When the Lua model is collected, its "ui" userdata will be garbage
   collected as well.  At this point, the C++ object has long been
   deleted. */
static int appui_destructor(lua_State *L)
{
  check_appui(L, 1);
  ipeDebug("AppUi Lua destructor");
  return 0;
}

// --------------------------------------------------------------------

static int appui_setPage(lua_State *L)
{
  AppUi **ui = check_appui(L, 1);
  Page *page = check_page(L, 2)->page;
  int view = check_viewno(L, 3, page);
  Cascade *sheets = check_cascade(L, 4)->cascade;
  (*ui)->canvas()->setPage(page, view, sheets);
  return 0;
}

static int appui_setFontPool(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  Document **d = check_document(L, 2);
  const FontPool *fonts = (*d)->fontPool();
  canvas->setFontPool(fonts);
  return 0;
}

static int appui_pan(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  Vector p = canvas->pan();
  push_vector(L, p);
  return 1;
}

static int appui_setPan(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  Vector *v = check_vector(L, 2);
  canvas->setPan(*v);
  return 0;
}

static int appui_zoom(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  lua_pushnumber(L, canvas->zoom());
  return 1;
}

static int appui_setZoom(lua_State *L)
{
  AppUi **ui = check_appui(L, 1);
  (*ui)->setZoom(luaL_checknumber(L, 2));
  return 0;
}

static int appui_pos(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  push_vector(L, canvas->pos());
  return 1;
}

static int appui_globalPos(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  push_vector(L, canvas->globalPos());
  return 1;
}

static int appui_unsnappedPos(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  push_vector(L, canvas->unsnappedPos());
  return 1;
}

static int appui_simpleSnapPos(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  push_vector(L, canvas->simpleSnapPos());
  return 1;
}

static int appui_setPretty(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  canvas->setPretty(lua_toboolean(L, 2));
  return 0;
}

static int appui_setFifiVisible(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  canvas->setFifiVisible(lua_toboolean(L, 2));
  return 0;
}

static void snapFlag(lua_State *L, int &flags, const char *mode, uint bits)
{
  lua_getfield(L, -1, mode);
  if (!lua_isnil(L, -1))
    flags = lua_toboolean(L, -1) ? (flags | bits) : (flags & ~bits);
  lua_pop(L, 1);
}

static int appui_setSnap(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);
  Snap snap = canvas->snap();
  snapFlag(L, snap.iSnap, "snapvtx", Snap::ESnapVtx);
  snapFlag(L, snap.iSnap, "snapbd", Snap::ESnapBd);
  snapFlag(L, snap.iSnap, "snapint", Snap::ESnapInt);
  snapFlag(L, snap.iSnap, "snapgrid", Snap::ESnapGrid);
  snapFlag(L, snap.iSnap, "snapangle", Snap::ESnapAngle);
  snapFlag(L, snap.iSnap, "snapauto", Snap::ESnapAuto);
  lua_getfield(L, -1, "grid_visible");
  if (!lua_isnil(L, -1))
    snap.iGridVisible = lua_toboolean(L, -1);
  lua_pop(L, 1);
  lua_getfield(L, -1, "gridsize");
  if (!lua_isnil(L, -1))
    snap.iGridSize = luaL_checkint(L, -1);
  lua_pop(L, 1);
  lua_getfield(L, -1, "anglesize");
  if (!lua_isnil(L, -1))
    snap.iAngleSize = IpePi * luaL_checknumber(L, -1) / 180.0;
  lua_pop(L, 1);
  lua_getfield(L, -1, "snap_distance");
  if (!lua_isnil(L, -1))
    snap.iSnapDistance = luaL_checkint(L, -1);
  lua_pop(L, 1);
  lua_getfield(L, -1, "with_axes");
  if (!lua_isnil(L, -1))
    snap.iWithAxes = lua_toboolean(L, -1);
  lua_pop(L, 1);
  lua_getfield(L, -1, "origin");
  if (is_type(L, -1, "Ipe.vector"))
    snap.iOrigin = *check_vector(L, -1);
  lua_pop(L, 1);
  lua_getfield(L, -1, "orientation");
  if (!lua_isnil(L, -1))
    snap.iDir = luaL_checknumber(L, -1);
  lua_pop(L, 1);
  canvas->setSnap(snap);
  return 0;
}

static int appui_setAutoOrigin(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  Vector *v = check_vector(L, 2);
  canvas->setAutoOrigin(*v);
  return 0;
}

static int appui_update(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  bool all = true;
  if (lua_gettop(L) == 2)
    all = lua_toboolean(L, 2);
  if (all)
    canvas->update();
  else
    canvas->updateTool();
  return 0;
}

static int appui_finishTool(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  canvas->finishTool();
  return 0;
}

static int appui_canvasSize(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  push_vector(L, Vector(canvas->width(), canvas->height()));
  return 1;
}

// --------------------------------------------------------------------

static int appui_pantool(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  Page *page = check_page(L, 2)->page;
  int view = check_viewno(L, 3, page);
  PanTool *tool = new PanTool(canvas, page, view);
  canvas->setTool(tool);
  return 0;
}

static int appui_selecttool(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  Page *page = check_page(L, 2)->page;
  int view = check_viewno(L, 3, page);
  bool nonDestructive = lua_toboolean(L, 4);
  SelectTool *tool = new SelectTool(canvas, page, view, nonDestructive);
  canvas->setTool(tool);
  return 0;
}

static int appui_transformtool(lua_State *L)
{
  static const char * const option_names[] = {
    "translate", "scale", "stretch", "rotate", 0 };
  Canvas *canvas = check_canvas(L, 1);
  Page *page = check_page(L, 2)->page;
  int view = check_viewno(L, 3, page);
  int type = luaL_checkoption(L, 4, 0, option_names);
  bool withShift = lua_toboolean(L, 5);
  lua_pushvalue(L, 6);
  int method = luaL_ref(L, LUA_REGISTRYINDEX);
  IpeTransformTool *tool =
    new IpeTransformTool(canvas, page, view, TransformTool::TType(type),
			 withShift, L, method);
  if (tool->isValid()) {
    canvas->setTool(tool);
    lua_pushboolean(L, true);
    return 1;
  } else {
    delete tool;
    return 0;
  }
}

static int luatool_setcolor(lua_State *L)
{
  LuaTool *tool = (LuaTool *) lua_touserdata(L, lua_upvalueindex(1));
  int r = int(1000 * luaL_checknumber(L, 1) + 0.5);
  int g = int(1000 * luaL_checknumber(L, 2) + 0.5);
  int b = int(1000 * luaL_checknumber(L, 3) + 0.5);
  luaL_argcheck(L, (0 <= r && r <= 1000 &&
		    0 <= g && g <= 1000 &&
		    0 <= b && b <= 1000), 2,
		"color components must be between 0.0 and 1.0");
  tool->setColor(Color(r, g, b));
  return 0;
}

static int shapetool_setshape(lua_State *L)
{
  ShapeTool *tool = (ShapeTool *) lua_touserdata(L, lua_upvalueindex(1));
  Shape shape = check_shape(L, 1);
  int which = 0;
  if (!lua_isnoneornil(L, 2))
    which = luaL_checkint(L, 2);
  tool->setShape(shape, which);
  return 0;
}

static int shapetool_setmarks(lua_State *L)
{
  ShapeTool *tool = (ShapeTool *) lua_touserdata(L, lua_upvalueindex(1));
  luaL_argcheck(L, lua_istable(L, 1), 1, "argument is not a table");
  int no = lua_objlen(L, 1);
  tool->clearMarks();
  for (int i = 1; i + 1 <= no; i += 2) {
    lua_rawgeti(L, 1, i);
    luaL_argcheck(L, is_type(L, -1, "Ipe.vector"), 1,
		  "element is not a vector");
    Vector *v = check_vector(L, -1);
    lua_rawgeti(L, 1, i+1);
    luaL_argcheck(L, lua_isnumber(L, -1), 1, "element is not a number");
    int t = lua_tointeger(L, -1);
    luaL_argcheck(L, ShapeTool::EVertex <= t && t < ShapeTool::ENumMarkTypes,
		  1, "number is not a mark type");
    lua_pop(L, 2); // v, t
    tool->addMark(*v, ShapeTool::TMarkType(t));
  }
  return 0;
}

static int pastetool_setmatrix(lua_State *L)
{
  PasteTool *tool = (PasteTool *) lua_touserdata(L, lua_upvalueindex(1));
  Matrix *m = check_matrix(L, 1);
  tool->setMatrix(*m);
  return 0;
}

static int appui_shapetool(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  lua_pushvalue(L, 2);
  int luatool = luaL_ref(L, LUA_REGISTRYINDEX);
  ShapeTool *tool = new ShapeTool(canvas, L, luatool);
  // add methods to Lua tool
  lua_rawgeti(L, LUA_REGISTRYINDEX, luatool);
  lua_pushlightuserdata(L, tool);
  lua_pushcclosure(L, luatool_setcolor, 1);
  lua_setfield(L, -2, "setColor");
  lua_pushlightuserdata(L, tool);
  lua_pushcclosure(L, shapetool_setshape, 1);
  lua_setfield(L, -2, "setShape");
  lua_pushlightuserdata(L, tool);
  lua_pushcclosure(L, shapetool_setmarks, 1);
  lua_setfield(L, -2, "setMarks");
  canvas->setTool(tool);
  return 0;
}

static int appui_pastetool(lua_State *L)
{
  Canvas *canvas = check_canvas(L, 1);
  Object *obj = check_object(L, 2)->obj;
  lua_pushvalue(L, 3);
  int luatool = luaL_ref(L, LUA_REGISTRYINDEX);
  PasteTool *tool = new PasteTool(canvas, L, luatool, obj->clone());
  // add methods to Lua tool
  lua_rawgeti(L, LUA_REGISTRYINDEX, luatool);
  lua_pushlightuserdata(L, tool);
  lua_pushcclosure(L, luatool_setcolor, 1);
  lua_setfield(L, -2, "setColor");
  lua_pushlightuserdata(L, tool);
  lua_pushcclosure(L, pastetool_setmatrix, 1);
  lua_setfield(L, -2, "setMatrix");
  canvas->setTool(tool);
  return 0;
}

// --------------------------------------------------------------------

static int appui_close(lua_State *L)
{
  AppUi **ui = check_appui(L, 1);
  lua_pushnumber(L, (*ui)->close());
  return 1;
}

static int appui_actionState(lua_State *L)
{
  AppUi **ui = check_appui(L, 1);
  const char *name = luaL_checkstring(L, 2);
  IpeAction *a = (*ui)->findAction(name);
  lua_pushboolean(L, a->isChecked());
  return 1;
}

static int appui_setActionState(lua_State *L)
{
  AppUi **ui = check_appui(L, 1);
  const char *name = luaL_checkstring(L, 2);
  bool val = lua_toboolean(L, 3);
  IpeAction *a = (*ui)->findAction(name);
  a->setChecked(val);
  return 0;
}

static int appui_setupSymbolicNames(lua_State *L)
{
  AppUi **ui = check_appui(L, 1);
  Cascade *sheets = check_cascade(L, 2)->cascade;
  (*ui)->setupSymbolicNames(sheets);
  return 0;
}

static int appui_setAttributes(lua_State *L)
{
  AppUi **ui = check_appui(L, 1);
  Cascade *sheets = check_cascade(L, 2)->cascade;
  AllAttributes all;
  check_allattributes(L, 3, all);
  (*ui)->setAttributes(all, sheets);
  return 0;
}

static int appui_setGridAngleSize(lua_State *L)
{
  AppUi **ui = check_appui(L, 1);
  Attribute gs = check_number_attribute(L, 2);
  Attribute as = check_number_attribute(L, 3);
  (*ui)->setGridAngleSize(gs, as);
  return 0;
}

static int appui_setLayers(lua_State *L)
{
  AppUi **ui = check_appui(L, 1);
  Page *page = check_page(L, 2)->page;
  int view = check_viewno(L, 3, page);
  (*ui)->setLayers(page, view);
  return 0;
}

static int appui_setNumbers(lua_State *L)
{
  AppUi **ui = check_appui(L, 1);
  String vno;
  String pno;
  if (!lua_isnil(L, 2))
    vno = luaL_checkstring(L, 2);
  if (!lua_isnil(L, 3))
    pno = luaL_checkstring(L, 3);
  (*ui)->setNumbers(vno, pno);
  return 0;
}

static int appui_setBookmarks(lua_State *L)
{
  AppUi **ui = check_appui(L, 1);
  return (*ui)->setBookmarks(L);
}

static int appui_setWindowTitle(lua_State *L)
{
  AppUi **ui = check_appui(L, 1);
  String s = luaL_checkstring(L, 2);
  (*ui)->setWindowTitle(QIpe(s));
  return 0;
}

static int appui_explain(lua_State *L)
{
  AppUi **ui = check_appui(L, 1);
  const char *s = luaL_checkstring(L, 2);
  (*ui)->statusBar()->showMessage(s, 4000);
  return 0;
}

// --------------------------------------------------------------------

static const struct luaL_Reg appui_methods[] = {
  { "__tostring", appui_tostring },
  { "__gc", appui_destructor },
  // --------------------------------------------------------------------
  { "setPage", appui_setPage},
  { "pan", appui_pan},
  { "setPan", appui_setPan},
  { "zoom", appui_zoom},
  { "setZoom", appui_setZoom},
  { "setFontPool", appui_setFontPool},
  { "pos", appui_pos},
  { "globalPos", appui_globalPos},
  { "unsnappedPos", appui_unsnappedPos},
  { "simpleSnapPos", appui_simpleSnapPos },
  { "setPretty", appui_setPretty},
  { "setFifiVisible", appui_setFifiVisible},
  { "setSnap", appui_setSnap},
  { "setAutoOrigin", appui_setAutoOrigin },
  { "update", appui_update},
  { "finishTool", appui_finishTool},
  { "canvasSize", appui_canvasSize },
  // --------------------------------------------------------------------
  { "panTool", appui_pantool },
  { "selectTool", appui_selecttool },
  { "transformTool", appui_transformtool },
  { "shapeTool", appui_shapetool },
  { "pasteTool", appui_pastetool },
  // --------------------------------------------------------------------
  { "close", appui_close},
  { "setActionState", appui_setActionState },
  { "actionState", appui_actionState },
  { "explain", appui_explain },
  { "setWindowTitle", appui_setWindowTitle },
  { "setupSymbolicNames", appui_setupSymbolicNames },
  { "setAttributes", appui_setAttributes },
  { "setGridAngleSize", appui_setGridAngleSize },
  { "setLayers", appui_setLayers },
  { "setNumbers", appui_setNumbers },
  { "setBookmarks", appui_setBookmarks },
  { 0, 0},
};

// --------------------------------------------------------------------

static int appui_constructor(lua_State *L)
{
  luaL_checktype(L, 1, LUA_TTABLE); // this is the model

  AppUi **ui = (AppUi **) lua_newuserdata(L, sizeof(AppUi *));
  *ui = 0;
  luaL_getmetatable(L, "Ipe.appui");
  lua_setmetatable(L, -2);

  lua_pushvalue(L, 1);
  int model = luaL_ref(L, LUA_REGISTRYINDEX);
  *ui = new AppUi(L, model);
  (*ui)->show();
  return 1;
}

// --------------------------------------------------------------------

int luaopen_appui(lua_State *L)
{
  lua_pushcfunction(L, appui_constructor);
  lua_setglobal(L, "AppUi");
  make_metatable(L, "Ipe.appui", appui_methods);
  return 0;
}

// --------------------------------------------------------------------
