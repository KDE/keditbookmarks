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

#include "kitemlistviewlayouter_p.h"

#include "kitemmodelbase.h"
#include "kitemlistsizehintresolver_p.h"

#include <KDebug>

#define KITEMLISTVIEWLAYOUTER_DEBUG

namespace {
    // TODO: Calculate dynamically
    const int GroupHeaderHeight = 50;
};

KItemListViewLayouter::KItemListViewLayouter(QObject* parent) :
    QObject(parent),
    m_dirty(true),
    m_visibleIndexesDirty(true),
    m_scrollOrientation(Qt::Vertical),
    m_size(),
    m_itemSize(128, 128),
    m_headerHeight(0),
    m_model(0),
    m_sizeHintResolver(0),
    m_scrollOffset(0),
    m_maximumScrollOffset(0),
    m_itemOffset(0),
    m_maximumItemOffset(0),
    m_firstVisibleIndex(-1),
    m_lastVisibleIndex(-1),
    m_columnWidth(0),
    m_xPosInc(0),
    m_columnCount(0),
    m_groups(),
    m_groupIndexes(),
    m_itemBoundingRects()
{
}

KItemListViewLayouter::~KItemListViewLayouter()
{
}

void KItemListViewLayouter::setScrollOrientation(Qt::Orientation orientation)
{
    if (m_scrollOrientation != orientation) {
        m_scrollOrientation = orientation;
        m_dirty = true;
    }
}

Qt::Orientation KItemListViewLayouter::scrollOrientation() const
{
    return m_scrollOrientation;
}

void KItemListViewLayouter::setSize(const QSizeF& size)
{
    if (m_size != size) {
        m_size = size;
        m_dirty = true;
    }
}

QSizeF KItemListViewLayouter::size() const
{
    return m_size;
}

void KItemListViewLayouter::setItemSize(const QSizeF& size)
{
    if (m_itemSize != size) {
        m_itemSize = size;
        m_dirty = true;
    }
}

QSizeF KItemListViewLayouter::itemSize() const
{
    return m_itemSize;
}

void KItemListViewLayouter::setHeaderHeight(qreal height)
{
    if (m_headerHeight != height) {
        m_headerHeight = height;
        m_dirty = true;
    }
}

qreal KItemListViewLayouter::headerHeight() const
{
    return m_headerHeight;
}

void KItemListViewLayouter::setScrollOffset(qreal offset)
{
    if (m_scrollOffset != offset) {
        m_scrollOffset = offset;
        m_visibleIndexesDirty = true;
    }
}

qreal KItemListViewLayouter::scrollOffset() const
{
    return m_scrollOffset;
}

qreal KItemListViewLayouter::maximumScrollOffset() const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    return m_maximumScrollOffset;
}

void KItemListViewLayouter::setItemOffset(qreal offset)
{
    if (m_itemOffset != offset) {
        m_itemOffset = offset;
        m_visibleIndexesDirty = true;
    }
}

qreal KItemListViewLayouter::itemOffset() const
{
    return m_itemOffset;
}

qreal KItemListViewLayouter::maximumItemOffset() const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    return m_maximumItemOffset;
}

void KItemListViewLayouter::setModel(const KItemModelBase* model)
{
    if (m_model != model) {
        m_model = model;
        m_dirty = true;
    }
}

const KItemModelBase* KItemListViewLayouter::model() const
{
    return m_model;
}

void KItemListViewLayouter::setSizeHintResolver(const KItemListSizeHintResolver* sizeHintResolver)
{
    if (m_sizeHintResolver != sizeHintResolver) {
        m_sizeHintResolver = sizeHintResolver;
        m_dirty = true;
    }
}

const KItemListSizeHintResolver* KItemListViewLayouter::sizeHintResolver() const
{
    return m_sizeHintResolver;
}

int KItemListViewLayouter::firstVisibleIndex() const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    return m_firstVisibleIndex;
}

int KItemListViewLayouter::lastVisibleIndex() const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    return m_lastVisibleIndex;
}

