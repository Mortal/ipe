// --------------------------------------------------------------------
// Lua bindings for Win32 dialogs
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

#include "ipeui_common.h"

#include <windowsx.h>

// --------------------------------------------------------------------

#define IDBASE 9000
#define PAD 4
#define BORDER 8
#define BUTTONHEIGHT 16

class PDialog : public Dialog {
public:
  PDialog(lua_State *L0, WINID parent, const char *caption);
  virtual ~PDialog();

protected:
  virtual void setMapped(lua_State *L, int idx);
  virtual bool buildAndRun(int w, int h);
  virtual void retrieveValues();
  virtual void enableItem(int idx, bool value);

  void computeDimensions(int &w, int &h);

  void buildFlags(std::vector<short> &t, DWORD flags);
  void buildString(std::vector<short> &t, const char *s);
  void buildDimensions(std::vector<short> &t, SElement &m, int id);
  void buildControl(std::vector<short> &t, short what, const char *s = 0);

  void initDialog();
  BOOL dlgCommand(WPARAM wParam, LPARAM lParam);
  static BOOL CALLBACK dialogProc(HWND hwndDlg, UINT message,
				  WPARAM wParam, LPARAM lParam);
private:
  int iBaseX, iBaseY;
  int iButtonX;
  std::vector<int> iRowHeight;
  std::vector<int> iColWidth;
};

PDialog::PDialog(lua_State *L0, WINID parent, const char *caption)
  : Dialog(L0, parent, caption)
{
  LONG base = GetDialogBaseUnits();
  iBaseX = LOWORD(base);
  iBaseY = HIWORD(base);
  // TODO: handle "Ctrl+Return"
}

PDialog::~PDialog()
{
  // nothing yet
}

void PDialog::setMapped(lua_State *L, int idx)
{
  SElement &m = iElements[idx];
  HWND h = GetDlgItem(hDialog, idx+IDBASE);
  switch (m.type) {
  case ETextEdit:
  case EInput:
  case ELabel:
    SetWindowText(h, m.text.z());
    break;
  case EList:
    if (!lua_isnumber(L, 3)) {
      ListBox_ResetContent(h);
      for (int j = 0; j < int(m.items.size()); ++j)
	ListBox_AddString(h, m.items[j].z());
    }
    ListBox_SetCurSel(h, m.value);
    break;
  case ECombo:
    if (!lua_isnumber(L, 3)) {
      ComboBox_ResetContent(h);
      for (int j = 0; j < int(m.items.size()); ++j)
	ComboBox_AddString(h, m.items[j].z());
    }
    ComboBox_SetCurSel(h, m.value);
    break;
  case ECheckBox:
    CheckDlgButton(hDialog, idx+IDBASE,
		   (m.value ? BST_CHECKED : BST_UNCHECKED));
    break;
  default:
    break; // already handled in setUnmapped
  }
}

void PDialog::initDialog()
{
  for (int i = 0; i < int(iElements.size()); ++i) {
    SElement &m = iElements[i];
    HWND h = GetDlgItem(hDialog, i+IDBASE);
    if (m.flags & EDisabled)
      EnableWindow(h, FALSE);
    switch (m.type) {
    case EInput:
    case ETextEdit:
      SetWindowText(h, m.text.z());
      break;
    case EList:
      for (int j = 0; j < int(m.items.size()); ++j)
	ListBox_AddString(h, m.items[j].z());
      ListBox_SetCurSel(h, m.value);
      break;
    case ECombo:
      for (int j = 0; j < int(m.items.size()); ++j)
	ComboBox_AddString(h, m.items[j].z());
      ComboBox_SetCurSel(h, m.value);
      break;
    case ECheckBox:
      CheckDlgButton(hDialog, i+IDBASE,
		     (m.value ? BST_CHECKED : BST_UNCHECKED));
      break;
    default:
      break;
    }
  }
}

static String getEditText(HWND h)
{
  int n = Edit_GetTextLength(h);
  if (n == 0)
    return String("");
  char buf[n+1];
  Edit_GetText(h, buf, n+1);
  return String(buf);
}

