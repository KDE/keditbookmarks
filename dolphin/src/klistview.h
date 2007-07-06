/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@gmail.com>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#ifndef KLISTVIEW_H
#define KLISTVIEW_H

#include <QtGui/QListView>

#include <libdolphin_export.h>

class KItemCategorizer;

/**
  * @short Item view for listing items
  *
  * KListView allows you to use it as it were a QListView. You can add an
  * itemCategorizer to it, so your items became categorized depending on the
  * KItemCategorizer inherited class rules.
  *
  * @see KItemCategorizer, KSortFilterProxyModel
  *
  * @author Rafael Fernández López <ereslibre@gmail.com>
  */
class LIBDOLPHINPRIVATE_EXPORT KListView
    : public QListView
{
    Q_OBJECT

public:
    KListView(QWidget *parent = 0);

    ~KListView();

    virtual void setModel(QAbstractItemModel *model);

    virtual QRect visualRect(const QModelIndex &index) const;

    /**
      * Will return the current categorizer. If none set, this method will
      * return 0
      */
    KItemCategorizer *itemCategorizer() const;

    /**
      * Sets the categorizer to be used. Causes the item view to repaint
      */
    void setItemCategorizer(KItemCategorizer *itemCategorizer);

    virtual QModelIndex indexAt(const QPoint &point) const;

public Q_SLOTS:
    virtual void reset();

protected:
    virtual void paintEvent(QPaintEvent *event);

    virtual void resizeEvent(QResizeEvent *event);

    virtual void setSelection(const QRect &rect,
                              QItemSelectionModel::SelectionFlags flags);

    virtual void mouseMoveEvent(QMouseEvent *event);

    virtual void mousePressEvent(QMouseEvent *event);

    virtual void mouseReleaseEvent(QMouseEvent *event);

    virtual void leaveEvent(QEvent *event);

    virtual void startDrag(Qt::DropActions supportedActions);

    virtual void dragMoveEvent(QDragMoveEvent *event);

    virtual void dragLeaveEvent(QDragLeaveEvent *event);

    virtual QModelIndex moveCursor(CursorAction cursorAction,
                                   Qt::KeyboardModifiers modifiers);

protected Q_SLOTS:
    virtual void rowsInserted(const QModelIndex &parent,
                              int start,
                              int end);

    virtual void rowsInsertedArtifficial(const QModelIndex &parent,
                                         int start,
                                         int end);

    virtual void rowsRemoved(const QModelIndex &parent,
                             int start,
                             int end);

    virtual void updateGeometries();

    virtual void slotSortingRoleChanged();


private:
    class Private;
    Private *d;
};

#endif // KLISTVIEW_H
