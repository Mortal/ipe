// --------------------------------------------------------------------
// AppUi for Win32
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2012  Otfried Cheong

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

#include "appui_win.h"
#include "ipecanvas_win.h"
#include "controls_win.h"

#include "ipelua.h"

// for version info only
#include "ipefonts.h"
#include <zlib.h>

#include <cstdio>
#include <cstdlib>

#include <cairo.h>
#include <windowsx.h>

#define IDI_MYICON 1
#define ID_STATUS_TIMER 1
#define IDC_STATUSBAR 7000
#define IDC_BOOKMARK 7100
#define IDC_NOTES 7200
#define IDBASE 8000
#define TEXT_STYLE_BASE 8300
#define SELECT_LAYER_BASE 8400
#define ID_SELECTOR_BASE 8500
#define ID_ABSOLUTE_BASE 8600
#define ID_PATHVIEW 8700

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

const int TBICONSIZE = 24;

static HINSTANCE win_hInstance = 0;
static int win_nCmdShow = 0;

const char AppUi::className[] = "ipeWindowClass";

// --------------------------------------------------------------------

static HBITMAP loadIcon(String action, int size)
{
  String dir = ipeIconDirectory();
  String fname = dir + action + ".png";
  if (!Platform::fileExists(fname))
    return NULL;

  cairo_surface_t *sf = cairo_image_surface_create_from_png(fname.z());
  assert(cairo_image_surface_get_format(sf) == CAIRO_FORMAT_ARGB32);
  unsigned char *p = cairo_image_surface_get_data(sf);
  int wd = cairo_image_surface_get_width(sf);
  int ht = cairo_image_surface_get_height(sf);
  int stride = cairo_image_surface_get_stride(sf);

  HDC hdc = GetDC(NULL);
  BITMAPINFO bmi;
  ZeroMemory(&bmi, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = size;
  bmi.bmiHeader.biHeight = size;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  void *pbits = 0;
  HBITMAP bm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS,
				&pbits, NULL, 0);
  // make it transparent
  unsigned char *q = (unsigned char *) pbits;
  for (unsigned char *r = q; r < q + 4 * size * size;)
    *r++ = 0x00;
  int x0 = wd < size ? (size - wd)/2 : 0;
  int y0 = ht < size ? (size - ht)/2 : 0;
  int xs = wd > size ? size : wd;
  int ys = ht > size ? size : ht;
  for (int y = 0; y < ys; ++y) {
    unsigned char *src = p + (ht - 1 - y) * stride;
    unsigned char *dst = q + (y0 + y) * (4 * size) + 4 * x0;
    int n = 4 * xs;
    while (n--)
      *dst++ = *src++;
  }
  cairo_surface_destroy(sf);
  ReleaseDC(NULL, hdc);
  return bm;
}

static int loadIcon(String action, HIMAGELIST il)
{
  HBITMAP bm = loadIcon(action, TBICONSIZE);
  if (bm == NULL)
    return -1;
  int r = ImageList_Add(il, bm, NULL);
  DeleteObject(bm);
  return r;
}

static HBITMAP loadButtonIcon(String action)
{
  String dir = ipeIconDirectory();
  String fname = dir + action + ".png";
  if (!Platform::fileExists(fname))
    return NULL;

  cairo_surface_t *sf = cairo_image_surface_create_from_png(fname.z());
  assert(cairo_image_surface_get_format(sf) == CAIRO_FORMAT_ARGB32);
  unsigned char *p = cairo_image_surface_get_data(sf);
  int wd = cairo_image_surface_get_width(sf);
  int ht = cairo_image_surface_get_height(sf);
  int stride = cairo_image_surface_get_stride(sf);

  HDC hdc = GetDC(NULL);
  BITMAPINFO bmi;
  ZeroMemory(&bmi, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = wd;
  bmi.bmiHeader.biHeight = ht;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 24;
  bmi.bmiHeader.biCompression = BI_RGB;
  void *pbits = 0;
  HBITMAP bm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS,
				&pbits, NULL, 0);

  DWORD rgb = GetSysColor(COLOR_BTNFACE);
  int r0 = GetRValue(rgb);
  int g0 = GetGValue(rgb);
  int b0 = GetBValue(rgb);

  unsigned char *q = (unsigned char *) pbits;
  int dstride = (3 * wd + 3) & (~3);

  for (int x = 0; x < wd; ++x) {
    for (int y = 0; y < ht; ++y) {
      unsigned char *src = p + (ht - 1 - y) * stride + 4 * x;
      unsigned char *dst = q + y * dstride + 3 * x;
      int r1 = *src++;
      int g1 = *src++;
      int b1 = *src++;
      int trans = *src++;
      *dst++ = r1 + int((0xff - trans) * r0 / 0xff);
      *dst++ = g1 + int((0xff - trans) * g0 / 0xff);
      *dst++ = b1 + int((0xff - trans) * b0 / 0xff);
    }
  }
  cairo_surface_destroy(sf);
  ReleaseDC(NULL, hdc);
  return bm;
}