QRectF KItemListViewLayouter::itemBoundingRect(int index) const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();
    if (index < 0 || index >= m_itemBoundingRects.count()) {
        return QRectF();
    }

    if (m_scrollOrientation == Qt::Horizontal) {
        // Rotate the logical direction which is always vertical by 90°
        // to get the physical horizontal direction
        const QRectF& b = m_itemBoundingRects[index];
        QRectF bounds(b.y(), b.x(), b.height(), b.width());
        QPointF pos = bounds.topLeft();
        pos.rx() -= m_scrollOffset;
        bounds.moveTo(pos);
        return bounds;
    }

    QRectF bounds = m_itemBoundingRects[index];
    bounds.moveTo(bounds.topLeft() - QPointF(m_itemOffset, m_scrollOffset));
    return bounds;
}

int KItemListViewLayouter::maximumVisibleItems() const
{
    const_cast<KItemListViewLayouter*>(this)->doLayout();

    const int height = static_cast<int>(m_size.height());
    const int rowHeight = static_cast<int>(m_itemSize.height());
    int rows = height / rowHeight;
    if (height % rowHeight != 0) {
        ++rows;
    }

    return rows * m_columnCount;
}

int KItemListViewLayouter::itemsPerOffset() const
{
    return m_columnCount;
}

bool KItemListViewLayouter::isFirstGroupItem(int itemIndex) const
{
    return m_groupIndexes.contains(itemIndex);
}

void KItemListViewLayouter::markAsDirty()
{
    m_dirty = true;
}

void KItemListViewLayouter::doLayout()
{
    if (m_dirty) {
#ifdef KITEMLISTVIEWLAYOUTER_DEBUG
        QElapsedTimer timer;
        timer.start();
#endif
        m_visibleIndexesDirty = true;

        QSizeF itemSize = m_itemSize;
        QSizeF size = m_size;

        const bool horizontalScrolling = (m_scrollOrientation == Qt::Horizontal);
        if (horizontalScrolling) {
            itemSize.setWidth(m_itemSize.height());
            itemSize.setHeight(m_itemSize.width());
            size.setWidth(m_size.height());
            size.setHeight(m_size.width());
        }

        m_columnWidth = itemSize.width();
        m_columnCount = qMax(1, int(size.width() / m_columnWidth));
        m_xPosInc = 0;

        const int itemCount = m_model->count();
        if (itemCount > m_columnCount) {
            // Apply the unused width equally to each column
            const qreal unusedWidth = size.width() - m_columnCount * m_columnWidth;
            if (unusedWidth > 0) {
                const qreal columnInc = unusedWidth / (m_columnCount + 1);
                m_columnWidth += columnInc;
                m_xPosInc += columnInc;
            }
        }

        int rowCount = itemCount / m_columnCount;
        if (itemCount % m_columnCount != 0) {
            ++rowCount;
        }

        m_itemBoundingRects.reserve(itemCount);

        qreal y = m_headerHeight;
        int rowIndex = 0;

        const bool grouped = createGroupHeaders();
        int groupIndex = 0;
        int firstItemIndexOfGroup = 0;

        int index = 0;
        while (index < itemCount) {
            qreal x = m_xPosInc;
            qreal maxItemHeight = itemSize.height();

            if (grouped) {
                if (horizontalScrolling) {
                    // All group headers will always be aligned on the top and not
                    // flipped like the other properties
                    x += GroupHeaderHeight;
                }

                if (index == firstItemIndexOfGroup) {
                    if (!horizontalScrolling) {
                        // The item is the first item of a group.
                        // Increase the y-position to provide space
                        // for the group header.
                        y += GroupHeaderHeight;
                    }

                    // Calculate the index of the first item for
                    // the next group
                    ++groupIndex;
                    if (groupIndex < m_groups.count()) {
                        firstItemIndexOfGroup = m_groups.at(groupIndex);
                    } else {
                        firstItemIndexOfGroup = -1;
                    }
                }
            }

            int column = 0;
            while (index < itemCount && column < m_columnCount) {
                qreal requiredItemHeight = itemSize.height();
                if (m_sizeHintResolver) {
                    const QSizeF sizeHint = m_sizeHintResolver->sizeHint(index);
                    const qreal sizeHintHeight = horizontalScrolling ? sizeHint.width() : sizeHint.height();
                    if (sizeHintHeight > requiredItemHeight) {
                        requiredItemHeight = sizeHintHeight;
                    }
                }

                const QRectF bounds(x, y, itemSize.width(), requiredItemHeight);
                if (index < m_itemBoundingRects.count()) {
                    m_itemBoundingRects[index] = bounds;
                } else {
                    m_itemBoundingRects.append(bounds);
                }

                maxItemHeight = qMax(maxItemHeight, requiredItemHeight);
                x += m_columnWidth;
                ++index;
                ++column;

                if (grouped && index == firstItemIndexOfGroup) {
                    // The item represents the first index of a group
                    // and must aligned in the first column
                    break;
                }
            }

            y += maxItemHeight;
            ++rowIndex;
        }
        if (m_itemBoundingRects.count() > itemCount) {
            m_itemBoundingRects.erase(m_itemBoundingRects.begin() + itemCount,
                                      m_itemBoundingRects.end());
        }

        if (itemCount > 0) {
            m_maximumScrollOffset = m_itemBoundingRects.last().bottom();
            m_maximumItemOffset = m_columnCount * m_columnWidth;
        } else {
            m_maximumScrollOffset = 0;
            m_maximumItemOffset = 0;
        }

#ifdef KITEMLISTVIEWLAYOUTER_DEBUG
        kDebug() << "[TIME] doLayout() for " << m_model->count() << "items:" << timer.elapsed();
#endif
        m_dirty = false;
    }

    updateVisibleIndexes();
}

