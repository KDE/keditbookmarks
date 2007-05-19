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

#ifndef __KLISTVIEW_P_H__
#define __KLISTVIEW_P_H__

class QSortFilterProxyModel;

class KListView::Private
{
public:
    Private(KListView *listView);
    ~Private();

    QModelIndexList intersectionSet(const QRect &rect) const;

    KListView *listView;
    QModelIndex hovered;
    bool modelSortCapable;
    int numCategories;
    QList<QString> categories;
    QHash<QString, int> elementsPerCategory;
    KItemCategorizer *itemCategorizer;
    QSortFilterProxyModel *proxyModel;
};

#endif // __KLISTVIEW_P_H__