static HBITMAP colorIcon(Color color)
{
  int r = int(color.iRed.internal() * 255 / 1000);
  int g = int(color.iGreen.internal() * 255 / 1000);
  int b = int(color.iBlue.internal() * 255 / 1000);
  COLORREF rgb = RGB(r, g, b);
  int cx = 22;
  int cy = 22;
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

// --------------------------------------------------------------------

void AppUi::addRootMenu(int id, const char *name)
{
  hRootMenu[id] = CreatePopupMenu();
  AppendMenu(hMenuBar, MF_STRING | MF_POPUP, UINT(hRootMenu[id]), name);
}

void AppUi::createAction(String name, String tooltip, bool canWhileDrawing)
{
  SAction s;
  s.name = String(name);
  s.icon = loadIcon(name, hIcons);
  s.tooltip = tooltip;
  s.alwaysOn = canWhileDrawing;
  iActions.push_back(s);
}

void AppUi::addItem(HMENU menu, const char *title, const char *name)
{
  if (title == 0) {
    AppendMenu(menu, MF_SEPARATOR, 0, 0);
  } else {
    bool canUseWhileDrawing = false;
    if (name[0] == '@') {
      canUseWhileDrawing = true;
      name = name + 1;
    }
    if (name[0] == '*')
      name += 1;  // in Win32 all menu items are checkable anyway

    // check for shortcut
    lua_getglobal(L, "shortcuts");
    lua_getfield(L, -1, name);
    String sc;
    if (lua_isstring(L, -1)) {
      sc = lua_tostring(L, -1);
    } else if (lua_istable(L, -1) && lua_objlen(L, -1) > 0) {
      lua_rawgeti(L, -1, 1);
      if (lua_isstring(L, -1))
	sc = lua_tostring(L, -1);
      lua_pop(L, 1); // pop string
    }
    lua_pop(L, 2); // shortcuts, shortcuts[name]

    String tooltip = title;
    if (!sc.isEmpty())
      tooltip = tooltip + " [" + sc + "]";
    createAction(name, tooltip, canUseWhileDrawing);

    if (sc.isEmpty())
      AppendMenu(menu, MF_STRING, iActions.size() - 1 + IDBASE, title);
    else {
      String t = title;
      t += "\t";
      t += sc;
      AppendMenu(menu, MF_STRING, iActions.size() - 1 + IDBASE, t.z());
    }
  }
}

void AppUi::addItem(int id, const char *title, const char *name)
{
  addItem(hRootMenu[id], title, name);
}

static HMENU submenu = 0;

void AppUi::startSubMenu(int id, const char *name)
{
  submenu = CreatePopupMenu();
  AppendMenu(hRootMenu[id], MF_STRING | MF_POPUP, UINT(submenu), name);
}

void AppUi::addSubItem(const char *title, const char *name)
{
  addItem(submenu, title, name);
}

MENUHANDLE AppUi::endSubMenu()
{
  return submenu;
}

void AppUi::addTButton(HWND tb, const char *name, int flags)
{
  TBBUTTON tbb;
  ZeroMemory(&tbb, sizeof(tbb));
  if (name == 0) {
    tbb.iBitmap = 1;
    tbb.fsStyle = TBSTYLE_SEP;
  } else {
    int i = findAction(name);
    tbb.iBitmap = iconId(name);
    tbb.fsState = TBSTATE_ENABLED;
    tbb.fsStyle = TBSTYLE_BUTTON | flags;
    tbb.idCommand = i + IDBASE;
    tbb.iString = INT_PTR(iActions[i].tooltip.z());
  }
  SendMessage(tb, TB_ADDBUTTONS, 1, (LPARAM) &tbb);
}

void AppUi::setTooltip(HWND h, String tip, bool isComboBoxEx)
{
  if (isComboBoxEx)
    h = (HWND) SendMessage(h, CBEM_GETCOMBOCONTROL, 0, 0);
  TOOLINFO toolInfo = { 0 };
  toolInfo.cbSize = sizeof(toolInfo);
  toolInfo.hwnd = hwnd;
  toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
  toolInfo.uId = (UINT_PTR) h;
  toolInfo.lpszText = (char *) tip.z();
  SendMessage(hTip, TTM_ADDTOOL, 0, (LPARAM) &toolInfo);
}

HWND AppUi::createButton(HINSTANCE hInst, int id, int flags)
{
  return CreateWindowEx(0, "button", NULL, WS_CHILD | WS_VISIBLE | flags,
			0, 0, 0, 0, hwnd, (HMENU) id, hInst, NULL);
}

// --------------------------------------------------------------------

AppUi::AppUi(lua_State *L0, int model)
  : AppUiBase(L0, model)
{
  // windows procedure may receive WM_SIZE message before canvas is created
  iCanvas = 0;
  HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, "Ipe",
			     WS_OVERLAPPEDWINDOW,
			     CW_USEDEFAULT, CW_USEDEFAULT,
			     CW_USEDEFAULT, CW_USEDEFAULT,
			     NULL, NULL, win_hInstance, this);

  if (hwnd == NULL) {
    MessageBox(NULL, "AppUi window creation failed!", "Error!",
	       MB_ICONEXCLAMATION | MB_OK);
    exit(9);
  }
  assert(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

// called during Window creation
void AppUi::initUi()
{
  hIcons = ImageList_Create(TBICONSIZE, TBICONSIZE, ILC_COLOR32, 4, 4);
  HINSTANCE hInst = (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE);

  hMenuBar = CreateMenu();
  buildMenus();
  SetMenu(hwnd, hMenuBar);

  hStatusBar = CreateWindowEx(0, STATUSCLASSNAME, NULL,
			      WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
			      0, 0, 0, 0,
			      hwnd, (HMENU) IDC_STATUSBAR, hInst, NULL);

  hTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			hwnd, NULL, hInst, NULL);
  SetWindowPos(hTip, HWND_TOPMOST, 0, 0, 0, 0,
	       SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

  // if used in Rebar, need CCS_NOPARENTALIGN | CCS_NORESIZE,
  hToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
			    WS_CHILD | WS_VISIBLE |
			    TBSTYLE_WRAPABLE | TBSTYLE_TOOLTIPS,
			    0, 0, 0, 0, hwnd, NULL, hInst, NULL);

  SendMessage(hToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
  SendMessage(hToolBar, TB_SETIMAGELIST, 0, (LPARAM) hIcons);
  // SendMessage(hToolBar, TB_SETBUTTONSIZE, 0, MAKELPARAM(48, 48));
  SendMessage(hToolBar, TB_SETMAXTEXTROWS, 0, (LPARAM) 0);

  HWND hSnapTools = hToolBar;

  addTButton(hSnapTools, "snapvtx", BTNS_CHECK);
  addTButton(hSnapTools, "snapbd", BTNS_CHECK);
  addTButton(hSnapTools, "snapint", BTNS_CHECK);
  addTButton(hSnapTools, "snapgrid", BTNS_CHECK);

  addTButton(hSnapTools, "snapangle", BTNS_CHECK);
  addTButton(hSnapTools, "snapauto", BTNS_CHECK);

  addTButton(hToolBar);  // separator

  HWND hEditTools = hToolBar;

  addTButton(hEditTools, "copy");
  addTButton(hEditTools, "cut");
  addTButton(hEditTools, "paste");
  addTButton(hEditTools, "delete");
  addTButton(hEditTools, "undo");
  addTButton(hEditTools, "redo");
  addTButton(hEditTools, "zoom_in");
  addTButton(hEditTools, "zoom_out");
  addTButton(hEditTools, "fit_objects");
  addTButton(hEditTools, "fit_page");
  addTButton(hEditTools, "fit_width");
  addTButton(hEditTools, "keyboard");
  createAction("shift_key", String());
  addTButton(hEditTools, "shift_key", BTNS_CHECK);

  addTButton(hToolBar); // separator

  HWND hObjectTools = hToolBar;

  addTButton(hObjectTools, "mode_select", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_translate", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_rotate", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_stretch", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_pan", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_label", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_math", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_paragraph", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_marks", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_rectangles", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_lines", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_polygons", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_splines", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_splinegons", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_arc1", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_arc2", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_arc3", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_circle1", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_circle2", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_circle3", BTNS_CHECKGROUP);
  addTButton(hObjectTools, "mode_ink", BTNS_CHECKGROUP);

  SendMessage(hToolBar, TB_AUTOSIZE, 0, 0);

#if 0
  // maybe later I'll put the toolbars in a Rebar
  // Create the rebar.
  hRebar =
    CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
		   WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
		   WS_CLIPCHILDREN | RBS_VARHEIGHT |
		   CCS_NODIVIDER | RBS_BANDBORDERS,
		   0, 0, 0, 0,
		   hwnd, NULL,
		   hInst, NULL);

  REBARINFO ri = {0};
  ri.cbSize = sizeof(REBARINFO);
  SendMessage(hRebar, RB_SETBARINFO, 0, (LPARAM) &ri);

  // Initialize band info for all bands
  REBARBANDINFO rbBand = { sizeof(REBARBANDINFO) };
  rbBand.fMask  =
    RBBIM_STYLE         // fStyle is valid.
    | RBBIM_TEXT        // lpText is valid.
    | RBBIM_CHILD       // hwndChild is valid.
    | RBBIM_CHILDSIZE   // child size members are valid.
    | RBBIM_SIZE;       // cx is valid
  rbBand.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;

  // Get the height of the toolbars.
  DWORD dwBtnSize =
    (DWORD) SendMessage(hEditTools, TB_GETBUTTONSIZE, 0, 0);

  rbBand.lpText = (char *) "Edit";
  rbBand.hwndChild = hEditTools;
  rbBand.cyChild = LOWORD(dwBtnSize);
  rbBand.cxMinChild = 14 * HIWORD(dwBtnSize);
  rbBand.cyMinChild = 40;
  // The default width is the width of the buttons.
  rbBand.cx = 0;
  // Add the band.
  SendMessage(hRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM) &rbBand);

  rbBand.lpText = (char *) "Snap";
  rbBand.hwndChild = hSnapTools;
  rbBand.cxMinChild = 6 * HIWORD(dwBtnSize);
  SendMessage(hRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM) &rbBand);
#endif

  hProperties = CreateWindowEx(0, "button", "Properties",
			       WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
			       0, 100, 200, 280, hwnd, NULL, hInst, NULL);

  for (int i = 0; i <= EUiSymbolSize; ++i) {
    if (i != EUiMarkShape)
      hButton[i] = createButton(hInst, ID_ABSOLUTE_BASE + i);
  }
  for (int i = 0; i < EUiView; ++i)
    hSelector[i] = CreateWindowEx(0, WC_COMBOBOXEX, NULL,
				  WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
				  0, 0, 0, 0, hwnd,
				  (HMENU) (ID_SELECTOR_BASE + i), hInst, NULL);
  SendMessage(hButton[EUiPen], BM_SETIMAGE, IMAGE_BITMAP,
	      (LPARAM) loadButtonIcon("pen"));
  SendMessage(hButton[EUiTextSize], BM_SETIMAGE, IMAGE_BITMAP,
	      (LPARAM) loadButtonIcon("mode_label"));
  SendMessage(hButton[EUiSymbolSize], BM_SETIMAGE, IMAGE_BITMAP,
	      (LPARAM) loadButtonIcon("mode_marks"));
  SendMessage(hButton[EUiStroke], BM_SETIMAGE, IMAGE_BITMAP,
	      (LPARAM) colorIcon(Color(1000, 0, 0)));
  SendMessage(hButton[EUiFill], BM_SETIMAGE, IMAGE_BITMAP,
	      (LPARAM) colorIcon(Color(1000, 1000, 0)));

  hButton[EUiMarkShape] =
    CreateWindowEx(0, "static", NULL,
		   WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
		   0, 0, 0, 0, hwnd, NULL, hInst, NULL);
  SendMessage(hButton[EUiMarkShape], STM_SETIMAGE, IMAGE_BITMAP,
	      (LPARAM) loadButtonIcon("mode_marks"));

  hViewNumber = createButton(hInst, ID_ABSOLUTE_BASE + EUiView,
			     BS_TEXT|BS_PUSHBUTTON);
  hPageNumber = createButton(hInst, ID_ABSOLUTE_BASE + EUiPage,
			     BS_TEXT|BS_PUSHBUTTON);
  hViewMarked = createButton(hInst, ID_ABSOLUTE_BASE + EUiViewMarked,
			     BS_AUTOCHECKBOX);
  hPageMarked = createButton(hInst, ID_ABSOLUTE_BASE + EUiPageMarked,
			     BS_AUTOCHECKBOX);

  hNotes = CreateWindowEx(0, "edit", NULL,
			  WS_CHILD | WS_VSCROLL | WS_BORDER |
			  ES_READONLY | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
			  0, 0, 0, 0, hwnd,
			  (HMENU) IDC_NOTES, hInst, NULL);

  hBookmarks = CreateWindowEx(0, "listbox", NULL,
			      WS_CHILD | WS_VSCROLL | WS_BORDER |
			      LBS_HASSTRINGS | LBS_NOTIFY | LBS_NOSEL,
                              0, 0, 0, 0,
			      hwnd, (HMENU) IDC_BOOKMARK, hInst, NULL);

  iPathView = new PathView(hwnd, ID_PATHVIEW);

  Canvas *canvas = new Canvas(hwnd);
  hwndCanvas = canvas->windowId();
  iCanvas = canvas;
  iCanvas->setObserver(this);

  // Tooltips
  setTooltip(hButton[EUiStroke], "Absolute stroke color");
  setTooltip(hButton[EUiFill], "Absolute fill color");
  setTooltip(hButton[EUiPen], "Absolute pen width");
  setTooltip(hButton[EUiTextSize], "Absolute text size");
  setTooltip(hButton[EUiSymbolSize], "Absolute symbol size");

  setTooltip(hSelector[EUiStroke], "Symbolic stroke color", true);
  setTooltip(hSelector[EUiFill], "Symbolic fill color", true);
  setTooltip(hSelector[EUiPen], "Symbolic pen width", true);
  setTooltip(hSelector[EUiTextSize], "Symbolic text size", true);
  setTooltip(hSelector[EUiMarkShape], "Mark shape", true);
  setTooltip(hSelector[EUiSymbolSize], "Symbolic symbol size", true);

  setTooltip(hSelector[EUiGridSize], "Grid size", true);
  setTooltip(hSelector[EUiAngleSize], "Angle for angular snap", true);

  setTooltip(hViewNumber, "Current view number");
  setTooltip(hPageNumber, "Current page number");
  setTooltip(hNotes, "Notes for this page");
  setTooltip(hBookmarks, "Bookmarks of this document");
  // TODO setTooltip(hLayerList, "Layers of this page");

  setCheckMark("coordinates|", "points");
  setCheckMark("mode_", "select");
}

