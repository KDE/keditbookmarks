/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs:                *
 *   http://qt.gitorious.org/qt-labs/itemviews-ng                          *
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

#ifndef KITEMLISTWIDGET_H
#define KITEMLISTWIDGET_H

#include <libdolphin_export.h>

#include <kitemviews/kitemliststyleoption.h>

#include <QGraphicsWidget>

class QPropertyAnimation;

/**
 * @brief Widget that shows a visible item from the model.
 *
 * For showing an item from a custom model it is required to at least overwrite KItemListWidget::paint().
 * All properties are set by KItemListView, for each property there is a corresponding
 * virtual protected method that allows to react on property changes.
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    KItemListWidget(QGraphicsItem* parent);
    virtual ~KItemListWidget();

    void setIndex(int index);
    int index() const;

    void setData(const QHash<QByteArray, QVariant>& data, const QSet<QByteArray>& roles = QSet<QByteArray>());
    QHash<QByteArray, QVariant> data() const;

    /**
     * Draws the hover-bounding-rectangle if the item is hovered. Overwrite this method
     * to show the data of the custom model provided by KItemListWidget::data().
     * @reimp
     */
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

    /**
     * Sets the visible roles to \p roles. The integer-value defines
     * the order of the visible role: Smaller values are ordered first.
     */
    void setVisibleRoles(const QHash<QByteArray, int>& roles);
    QHash<QByteArray, int> visibleRoles() const;

    void setVisibleRolesSizes(const QHash<QByteArray, QSizeF> rolesSizes);
    QHash<QByteArray, QSizeF> visibleRolesSizes() const;

    void setStyleOption(const KItemListStyleOption& option);
    const KItemListStyleOption& styleOption() const;

    /**
     * @return True if \a point is inside KItemListWidget::hoverBoundingRect(),
     *         KItemListWidget::selectionToggleRect() or KItemListWidget::expansionToggleRect().
     * @reimp
     */
    virtual bool contains(const QPointF& point) const;

    /**
     * @return Bounding rectangle for the area that acts as hovering-area. Per default
     *         the bounding rectangle of the KItemListWidget is returned.
     */
    virtual QRectF hoverBoundingRect() const;

    /**
     * @return Rectangle for the selection-toggle that is used to select or deselect an item.
     *         Per default an empty rectangle is returned which means that no selection-toggle
     *         is available.
     */
    virtual QRectF selectionToggleRect() const;

    /**
     * @return Rectangle for the expansion-toggle that is used to open a sub-tree of the model.
     *         Per default an empty rectangle is returned which means that no opening of sub-trees
     *         is supported.
     */
    virtual QRectF expansionToggleRect() const;

protected:
    virtual void dataChanged(const QHash<QByteArray, QVariant>& current, const QSet<QByteArray>& roles = QSet<QByteArray>());
    virtual void visibleRolesChanged(const QHash<QByteArray, int>& current, const QHash<QByteArray, int>& previous);
    virtual void visibleRolesSizesChanged(const QHash<QByteArray, QSizeF>& current, const QHash<QByteArray, QSizeF>& previous);
    virtual void styleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous);
    virtual void resizeEvent(QGraphicsSceneResizeEvent* event);

    /**
     * @return The current opacity of the hover-animation. When implementing a custom painting-code for a hover-state
     *         this opacity value should be respected.
     */
    qreal hoverOpacity() const;

private:
    void setHoverOpacity(qreal opacity);
    void clearCache();

private:
    Q_PROPERTY(qreal hoverOpacity READ hoverOpacity WRITE setHoverOpacity)

    int m_index;
    QHash<QByteArray, QVariant> m_data;
    QHash<QByteArray, int> m_visibleRoles;
    QHash<QByteArray, QSizeF> m_visibleRolesSizes;
    KItemListStyleOption m_styleOption;

    qreal m_hoverOpacity;
    mutable QPixmap* m_hoverCache;
    QPropertyAnimation* m_hoverAnimation;
};
#endif


