/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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

#include "dolphindetailsview.h"

#include "dolphinmodel.h"
#include "dolphincontroller.h"
#include "dolphinsettings.h"
#include "dolphinsortfilterproxymodel.h"
#include "draganddrophelper.h"
#include "selectionmanager.h"
#include "viewproperties.h"

#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"

#include <kdirmodel.h>
#include <klocale.h>
#include <kmenu.h>

#include <QAbstractProxyModel>
#include <QAction>
#include <QApplication>
#include <QHeaderView>
#include <QRubberBand>
#include <QPainter>
#include <QScrollBar>

DolphinDetailsView::DolphinDetailsView(QWidget* parent, DolphinController* controller) :
    QTreeView(parent),
	m_autoResize(true),
    m_controller(controller),
    m_selectionManager(0),
    m_font(),
    m_decorationSize(),
    m_showElasticBand(false),
    m_elasticBandOrigin(),
    m_elasticBandDestination()
{
    const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    Q_ASSERT(settings != 0);
    Q_ASSERT(controller != 0);

    setAcceptDrops(true);
    setSortingEnabled(true);
    setUniformRowHeights(true);
    setSelectionBehavior(SelectItems);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);
    setAlternatingRowColors(true);
    setRootIsDecorated(settings->expandableFolders());
    setItemsExpandable(settings->expandableFolders());
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    setMouseTracking(true);

    const ViewProperties props(controller->url());
    setSortIndicatorSection(props.sorting());
    setSortIndicatorOrder(props.sortOrder());

    QHeaderView* headerView = header();
    connect(headerView, SIGNAL(sectionClicked(int)),
            this, SLOT(synchronizeSortingState(int)));
    headerView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(headerView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(configureColumns(const QPoint&)));
    connect(headerView, SIGNAL(sectionResized(int, int, int)),
            this, SLOT(slotHeaderSectionResized(int, int, int)));
    connect(headerView, SIGNAL(sectionHandleDoubleClicked(int)),
            this, SLOT(disableAutoResizing()));

    connect(parent, SIGNAL(sortingChanged(DolphinView::Sorting)),
            this, SLOT(setSortIndicatorSection(DolphinView::Sorting)));
    connect(parent, SIGNAL(sortOrderChanged(Qt::SortOrder)),
            this, SLOT(setSortIndicatorOrder(Qt::SortOrder)));

    // TODO: Connecting to the signal 'activated()' is not possible, as kstyle
    // does not forward the single vs. doubleclick to it yet (KDE 4.1?). Hence it is
    // necessary connecting the signal 'singleClick()' or 'doubleClick' and to handle the
    // RETURN-key in keyPressEvent().
    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(const QModelIndex&)),
                controller, SLOT(triggerItem(const QModelIndex&)));
        if (DolphinSettings::instance().generalSettings()->showSelectionToggle()) {
            m_selectionManager = new SelectionManager(this);
            connect(m_selectionManager, SIGNAL(selectionChanged()),
                    this, SLOT(requestActivation()));
            connect(m_controller, SIGNAL(urlChanged(const KUrl&)),
                    m_selectionManager, SLOT(reset()));
         }
    } else {
        connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                controller, SLOT(triggerItem(const QModelIndex&)));
    }
    connect(this, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(slotEntered(const QModelIndex&)));
    connect(this, SIGNAL(viewportEntered()),
            controller, SLOT(emitViewportEntered()));
    connect(controller, SIGNAL(zoomIn()),
            this, SLOT(zoomIn()));
    connect(controller, SIGNAL(zoomOut()),
            this, SLOT(zoomOut()));
    connect(controller->dolphinView(), SIGNAL(additionalInfoChanged()),
            this, SLOT(updateColumnVisibility()));

    if (settings->useSystemFont()) {
        m_font = KGlobalSettings::generalFont();
    } else {
        m_font = QFont(settings->fontFamily(),
                       settings->fontSize(),
                       settings->fontWeight(),
                       settings->italicFont());
    }

    setVerticalScrollMode(QTreeView::ScrollPerPixel);
    setHorizontalScrollMode(QTreeView::ScrollPerPixel);

    updateDecorationSize();

    setFocus();
    viewport()->installEventFilter(this);

    connect(KGlobalSettings::self(), SIGNAL(kdisplayFontChanged()),
            this, SLOT(updateFont()));
}