AppUi::~AppUi()
{
  // canvas is deleted by Windows ??
  ImageList_Destroy(hIcons);
  KillTimer(hwnd, ID_STATUS_TIMER);
}

void AppUi::layoutChildren()
{
  RECT rc, rc1, rc2;
  GetClientRect(hwnd, &rc);
  SendMessage(hStatusBar, WM_SIZE, 0, 0);
  SendMessage(hToolBar, TB_AUTOSIZE, 0, 0);
  GetClientRect(hStatusBar, &rc1);
  GetClientRect(hToolBar, &rc2);
  int sbh = rc1.bottom - rc1.top;
  int tbh = rc2.bottom - rc2.top;
  int cvh = rc.bottom - sbh - tbh;
  RECT rect;
  rect.left = 0;
  rect.top = tbh;
  rect.right = 200;
  rect.bottom = rc.bottom;
  InvalidateRect(hwnd, &rect, TRUE);
  MoveWindow(hProperties, 10, tbh, 180, 320, TRUE);

  for (int i = 0; i <= EUiSymbolSize; ++i)
    MoveWindow(hButton[i], 20, tbh + 50 + 30 * i, 26, 26, TRUE);

  for (int i = 0; i < EUiView; ++i) {
    int x = (i == EUiGridSize) ? 10 : (i == EUiAngleSize) ? 100 : 60;
    int y = (i >= EUiGridSize) ? 20 : 50 + 30 * i;
    int w = (i >= EUiGridSize) ? 70 : 110;
    MoveWindow(hSelector[i], 10 + x, tbh + y, w, 26, TRUE);
  }

  MoveWindow(hViewNumber, 20, tbh + 240, 60, 26, TRUE);
  MoveWindow(hPageNumber, 120, tbh + 240, 60, 26, TRUE);
  MoveWindow(hViewMarked, 85, tbh + 245, 20, 20, TRUE);
  MoveWindow(hPageMarked, 100, tbh + 245, 20, 20, TRUE);

  MoveWindow(iPathView->windowId(), 20, tbh + 270, 160, 40, TRUE);

  bool visNotes = IsWindowVisible(hNotes);
  bool visBookmarks = IsWindowVisible(hBookmarks);
  int cvw = (visNotes || visBookmarks) ? rc.right - 400 : rc.right - 200;
  MoveWindow(hwndCanvas, 200, tbh, cvw, cvh, TRUE);
  int nth = (visNotes && visBookmarks) ? cvh / 2 : cvh;
  int nty = tbh;
  if (visNotes) {
    MoveWindow(hNotes, rc.right - 200, nty, 200, nth, TRUE);
    nty += nth;
  }
  if (visBookmarks)
    MoveWindow(hBookmarks, rc.right - 200, nty, 200, nth, TRUE);
  int statwidths[] = {rc.right - 200, rc.right - 80, rc.right };
  SendMessage(hStatusBar, SB_SETPARTS, 3, (LPARAM) statwidths);
  iCanvas->update();
}

