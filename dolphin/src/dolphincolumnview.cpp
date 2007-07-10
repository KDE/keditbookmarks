/***************************************************************************
 *   Copyright (C) 2007 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "dolphincolumnview.h"

#include "dolphincontroller.h"
#include "dolphinsettings.h"

#include "dolphin_columnmodesettings.h"

#include <kcolorutils.h>
#include <kcolorscheme.h>
#include <kdirlister.h>
#include <kdirmodel.h>

#include <QAbstractProxyModel>
#include <QPoint>

/**
 * Represents one column inside the DolphinColumnView and has been
 * extended to respect view options and hovering information.
 */
class ColumnWidget : public QListView
{
public:
    ColumnWidget(QWidget* parent,
                 DolphinColumnView* columnView,
                 const KUrl& url);
    virtual ~ColumnWidget();

    /** Sets the size of the icons. */
    void setDecorationSize(const QSize& size);

    /**
     * An active column is defined as column, which shows the same URL
     * as indicated by the URL navigator. The active column is usually
     * drawn in a lighter color. All operations are applied to this column.
     */
    void setActive(bool active);

    inline const KUrl& url() const;

protected:
    virtual QStyleOptionViewItem viewOptions() const;
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragLeaveEvent(QDragLeaveEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);

private:
    /** Used by ColumnWidget::setActive(). */
    void activate();

    /** Used by ColumnWidget::setActive(). */
    void deactivate();

private:
    bool m_active;
    DolphinColumnView* m_view;
    KUrl m_url;
    QStyleOptionViewItem m_viewOptions;

    bool m_dragging;   // TODO: remove this property when the issue #160611 is solved in Qt 4.4
    QRect m_dropRect;  // TODO: remove this property when the issue #160611 is solved in Qt 4.4
};

ColumnWidget::ColumnWidget(QWidget* parent,
                           DolphinColumnView* columnView,
                           const KUrl& url) :
    QListView(parent),
    m_active(true),
    m_view(columnView),
    m_url(url),
    m_dragging(false),
    m_dropRect()
{
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);

    setMouseTracking(true);
    viewport()->setAttribute(Qt::WA_Hover);

    // apply the column mode settings to the widget
    const ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    Q_ASSERT(settings != 0);

    m_viewOptions = QListView::viewOptions();

    QFont font(settings->fontFamily(), settings->fontSize());
    font.setItalic(settings->italicFont());
    font.setBold(settings->boldFont());
    m_viewOptions.font = font;

    const int iconSize = settings->iconSize();
    m_viewOptions.decorationSize = QSize(iconSize, iconSize);

    activate();
}

ColumnWidget::~ColumnWidget()
{
}

void ColumnWidget::setDecorationSize(const QSize& size)
{
    m_viewOptions.decorationSize = size;
    doItemsLayout();
}

void ColumnWidget::setActive(bool active)
{
    if (m_active == active) {
        return;
    }

    m_active = active;

    if (active) {
        activate();
    } else {
        deactivate();
    }
}

const KUrl& ColumnWidget::url() const
{
    return m_url;
}

QStyleOptionViewItem ColumnWidget::viewOptions() const
{
    return m_viewOptions;
}

void ColumnWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }

    m_dragging = true;
}

void ColumnWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    QListView::dragLeaveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    m_dragging = false;
    setDirtyRegion(m_dropRect);
}

void ColumnWidget::dragMoveEvent(QDragMoveEvent* event)
{
    QListView::dragMoveEvent(event);

   // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    const QModelIndex index = indexAt(event->pos());
    setDirtyRegion(m_dropRect);
    m_dropRect = visualRect(index);
    setDirtyRegion(m_dropRect);
}

void ColumnWidget::dropEvent(QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (!urls.isEmpty()) {
        event->acceptProposedAction();
        m_view->m_controller->indicateDroppedUrls(urls,
                                                        indexAt(event->pos()),
                                                        event->source());
    }
    QListView::dropEvent(event);
    m_dragging = false;
}

void ColumnWidget::mouseReleaseEvent(QMouseEvent* event)
{
    m_view->requestActivation(this);
    QListView::mouseReleaseEvent(event);
}

