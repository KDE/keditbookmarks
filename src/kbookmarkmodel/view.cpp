/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>
   SPDX-FileCopyrightText: 2005 Daniel Teske <teske@squorn.de>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#include "view.h"

#include <KBookmark>

KBookmarkView::KBookmarkView(QWidget *parent)
    : QTreeView(parent)
    , m_loadingState(false)
{
    setAcceptDrops(true);
    setDefaultDropAction(Qt::MoveAction);
    connect(this, &QTreeView::expanded, this, &KBookmarkView::slotExpanded);
    connect(this, &QTreeView::collapsed, this, &KBookmarkView::slotCollapsed);
}

KBookmarkView::~KBookmarkView()
{
}

void KBookmarkView::loadFoldedState()
{
    m_loadingState = true;
    loadFoldedState(QModelIndex());
    m_loadingState = false;
}

void KBookmarkView::loadFoldedState(const QModelIndex &parentIndex)
{
    const int count = model()->rowCount(parentIndex);
    for (int row = 0; row < count; ++row) {
        const QModelIndex index = model()->index(row, 0, parentIndex);
        const KBookmark bk = bookmarkForIndex(index);
        if (bk.isNull()) {
            expand(index);
        } else if (bk.isGroup()) {
            setExpanded(index, bk.toGroup().isOpen());
            loadFoldedState(index);
        }
    }
}

void KBookmarkView::slotExpanded(const QModelIndex &index)
{
    if (!m_loadingState) {
        KBookmark bk = bookmarkForIndex(index);
        bk.internalElement().setAttribute(QStringLiteral("folded"), QStringLiteral("no"));
    }
}

void KBookmarkView::slotCollapsed(const QModelIndex &index)
{
    if (!m_loadingState) {
        KBookmark bk = bookmarkForIndex(index);
        bk.internalElement().setAttribute(QStringLiteral("folded"), QStringLiteral("yes"));
    }
}

#include "moc_view.cpp"
