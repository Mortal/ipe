// -*- C++ -*-
// --------------------------------------------------------------------
// ipe::Canvas for GTK
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

#ifndef IPECANVAS_GTK_H
#define IPECANVAS_GTK_H
// --------------------------------------------------------------------

#include "ipecanvas.h"

#include <gtk/gtk.h>

namespace ipe {

  class Canvas : public CanvasBase {
  public:
    Canvas(GtkWidget *parent);
    ~Canvas();

    GtkWidget *window() const { return GTK_WIDGET(iWindow); }

  private:
    virtual void invalidate();
    virtual void invalidate(int x, int y, int w, int h);

    void exposeHandler(GdkEventExpose *event);
    void buttonHandler(GdkEventButton *event);
    void motionHandler(GdkEventMotion *event);
    void scrollHandler(GdkEventScroll *event);

    static gboolean expose_cb(GtkWidget *widget, GdkEvent *event,
			      Canvas *canvas);
    static gboolean button_cb(GtkWidget *widget, GdkEvent *event,
			      Canvas *data);
    static gboolean motion_cb(GtkWidget *widget, GdkEvent *event,
			      Canvas *canvas);
    static gboolean scroll_cb(GtkWidget *widget, GdkEvent *event,
			      Canvas *canvas);

  private:
    GtkWidget *iWindow;
  };

} // namespace

// --------------------------------------------------------------------
#endif