void ColumnWidget::paintEvent(QPaintEvent* event)
{
    QListView::paintEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    if (m_dragging) {
        const QBrush& brush = m_viewOptions.palette.brush(QPalette::Normal, QPalette::Highlight);
        DolphinController::drawHoverIndication(viewport(), m_dropRect, brush);
    }
}

void ColumnWidget::contextMenuEvent(QContextMenuEvent* event)
{
    m_view->requestActivation(this);

    QListView::contextMenuEvent(event);

    const QModelIndex index = indexAt(event->pos());
    if (index.isValid() || m_active) {
        // Only open a context menu above an item or if the mouse is above
        // the active column.
        const QPoint pos = m_view->viewport()->mapFromGlobal(event->globalPos());
        m_view->m_controller->triggerContextMenuRequest(pos);
    }
}

void ColumnWidget::activate()
{
    const QColor bgColor = KColorScheme(KColorScheme::View).background();
    QPalette palette = viewport()->palette();
    palette.setColor(viewport()->backgroundRole(), bgColor);
    viewport()->setPalette(palette);

    setSelectionMode(MultiSelection);
    update();
}

void ColumnWidget::deactivate()
{
    QColor bgColor = KColorScheme(KColorScheme::View).background();
    const QColor fgColor = KColorScheme(KColorScheme::View).foreground();
    bgColor = KColorUtils::mix(bgColor, fgColor, 0.04);

    QPalette palette = viewport()->palette();
    palette.setColor(viewport()->backgroundRole(), bgColor);
    viewport()->setPalette(palette);

    setSelectionMode(SingleSelection);
    update();
}

// ---

DolphinColumnView::DolphinColumnView(QWidget* parent, DolphinController* controller) :
    QColumnView(parent),
    m_controller(controller)
{
    Q_ASSERT(controller != 0);

    setAcceptDrops(true);
    setSelectionBehavior(SelectItems);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);

    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(const QModelIndex&)),
                this, SLOT(triggerItem(const QModelIndex&)));
    } else {
        connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                this, SLOT(triggerItem(const QModelIndex&)));
    }
    connect(this, SIGNAL(entered(const QModelIndex&)),
            controller, SLOT(emitItemEntered(const QModelIndex&)));
    connect(this, SIGNAL(viewportEntered()),
            controller, SLOT(emitViewportEntered()));
    connect(controller, SIGNAL(zoomIn()),
            this, SLOT(zoomIn()));
    connect(controller, SIGNAL(zoomOut()),
            this, SLOT(zoomOut()));
    connect(controller, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(updateColumnsState(const KUrl&)));

    updateDecorationSize();
}

DolphinColumnView::~DolphinColumnView()
{
}

QAbstractItemView* DolphinColumnView::createColumn(const QModelIndex& index)
{
    // let the column widget be aware about its URL...
    KUrl columnUrl;
    if (viewport()->children().count() == 0) {
        // For the first column widget the directory lister has not been started
        // yet, hence use the URL from the controller instead.
        columnUrl = m_controller->url();
    } else {
        const QAbstractProxyModel* proxyModel = static_cast<const QAbstractProxyModel*>(model());
        const KDirModel* dirModel = static_cast<const KDirModel*>(proxyModel->sourceModel());

        const QModelIndex dirModelIndex = proxyModel->mapToSource(index);
        KFileItem* fileItem = dirModel->itemForIndex(dirModelIndex);
        if (fileItem != 0) {
            columnUrl = fileItem->url();
        }
    }

    ColumnWidget* view = new ColumnWidget(viewport(), this, columnUrl);

    // The following code has been copied 1:1 from QColumnView::createColumn().
    // Copyright (C) 1992-2007 Trolltech ASA. In Qt 4.4 the new method
    // QColumnView::initializeColumn() will be available for this.

    view->setFrameShape(QFrame::NoFrame);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setMinimumWidth(100);
    view->setAttribute(Qt::WA_MacShowFocusRect, false);

    // copy the 'view' behavior
    view->setDragDropMode(dragDropMode());
    view->setDragDropOverwriteMode(dragDropOverwriteMode());
    view->setDropIndicatorShown(showDropIndicator());
    view->setAlternatingRowColors(alternatingRowColors());
    view->setAutoScroll(hasAutoScroll());
    view->setEditTriggers(editTriggers());
    view->setHorizontalScrollMode(horizontalScrollMode());
    view->setIconSize(iconSize());
    view->setSelectionBehavior(selectionBehavior());
    view->setSelectionMode(selectionMode());
    view->setTabKeyNavigation(tabKeyNavigation());
    view->setTextElideMode(textElideMode());
    view->setVerticalScrollMode(verticalScrollMode());

    view->setModel(model());

    // set the delegate to be the columnview delegate
    QAbstractItemDelegate* delegate = view->itemDelegate();
    view->setItemDelegate(itemDelegate());
    delete delegate;

    view->setRootIndex(index);

    if (model()->canFetchMore(index)) {
        model()->fetchMore(index);
    }

    return view;
}

