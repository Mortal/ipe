// -*- C++ -*-
// --------------------------------------------------------------------
// Ipeview for QT
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

#ifndef IPEVIEW_H
#define IPEVIEW_H

#include "ipecanvas.h"

#include <QMainWindow>
#include <QAction>

using namespace ipe;

// --------------------------------------------------------------------

class IpeAction : public QAction {
  Q_OBJECT
public:
  IpeAction(int cmd, const QString &text,
	    const char *shortcut, QObject *parent);
signals:
  void triggered(int cmd);
private slots:
  void forwardTrigger();
private:
  int iCommand;
};

// --------------------------------------------------------------------

class AppUi : public QMainWindow, public CanvasObserver {
  Q_OBJECT

public:
  enum TAction { EOpen, EQuit, EGridVisible, EFullScreen, EFitPage,
		 EZoomIn, EZoomOut, ENextView, EPreviousView,
		 ENumActions };

public:
  AppUi(Qt::WFlags f=0);

  void zoom(int delta);
  void nextView(int delta);
  void fitBox(const Rect &box);
  void updateLabel();
  bool load(const char* fn);

protected: // from CanvasObserver
  virtual void canvasObserverWheelMoved(int degrees);
  virtual void canvasObserverMouseAction(int button);

public slots:
  void cmd(int cmd);

private:
  Document *iDoc;
  CanvasBase *iCanvas;
  Snap iSnap;

  QMenu *iFileMenu;
  QMenu *iViewMenu;
  QMenu *iMoveMenu;
  IpeAction *iGridVisible;

  String iFileName;
  int iPageNo;
  int iViewNo;
};

// --------------------------------------------------------------------
#endif