void PDialog::retrieveValues()
{
  for (int i = 0; i < int(iElements.size()); ++i) {
    SElement &m = iElements[i];
    HWND h = GetDlgItem(hDialog, i+IDBASE);
    switch (m.type) {
    case ETextEdit:
    case EInput:
      m.text = getEditText(h);
      break;
    case EList:
      m.value = ListBox_GetCurSel(h);
      if (m.value == LB_ERR)
	m.value = 0;
      break;
    case ECombo:
      m.value = ComboBox_GetCurSel(h);
      if (m.value == CB_ERR)
	m.value = 0;
      break;
    case ECheckBox:
      m.value = (IsDlgButtonChecked(hDialog, i+IDBASE) == BST_CHECKED);
      break;
    case ELabel:
    default:
      break; // nothing to do
    }
  }
}

void PDialog::buildFlags(std::vector<short> &t, DWORD flags)
{
  union {
    DWORD dw;
    short s[2];
  } a;
  a.dw = flags;
  t.push_back(a.s[0]);
  t.push_back(a.s[1]);
  t.push_back(0);
  t.push_back(0);
}

void PDialog::buildDimensions(std::vector<short> &t, SElement &m, int id)
{
  int x = BORDER;
  int y = BORDER;
  int w = 0;
  int h = 0;
  if (m.row < 0) {
    y = BORDER;
    for (int i = 0; i < iNoRows; ++i)
      y += iRowHeight[i] + PAD;
    w = m.minWidth;
    h = BUTTONHEIGHT;
    x = iButtonX - w;
    iButtonX -= w + PAD;
  } else {
    for (int i = 0; i < m.col; ++i)
      x += iColWidth[i] + PAD;
    for (int i = 0; i < m.row; ++i)
      y += iRowHeight[i] + PAD;
    w = iColWidth[m.col];
    for (int i = m.col + 1; i < m.col + m.colspan; ++i)
      w += iColWidth[i] + PAD;
    h = iRowHeight[m.row];
    for (int i = m.row + 1; i < m.row + m.rowspan; ++i)
      h += iRowHeight[i] + PAD;
  }
  t.push_back(x);
  t.push_back(y);
  t.push_back(w);
  t.push_back(h);
  t.push_back(id); // control id
}

void PDialog::buildString(std::vector<short> &t, const char *s)
{
  int rw = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
  wchar_t wide[rw];
  MultiByteToWideChar(CP_UTF8, 0, s, -1, wide, rw);
  for (int i = 0; i < rw; ++i)
    t.push_back(wide[i]);
}

void PDialog::buildControl(std::vector<short> &t, short what,
			  const char *s)
{
  t.push_back(0xffff);
  t.push_back(what);
  if (s)
    buildString(t, s);
  else
    t.push_back(0);      // text
  t.push_back(0);      // creation data
}

BOOL PDialog::dlgCommand(WPARAM wParam, LPARAM lParam)
{
  int id = LOWORD(wParam);
  if (id == IDOK) {
    retrieveValues();
    EndDialog(hDialog, TRUE);
    return TRUE;
  }
  if (id == IDCANCEL && !iIgnoreEscape) {
    retrieveValues();
    EndDialog(hDialog, FALSE);
    return TRUE;
  }
  if (id < IDBASE || id >= IDBASE + int(iElements.size()))
    return FALSE;
  SElement &m = iElements[id - IDBASE];
  if (m.flags & EAccept) {
    retrieveValues();
    EndDialog(hDialog, TRUE);
    return TRUE;
  } else if (m.flags & EReject) {
    retrieveValues();
    EndDialog(hDialog, FALSE);
    return TRUE;
  } else if (m.lua_method != LUA_NOREF)
    callLua(m.lua_method);
  return FALSE;
}

static WNDPROC wpOrigProc;
static LRESULT subclassProc(HWND hwnd, UINT message,
			    WPARAM wParam, LPARAM lParam)
{
  if (message == WM_KEYDOWN) {
    if (wParam == VK_RETURN && GetKeyState(VK_CONTROL)) {
      SendMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
      return 0;
    }
  }
  return CallWindowProc(wpOrigProc, hwnd, message, wParam, lParam);
}


