/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Daniel Teske <teske@squorn.de>

   SPDX-License-Identifier: GPL-2.0-only
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
    ~BookmarkFolderView() override;
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
    ~BookmarkListView() override;
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
    ~BookmarkFolderViewFilterModel() override;
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
