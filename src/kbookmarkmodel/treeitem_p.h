/* This file is part of the KDE project
   Copyright (C) 2005 Daniel Teske <teske@squorn.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>
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
