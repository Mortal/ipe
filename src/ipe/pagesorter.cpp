// --------------------------------------------------------------------
// PageSorter
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

#include "pagesorter.h"

#include "ipethumbs.h"

#include <QMenu>
#include <QContextMenuEvent>

using namespace ipe;

// --------------------------------------------------------------------

PageSorter::PageSorter(Document *doc, int itemWidth, QWidget *parent)
  : QListWidget(parent)
{
  iDoc = doc;
  setViewMode(QListView::IconMode);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setResizeMode(QListView::Adjust);
  setWrapping(true);
  setUniformItemSizes(true);
  setFlow(QListView::LeftToRight);
  setSpacing(10);
  setMovement(QListView::Static);

  Thumbnail r(iDoc, itemWidth);
  setGridSize(QSize(itemWidth, r.height() + 50));
  setIconSize(QSize(itemWidth, r.height()));

  for (int i = 0; i < doc->countPages(); ++i) {
    Page *p = doc->page(i);
    Buffer b = r.render(p, p->countViews() - 1);
    QImage bits((const uchar *) b.data(), itemWidth, r.height(),
		QImage::Format_RGB32);
    QIcon icon(QPixmap::fromImage(bits));

    QString s;
    QString t = QString::fromUtf8(p->title().z());
    if (t != "") {
      s.sprintf("%d: ", i+1);
      s += t;
    } else {
      s.sprintf("Page %d", i+1);
    }
    QListWidgetItem *item = new QListWidgetItem(icon, s);
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setToolTip(s);
    item->setData(Qt::UserRole, QVariant(i)); // page number
    addItem(item);
  }
}

int PageSorter::pageAt(int r) const
{
  return item(r)->data(Qt::UserRole).toInt();
}

void PageSorter::deletePages()
{
  QList<QListWidgetItem *> items = selectedItems();
  for (int i = 0; i < items.count(); ++i) {
    int r = row(items[i]);
    QListWidgetItem *item = takeItem(r);
    delete item;
  }
}

void PageSorter::cutPages()
{
  // delete items in old cut list
  for (int i = 0; i < iCutList.count(); ++i)
    delete iCutList[i];
  iCutList.clear();

  QList<QListWidgetItem *> items = selectedItems();
  for (int i = 0; i < items.count(); ++i) {
    int r = row(items[i]);
    QListWidgetItem *item = takeItem(r);
    iCutList.append(item);
  }
}

void PageSorter::insertPages()
{
  // deselect everything
  for (int i = 0; i < count(); ++i)
    item(i)->setSelected(false);
  int r = (iActionRow >= 0) ? iActionRow : count();
  for (int i = 0; i < iCutList.count(); ++i) {
    insertItem(r, iCutList[i]);
    item(r++)->setSelected(true);
  }
  iCutList.clear();
}

void PageSorter::contextMenuEvent(QContextMenuEvent *ev)
{
  ev->accept();

  QListWidgetItem *item = itemAt(ev->pos());
  iActionRow = row(item);

  QMenu *m = new QMenu();
  QAction *action_delete = new QAction("&Delete", this);
  connect(action_delete, SIGNAL(triggered()), SLOT(deletePages()));
  QAction *action_cut = new QAction("&Cut", this);
  connect(action_cut, SIGNAL(triggered()), SLOT(cutPages()));
  QAction *action_insert = new QAction("&Insert", this);
  connect(action_insert, SIGNAL(triggered()), SLOT(insertPages()));
  if (selectedItems().count() > 0) {
    m->addAction(action_delete);
    m->addAction(action_cut);
  }
  if (iCutList.count() > 0)
    m->addAction(action_insert);
  m->exec(ev->globalPos());
  delete m;
}

// --------------------------------------------------------------------