// --------------------------------------------------------------------

// add item to ComboBoxEx
static void addComboEx(HWND h, String s)
{
  COMBOBOXEXITEM item;
  item.mask = CBEIF_TEXT;
  item.iItem = (INT_PTR) -1;
  item.pszText = (char *) s.z();
  SendMessage(h, CBEM_INSERTITEM, 0, (LPARAM) &item);
}

void AppUi::resetCombos()
{
  for (int i = 0; i < EUiView; ++i)
    SendMessage(hSelector[i], CB_RESETCONTENT, 0, 0);
}

void AppUi::addComboColor(int sel, String s, Color color)
{
  // TODO QIcon icon = prefsColorIcon(color);
  addComboEx(hSelector[sel], s);
}

void AppUi::addCombo(int sel, String s)
{
  addComboEx(hSelector[sel], s);
}

void AppUi::setComboCurrent(int sel, int idx)
{
  SendMessage(hSelector[sel], CB_SETCURSEL, idx, 0);
}

void AppUi::setButtonColor(int sel, Color color)
{
  SendMessage(hButton[sel], BM_SETIMAGE, IMAGE_BITMAP,
	      (LPARAM) colorIcon(color));
}

void AppUi::setPathView(const AllAttributes &all, Cascade *sheet)
{
  iPathView->set(all, sheet);
}

