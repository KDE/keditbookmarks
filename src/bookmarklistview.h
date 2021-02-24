/* This file is part of the KDE project
   Copyright (C) 2005 Daniel Teske <teske@squorn.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __bookmarklistview_h
#define __bookmarklistview_h

#include <QSortFilterProxyModel>
#include <QTreeView>

#include "kbookmarkmodel/view.h"

class KBookmarkModel;
class BookmarkListView;
class BookmarkFolderViewFilterModel;

class BookmarkFolderView : public KBookmarkView
{
    Q_OBJECT
public:
    explicit BookmarkFolderView(BookmarkListView *view, QWidget *parent = nullptr);
    virtual ~BookmarkFolderView();
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    KBookmark bookmarkForIndex(const QModelIndex &idx) const override;
private Q_SLOTS:
    void slotReset();

private:
    BookmarkListView *mview;
    BookmarkFolderViewFilterModel *mmodel;
};

class BookmarkListView : public KBookmarkView
{
    Q_OBJECT
public:
    explicit BookmarkListView(QWidget *parent = nullptr);
    virtual ~BookmarkListView();
    void loadColumnSetting();
    void saveColumnSetting();
    void setModel(QAbstractItemModel *model) override;
    KBookmark bookmarkForIndex(const QModelIndex &idx) const override;
    KBookmarkModel *bookmarkModel() const;

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;
};

class BookmarkFolderViewFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit BookmarkFolderViewFilterModel(QObject *parent = nullptr);
    virtual ~BookmarkFolderViewFilterModel();
    QStringList mimeTypes() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

protected:
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    // FIXME check
    Qt::DropActions supportedDropActions() const override
    {
        return sourceModel()->supportedDropActions();
    }
};
#endif
