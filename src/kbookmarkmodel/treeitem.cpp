/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Daniel Teske <teske@squorn.de>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#include "treeitem_p.h"

TreeItem::TreeItem(const KBookmark &bk, TreeItem *parent)
    : mParent(parent)
    , mBookmark(bk)
    , mInitDone(false)
{
}

TreeItem::~TreeItem()
{
    qDeleteAll(children);
    children.clear();
}

TreeItem *TreeItem::child(int row)
{
    if (!mInitDone)
        initChildren();
    if (row < 0 || row >= children.count())
        return parent();
    return children.at(row);
}

int TreeItem::childCount()
{
    if (!mInitDone)
        initChildren();
    return children.count();
}

TreeItem *TreeItem::parent() const
{
    return mParent;
}

void TreeItem::insertChildren(int first, int last)
{
    // Find child number last
    KBookmarkGroup parent = bookmark().toGroup();
    KBookmark child = parent.first();
    for (int j = 0; j < last; ++j)
        child = parent.next(child);

    // insert children
    int i = last;
    do {
        children.insert(i, new TreeItem(child, this));
        child = parent.previous(child);
        --i;
    } while (i >= first);
}

void TreeItem::deleteChildren(int first, int last)
{
    if (!mInitDone) {
        return;
    }
    Q_ASSERT(first <= last);
    Q_ASSERT(last < children.count());
    QList<TreeItem *>::iterator firstIt, lastIt, it;
    firstIt = children.begin() + first;
    lastIt = children.begin() + last + 1;
    for (it = firstIt; it != lastIt; ++it) {
        delete *it;
    }
    children.erase(firstIt, lastIt);
}

KBookmark TreeItem::bookmark() const
{
    return mBookmark;
}

void TreeItem::initChildren()
{
    mInitDone = true;
    if (mBookmark.isGroup()) {
        KBookmarkGroup parent = mBookmark.toGroup();
        for (KBookmark child = parent.first(); child.hasParent(); child = parent.next(child)) {
            TreeItem *item = new TreeItem(child, this);
            children.append(item);
        }
    }
}

TreeItem *TreeItem::treeItemForBookmark(const KBookmark &bk)
{
    if (bk.address() == mBookmark.address())
        return this;
    if (!mInitDone) {
        initChildren();
    }
    QString commonParent = KBookmark::commonParent(bk.address(), mBookmark.address());
    if (commonParent == mBookmark.address()) // mBookmark is a parent of bk
    {
        QList<TreeItem *>::const_iterator it, end;
        end = children.constEnd();
        for (it = children.constBegin(); it != end; ++it) {
            KBookmark child = (*it)->bookmark();
            if (KBookmark::commonParent(child.address(), bk.address()) == child.address())
                return (*it)->treeItemForBookmark(bk);
        }
        return nullptr;
    } else {
        if (parent() == nullptr)
            return nullptr;
        return parent()->treeItemForBookmark(bk);
    }
}

// kate: space-indent on; indent-width 4; replace-tabs on;