void DolphinColumnView::mousePressEvent(QMouseEvent* event)
{
    m_controller->triggerActivation();
    QColumnView::mousePressEvent(event);
}

void DolphinColumnView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void DolphinColumnView::dropEvent(QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (!urls.isEmpty()) {
        m_controller->indicateDroppedUrls(urls,
                                          indexAt(event->pos()),
                                          event->source());
        event->acceptProposedAction();
    }
    QColumnView::dropEvent(event);
}

void DolphinColumnView::zoomIn()
{
    if (isZoomInPossible()) {
        ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
        // TODO: get rid of K3Icon sizes
        switch (settings->iconSize()) {
        case K3Icon::SizeSmall:  settings->setIconSize(K3Icon::SizeMedium); break;
        case K3Icon::SizeMedium: settings->setIconSize(K3Icon::SizeLarge); break;
        default: Q_ASSERT(false); break;
        }
        updateDecorationSize();
    }
}

void DolphinColumnView::zoomOut()
{
    if (isZoomOutPossible()) {
        ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
        // TODO: get rid of K3Icon sizes
        switch (settings->iconSize()) {
        case K3Icon::SizeLarge:  settings->setIconSize(K3Icon::SizeMedium); break;
        case K3Icon::SizeMedium: settings->setIconSize(K3Icon::SizeSmall); break;
        default: Q_ASSERT(false); break;
        }
        updateDecorationSize();
    }
}

void DolphinColumnView::triggerItem(const QModelIndex& index)
{
    m_controller->triggerItem(index);
    updateColumnsState(m_controller->url());
}

void DolphinColumnView::updateColumnsState(const KUrl& url)
{
    foreach (QObject* object, viewport()->children()) {
        if (object->inherits("QListView")) {
            ColumnWidget* widget = static_cast<ColumnWidget*>(object);
            widget->setActive(widget->url() == url);
        }
    }
}

bool DolphinColumnView::isZoomInPossible() const
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    return settings->iconSize() < K3Icon::SizeLarge;
}

bool DolphinColumnView::isZoomOutPossible() const
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    return settings->iconSize() > K3Icon::SizeSmall;
}

void DolphinColumnView::requestActivation(QWidget* column)
{
    foreach (QObject* object, viewport()->children()) {
        if (object->inherits("QListView")) {
            ColumnWidget* widget = static_cast<ColumnWidget*>(object);
            const bool isActive = (widget == column);
            widget->setActive(isActive);
            if (isActive) {
                m_controller->setUrl(widget->url());
            }
        }
    }
}

void DolphinColumnView::updateDecorationSize()
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    const int iconSize = settings->iconSize();

    foreach (QObject* object, viewport()->children()) {
        if (object->inherits("QListView")) {
            ColumnWidget* widget = static_cast<ColumnWidget*>(object);
            widget->setDecorationSize(QSize(iconSize, iconSize));
        }
    }

    m_controller->setZoomInPossible(isZoomInPossible());
    m_controller->setZoomOutPossible(isZoomOutPossible());

    doItemsLayout();
}

#include "dolphincolumnview.moc"