void AppUi::setCheckMark(String name, Attribute a)
{
  setCheckMark(name + "|", a.string());
}

void AppUi::setCheckMark(String name, String value)
{
  String sa = name;
  int na = sa.size();
  String sb = sa + value;
  int first = -1;
  int last = -1;
  int check = -1;
  for (int i = 0; i < int(iActions.size()); ++i) {
    if (iActions[i].name.left(na) == sa) {
      if (first < 0) first = IDBASE + i;
      last = IDBASE + i;
      if (iActions[i].name == sb)
	check = IDBASE + i;
    }
  }
  ipeDebug("CheckMenuRadioItem %s %d %d %d", name.z(), first, last, check);
  if (check > 0)
    CheckMenuRadioItem(hMenuBar, first, last, check, MF_BYCOMMAND);

  if (name == "mode_") {
    for (int i = 0; i < int(iActions.size()); ++i) {
      if (iActions[i].name.left(na) == sa) {
	int id = IDBASE + i;
	int state = TBSTATE_ENABLED;
	if (id == check) state |= TBSTATE_CHECKED;
	SendMessage(hToolBar, TB_SETSTATE, id, (LPARAM) state);
      }
    }
  }
}

void AppUi::setLayers(const Page *page, int view)
{
  // TODO
}

void AppUi::setActionsEnabled(bool mode)
{
  int mstate = mode ? MF_ENABLED : MF_GRAYED;
  int tstate = mode ? TBSTATE_ENABLED : 0;
  for (int i = 0; i < int(iActions.size()); ++i) {
    int id = i + IDBASE;
    if (!iActions[i].alwaysOn) {
      EnableMenuItem(hMenuBar, id, mstate);
      int ostate = SendMessage(hToolBar, TB_GETSTATE, id, 0);
      if (ostate != -1)
	SendMessage(hToolBar, TB_SETSTATE, id,
		    (LPARAM) ((ostate & TBSTATE_CHECKED)|tstate));
    }
  }
  EnableWindow(hNotes, mode);
  EnableWindow(hBookmarks, mode);
}

