/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef KITEMLISTVIEWLAYOUTER_H
#define KITEMLISTVIEWLAYOUTER_H

#include <libdolphin_export.h>

#include <QObject>
#include <QRectF>
#include <QSet>
#include <QSizeF>

class KItemModelBase;
class KItemListSizeHintResolver;

class LIBDOLPHINPRIVATE_EXPORT KItemListViewLayouter : public QObject
{
    Q_OBJECT

public:
    KItemListViewLayouter(QObject* parent = 0);
    virtual ~KItemListViewLayouter();

    void setScrollOrientation(Qt::Orientation orientation);
    Qt::Orientation scrollOrientation() const;

    void setSize(const QSizeF& size);
    QSizeF size() const;

    void setItemSize(const QSizeF& size);
    QSizeF itemSize() const;

    // TODO: add note that offset can be < 0 or > maximumOffset!
    void setOffset(qreal offset);
    qreal offset() const;

    void setModel(const KItemModelBase* model);
    const KItemModelBase* model() const;

    void setSizeHintResolver(const KItemListSizeHintResolver* sizeHintResolver);
    const KItemListSizeHintResolver* sizeHintResolver() const;

    qreal maximumOffset() const;

    // TODO: mention that return value is -1 if count == 0
    int firstVisibleIndex() const;

    // TODO: mention that return value is -1 if count == 0
    int lastVisibleIndex() const;

    QRectF itemBoundingRect(int index) const;

    int maximumVisibleItems() const;

    /**
     * @return True if the item with the index \p itemIndex
     *         is the first item within a group.
     */
    bool isFirstGroupItem(int itemIndex) const;

    void markAsDirty();

private:
    void doLayout();

    void updateVisibleIndexes();
    void updateGroupedVisibleIndexes();
    void createGroupHeaders();

private:
    bool m_dirty;
    bool m_visibleIndexesDirty;
    bool m_grouped;

    Qt::Orientation m_scrollOrientation;
    QSizeF m_size;

    QSizeF m_itemSize;
    const KItemModelBase* m_model;
    const KItemListSizeHintResolver* m_sizeHintResolver;

    qreal m_offset;
    qreal m_maximumOffset;

    int m_firstVisibleIndex;
    int m_lastVisibleIndex;

    int m_firstVisibleGroupIndex;

    qreal m_columnWidth;
    qreal m_xPosInc;
    int m_columnCount;

    struct ItemGroup {
        int firstItemIndex;
        qreal y;
    };
    QList<ItemGroup> m_groups;

    // Stores all item indexes that are the first item of a group.
    // Assures fast access for KItemListViewLayouter::isFirstGroupItem().
    QSet<int> m_groupIndexes;

    QList<QRectF> m_itemBoundingRects;
};

#endif


