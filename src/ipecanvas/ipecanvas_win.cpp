// --------------------------------------------------------------------
// ipe::Canvas for Win
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

#include "ipecanvas_win.h"
#include "ipecairopainter.h"
#include "ipetool.h"

#include <cairo.h>
#include <cairo-win32.h>

#include <windowsx.h>

using namespace ipe;

const char Canvas::className[] = "ipeCanvasWindowClass";

// --------------------------------------------------------------------

void Canvas::invalidate()
{
  InvalidateRect(hwnd, NULL, FALSE);
}

void Canvas::invalidate(int x, int y, int w, int h)
{
  RECT r;
  r.left = x;
  r.right = x + w;
  r.top = y;
  r.bottom = y + h;
  // ipeDebug("invalidate %d %d %d %d\n", r.left, r.right, r.top, r.bottom);
  InvalidateRect(hwnd, &r, FALSE);
}

// --------------------------------------------------------------------

int Canvas::getModifiers()
{
  int mod = 0;
  if (GetKeyState(VK_SHIFT))
    mod |= EShift;
  if (GetKeyState(VK_CONTROL))
    mod |= EControl;
  if (GetKeyState(VK_MENU))
    mod |= EAlt;
  return mod;
}

void Canvas::button(int button, bool down, int xPos, int yPos)
{
  // ipeDebug("Canvas::button %d %d %d %d", button, down, xPos, yPos);
  POINT p;
  p.x = xPos;
  p.y = yPos;
  ClientToScreen(hwnd, &p);
  iGlobalPos = Vector(p.x, p.y);
  computeFifi(xPos, yPos);
  int mod = getModifiers() | iAdditionalModifiers;
  if (iTool)
    iTool->mouseButton(button | mod, down);
  else if (down && iObserver)
    iObserver->canvasObserverMouseAction(button | mod);
}

void Canvas::mouseMove(int xPos, int yPos)
{
  // ipeDebug("Canvas::mouseMove %d %d", xPos, yPos);
  computeFifi(xPos, yPos);
  if (iTool)
    iTool->mouseMove();
  if (iObserver)
    iObserver->canvasObserverPositionChanged();
}

void Canvas::wheel(int zDelta)
{
  // ipeDebug("Canvas::wheel %d", zDelta);
  if (iObserver)
    iObserver->canvasObserverWheelMoved(zDelta);
}

void Canvas::setCursor(TCursor cursor, double w, Color *color)
{
  // TODO
}

// --------------------------------------------------------------------

void Canvas::updateSize()
{
  RECT rc;
  GetClientRect(hwnd, &rc);
  iWidth = rc.right;
  iHeight = rc.bottom;
}

void Canvas::wndPaint()
{
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd, &ps);

  if (!iWidth)
    updateSize();
  refreshSurface();

  int x0 = ps.rcPaint.left;
  int y0 = ps.rcPaint.top;
  int w = ps.rcPaint.right - x0;
  int h = ps.rcPaint.bottom - y0;

  HBITMAP bits = CreateCompatibleBitmap(hdc, w, h);
  HDC bitsDC = CreateCompatibleDC(hdc);
  SelectObject(bitsDC, bits);
  cairo_surface_t *surface = cairo_win32_surface_create(bitsDC);

  cairo_t *cr = cairo_create(surface);
  cairo_translate(cr, -x0, -y0);
  cairo_set_source_surface(cr, iSurface, 0.0, 0.0);
  cairo_paint(cr);

  if (iFifiVisible) {
    Vector p = userToDev(iMousePos);
    cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
    cairo_move_to(cr, p.x - 8, p.y);
    cairo_line_to(cr, p.x + 8, p.y);
    cairo_move_to(cr, p.x, p.y - 8);
    cairo_line_to(cr, p.x, p.y + 8);
    cairo_stroke(cr);
    iOldFifi = p;
  }
  if (iPage) {
    CairoPainter cp(iCascade, iFonts, cr, iZoom, false);
    cp.transform(canvasTfm());
    cp.pushMatrix();
    drawTool(cp);
    cp.popMatrix();
  }
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  BitBlt(hdc, x0, y0, w, h, bitsDC, 0, 0, SRCCOPY);
  DeleteDC(bitsDC);
  DeleteObject(bits);
  EndPaint(hwnd, &ps);
}