void AppUi::setNumbers(String vno, bool vm, String pno, bool pm)
{
  SetWindowText(hViewNumber, vno.z());
  SetWindowText(hPageNumber, pno.z());
  Button_SetCheck(hViewMarked, vm ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(hPageMarked, pm ? BST_CHECKED : BST_UNCHECKED);
  EnableWindow(hViewNumber, !vno.isEmpty());
  EnableWindow(hViewMarked, !vno.isEmpty());
  EnableWindow(hPageNumber, !pno.isEmpty());
  EnableWindow(hPageMarked, !pno.isEmpty());
}

void AppUi::setNotes(String notes)
{
  Edit_SetText(hNotes, notes.z());
}

void AppUi::closeRequested()
{
  // calls model
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "closeEvent");
  lua_pushvalue(L, -2); // model
  lua_remove(L, -3);
  lua_call(L, 1, 1);
  bool result = lua_toboolean(L, -1);
  if (result)
    DestroyWindow(hwnd);
}

void AppUi::closeWindow()
{
  SendMessage(hwnd, WM_CLOSE, 0, 0);
}

// Determine if an action is checked.
// Used for viewmarked, pagemarked, snapXXX, and grid_visible
bool AppUi::actionState(const char *name)
{
  ipeDebug("actionState %s", name);
  int idx = actionId(name);
  return GetMenuState(hMenuBar, idx, MF_CHECKED);
}

// Check/uncheck an action.
// Used by Lua for snapangle and grid_visible
// Also to initialize mode_select
// Used from AppUi::action to toggle snapXXX, grid_visible
void AppUi::setActionState(const char *name, bool value)
{
  ipeDebug("setActionState %s %d", name, value);
  int idx = actionId(name);
  CheckMenuItem(hMenuBar, idx, value ? MF_CHECKED : MF_UNCHECKED);
  int state = TBSTATE_ENABLED;
  if (value) state |= TBSTATE_CHECKED;
  SendMessage(hToolBar, TB_SETSTATE, idx, (LPARAM) state);
}

void AppUi::setBookmarks(int no, const String *s)
{
  SendMessage(hBookmarks, LB_RESETCONTENT, 0, 0);
  for (int i = 0; i < no; ++i)
    SendMessage(hBookmarks, LB_ADDSTRING, 0, (LPARAM) s[i].z());
}

void AppUi::setToolVisible(int m, bool vis)
{
  if (m == 1) {
    ShowWindow(hBookmarks, vis ? SW_SHOW : SW_HIDE);
    CheckMenuItem(hMenuBar, actionId("toggle_bookmarks"),
		  vis ? MF_CHECKED : MF_UNCHECKED);
  } else if (m == 2) {
    ShowWindow(hNotes, vis ? SW_SHOW : SW_HIDE);
    CheckMenuItem(hMenuBar, actionId("toggle_notes"),
		  vis ? MF_CHECKED : MF_UNCHECKED);
  }
}

static void clearMenu(HMENU h)
{
  for (int i = GetMenuItemCount(h) - 1; i >= 0; --i)
    DeleteMenu(h, i, MF_BYPOSITION);
}

void AppUi::populateTextStyleMenu()
{
  AttributeSeq seq;
  iCascade->allNames(ETextStyle, seq);
  clearMenu(iTextStyleMenu);
  int check = 0;
  for (uint i = 0; i < seq.size(); ++i) {
    String s = seq[i].string();
    AppendMenu(iTextStyleMenu, MF_STRING, TEXT_STYLE_BASE + i, s.z());
    if (s == iAll.iTextStyle.string())
      check = i;
  }
  CheckMenuRadioItem(iTextStyleMenu, TEXT_STYLE_BASE,
		     TEXT_STYLE_BASE + seq.size() - 1,
		     TEXT_STYLE_BASE + check, MF_BYCOMMAND);
}