BOOL CALLBACK PDialog::dialogProc(HWND hwnd, UINT message,
				  WPARAM wParam, LPARAM lParam)
{
  PDialog *d = (PDialog *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

  switch (message) {
  case WM_INITDIALOG:
    d = (PDialog *) lParam;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) d);
    d->hDialog = hwnd;
    d->initDialog();
    // subclass all Edit controls
    for (int i = 0; i < int(d->iElements.size()); ++i) {
      if (d->iElements[i].type == ETextEdit)
	wpOrigProc =
	  (WNDPROC) SetWindowLong(GetDlgItem(hwnd, i + IDBASE),
				  GWL_WNDPROC, (LONG) subclassProc);
    }
    return TRUE;

  case WM_COMMAND:
    if (d)
      return d->dlgCommand(wParam, lParam);
    else
      return FALSE;
  case WM_DESTROY:
    // Remove the subclasses from text edits
    for (int i = 0; i < int(d->iElements.size()); ++i) {
      if (d->iElements[i].type == ETextEdit)
	SetWindowLong(GetDlgItem(hwnd, i + IDBASE),
		      GWL_WNDPROC, (LONG) wpOrigProc);
    }
    return FALSE;
  default:
    return FALSE;
  }
}

bool PDialog::buildAndRun(int w, int h)
{
  computeDimensions(w, h);

  RECT rect;
  GetWindowRect(iParent, &rect);
  int pw = (rect.right - rect.left) * 4 / iBaseX;
  int ph = (rect.bottom - rect.top) * 8 / iBaseY;

  std::vector<short> t;

  // Dialog flags
  buildFlags(t, WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION);
  t.push_back(iElements.size());
  t.push_back((pw - w)/2);  // offset of popup-window from parent window
  t.push_back((ph - h)/2);
  t.push_back(w);
  t.push_back(h);
  // menu
  t.push_back(0);
  // class
  t.push_back(0);
  // title
  buildString(t, iCaption.z());
  // add elements
  for (int i = 0; i < int(iElements.size()); ++i) {
    if (t.size() % 2 != 0)
      t.push_back(0);
    SElement &m = iElements[i];
    int id = i + IDBASE;
    DWORD flags = WS_CHILD|WS_VISIBLE;
    switch (m.type) {
    case EButton:
      flags |= BS_TEXT|WS_TABSTOP;
      if (m.flags & EAccept)
	flags |= BS_DEFPUSHBUTTON;
      else
	flags |= BS_PUSHBUTTON;
      buildFlags(t, flags);
      buildDimensions(t, m, id);
      buildControl(t, 0x0080, m.text.z());  // button
      break;
    case ECheckBox:
      buildFlags(t, flags|BS_AUTOCHECKBOX|BS_TEXT|WS_TABSTOP);
      buildDimensions(t, m, id);
      buildControl(t, 0x0080, m.text.z());  // button
      break;
    case ELabel:
      buildFlags(t, flags|SS_LEFT);
      buildDimensions(t, m, id);
      buildControl(t, 0x0082, m.text.z());  // static text
      break;
    case EInput:
      buildFlags(t, flags|ES_LEFT|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL);
      buildDimensions(t, m, id);
      buildControl(t, 0x0081);  // edit
      break;
    case ETextEdit:
      flags |= ES_LEFT|WS_TABSTOP|WS_BORDER;
      flags |= ES_MULTILINE|ES_WANTRETURN|WS_VSCROLL;
      if (m.flags & EReadOnly)
	flags |= ES_READONLY;
      buildFlags(t, flags);
      buildDimensions(t, m, id);
      buildControl(t, 0x0081);  // edit
      break;
    case EList:
      buildFlags(t, flags|WS_TABSTOP|WS_VSCROLL|WS_BORDER);
      buildDimensions(t, m, id);
      buildControl(t, 0x0083);  // list box
      break;
    case ECombo:
      buildFlags(t, flags|CBS_DROPDOWNLIST|WS_TABSTOP);
      buildDimensions(t, m, id);
      buildControl(t, 0x0085);  // combo box
      break;
    default:
      break;
    }
  }
  int res =
    DialogBoxIndirectParam((HINSTANCE) GetWindowLong(iParent, GWL_HINSTANCE),
			   (LPCDLGTEMPLATE) &t[0],
			   iParent, dialogProc, (LPARAM) this);
  // retrieveValues() has been called before EndDialog!
  hDialog = NULL; // already destroyed by Windows
  return (res > 0);
}