DolphinDetailsView::~DolphinDetailsView()
{
}

bool DolphinDetailsView::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        QHeaderView* headerView = header();
        headerView->setResizeMode(QHeaderView::Interactive);
        headerView->setMovable(false);

        updateColumnVisibility();

        hideColumn(DolphinModel::Rating);
        hideColumn(DolphinModel::Tags);
    } else if (event->type() == QEvent::UpdateRequest) {
        // a wheel movement will scroll 4 items
        if (model()->rowCount() > 0) {
            verticalScrollBar()->setSingleStep((sizeHintForRow(0) / 3) * 4);
        }
    }

    return QTreeView::event(event);
}

QStyleOptionViewItem DolphinDetailsView::viewOptions() const
{
    QStyleOptionViewItem viewOptions = QTreeView::viewOptions();
    viewOptions.font = m_font;
    viewOptions.showDecorationSelected = true;
    viewOptions.decorationSize = m_decorationSize;
    return viewOptions;
}

void DolphinDetailsView::contextMenuEvent(QContextMenuEvent* event)
{
    QTreeView::contextMenuEvent(event);
    m_controller->triggerContextMenuRequest(event->pos());
}

void DolphinDetailsView::mousePressEvent(QMouseEvent* event)
{
    m_controller->requestActivation();

    QTreeView::mousePressEvent(event);

    const QModelIndex index = indexAt(event->pos());
    if (!index.isValid() || (index.column() != DolphinModel::Name)) {
        const Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
        if (!(modifier & Qt::ShiftModifier) && !(modifier & Qt::ControlModifier)) {
            clearSelection();
        }
    }

    if (event->button() == Qt::LeftButton) {
        m_showElasticBand = true;

        const QPoint pos(contentsPos());
        m_elasticBandOrigin = event->pos();
        m_elasticBandOrigin.setX(m_elasticBandOrigin.x() + pos.x());
        m_elasticBandOrigin.setY(m_elasticBandOrigin.y() + pos.y());
        m_elasticBandDestination = event->pos();
    }
}

void DolphinDetailsView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_showElasticBand) {
        const QPoint mousePos = event->pos();
        const QModelIndex index = indexAt(mousePos);
        if (!index.isValid()) {
            // the destination of the selection rectangle is above the viewport. In this
            // case QTreeView does no selection at all, which is not the wanted behavior
            // in Dolphin -> select all items within the elastic band rectangle
            clearSelection();

            const int nameColumnWidth = header()->sectionSize(DolphinModel::Name);
            QRect selRect = QRect(m_elasticBandOrigin, m_elasticBandDestination).normalized();
            const QRect nameColumnsRect(0, 0, nameColumnWidth, viewport()->height());
            selRect = nameColumnsRect.intersected(selRect);

            setSelection(selRect, QItemSelectionModel::Select);
        }

        // TODO: enable QTreeView::mouseMoveEvent(event) again, as soon
        // as the Qt-issue #199631 has been fixed.
        // QTreeView::mouseMoveEvent(event);
        QAbstractItemView::mouseMoveEvent(event);
        updateElasticBand();
    } else {
        // TODO: enable QTreeView::mouseMoveEvent(event) again, as soon
        // as the Qt-issue #199631 has been fixed.
        // QTreeView::mouseMoveEvent(event);
        QAbstractItemView::mouseMoveEvent(event);
    }
}

void DolphinDetailsView::mouseReleaseEvent(QMouseEvent* event)
{
    QTreeView::mouseReleaseEvent(event);
    if (m_showElasticBand) {
        updateElasticBand();
        m_showElasticBand = false;
    }
}

void DolphinDetailsView::startDrag(Qt::DropActions supportedActions)
{
    DragAndDropHelper::startDrag(this, supportedActions);
}

void DolphinDetailsView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }

    if (m_showElasticBand) {
        updateElasticBand();
        m_showElasticBand = false;
    }
}

void DolphinDetailsView::dragLeaveEvent(QDragLeaveEvent* event)
{
    QTreeView::dragLeaveEvent(event);
    setDirtyRegion(m_dropRect);
}

