/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "dolphincontroller.h"
#include "zoomlevelinfo.h"

#include <kdirmodel.h>
#include <QAbstractProxyModel>
#include <QApplication>
#include <QClipboard>
#include <QDir>

Qt::MouseButtons DolphinController::m_mouseButtons = Qt::NoButton;

DolphinController::DolphinController(DolphinView* dolphinView) :
    QObject(dolphinView),
    m_zoomLevel(0),
    m_url(),
    m_dolphinView(dolphinView),
    m_itemView(0)
{
}

DolphinController::~DolphinController()
{
}

void DolphinController::setUrl(const KUrl& url)
{
    if (m_url != url) {
        m_url = url;
        emit urlChanged(url);
    }
}

void DolphinController::setItemView(QAbstractItemView* view)
{
    if (m_itemView != 0) {
        disconnect(m_itemView, SIGNAL(pressed(const QModelIndex&)),
                   this, SLOT(updateMouseButtonState()));
    }

    m_itemView = view;

    if (m_itemView != 0) {
        m_zoomLevel = ZoomLevelInfo::zoomLevelForIconSize(m_itemView->iconSize());

        // TODO: this is a workaround until  Qt-issue 176832 has been fixed
        connect(m_itemView, SIGNAL(pressed(const QModelIndex&)),
                this, SLOT(updateMouseButtonState()));
    }
}

void DolphinController::triggerUrlChangeRequest(const KUrl& url)
{
    if (m_url != url) {
        emit requestUrlChange(url);
    }
}

void DolphinController::triggerContextMenuRequest(const QPoint& pos)
{
    emit activated();
    emit requestContextMenu(pos);
}

void DolphinController::requestActivation()
{
    emit activated();
}

void DolphinController::indicateDroppedUrls(const KFileItem& destItem,
                                            const KUrl& destPath,
                                            QDropEvent* event)
{
    emit urlsDropped(destItem, destPath, event);
}


void DolphinController::indicateSortingChange(DolphinView::Sorting sorting)
{
    emit sortingChanged(sorting);
}

void DolphinController::indicateSortOrderChange(Qt::SortOrder order)
{
    emit sortOrderChanged(order);
}

void DolphinController::indicateAdditionalInfoChange(const KFileItemDelegate::InformationList& info)
{
    emit additionalInfoChanged(info);
}

void DolphinController::indicateActivationChange(bool active)
{
    emit activationChanged(active);
}

void DolphinController::setZoomLevel(int level)
{
    Q_ASSERT(level >= ZoomLevelInfo::minimumLevel());
    Q_ASSERT(level <= ZoomLevelInfo::maximumLevel());
    if (level != m_zoomLevel) {
        m_zoomLevel = level;
        emit zoomLevelChanged(m_zoomLevel);
    }
}

void DolphinController::handleKeyPressEvent(QKeyEvent* event)
{
    Q_ASSERT(m_itemView != 0);

    const QItemSelectionModel* selModel = m_itemView->selectionModel();
    const QModelIndex currentIndex = selModel->currentIndex();

    if (currentIndex.isValid() && selModel->selectedIndexes().count() > 0) {
        const int key = event->key();

        if ((key == Qt::Key_Return) || (key == Qt::Key_Enter) || (key == Qt::Key_Right)) {

            const QModelIndexList indexList = selModel->selectedIndexes();
            const bool isColumnView = m_dolphinView->mode() == m_dolphinView->ColumnView;

            if (key == Qt::Key_Right) {
                if (isColumnView) {
                    // If it is the right arrow key and in the column view-only.
                    KFileItem curFileItem;
                    foreach(const QModelIndex& index, indexList) {
                        curFileItem = itemForIndex(index);
                        if (!curFileItem.isFile()) {
                            /* We want
                            *  to make sure that the selected item
                            *  is only a folder. If we did not have this check, it would be possible to use
                            *  the right arrow to open a file when in the column view */
                            emit itemTriggered(curFileItem);
                        }
                    }
                }
            } else {
                //Else it is Return or Enter keypress, so it is okay to perform the action of triggering, on files also.
                foreach(const QModelIndex& index, indexList) {
                    emit itemTriggered(itemForIndex(index));
                }
            }
        }
    }
}

void DolphinController::replaceUrlByClipboard()
{
    const QClipboard* clipboard = QApplication::clipboard();
    QString text;
    if (clipboard->mimeData(QClipboard::Selection)->hasText()) {
        text = clipboard->mimeData(QClipboard::Selection)->text();
    } else if (clipboard->mimeData(QClipboard::Clipboard)->hasText()) {
        text = clipboard->mimeData(QClipboard::Clipboard)->text();
    }
    if (!text.isEmpty() && QDir::isAbsolutePath(text)) {
        m_dolphinView->setUrl(KUrl(text));
    }
}

void DolphinController::emitHideToolTip()
{
    emit hideToolTip();
}

KFileItem DolphinController::itemForIndex(const QModelIndex& index) const
{
    Q_ASSERT(m_itemView != 0);

    QAbstractProxyModel* proxyModel = static_cast<QAbstractProxyModel*>(m_itemView->model());
    KDirModel* dirModel = static_cast<KDirModel*>(proxyModel->sourceModel());
    const QModelIndex dirIndex = proxyModel->mapToSource(index);
    return dirModel->itemForIndex(dirIndex);
}

void DolphinController::triggerItem(const QModelIndex& index)
{
    if (m_mouseButtons & Qt::LeftButton) {
        const KFileItem item = itemForIndex(index);
        if (index.isValid() && (index.column() == KDirModel::Name)) {
            emit itemTriggered(item);
        } else {
            m_itemView->clearSelection();
            emit itemEntered(KFileItem());
        }
    }
}

void DolphinController::requestTab(const QModelIndex& index)
{
    if (m_mouseButtons & Qt::MidButton) {
        const KFileItem item = itemForIndex(index);
        const bool validRequest = index.isValid() &&
                                  (index.column() == KDirModel::Name) &&
                                  (item.isDir() || m_dolphinView->isTabsForFilesEnabled());
        if (validRequest) {
            emit tabRequested(item.url());
        }
    }
}

void DolphinController::emitItemEntered(const QModelIndex& index)
{
    KFileItem item = itemForIndex(index);
    if (!item.isNull()) {
        emit itemEntered(item);
    }
}

void DolphinController::emitViewportEntered()
{
    emit viewportEntered();
}

void DolphinController::updateMouseButtonState()
{
    m_mouseButtons = QApplication::mouseButtons();
}

#include "dolphincontroller.moc"