void PDialog::computeDimensions(int &w, int &h)
{
  int minWidth[iNoCols];
  int minHeight[iNoRows];

  for (int i = 0; i < iNoCols; ++i)
    minWidth[i] = 0;
  for (int i = 0; i < iNoRows; ++i)
    minHeight[i] = 0;
  int buttonWidth = -PAD;

  for (int i = 0; i < int(iElements.size()); ++i) {
    SElement &m = iElements[i];
    if (m.row < 0) {  // button row
      buttonWidth += m.minWidth + PAD;
    } else {
      int wd = m.minWidth / m.colspan;
      for (int j = m.col; j < m.col + m.colspan; ++j) {
	if (wd > minWidth[j])
	  minWidth[j] = wd;
      }
      int ht = m.minHeight / m.rowspan;
      for (int j = m.row; j < m.row + m.rowspan; ++j) {
	if (ht > minHeight[j])
	  minHeight[j] = ht;
      }
    }
  }

  // Convert w and h to dialog units:
  w = w * 4 / iBaseX;
  h = h * 8 / iBaseY;

  while (int(iColStretch.size()) < iNoCols)
    iColStretch.push_back(0);
  while (int(iRowStretch.size()) < iNoRows)
    iRowStretch.push_back(0);

  int totalW = BORDER + BORDER - PAD;
  int totalWStretch = 0;
  for (int i = 0; i < iNoCols; ++i) {
    totalW += minWidth[i] + PAD;
    totalWStretch += iColStretch[i];
  }
  int totalH = BORDER + BORDER + BUTTONHEIGHT;
  int totalHStretch = 0;
  for (int i = 0; i < iNoRows; ++i) {
    totalH += minHeight[i] + PAD;
    totalHStretch += iRowStretch[i];
  }

  if (totalW > w)
    w = totalW;
  if (totalH > h)
    h = totalH;
  if (buttonWidth + BORDER + BORDER > w)
    w = buttonWidth + BORDER + BORDER;

  iButtonX = w - (w - buttonWidth) / 2;

  int spareW = w - totalW;
  int spareH = h - totalH;
  iColWidth.resize(iNoCols);
  iRowHeight.resize(iNoRows);

  if (totalWStretch == 0) {
    // spread spareW equally
    int extra = spareW / iNoCols;
    for (int i = 0; i < iNoCols; ++i)
      iColWidth[i] = minWidth[i] + extra;
  } else {
    for (int i = 0; i < iNoCols; ++i) {
      int extra = spareW * iColStretch[i] / totalWStretch;
      iColWidth[i] = minWidth[i] + extra;
    }
  }

  if (totalHStretch == 0) {
    // spread spareH equally
    int extra = spareH / iNoRows;
    for (int i = 0; i < iNoRows; ++i)
      iRowHeight[i] = minHeight[i] + extra;
  } else {
    for (int i = 0; i < iNoRows; ++i) {
      int extra = spareH * iRowStretch[i] / totalHStretch;
      iRowHeight[i] = minHeight[i] + extra;
    }
  }
}

void PDialog::enableItem(int idx, bool value)
{
  EnableWindow(GetDlgItem(hDialog, idx+IDBASE), value);
}

// --------------------------------------------------------------------

static int dialog_constructor(lua_State *L)
{
  HWND parent = check_winid(L, 1);
  const char *s = luaL_checkstring(L, 2);

  Dialog **dlg = (Dialog **) lua_newuserdata(L, sizeof(Dialog *));
  *dlg = 0;
  luaL_getmetatable(L, "Ipe.dialog");
  lua_setmetatable(L, -2);
  *dlg = new PDialog(L, parent, s);
  return 1;
}

// --------------------------------------------------------------------

