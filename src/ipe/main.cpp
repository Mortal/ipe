// --------------------------------------------------------------------
// Main function
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2011  Otfried Cheong

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

#include "ipebase.h"
#include "ipelua.h"

#include <cstdio>
#include <cstdlib>

#ifdef IPEUI_QT
#include "appui_qt.h"
#include <locale.h>
#include <QLocale>
#include <QDir>
#include <QDesktopWidget>
#endif

#ifdef IPEUI_WIN32
#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0500
#include <windows.h>
#include <commctrl.h>
extern void ipe_init_canvas(HINSTANCE hInstance);
extern void ipe_init_appui(HINSTANCE hInstance, int nCmdShow);
#endif

#ifdef IPEUI_GTK
#include <gtk/gtk.h>
#endif

using namespace ipe;
using namespace ipelua;

extern int luaopen_ipeui(lua_State *L);
extern int luaopen_appui(lua_State *L);
extern int luaopen_lfs (lua_State *L);

// --------------------------------------------------------------------

String ipeIconDirectory()
{
  static String iconDir;
  if (iconDir.empty()) {
    const char *p = getenv("IPEICONDIR");
#ifdef WIN32
    String s(p ? p : Platform::ipeDir("icons\\"));
#else
    String s(p ? p : IPEICONDIR);
    if (s.right(1) != "/")
      s += "/";
#endif
    iconDir = s;
  }
  return iconDir;
}

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

static lua_State *setup_lua()
{
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  luaopen_ipe(L);
  luaopen_lfs(L);
  luaopen_ipeui(L);
  luaopen_appui(L);
  return L;
}

static void setup_globals(lua_State *L, const char *home,
			  int width, int height, const char *tk)
{
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
#elif defined(__APPLE__)
  lua_pushliteral(L, "apple");
#else
  lua_pushliteral(L, "unix");
#endif
  lua_setfield(L, -2, "platform");
  lua_pushstring(L, tk);
  lua_setfield(L, -2, "toolkit");

#ifdef WIN32
  setup_config(L, "styles", "IPESTYLES", "styles");
  setup_config(L, "docdir", "IPEDOCDIR", "doc");
  setup_config(L, "ipelets", "IPELETPATH", "ipelets");
#else
  setup_config(L, "styles", "IPESTYLES", IPESTYLEDIR);
  setup_config(L, "docdir", "IPEDOCDIR", IPEDOCDIR);
  setup_config(L, "ipelets", "IPELETPATH", IPELETPATH);
#endif
  push_string(L, Platform::fontmapFile());
  lua_setfield(L, -2, "fontmap");
  push_string(L, Platform::latexDirectory());
  lua_setfield(L, -2, "latexdir");
  push_string(L, ipeIconDirectory());
  lua_setfield(L, -2, "icons");

  if (home) {
    lua_pushstring(L, home);
    lua_setfield(L, -2, "home");
  }

  lua_pushfstring(L, "Ipe %d.%d.%d",
		  IPELIB_VERSION / 10000,
		  (IPELIB_VERSION / 100) % 100,
		  IPELIB_VERSION % 100);
  lua_setfield(L, -2, "version");

  lua_createtable(L, 0, 2);
  lua_pushinteger(L, width);
  lua_rawseti(L, -2, 1);
  lua_pushinteger(L, height);
  lua_rawseti(L, -2, 2);
  lua_setfield(L, -2, "screen_geometry");

  lua_setglobal(L, "config");

}

static bool lua_run_ipe(lua_State *L, lua_CFunction fn)
{
  lua_pushcfunction(L, fn);
  lua_setglobal(L, "mainloop");

  // run Ipe
  lua_pushcfunction(L, traceback);
  assert(luaL_loadstring(L, "require \"main\"") == 0);

  if (lua_pcall(L, 0, 0, -2)) {
    const char *errmsg = lua_tostring(L, -1);
    fprintf(stderr, "%s\n", errmsg);
    return false;
  }
  return true;
}