// --------------------------------------------------------------------

void Canvas::mouseButton(Canvas *canvas, int button, bool down, LPARAM lParam)
{
  if (canvas) {
    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);
    canvas->button(button, down, xPos, yPos);
  }
}

LRESULT CALLBACK Canvas::wndProc(HWND hwnd, UINT message, WPARAM wParam,
				 LPARAM lParam)
{
  Canvas *canvas = (Canvas *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
  switch (message) {
  case WM_CREATE:
    {
      assert(canvas == 0);
      LPCREATESTRUCT p = (LPCREATESTRUCT) lParam;
      canvas = (Canvas *) p->lpCreateParams;
      canvas->hwnd = hwnd;
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) canvas);
    }
    break;
  case WM_PAINT:
    if (canvas)
      canvas->wndPaint();
    return 0;
  case WM_MOUSEMOVE:
    if (canvas) {
      int xPos = GET_X_LPARAM(lParam);
      int yPos = GET_Y_LPARAM(lParam);
      canvas->mouseMove(xPos, yPos);
    }
    break;
  case WM_SIZE:
    if (canvas)
      canvas->updateSize();
    break;
  case WM_LBUTTONDOWN:
    mouseButton(canvas, 1, true, lParam);
    break;
  case WM_LBUTTONUP:
    mouseButton(canvas, 1, false, lParam);
    break;
  case WM_RBUTTONDOWN:
    mouseButton(canvas, 2, true, lParam);
    break;
  case WM_RBUTTONUP:
    mouseButton(canvas, 2, false, lParam);
    break;
  case WM_MBUTTONDOWN:
    mouseButton(canvas, 3, true, lParam);
    break;
  case WM_MBUTTONUP:
    mouseButton(canvas, 3, false, lParam);
    break;
  case WM_MOUSEWHEEL:
    if (canvas)
      canvas->wheel(GET_WHEEL_DELTA_WPARAM(wParam));
    break;
  case WM_GETMINMAXINFO:
    {
      LPMINMAXINFO mm = LPMINMAXINFO(lParam);
      fprintf(stderr, "Canvas MINMAX %ld %ld\n", mm->ptMinTrackSize.x,
	      mm->ptMinTrackSize.y);
    }
    break;
  case WM_DESTROY:
    delete canvas;
    break;
  default:
    // ipeDebug("Canvas wndProc(%d)", message);
    break;
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}

// --------------------------------------------------------------------

Canvas::Canvas(HWND parent)
{
  HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, "",
			     WS_CHILD | WS_VISIBLE,
			     0, 0, 0, 0,
			     parent, NULL,
			     (HINSTANCE) GetWindowLong(parent, GWL_HINSTANCE),
			     this);
  if (hwnd == NULL) {
    MessageBox(NULL, "Canvas creation failed!", "Error!",
	       MB_ICONEXCLAMATION | MB_OK);
    exit(9);
  }
  assert(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

void Canvas::init(HINSTANCE hInstance)
{
  WNDCLASSEX wc;
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = 0; // CS_VREDRAW | CS_HREDRAW;
  wc.lpfnWndProc = wndProc;
  wc.cbClsExtra	 = 0;
  wc.cbWndExtra	 = 0;
  wc.hInstance	 = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH) GetStockObject(NULL_BRUSH);
  wc.lpszMenuName = NULL;  // actually child id
  wc.lpszClassName = className;
  wc.hIcon = NULL;
  wc.hIconSm = NULL;

  if (!RegisterClassEx(&wc)) {
    MessageBox(NULL, "Canvas control registration failed!", "Error!",
	       MB_ICONEXCLAMATION | MB_OK);
    exit(9);
  }
}

void ipe_init_canvas(HINSTANCE hInstance)
{
  Canvas::init(hInstance);
}

// --------------------------------------------------------------------
