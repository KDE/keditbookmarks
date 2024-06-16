/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Daniel Teske <teske@squorn.de>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef TREEITEM_P_H
#define TREEITEM_P_H

#include <KBookmark>
#include <QList>

class TreeItem
{
public:
    TreeItem(const KBookmark &bk, TreeItem *parent);
    ~TreeItem();
    TreeItem *child(int row);
    TreeItem *parent() const;

    void insertChildren(int first, int last);
    void deleteChildren(int first, int last);
    void moveChildren(int first, int last, TreeItem *newParent, int position);
    KBookmark bookmark() const;
    int childCount();
    TreeItem *treeItemForBookmark(const KBookmark &bk);

private:
    void initChildren();

    QList<TreeItem *> children;
    TreeItem *mParent;
    KBookmark mBookmark;
    bool mInitDone;
};
#endif