// --------------------------------------------------------------------

#ifdef IPEUI_QT

int mainloop(lua_State *L)
{
  QApplication::exec();
  return 0;
}

int main(int argc, char *argv[])
{
  Platform::initLib(IPELIB_VERSION);
  lua_State *L = setup_lua();

  IpeApp a(L, argc, argv);
  a.setQuitOnLastWindowClosed(true);
  setlocale(LC_NUMERIC, "C");

  // create table with arguments
  lua_createtable(L, 0, argc - 1);
  for (int i = 1; i < argc; ++i) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "argv");

  QRect r = a.desktop()->screenGeometry();

  setup_globals(L, QDir::homePath().toUtf8(),
		r.width(), r.height(), "qt");

  lua_run_ipe(L, mainloop);

  lua_close(L);
  return 0;
}
#endif

// --------------------------------------------------------------------

#ifdef IPEUI_WIN32
int mainloop(lua_State *L)
{
  luaL_argcheck(L, lua_istable(L, 1), 1, "Argument is not a table");
  int no = lua_objlen(L, 1);
  luaL_argcheck(L, no > 0, 1, "Table must have at least one shortcut");
  std::vector<ACCEL> accel;
  for (int i = 1; i <= no; i += 2) {
    lua_rawgeti(L, 1, i);
    lua_rawgeti(L, 1, i+1);
    ACCEL a;
    int key = luaL_checkinteger(L, -2);
    a.key = key & 0xffff;
    a.cmd = luaL_checkinteger(L, -1);
    a.fVirt = (((key & 0x10000) ? FALT : 0) |
	       ((key & 0x20000) ? FCONTROL : 0) |
	       ((key & 0x40000) ? FSHIFT : 0) |
	       FVIRTKEY);
    accel.push_back(a);
    lua_pop(L, 2);
  }

  HACCEL hAccel = CreateAcceleratorTable(&accel[0], accel.size());

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0) > 0) {
    HWND target = GetAncestor(msg.hwnd, GA_ROOT);
    if (!TranslateAccelerator(target, hAccel, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		   LPSTR lpCmdLine, int nCmdShow)
{
  Platform::initLib(IPELIB_VERSION);

  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC   = ICC_COOL_CLASSES | ICC_BAR_CLASSES |
    ICC_USEREX_CLASSES | ICC_STANDARD_CLASSES;
  InitCommonControlsEx(&icex);

  ipe_init_canvas(hInstance);
  ipe_init_appui(hInstance, nCmdShow);

  lua_State *L = setup_lua();

  // setup command line argument
  lua_pushstring(L, lpCmdLine);
  lua_setglobal(L, "argv");

  int cx = GetSystemMetrics(SM_CXSCREEN);
  int cy = GetSystemMetrics(SM_CYSCREEN);
  setup_globals(L, "C:\\", cx, cy, "win32");

  lua_run_ipe(L, mainloop);

  lua_close(L);
  return 0;
}
#endif

// --------------------------------------------------------------------

#ifdef IPEUI_GTK
int mainloop(lua_State *L)
{
  gtk_main();
  return 0;
}

int main(int argc, char *argv[])
{
  Platform::initLib(IPELIB_VERSION);
  gtk_init(&argc, &argv);
  lua_State *L = setup_lua();

  // create table with arguments
  lua_createtable(L, 0, argc - 1);
  for (int i = 1; i < argc; ++i) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "argv");

  GdkScreen* screen = gdk_screen_get_default();
  int width = gdk_screen_get_width(screen);
  int height = gdk_screen_get_height(screen);
  ipeDebug("Screen resolution is (%d x %d)", width, height);
  setup_globals(L, getenv("HOME"), width, height, "gtk");

  lua_run_ipe(L, mainloop);

  lua_close(L);
  return 0;
}

#endif

// --------------------------------------------------------------------