void DolphinDetailsView::dragMoveEvent(QDragMoveEvent* event)
{
    QTreeView::dragMoveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    setDirtyRegion(m_dropRect);
    const QModelIndex index = indexAt(event->pos());
    if (index.isValid() && (index.column() == DolphinModel::Name)) {
        const KFileItem item = m_controller->itemForIndex(index);
        if (!item.isNull() && item.isDir()) {
            m_dropRect = visualRect(index);
        } else {
            m_dropRect.setSize(QSize()); // set as invalid
        }
        setDirtyRegion(m_dropRect);
    }

    if (event->mimeData()->hasUrls()) {
        // accept url drops, independently from the destination item
        event->acceptProposedAction();
    }
}

void DolphinDetailsView::dropEvent(QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (!urls.isEmpty()) {
        event->acceptProposedAction();
        const QModelIndex index = indexAt(event->pos());
        KFileItem item;
        if (index.isValid() && (index.column() == DolphinModel::Name)) {
            item = m_controller->itemForIndex(index);
        }
        m_controller->indicateDroppedUrls(urls,
                                          m_controller->url(),
                                          item);
    }
    QTreeView::dropEvent(event);
}

void DolphinDetailsView::paintEvent(QPaintEvent* event)
{
    QTreeView::paintEvent(event);
    if (m_showElasticBand) {
        // The following code has been taken from QListView
        // and adapted to DolphinDetailsView.
        // (C) 1992-2007 Trolltech ASA
        QStyleOptionRubberBand opt;
        opt.initFrom(this);
        opt.shape = QRubberBand::Rectangle;
        opt.opaque = false;
        opt.rect = elasticBandRect();

        QPainter painter(viewport());
        painter.save();
        style()->drawControl(QStyle::CE_RubberBand, &opt, &painter);
        painter.restore();
    }
}

void DolphinDetailsView::keyPressEvent(QKeyEvent* event)
{
    QTreeView::keyPressEvent(event);
    m_controller->handleKeyPressEvent(event);
}

void DolphinDetailsView::resizeEvent(QResizeEvent* event)
{
    if (m_autoResize) {
        resizeColumns();
    }
    QTreeView::resizeEvent(event);
}

void DolphinDetailsView::wheelEvent(QWheelEvent* event)
{
    if (m_selectionManager != 0) {
        m_selectionManager->reset();
    }

    // let Ctrl+wheel events propagate to the DolphinView for icon zooming
    if (event->modifiers() & Qt::ControlModifier) {
        event->ignore();
        return;
    }

    QTreeView::wheelEvent(event);
}

void DolphinDetailsView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    QTreeView::currentChanged(current, previous);

    // Stay consistent with QListView: When changing the current index by key presses,
    // also change the selection.
    if (QApplication::mouseButtons() == Qt::NoButton) {
        selectionModel()->select(current, QItemSelectionModel::ClearAndSelect);
    }
}

bool DolphinDetailsView::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == viewport()) && (event->type() == QEvent::Leave)) {
        // if the mouse is above an item and moved very fast outside the widget,
        // no viewportEntered() signal might be emitted although the mouse has been moved
        // above the viewport
        m_controller->emitViewportEntered();
    }

    return QTreeView::eventFilter(watched, event);
}

void DolphinDetailsView::setSortIndicatorSection(DolphinView::Sorting sorting)
{
    QHeaderView* headerView = header();
    headerView->setSortIndicator(sorting, headerView->sortIndicatorOrder());
}

void DolphinDetailsView::setSortIndicatorOrder(Qt::SortOrder sortOrder)
{
    QHeaderView* headerView = header();
    headerView->setSortIndicator(headerView->sortIndicatorSection(), sortOrder);
}

void DolphinDetailsView::synchronizeSortingState(int column)
{
    // The sorting has already been changed in QTreeView if this slot is
    // invoked, but Dolphin is not informed about this.
    DolphinView::Sorting sorting = DolphinSortFilterProxyModel::sortingForColumn(column);
    const Qt::SortOrder sortOrder = header()->sortIndicatorOrder();
    m_controller->indicateSortingChange(sorting);
    m_controller->indicateSortOrderChange(sortOrder);
}

