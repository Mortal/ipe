// -*- C++ -*-
// --------------------------------------------------------------------
// ipe::Canvas for Win32
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

#ifndef IPECANVASWIN_H
#define IPECANVASWIN_H
// --------------------------------------------------------------------

#include "ipecanvas.h"

#include <windows.h>

namespace ipe {

  class Canvas : public CanvasBase {
  public:
    static void init(HINSTANCE hInstance);
    Canvas(HWND parent);

    HWND windowId() const { return hwnd; }

  private:
    int getModifiers();
    void button(int button, bool down, int xPos, int yPos);
    void mouseMove(int xPos, int yPos);
    void wheel(int zDelta);
    void wndPaint();
    void updateSize();

    virtual void invalidate();
    virtual void invalidate(int x, int y, int w, int h);
  private:
    static const char className[];
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message,
				    WPARAM wParam, LPARAM lParam);
    static void mouseButton(Canvas *canvas, int button, bool down,
			    LPARAM lParam);
  private:
    HWND hwnd;
  };

} // namespace

extern void ipe_init_canvas(HINSTANCE hInstance);

// --------------------------------------------------------------------
#endif