class PMenu : public Menu {
public:
  PMenu(HWND parent);
  virtual ~PMenu();
  virtual int add(lua_State *L);
  virtual int execute(lua_State *L);
private:
  struct Item {
    String name;
    String itemName;
    int itemIndex;
  };
  std::vector<Item> items;
  int currentId;
  HMENU hMenu;
  HWND hwnd;
  std::vector<HBITMAP> bitmaps;
};

PMenu::PMenu(HWND parent)
{
  hMenu = CreatePopupMenu();
  hwnd = parent;
}

PMenu::~PMenu()
{
  if (hMenu)
    DestroyMenu(hMenu);
  hMenu = 0;
  for (int i = 0; i < int(bitmaps.size()); ++i)
    DeleteObject(bitmaps[i]);
}

int PMenu::execute(lua_State *L)
{
  int vx = luaL_checkint(L, 2);
  int vy = luaL_checkint(L, 3);
  int result = TrackPopupMenu(hMenu,
			      TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON,
			      vx, vy, 0, hwnd, NULL);
  if (1 <= result && result <= int(items.size())) {
    result -= 1;
    lua_pushstring(L, items[result].name.z());
    lua_pushinteger(L, items[result].itemIndex);
    if (items[result].itemName.z())
      lua_pushstring(L, items[result].itemName.z());
    else
      lua_pushstring(L, "");
    return 3;
  }
  return 0;
}

static HBITMAP colorIcon(double red, double green, double blue)
{
  int r = int(red * 255.0);
  int g = int(green * 255.0);
  int b = int(blue * 255.0);
  COLORREF rgb = RGB(r, g, b);
  int cx = GetSystemMetrics(SM_CXMENUCHECK);
  int cy = GetSystemMetrics(SM_CYMENUCHECK);
  HDC hdc = GetDC(NULL);
  HDC memDC = CreateCompatibleDC(hdc);
  HBITMAP bm = CreateCompatibleBitmap(hdc, cx, cy);
  SelectObject(memDC, bm);
  for (int y = 0; y < cy; ++y) {
    for (int x = 0; x < cx; ++x) {
      SetPixel(memDC, x, y, rgb);
    }
  }
  DeleteDC(memDC);
  ReleaseDC(NULL, hdc);
  return bm;
}

int PMenu::add(lua_State *L)
{
  const char *name = luaL_checkstring(L, 2);
  const char *title = luaL_checkstring(L, 3);
  if (lua_gettop(L) == 3) {
    AppendMenu(hMenu, MF_STRING, items.size() + 1, title);
    Item item;
    item.name = String(name);
    item.itemIndex = 0;
    items.push_back(item);
  } else {
    luaL_argcheck(L, lua_istable(L, 4), 4, "argument is not a table");
    bool hasmap = !lua_isnoneornil(L, 5) && lua_isfunction(L, 5);
    bool hastable = !hasmap && !lua_isnoneornil(L, 5);
    bool hascolor = !lua_isnoneornil(L, 6) && lua_isfunction(L, 6);
    bool hascheck = !hascolor && !lua_isnoneornil(L, 6);
    if (hastable)
      luaL_argcheck(L, lua_istable(L, 5), 5,
		    "argument is not a function or table");
    const char *current = 0;
    if (hascheck) {
      luaL_argcheck(L, lua_isstring(L, 6), 6,
		    "argument is not a function or string");
      current = luaL_checkstring(L, 6);
    }
    int no = lua_objlen(L, 4);
    HMENU sm = CreatePopupMenu();
    for (int i = 1; i <= no; ++i) {
      lua_rawgeti(L, 4, i);
      luaL_argcheck(L, lua_isstring(L, -1), 4, "items must be strings");
      int id = items.size() + 1;
      const char *item = lua_tostring(L, -1);
      if (hastable) {
	lua_rawgeti(L, 5, i);
	luaL_argcheck(L, lua_isstring(L, -1), 5, "labels must be strings");
      } else if (hasmap) {
	lua_pushvalue(L, 5);   // function
	lua_pushnumber(L, i);  // index
 	lua_pushvalue(L, -3);  // name
	lua_call(L, 2, 1);     // function returns label
	luaL_argcheck(L, lua_isstring(L, -1), 5,
		      "function does not return string");
      } else
	lua_pushvalue(L, -1);

      const char *text = lua_tostring(L, -1);
      AppendMenu(sm, MF_STRING, id, text);
      Item mitem;
      mitem.name = String(name);
      mitem.itemName = String(item);
      mitem.itemIndex = i;
      items.push_back(mitem);
      if (hascheck && !strcmp(item, current))
	CheckMenuItem(sm, id, MF_CHECKED);

      if (hascolor) {
	lua_pushvalue(L, 6);   // function
	lua_pushnumber(L, i);  // index
 	lua_pushvalue(L, -4);  // name
	lua_call(L, 2, 3);     // function returns red, green, blue
	double red = luaL_checknumber(L, -3);
	double green = luaL_checknumber(L, -2);
	double blue = luaL_checknumber(L, -1);
	lua_pop(L, 3);         // pop result
	HBITMAP bits = colorIcon(red, green, blue);
	bitmaps.push_back(bits);
	SetMenuItemBitmaps(sm, id, MF_BYCOMMAND, bits, bits);
      }
      lua_pop(L, 2); // item, text
    }
    AppendMenu(hMenu, MF_STRING | MF_POPUP, UINT(sm), title);
  }
  return 0;
}