void DolphinDetailsView::slotEntered(const QModelIndex& index)
{
    const QPoint pos = viewport()->mapFromGlobal(QCursor::pos());
    const int nameColumnWidth = header()->sectionSize(DolphinModel::Name);
    if (pos.x() < nameColumnWidth) {
        m_controller->emitItemEntered(index);
    }
    else {
        m_controller->emitViewportEntered();
    }
}

void DolphinDetailsView::updateElasticBand()
{
    if (m_showElasticBand) {
        QRect dirtyRegion(elasticBandRect());
        m_elasticBandDestination = viewport()->mapFromGlobal(QCursor::pos());
        dirtyRegion = dirtyRegion.united(elasticBandRect());
        setDirtyRegion(dirtyRegion);
    }
}

QRect DolphinDetailsView::elasticBandRect() const
{
    const QPoint pos(contentsPos());
    const QPoint topLeft(m_elasticBandOrigin.x() - pos.x(), m_elasticBandOrigin.y() - pos.y());
    return QRect(topLeft, m_elasticBandDestination).normalized();
}

void DolphinDetailsView::zoomIn()
{
    if (isZoomInPossible()) {
        DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
        switch (settings->iconSize()) {
        case KIconLoader::SizeSmall:  settings->setIconSize(KIconLoader::SizeMedium); break;
        case KIconLoader::SizeMedium: settings->setIconSize(KIconLoader::SizeLarge); break;
        default: Q_ASSERT(false); break;
        }
        updateDecorationSize();
    }
}

void DolphinDetailsView::zoomOut()
{
    if (isZoomOutPossible()) {
        DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
        switch (settings->iconSize()) {
        case KIconLoader::SizeLarge:  settings->setIconSize(KIconLoader::SizeMedium); break;
        case KIconLoader::SizeMedium: settings->setIconSize(KIconLoader::SizeSmall); break;
        default: Q_ASSERT(false); break;
        }
        updateDecorationSize();
    }
}

void DolphinDetailsView::configureColumns(const QPoint& pos)
{
    KMenu popup(this);
    popup.addTitle(i18nc("@title:menu", "Columns"));

    QHeaderView* headerView = header();
    for (int i = DolphinModel::Size; i <= DolphinModel::Type; ++i) {
        const int logicalIndex = headerView->logicalIndex(i);
        const QString text = model()->headerData(i, Qt::Horizontal).toString();
        QAction* action = popup.addAction(text);
        action->setCheckable(true);
        action->setChecked(!headerView->isSectionHidden(logicalIndex));
        action->setData(i);
    }

    QAction* activatedAction = popup.exec(header()->mapToGlobal(pos));
    if (activatedAction != 0) {
        const bool show = activatedAction->isChecked();
        const int columnIndex = activatedAction->data().toInt();

        KFileItemDelegate::InformationList list = m_controller->dolphinView()->additionalInfo();
        const KFileItemDelegate::Information info = infoForColumn(columnIndex);
        if (show) {
            Q_ASSERT(!list.contains(info));
            list.append(info);
        } else {
            Q_ASSERT(list.contains(info));
            const int index = list.indexOf(info);
            list.removeAt(index);
        }

        m_controller->indicateAdditionalInfoChange(list);
        setColumnHidden(columnIndex, !show);
    }
}

void DolphinDetailsView::updateColumnVisibility()
{
    const KFileItemDelegate::InformationList list = m_controller->dolphinView()->additionalInfo();
    for (int i = DolphinModel::Size; i <= DolphinModel::Type; ++i) {
        const KFileItemDelegate::Information info = infoForColumn(i);
        const bool hide = !list.contains(info);
        if (isColumnHidden(i) != hide) {
            setColumnHidden(i, hide);
        }
    }

    resizeColumns();
}

void DolphinDetailsView::slotHeaderSectionResized(int logicalIndex, int oldSize, int newSize)
{
    Q_UNUSED(logicalIndex);
    Q_UNUSED(oldSize);
    Q_UNUSED(newSize);
    if (QApplication::mouseButtons() & Qt::LeftButton) {
        disableAutoResizing();
    }
}

void DolphinDetailsView::disableAutoResizing()
{
    m_autoResize = false;
}

void DolphinDetailsView::requestActivation()
{
    m_controller->requestActivation();
}

