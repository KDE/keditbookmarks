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

#ifndef KLISTVIEW_P_H
#define KLISTVIEW_P_H

class KSortFilterProxyModel;

/**
  * @internal
  */
class KListView::Private
{
public:
    Private(KListView *listView);
    ~Private();


    // Methods

    /**
      * Returns the list of items that intersects with @p rect
      */
    const QModelIndexList &intersectionSet(const QRect &rect);

    /**
      * Gets the item rect in the viewport for @p index
      */
    QRect visualRectInViewport(const QModelIndex &index) const;

    /**
      * Returns the category rect in the viewport for @p category
      */
    QRect visualCategoryRectInViewport(const QString &category) const;

    /**
      * Caches and returns the rect that corresponds to @p index
      */
    const QRect &cacheIndex(const QModelIndex &index);

    /**
      * Caches and returns the rect that corresponds to @p category
      */
    const QRect &cacheCategory(const QString &category);

    /**
      * Returns the rect that corresponds to @p index
      * @note If the rect is not cached, it becomes cached
      */
    const QRect &cachedRectIndex(const QModelIndex &index);

    /**
      * Returns the rect that corresponds to @p category
      * @note If the rect is not cached, it becomes cached
      */
    const QRect &cachedRectCategory(const QString &category);

    /**
      * Returns the visual rect (taking in count x and y offsets) for @p index
      * @note If the rect is not cached, it becomes cached
      */
    QRect visualRect(const QModelIndex &index);

    /**
      * Returns the visual rect (taking in count x and y offsets) for @p category
      * @note If the rect is not cached, it becomes cached
      */
    QRect categoryVisualRect(const QString &category);

    /**
      * This method will draw a new category represented by index
      * @param index  on the rect specified by @p option.rect, with
      * painter @p painter
      */
    void drawNewCategory(const QModelIndex &index,
                         int sortRole,
                         const QStyleOption &option,
                         QPainter *painter);

    /**
      * This method will update scrollbars ranges. Called when our model changes
      * or when the view is resized
      */
    void updateScrollbars();

    /**
      * This method will draw dragged items in the painting operation
      */
    void drawDraggedItems(QPainter *painter);

    /**
      * This method will determine which rect needs to be updated because of a
      * dragging operation
      */
    void drawDraggedItems();


    // Attributes

    struct ElementInfo
    {
        QString category;
        int relativeOffsetToCategory;
    };

    // Basic data
    KListView *listView;
    KItemCategorizer *itemCategorizer;

    // Behavior data
    bool mouseButtonPressed;
    bool isDragging;
    bool dragLeftViewport;
    QModelIndex hovered;
    QString hoveredCategory;
    QPoint initialPressPosition;
    QPoint mousePosition;
    QItemSelection lastSelection;
    QModelIndex currentViewIndex;
    int forcedSelectionPosition;

    // Cache data
    // We cannot merge some of them into structs because it would affect
    // performance
    QHash<QModelIndex, struct ElementInfo> elementsInfo; // in source model
    QHash<QModelIndex, QRect> elementsPosition;          // in source model
    QHash<QModelIndex, QModelIndex> elementDictionary;   // mapped indexes
    QHash<QModelIndex, QModelIndex> invertedElementDictionary;   // mapped indexes
    QHash<QString, QModelIndexList> categoriesIndexes;
    QHash<QString, QRect> categoriesPosition;
    QStringList categories;
    QModelIndexList intersectedIndexes;
    QRect lastDraggedItemsRect;
    QRect lastSelectionRect;

    // Attributes for speed reasons
    KSortFilterProxyModel *proxyModel;
    QModelIndexList sourceModelIndexList;                // in source model
    QModelIndex lastIndex;
};

#endif // KLISTVIEW_P_H
