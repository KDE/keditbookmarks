/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Daniel Teske <teske@squorn.de>
   SPDX-FileCopyrightText: 2010 David Faure <faure@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef BOOKMARKMODEL_MODEL_H
#define BOOKMARKMODEL_MODEL_H

#include "kbookmarkmodel_export.h"
#include <QAbstractItemModel>

class CommandHistory;
class KBookmarkGroup;
class KBookmarkManager;
class KBookmark;

class KBOOKMARKMODEL_EXPORT KBookmarkModel : public QAbstractItemModel
{
    Q_OBJECT

    enum ColumnIds {
        NameColumnId = 0,
        UrlColumnId = 1,
        CommentColumnId = 2,
        StatusColumnId = 3,
        LastColumnId = 3,
        NoOfColumnIds = LastColumnId + 1,
    };

public:
    KBookmarkModel(const KBookmark &root, CommandHistory *commandHistory, QObject *parent = nullptr);
    void setRoot(const KBookmark &root);

    ~KBookmarkModel() override;

    KBookmarkManager *bookmarkManager();
    CommandHistory *commandHistory();

    enum AdditionalRoles {
        // Note: use   printf "0x%08X\n" $(($RANDOM*$RANDOM))
        // to define additional roles.
        KBookmarkRole = 0x161BEC30,
    };

    // reimplemented functions
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual void resetModel();

    QModelIndex indexForBookmark(const KBookmark &bk) const;
    KBookmark bookmarkForIndex(const QModelIndex &index) const;
    void emitDataChanged(const KBookmark &bk);

    /// Call this before inserting items into the bookmark group
    void beginInsert(const KBookmarkGroup &group, int first, int last);
    /// Call this after item insertion is done
    void endInsert();

    /// Remove the bookmark
    void removeBookmark(const KBookmark &bookmark);

    // drag and drop
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    Qt::DropActions supportedDropActions() const override;

public Q_SLOTS:
    void notifyManagers(const KBookmarkGroup &grp);

private:
    class Private;
    Private *const d;
};

#endif