void AppUi::populateLayerMenus()
{
  clearMenu(iSelectLayerMenu);
  clearMenu(iMoveToLayerMenu);

  /* TODO
  for (int i = 0; i < iLayerList->count(); ++i) {
    QAction *a = new QAction(iLayerList->item(i)->text(), iSelectLayerMenu);
    QAction *a = new QAction(iLayerList->item(i)->text(), iMoveToLayerMenu);
  }
  */
}

int AppUi::findAction(const char *name) const
{
  for (int i = 0; i < int(iActions.size()); ++i) {
    if (iActions[i].name == name)
      return i;
  }
  return -1;
}

int AppUi::iconId(const char *name) const
{
  int i = findAction(name);
  if (i >= 0 && iActions[i].icon >= 0)
    return iActions[i].icon;
  else
    return I_IMAGENONE;
}

int AppUi::actionId(const char *name) const
{
  int i = findAction(name);
  return (i >= 0) ? i + IDBASE : 0;
}

void AppUi::cmd(int id, int notification)
{
  ipeDebug("WM_COMMAND %d %d", id, notification);
  if (id == IDC_BOOKMARK && notification == LBN_SELCHANGE)
    luaBookmarkSelected(ListBox_GetCurSel(hBookmarks));
  else if (ID_ABSOLUTE_BASE <= id && id <= ID_ABSOLUTE_BASE + EUiPageMarked)
    luaAbsoluteButton(selectorNames[id - ID_ABSOLUTE_BASE]);
  else if (ID_SELECTOR_BASE <= id && id <= ID_SELECTOR_BASE + EUiAngleSize) {
    int sel = id - ID_SELECTOR_BASE;
    int idx = SendMessage(hSelector[sel], CB_GETCURSEL, 0, 0);
    luaSelector(String(selectorNames[sel]), iComboContents[sel][idx]);
  } else if (id == ID_PATHVIEW) {
    luaShowPathStylePopup(Vector(iPathView->popupPos().x,
				 iPathView->popupPos().y));
  } else if (id == ID_PATHVIEW+1) {
    action(iPathView->action());
  } else if (IDBASE <= id && id < IDBASE + int(iActions.size()))
    action(iActions[id - IDBASE].name);
  else
    ipeDebug("Unknown cmd %d", id);
}

static const char * const aboutText =
  "Ipe %d.%d.%d\n\n"
  "Copyright (c) 1993-2012 Otfried Cheong\n\n"
  "The extensible drawing editor Ipe creates figures "
  "in Postscript and PDF format, "
  "using LaTeX to format the text in the figures.\n"
  "Ipe relies on the following fine pieces of software:\n\n"
  " * Pdflatex\n"
  " * %s (%d kB used)\n" // Lua
  " * The font rendering library %s\n"
  " * The rendering library Cairo %s / %s\n"
  " * The compression library zlib %s\n"
  " * Some code from Xpdf\n\n"
  "Ipe is released under the GNU Public License.\n"
  "See ipe7.sourceforge.net for details.";

void AppUi::aboutIpe()
{
  int luaSize = lua_gc(L, LUA_GCCOUNT, 0);
  std::vector<char> buf(strlen(aboutText) + 100);
  sprintf(&buf[0], aboutText,
	  IPELIB_VERSION / 10000,
	  (IPELIB_VERSION / 100) % 100,
	  IPELIB_VERSION % 100,
	  LUA_RELEASE, luaSize,
	  Fonts::freetypeVersion().z(),
	  CAIRO_VERSION_STRING, cairo_version_string(),
	  ZLIB_VERSION);

  MessageBox(hwnd, &buf[0], "About Ipe",
	     MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);
}

void AppUi::toggleVisibility(String action, HWND h)
{
  bool vis = IsWindowVisible(h);
  ShowWindow(h, vis ? SW_HIDE : SW_SHOW);
  int idx = actionId(action.z());
  CheckMenuItem(hMenuBar, idx, vis ? MF_UNCHECKED : MF_CHECKED);
  layoutChildren();
}

void AppUi::action(String name)
{
  ipeDebug("action %s", name.z());
  int id = actionId(name.z());
  if (name == "toggle_notes")
    toggleVisibility(name, hNotes);
  else if (name == "toggle_bookmarks")
    toggleVisibility(name, hBookmarks);
  else if (name == "about")
    aboutIpe();
  else if (name == "shift_key") {
    if (iCanvas) {
      int mod = 0;
      if (GetMenuState(hMenuBar, id, MF_CHECKED))
	mod |= CanvasBase::EShift;
      iCanvas->setAdditionalModifiers(mod);
    }
  } else {
    // Implement radio buttons
    int i = name.find("|");
    if (i >= 0)
      setCheckMark(name.left(i+1), name.substr(i+1));
    if (name.left(5) == "mode_")
      setCheckMark("mode_", name.substr(5));
    if (name.left(4) == "snap" || name == "grid_visible")
      setActionState(name.z(), !GetMenuState(hMenuBar, id, MF_CHECKED));
    luaAction(name);
  }
}

