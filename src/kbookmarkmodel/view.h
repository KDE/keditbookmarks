/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Daniel Teske <teske@squorn.de>

   SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef BOOKMARKMODEL_BOOKMARKVIEW_H
#define BOOKMARKMODEL_BOOKMARKVIEW_H

#include <QTreeView>

#include "kbookmarkmodel_export.h"

class KBookmark;

class KBOOKMARKMODEL_EXPORT KBookmarkView : public QTreeView
{
    Q_OBJECT
public:
    explicit KBookmarkView(QWidget *parent = nullptr);
    ~KBookmarkView() override;
    virtual KBookmark bookmarkForIndex(const QModelIndex &idx) const = 0;
    void loadFoldedState();

private Q_SLOTS:
    void slotExpanded(const QModelIndex &index);
    void slotCollapsed(const QModelIndex &index);

private:
    void loadFoldedState(const QModelIndex &parentIndex);
    bool m_loadingState;
};

#endif