void DolphinDetailsView::updateFont()
{
    const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    Q_ASSERT(settings != 0);

    if (settings->useSystemFont()) {
        m_font = KGlobalSettings::generalFont();
    }
}

bool DolphinDetailsView::isZoomInPossible() const
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    return settings->iconSize() < KIconLoader::SizeLarge;
}

bool DolphinDetailsView::isZoomOutPossible() const
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    return settings->iconSize() > KIconLoader::SizeSmall;
}

void DolphinDetailsView::updateDecorationSize()
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    const int iconSize = settings->iconSize();
    setIconSize(QSize(iconSize, iconSize));
    m_decorationSize = QSize(iconSize, iconSize);

    m_controller->setZoomInPossible(isZoomInPossible());
    m_controller->setZoomOutPossible(isZoomOutPossible());

    if (m_selectionManager != 0) {
        m_selectionManager->reset();
    }

    doItemsLayout();
}

QPoint DolphinDetailsView::contentsPos() const
{
    // implementation note: the horizonal position is ignored currently, as no
    // horizontal scrolling is done anyway during a selection
    const QScrollBar* scrollbar = verticalScrollBar();
    Q_ASSERT(scrollbar != 0);

    const int maxHeight = maximumViewportSize().height();
    const int height = scrollbar->maximum() - scrollbar->minimum() + 1;
    const int visibleHeight = model()->rowCount() + 1 - height;
    if (visibleHeight <= 0) {
        return QPoint(0, 0);
    }

    const int y = scrollbar->sliderPosition() * maxHeight / visibleHeight;
    return QPoint(0, y);
}

KFileItemDelegate::Information DolphinDetailsView::infoForColumn(int columnIndex) const
{
    KFileItemDelegate::Information info = KFileItemDelegate::NoInformation;

    switch (columnIndex) {
    case DolphinModel::Size:         info = KFileItemDelegate::Size; break;
    case DolphinModel::ModifiedTime: info = KFileItemDelegate::ModificationTime; break;
    case DolphinModel::Permissions:  info = KFileItemDelegate::Permissions; break;
    case DolphinModel::Owner:        info = KFileItemDelegate::Owner; break;
    case DolphinModel::Group:        info = KFileItemDelegate::OwnerAndGroup; break;
    case DolphinModel::Type:         info = KFileItemDelegate::FriendlyMimeType; break;
    default: break;
    }

    return info;
}

void DolphinDetailsView::resizeColumns()
{
    // Using the resize mode QHeaderView::ResizeToContents is too slow (it takes
    // around 3 seconds for each (!) resize operation when having > 10000 items).
    // This gets a problem especially when opening large directories, where several
    // resize operations are received for showing the currently available items during
    // loading (the application hangs around 20 seconds when loading > 10000 items).

    QHeaderView* headerView = header();
    QFontMetrics fontMetrics(viewport()->font());

    int columnWidth[KDirModel::ColumnCount];
    columnWidth[KDirModel::Size] = fontMetrics.width("00000 Items");
    columnWidth[KDirModel::ModifiedTime] = fontMetrics.width("0000-00-00 00:00");
    columnWidth[KDirModel::Permissions] = fontMetrics.width("xxxxxxxxxx");
    columnWidth[KDirModel::Owner] = fontMetrics.width("xxxxxxxxxx");
    columnWidth[KDirModel::Group] = fontMetrics.width("xxxxxxxxxx");
    columnWidth[KDirModel::Type] = fontMetrics.width("XXXX Xxxxxxx");

    int requiredWidth = 0;
    for (int i = KDirModel::Size; i <= KDirModel::Type; ++i) {
        if (!isColumnHidden(i)) {
            columnWidth[i] += 20; // provide a default gap
            requiredWidth += columnWidth[i];
            headerView->resizeSection(i, columnWidth[i]);
        }
    }

    // resize the name column in a way that the whole available width is used
    columnWidth[KDirModel::Name] = viewport()->width() - requiredWidth;
    if (columnWidth[KDirModel::Name] < 120) {
        columnWidth[KDirModel::Name] = 120;
    }
    headerView->resizeSection(KDirModel::Name, columnWidth[KDirModel::Name]);
}

#include "dolphindetailsview.moc"