WINID AppUi::windowId()
{
  return hwnd;
}

void AppUi::setWindowCaption(bool mod, const char *s)
{
  ipeDebug("setWindowCaption %d %s", mod, s);
  SetWindowText(hwnd, s);
}

void AppUi::showWindow(int width, int height)
{
  ShowWindow(hwnd, win_nCmdShow);
  UpdateWindow(hwnd);
}

// show for t milliseconds, or permanently if t == 0
void AppUi::explain(const char *s, int t)
{
  if (t)
    SetTimer(hwnd, ID_STATUS_TIMER, t, NULL);
  else
    KillTimer(hwnd, ID_STATUS_TIMER);
  SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM) s);
}

void AppUi::setMouseIndicator(const char *s)
{
  SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) s);
}

void AppUi::setZoom(double zoom)
{
  char s[32];
  sprintf(s, "(%dppi)", int(72.0 * zoom));
  iCanvas->setZoom(zoom);
  SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) s);
}

// --------------------------------------------------------------------

struct WindowCounter {
  int count;
  HWND hwnd;
};

BOOL CALLBACK AppUi::enumThreadWndProc(HWND hwnd, LPARAM lParam)
{
  WindowCounter *wc = (WindowCounter *) lParam;
  // ipeDebug("Enumerating Window %d", int(hwnd));
  char cname[100];
  if (GetClassName(hwnd, cname, 100) && !strcmp(cname, className) &&
      wc->hwnd != hwnd)
    wc->count += 1;
  return TRUE;
}

LRESULT CALLBACK AppUi::wndProc(HWND hwnd, UINT message, WPARAM wParam,
				LPARAM lParam)
{
  AppUi *ui = (AppUi*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
  switch (message) {
  case WM_CREATE:
    {
      LPCREATESTRUCT p = (LPCREATESTRUCT) lParam;
      ui = (AppUi *) p->lpCreateParams;
      ui->hwnd = hwnd;
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) ui);
      ui->initUi();
    }
    break;
  case WM_INITMENUPOPUP:
    if (ui) {
      ipeDebug("menupopup %d",  lParam);
      if (lParam == 2)
	ui->populateTextStyleMenu();
      else if (lParam == 6)
	ui->populateLayerMenus();
    }
    break;
  case WM_COMMAND:
    if (ui)
      ui->cmd(LOWORD(wParam), HIWORD(wParam));
    break;
  case WM_CTLCOLORSTATIC:
    if (ui && ui->iCanvas && HWND(lParam) == ui->hNotes)
      return (INT_PTR) (HBRUSH) GetStockObject(WHITE_BRUSH);
    break;
  case WM_SETFOCUS:
    if (ui && ui->iCanvas)
      SetFocus(ui->hwndCanvas);
    return 0;
  case WM_TIMER:
    if (ui && ui->iCanvas)
      SendMessage(ui->hStatusBar, SB_SETTEXT, 0, (LPARAM) 0);
    return 0;
  case WM_SIZE:
    if (ui && ui->iCanvas)
      ui->layoutChildren();
    break;
  case WM_CLOSE:
    if (ui) {
      ui->closeRequested();
      return 0; // do NOT allow default procedure to close
    }
    break;
  case WM_DESTROY:
    {
      delete ui;
      // only post quit message when this was the last window
      WindowCounter wc;
      wc.count = 0;
      wc.hwnd = hwnd;
      EnumThreadWindows(GetCurrentThreadId(), enumThreadWndProc, (LPARAM) &wc);
      if (wc.count == 0)
	PostQuitMessage(0);
    }
    break;
  default:
    break;
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}

// --------------------------------------------------------------------

void AppUi::init(HINSTANCE hInstance)
{
  WNDCLASSEX wc;
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = 0;
  wc.lpfnWndProc = wndProc;
  wc.cbClsExtra	 = 0;
  wc.cbWndExtra	 = 0;
  wc.hInstance	 = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = className;
  wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON));
  wc.hIconSm =
    HICON(LoadImage(GetModuleHandle(NULL),
		    MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, 16, 16, 0));

  if (!RegisterClassEx(&wc)) {
    MessageBox(NULL, "AppUi registration failed!", "Error!",
	       MB_ICONEXCLAMATION | MB_OK);
    exit(9);
  }
}

AppUiBase *createAppUi(lua_State *L0, int model)
{
  return new AppUi(L0, model);
}

void ipe_init_appui(HINSTANCE hInstance, int nCmdShow)
{
  AppUi::init(hInstance);
  PathView::init(hInstance);
  // save for later use
  win_hInstance = hInstance;
  win_nCmdShow = nCmdShow;
}

// --------------------------------------------------------------------