void KItemListViewLayouter::updateVisibleIndexes()
{
    if (!m_visibleIndexesDirty) {
        return;
    }

    Q_ASSERT(!m_dirty);

    if (m_model->count() <= 0) {
        m_firstVisibleIndex = -1;
        m_lastVisibleIndex = -1;
        m_visibleIndexesDirty = false;
        return;
    }

    const int maxIndex = m_model->count() - 1;

    // Calculate the first visible index that is (at least partly) visible
    int min = 0;
    int max = maxIndex;
    int mid = 0;
    do {
        mid = (min + max) / 2;
        if (m_itemBoundingRects[mid].bottom() < m_scrollOffset) {
            min = mid + 1;
        } else {
            max = mid - 1;
        }
    } while (min <= max);

    while (mid < maxIndex && m_itemBoundingRects[mid].bottom() < m_scrollOffset) {
        ++mid;
    }
    m_firstVisibleIndex = mid;

    // Calculate the last visible index that is (at least partly) visible
    const int visibleHeight = (m_scrollOrientation == Qt::Horizontal) ? m_size.width() : m_size.height();
    qreal bottom = m_scrollOffset + visibleHeight;
    if (m_model->groupedSorting()) {
        bottom += GroupHeaderHeight;
    }

    min = m_firstVisibleIndex;
    max = maxIndex;
    do {
        mid = (min + max) / 2;
        if (m_itemBoundingRects[mid].y() <= bottom) {
            min = mid + 1;
        } else {
            max = mid - 1;
        }
    } while (min <= max);

    while (mid > 0 && m_itemBoundingRects[mid].y() > bottom) {
        --mid;
    }
    m_lastVisibleIndex = mid;

    m_visibleIndexesDirty = false;
}

bool KItemListViewLayouter::createGroupHeaders()
{
    if (!m_model->groupedSorting()) {
        return false;
    }

    m_groups.clear();
    m_groupIndexes.clear();

    const QList<QPair<int, QVariant> > groups = m_model->groups();
    if (groups.isEmpty()) {
        return false;
    }

    m_groups.reserve(groups.count());
    for (int i = 0; i < groups.count(); ++i) {
        const int firstItemIndex = groups.at(i).first;
        m_groups.append(firstItemIndex);
        m_groupIndexes.insert(firstItemIndex);
    }

    return true;
}

#include "kitemlistviewlayouter_p.moc"
