// --------------------------------------------------------------------
// Main function
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

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "lfs.h"
}

#include "appui.h"

#include "ipelua.h"

#include <cstdio>
#include <cstdlib>
#include <clocale>

#include <QApplication>
#include <QLocale>
#include <QDir>

extern int luaopen_ipeui(lua_State *L);
extern int luaopen_appui(lua_State *L);

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

static int traceback (lua_State *L)
{
  if (!lua_isstring(L, 1))  /* 'message' not a string? */
    return 1;  /* keep it intact */
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);    // pass error message
  lua_pushinteger(L, 2);  // skip this function and traceback
  lua_call(L, 2, 1);      // call debug.traceback
  return 1;
}

static void setup_config(lua_State *L, const char *var,
			 const char *env, const char *conf)
{
  const char *p = getenv(env);
#ifdef WIN32
  push_string(L, p ? p : Platform::ipeDir(conf));
#else
  lua_pushstring(L, p ? p : conf);
#endif
  lua_setfield(L, -2, var);
}

// --------------------------------------------------------------------

int main(int argc, char *argv[])
{
  Platform::initLib(IPELIB_VERSION);
  const char *qts = getenv("IPEQTSTYLE");
  if (qts)
    QApplication::setStyle(qts);
  QApplication a(argc, argv);
  setlocale(LC_NUMERIC, "C");
  a.setQuitOnLastWindowClosed(true);

  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  luaopen_ipe(L);
  luaopen_lfs(L);
  luaopen_ipeui(L);
  luaopen_appui(L);

  // create table with arguments
  lua_createtable(L, 0, argc - 1);
  for (int i = 1; i < argc; ++i) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "argv");

  lua_getglobal(L, "package");
  const char *luapath = getenv("IPELUAPATH");
#ifdef WIN32
  push_string(L, luapath ? luapath : Platform::ipeDir("lua/?.lua"));
#else
  lua_pushstring(L, luapath ? luapath : IPELUAPATH);
#endif
  lua_setfield(L, -2, "path");

  lua_newtable(L);  // config table
#if defined(WIN32)
  lua_pushliteral(L, "win");
#elif defined(__MacOSX__)
  lua_pushliteral(L, "apple");
#else
  lua_pushliteral(L, "unix");
#endif
  lua_setfield(L, -2, "platform");

#ifdef WIN32
  setup_config(L, "styles", "IPESTYLES", "styles");
  setup_config(L, "docdir", "IPEDOCDIR", "doc");
  setup_config(L, "ipelets", "IPELETPATH", "ipelets");
#else
  setup_config(L, "styles", "IPESTYLES", IPESTYLES);
  setup_config(L, "docdir", "IPEDOCDIR", IPEDOCDIR);
  setup_config(L, "ipelets", "IPELETPATH", IPELETPATH);
#endif
  push_string(L, Platform::fontmapFile());
  lua_setfield(L, -2, "fontmap");
  push_string(L, Platform::latexDirectory());
  lua_setfield(L, -2, "latexdir");
  push_string(L, Prefs::get()->iconDir());
  lua_setfield(L, -2, "icons");

  lua_pushstring(L, QDir::homePath().toUtf8());
  lua_setfield(L, -2, "home");

  lua_pushfstring(L, "Ipe %d.%d.%d",
		  IPELIB_VERSION / 10000,
		  (IPELIB_VERSION / 100) % 100,
		  IPELIB_VERSION % 100);
  lua_setfield(L, -2, "version");

  lua_setglobal(L, "config");

  // run Ipe
  lua_pushcfunction(L, traceback);
  assert(luaL_loadstring(L, "require \"main\"") == 0);

  if (lua_pcall(L, 0, 0, -2)) {
    const char *errmsg = lua_tostring(L, -1);
    fprintf(stderr, "%s\n", errmsg);
    return 1;
  }

  lua_close(L);
  return 0;
}

// --------------------------------------------------------------------
