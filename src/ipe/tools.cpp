// --------------------------------------------------------------------
// Canvas tools used from Lua
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

#include "tools.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "ipeqtcanvas.h"

#include "ipepainter.h"

#include "ipelua.h"

using namespace ipelua;

// --------------------------------------------------------------------

IpeTransformTool::IpeTransformTool(Canvas *canvas, Page *page, int view,
				   TType type, bool withShift,
				   lua_State *L0, int method)
  : TransformTool(canvas, page, view, type, withShift)
{
  L = L0;
  iMethod = method;
}

IpeTransformTool::~IpeTransformTool()
{
  luaL_unref(L, LUA_REGISTRYINDEX, iMethod);
}

void IpeTransformTool::report()
{
  // call back to Lua to report final transformation
  lua_rawgeti(L, LUA_REGISTRYINDEX, iMethod);
  push_matrix(L, iTransform);
  lua_call(L, 1, 0);
}

// --------------------------------------------------------------------

static void push_modifiers(lua_State *L, int button)
{
  lua_createtable(L, 0, 3);
  lua_pushboolean(L, (button & Qt::ShiftModifier));
  lua_setfield(L, -2, "shift");
  lua_pushboolean(L, (button & Qt::ControlModifier));
  lua_setfield(L, -2, "control");
  lua_pushboolean(L, (button & Qt::AltModifier));
  lua_setfield(L, -2, "alt");
  lua_pushboolean(L, (button & Qt::MetaModifier));
  lua_setfield(L, -2, "meta");
}

void push_button(lua_State *L, int button)
{
  lua_pushinteger(L, (button & 0xff)); // button number
  push_modifiers(L, button);
}

// --------------------------------------------------------------------

LuaTool::LuaTool(Canvas *canvas, lua_State *L0, int luatool)
  : Tool(canvas)
{
  L = L0;
  iLuaTool = luatool;
  iColor = Color(0, 0, 0);
}

LuaTool::~LuaTool()
{
  luaL_unref(L, LUA_REGISTRYINDEX, iLuaTool);
}

void LuaTool::mouseButton(int button, bool press)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iLuaTool);
  lua_getfield(L, -1, "mouseButton");
  lua_pushvalue(L, -2); // model
  lua_remove(L, -3);
  push_button(L, button);
  lua_pushboolean(L, press);
  lua_call(L, 4, 0);
}

void LuaTool::mouseMove(int button)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iLuaTool);
  lua_getfield(L, -1, "mouseMove");
  lua_pushvalue(L, -2); // model
  lua_remove(L, -3);
  push_button(L, button);
  lua_call(L, 3, 0);
}

bool LuaTool::key(int code, int modifiers, String text)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iLuaTool);
  lua_getfield(L, -1, "key");
  lua_pushvalue(L, -2); // model
  lua_remove(L, -3);
  lua_pushnumber(L, code);
  push_modifiers(L, modifiers);
  push_string(L, text);
  lua_State *L0 = L; // need to save L since
  lua_call(L, 4, 1); // this may delete tool
  return lua_toboolean(L0, 1);
}

// --------------------------------------------------------------------

ShapeTool::ShapeTool(Canvas *canvas, lua_State *L0, int luatool)
  : LuaTool(canvas, L0, luatool)
{
  // nothing else
}

void ShapeTool::draw(Painter &painter) const
{
  double z = 1.0 / iCanvas->zoom();
  painter.setStroke(Attribute(iColor));
  painter.newPath();
  iShape.draw(painter);
  painter.drawPath(EStrokedOnly);
  painter.setStroke(Attribute(Color(0, 1000, 0)));
  painter.newPath();
  iAuxShape.draw(painter);
  painter.drawPath(EStrokedOnly);
  for (uint i = 0; i < iMarks.size(); ++i) {
    switch (iMarks[i].t) {
    case EVertex:
      painter.setFill(Attribute(Color(1000, 0, 1000)));
      break;
    case ECenter:
    case ERadius:
      painter.setFill(Attribute(Color(0, 0, 1000)));
      break;
    case ESplineCP:
      painter.setFill(Attribute(Color(0, 0, 800)));
      break;
    case EBezierCP:
    case EMinor:
      painter.setFill(Attribute(Color(0, 800, 0)));
      break;
    case ECurrent:
      painter.setStroke(Attribute(Color(1000, 0, 0)));
      break;
    case EScissor:
      painter.setFill(Attribute(Color(1000, 0, 0)));
      break;
    default:
      break;
    }
    painter.pushMatrix();
    painter.translate(iMarks[i].v);
    painter.untransform(ETransformationsTranslations);
    switch (iMarks[i].t) {
    case EVertex:
    case ECenter:
    default:
      painter.newPath();
      painter.moveTo(Vector(6*z, 0));
      painter.drawArc(Arc(Matrix(6*z, 0, 0, 6*z, 0, 0)));
      painter.closePath();
      painter.drawPath(EFilledOnly);
      break;
    case ECurrent:
      painter.newPath();
      painter.moveTo(Vector(9*z, 0));
      painter.drawArc(Arc(Matrix(9*z, 0, 0, 9*z, 0, 0)));
      painter.closePath();
      painter.drawPath(EStrokedOnly);
      break;
    case ESplineCP:
    case EBezierCP:
    case ERadius:
    case EMinor:
      painter.newPath();
      painter.moveTo(Vector(-4*z, -4*z));
      painter.lineTo(Vector(4*z, -4*z));
      painter.lineTo(Vector(4*z, 4*z));
      painter.lineTo(Vector(-4*z, 4*z));
      painter.closePath();
      painter.drawPath(EFilledOnly);
      break;
    case EScissor:
      painter.newPath();
      painter.moveTo(Vector(5*z, 0));
      painter.lineTo(Vector(0, 5*z));
      painter.lineTo(Vector(-5*z, 0));
      painter.lineTo(Vector(0, -5*z));
      painter.closePath();
      painter.drawPath(EFilledOnly);
      break;
    }
    painter.popMatrix();
  }
}

void ShapeTool::setShape(Shape shape, int which)
{
  if (which == 1)
    iAuxShape = shape;
  else
    iShape = shape;
}

void ShapeTool::clearMarks()
{
  iMarks.clear();
}

void ShapeTool::addMark(const Vector &v, TMarkType t)
{
  SMark m;
  m.v = v;
  m.t = t;
  iMarks.push_back(m);
}

// --------------------------------------------------------------------

PasteTool::PasteTool(Canvas *canvas, lua_State *L0, int luatool, Object *obj)
  : LuaTool(canvas, L0, luatool)
{
  iObject = obj;
}

PasteTool::~PasteTool()
{
  delete iObject;
}

void PasteTool::draw(Painter &painter) const
{
  painter.transform(iMatrix);
  painter.setStroke(Attribute(iColor));
  iObject->drawSimple(painter);
}

// --------------------------------------------------------------------