// --------------------------------------------------------------------

static int menu_constructor(lua_State *L)
{
  HWND hwnd = check_winid(L, 1);
  Menu **m = (Menu **) lua_newuserdata(L, sizeof(Menu *));
  *m = 0;
  luaL_getmetatable(L, "Ipe.menu");
  lua_setmetatable(L, -2);
  *m = new PMenu(hwnd);
  return 1;
}

// --------------------------------------------------------------------

class PTimer : public Timer {
public:
  PTimer(lua_State *L0, int lua_object, const char *method);
  virtual ~PTimer();

  virtual int setInterval(lua_State *L);
  virtual int active(lua_State *L);
  virtual int start(lua_State *L);
  virtual int stop(lua_State *L);

protected:
  void elapsed();
  static void CALLBACK TimerProc(HWND hwnd, UINT uMsg,
				 UINT_PTR idEvent, DWORD dwTime);
  static std::vector<PTimer *> all_timers;

protected:
  UINT_PTR iTimer;
  UINT iInterval;
};

std::vector<PTimer *> PTimer::all_timers;

void CALLBACK PTimer::TimerProc(HWND, UINT, UINT_PTR id, DWORD)
{
  for (int i = 0; i < int(all_timers.size()); ++i) {
    if (id == all_timers[i]->iTimer) {
      all_timers[i]->elapsed();
      return;
    }
  }
}

PTimer::PTimer(lua_State *L0, int lua_object, const char *method)
  : Timer(L0, lua_object, method)
{
  iTimer = 0;
  iInterval = 0;
  all_timers.push_back(this);
}

PTimer::~PTimer()
{
  if (iTimer)
    KillTimer(NULL, iTimer);
  // remove it from all_timers
  for (int i = 0; i < int(all_timers.size()); ++i) {
    if (all_timers[i] == this) {
      all_timers.erase(all_timers.begin() + i);
      return;
    }
  }
}

void PTimer::elapsed()
{
  callLua();
  if (iSingleShot) {
    KillTimer(NULL, iTimer);
    iTimer = 0;
  }
}

int PTimer::setInterval(lua_State *L)
{
  int t = luaL_checkint(L, 2);
  iInterval = t;
  if (iTimer)
    SetTimer(NULL, iTimer, iInterval, TimerProc);
  return 0;
}

int PTimer::active(lua_State *L)
{
  lua_pushboolean(L, (iTimer != 0));
  return 1;
}

int PTimer::start(lua_State *L)
{
  if (iTimer == 0)
    iTimer = SetTimer(NULL, 0, iInterval, TimerProc);
  return 0;
}

int PTimer::stop(lua_State *L)
{
  if (iTimer) {
    KillTimer(NULL, iTimer);
    iTimer = 0;
  }
  return 0;
}

// --------------------------------------------------------------------

static int timer_constructor(lua_State *L)
{
  luaL_argcheck(L, lua_istable(L, 1), 1, "argument is not a table");
  const char *method = luaL_checkstring(L, 2);

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
  *t = new PTimer(L, lua_object, method);
  return 1;
}

