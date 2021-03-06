// -*- C++ -*-
// --------------------------------------------------------------------
// Special widgets for Win32
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2014  Otfried Cheong

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

#ifndef CONTROLS_WIN_H
#define CONTROLS_WIN_H

#include "ipelib.h"

#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0501
#include <windows.h>

using namespace ipe;

// --------------------------------------------------------------------

#if 0
class LayerBox : public QListWidget {
  Q_OBJECT
public:
  LayerBox(QWidget *parent = 0);

  void set(const Page *page, int view);

signals:
  void activated(String name, String layer);
  void showLayerBoxPopup(Vector v, String layer);

public slots:
  void layerChanged(QListWidgetItem *item);

protected:
  virtual void mouseReleaseEvent(QMouseEvent *e);
  virtual void mousePressEvent(QMouseEvent *e);

  void addAction(QMenu *m, QListWidgetItem *item, String name,
		 const QString &text);

private:
  bool iInSet;
};
#endif

// --------------------------------------------------------------------

class PathView {

public:
  static void init(HINSTANCE hInstance);
  PathView(HWND parent, int id);

  HWND windowId() const { return hwnd; }
  void set(const AllAttributes &all, Cascade *sheet);

  inline POINT popupPos() const { return pos; }
  inline String action() const { return iAction; }
private:
  static const char className[];
  static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message,
				  WPARAM wParam, LPARAM lParam);
  void wndPaint();
  void button(int x, int y);
private:
  HWND hwnd;
  int idBase;
  POINT pos;
  Cascade *iCascade;
  AllAttributes iAll;
  String iAction;
};

// --------------------------------------------------------------------

#if 0
class PageSorter : public QListWidget {
  Q_OBJECT

public:
  PageSorter(Document *doc, int width, QWidget *parent = 0);

  int pageAt(int r) const;

public slots:
  void deletePages();
  void cutPages();
  void insertPages();

private:
  virtual void contextMenuEvent(QContextMenuEvent *event);

private:
  Document *iDoc;
  QList<QListWidgetItem *> iCutList;
  int iActionRow;
};
#endif

extern void ipe_init_controls(HINSTANCE hInstance);

// --------------------------------------------------------------------
#endif