// --------------------------------------------------------------------

static COLORREF custom[16] = {
  0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff,
  0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff,
  0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff,
  0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff };

static int ipeui_getColor(lua_State *L)
{
  HWND hwnd = check_winid(L, 1);
  // const char *title = luaL_checkstring(L, 2);
  double r = luaL_checknumber(L, 3);
  double g = luaL_checknumber(L, 4);
  double b = luaL_checknumber(L, 5);

  CHOOSECOLOR cc;
  ZeroMemory(&cc, sizeof(cc));
  cc.lStructSize = sizeof(cc);
  cc.hwndOwner = hwnd;
  cc.Flags = CC_FULLOPEN | CC_RGBINIT;
  cc.rgbResult = RGB(int(r * 255), int(g * 255), int(b * 255));
  cc.lpCustColors = custom;

  if (ChooseColor(&cc)) {
    lua_pushnumber(L, GetRValue(cc.rgbResult) / 255.0);
    lua_pushnumber(L, GetGValue(cc.rgbResult) / 255.0);
    lua_pushnumber(L, GetBValue(cc.rgbResult) / 255.0);
    return 3;
  } else
    return 0;
}

// --------------------------------------------------------------------


static int ipeui_fileDialog(lua_State *L)
{
  static const char * const typenames[] = { "open", "save", 0 };
  HWND hwnd = check_winid(L, 1);
  int type = luaL_checkoption(L, 2, 0, typenames);
  const char *caption = luaL_checkstring(L, 3);
  // const char *filter = luaL_checkstring(L, 4);
  const char *dir = 0;
  if (!lua_isnoneornil(L, 5))
    dir = luaL_checkstring(L, 5);
  // const char *selected = 0;
  // if (!lua_isnoneornil(L, 6))
  //   selected = luaL_checkstring(L, 6);

  OPENFILENAME ofn;
  char szFileName[MAX_PATH] = "";

  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd;

  // TODO!
  ofn.lpstrFilter = "Ipe Files\0*.ipe;*.pdf;*.eps\0All Files\0*.*\0";
  ofn.nFilterIndex = 1;

  ofn.lpstrFile = szFileName;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrDefExt = "ipe";
  ofn.lpstrInitialDir = dir;
  ofn.lpstrTitle = caption;

  if (type == 0) {
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileName(&ofn)) {
      lua_pushstring(L, ofn.lpstrFile);
      lua_pushstring(L, "ipe");
      return 2;
    }
  } else {
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (GetSaveFileName(&ofn)) {
      lua_pushstring(L, ofn.lpstrFile);
      lua_pushstring(L, "ipe");
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

  HWND hwnd = check_winid(L, 1);
  int type = luaL_checkoption(L, 2, "none", options);
  const char *text = luaL_checkstring(L, 3);
  const char *details = 0;
  if (!lua_isnoneornil(L, 4))
    details = luaL_checkstring(L, 4);
  int buttons = 0;
  if (lua_isnumber(L, 5))
    buttons = luaL_checkint(L, 5);
  else if (!lua_isnoneornil(L, 5))
    buttons = luaL_checkoption(L, 5, 0, buttontype);

  UINT uType = MB_APPLMODAL;
  switch (type) {
  case 0:
  default:
    break;
  case 1:
    uType |= MB_ICONWARNING;
    break;
  case 2:
    uType |= MB_ICONINFORMATION;
    break;
  case 3:
    uType |= MB_ICONQUESTION;
    break;
  case 4:
    uType |= MB_ICONERROR;
    break;
  }

  switch (buttons) {
  case 0:
  default:
    uType |= MB_OK;
    break;
  case 1:
    uType |= MB_OKCANCEL;
    break;
  case 2:
    uType |= MB_YESNOCANCEL;
    break;
  case 3:
    uType |= MB_OKCANCEL;
    // TODO should be Discard Cancel
    break;
  case 4:
    uType |= MB_YESNOCANCEL;
    // TODO should be Save Discard Cancel
    break;
  }

  int ret = -1;
  if (details) {
    char buf[strlen(text) + strlen(details) + 8];
    sprintf(buf, "%s\n\n%s", text, details);
    ret = MessageBox(hwnd, buf, "Ipe", uType);
  } else
    ret = MessageBox(hwnd, text, "Ipe", uType);

  switch (ret) {
  case IDOK:
  case IDYES:
    lua_pushnumber(L, 1);
    break;
  case IDNO:
  case IDIGNORE:
    lua_pushnumber(L, 0);
    break;
  case IDCANCEL:
  default:
    lua_pushnumber(L, -1);
    break;
  }
  return 1;
}

// --------------------------------------------------------------------

static int ipeui_wait(lua_State *L)
{
  luaL_error(L, "'WaitDialog' is not implemented on Windows.");
  return 0;
}

static int ipeui_closeAllWindows(lua_State *L)
{
  luaL_error(L, "'closeAllWindows' is not implemented on Windows.");
  return 0;
}

// --------------------------------------------------------------------

static int ipeui_setClipboard(lua_State *L)
{
  const char *data = luaL_checkstring(L, 1);
  int l = strlen(data);
  GLOBALHANDLE hGlobal = GlobalAlloc(GHND | GMEM_SHARE, l + 1);
  char *p = (char *) GlobalLock(hGlobal);
  for (int i = 0; i < l; ++i)
    *p++ = *data++;
  GlobalUnlock(hGlobal);
  OpenClipboard(NULL);  // hwnd!
  EmptyClipboard();
  SetClipboardData(CF_TEXT, hGlobal);
  CloseClipboard();
  return 0;
}

static int ipeui_clipboard(lua_State *L)
{
  OpenClipboard(NULL);  // hwnd!
  GLOBALHANDLE hGlobal = GetClipboardData(CF_TEXT);
  if (hGlobal == NULL)
    return 0;
  char *q = (char *) GlobalLock(hGlobal);
  lua_pushstring(L, q);  // this makes a copy!
  GlobalUnlock(hGlobal);
  CloseClipboard();
  return 1;
}

static int ipeui_local8bit(lua_State *L)
{
  const char *in = luaL_checkstring(L, 1);
  int rw = MultiByteToWideChar(CP_UTF8, 0, in, -1, NULL, 0);
  wchar_t wide[rw];
  MultiByteToWideChar(CP_UTF8, 0, in, -1, wide, rw);
  int rm = WideCharToMultiByte(CP_ACP, 0, wide, -1, NULL, 0, NULL, NULL);
  char multi[rm];
  WideCharToMultiByte(CP_ACP, 0, wide, -1, multi, rm, NULL, NULL);
  lua_pushstring(L, multi);
  return 1;
}

static int ipeui_currentDateTime(lua_State *L)
{
  SYSTEMTIME st;
  GetLocalTime(&st);
  char buf[16];
  sprintf(buf, "%04d%02d%02d%02d%02d%02d",
	  st.wYear, st.wMonth, st.wDay,
	  st.wHour, st.wMinute, st.wSecond);
  lua_pushstring(L, buf);
  return 1;
}

static int ipeui_startBrowser(lua_State *L)
{
  const char *url = luaL_checkstring(L, 1);
  ShellExecuteA(0, "open", url, 0, 0, 0);
  return 0;
}

// --------------------------------------------------------------------

static const struct luaL_Reg ipeui_functions[] = {
  { "Dialog", dialog_constructor },
  { "Menu", menu_constructor },
  { "Timer", timer_constructor },
  { "getColor", ipeui_getColor },
  { "fileDialog", ipeui_fileDialog },
  { "messageBox", ipeui_messageBox },
  { "WaitDialog", ipeui_wait },
  { "closeAllWindows", ipeui_closeAllWindows },
  { "currentDateTime", ipeui_currentDateTime },
  { "setClipboard", ipeui_setClipboard },
  { "clipboard", ipeui_clipboard },
  { "local8bit", ipeui_local8bit },
  { "startBrowser", ipeui_startBrowser },
  { 0, 0},
};

// --------------------------------------------------------------------

int luaopen_ipeui(lua_State *L)
{
  luaL_register(L, "ipeui", ipeui_functions);
  luaopen_ipeui_common(L);
  return 0;
}

// --------------------------------------------------------------------
