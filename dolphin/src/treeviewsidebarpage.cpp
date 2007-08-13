/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "treeviewsidebarpage.h"

#include "dolphinmainwindow.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphinview.h"
#include "dolphinsettings.h"
#include "sidebartreeview.h"
#include "treeviewcontextmenu.h"

#include <kfileplacesmodel.h>
#include <kdirlister.h>
#include <kdirmodel.h>
#include <kfileitem.h>

#include <QtGui/QHeaderView>
#include <QtGui/QItemSelection>
#include <QtGui/QTreeView>
#include <QtGui/QBoxLayout>

TreeViewSidebarPage::TreeViewSidebarPage(QWidget* parent) :
    SidebarPage(parent),
    m_dirLister(0),
    m_dirModel(0),
    m_proxyModel(0),
    m_treeView(0)
{
}

TreeViewSidebarPage::~TreeViewSidebarPage()
{
    delete m_dirLister;
    m_dirLister = 0;
}

QSize TreeViewSidebarPage::sizeHint() const
{
    QSize size = SidebarPage::sizeHint();
    size.setWidth(200);
    return size;
}

void TreeViewSidebarPage::setUrl(const KUrl& url)
{
    if (!url.isValid() || (url == SidebarPage::url())) {
        return;
    }

    SidebarPage::setUrl(url);
    if (m_dirLister != 0) {
        loadTree(url);
    }
}

void TreeViewSidebarPage::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) {
        SidebarPage::showEvent(event);
        return;
    }

    if (m_dirLister == 0) {
        // Postpone the creating of the dir lister to the first show event.
        // This assures that no performance and memory overhead is given when the TreeView is not
        // used at all (see TreeViewSidebarPage::setUrl()).
        m_dirLister = new KDirLister();
        m_dirLister->setDirOnlyMode(true);
        m_dirLister->setAutoUpdate(true);
        m_dirLister->setMainWindow(this);
        m_dirLister->setDelayedMimeTypes(true);
        m_dirLister->setAutoErrorHandlingEnabled(false, this);

        Q_ASSERT(m_dirModel == 0);
        m_dirModel = new KDirModel(this);
        m_dirModel->setDirLister(m_dirLister);
        m_dirModel->setDropsAllowed(KDirModel::DropOnDirectory);

        Q_ASSERT(m_proxyModel == 0);
        m_proxyModel = new DolphinSortFilterProxyModel(this);
        m_proxyModel->setSourceModel(m_dirModel);

        Q_ASSERT(m_treeView == 0);
        m_treeView = new SidebarTreeView(this);
        m_treeView->setModel(m_proxyModel);
        m_proxyModel->setSorting(DolphinView::SortByName);
        m_proxyModel->setSortOrder(Qt::AscendingOrder);

        connect(m_treeView, SIGNAL(clicked(const QModelIndex&)),
                this, SLOT(updateActiveView(const QModelIndex&)));
        connect(m_treeView, SIGNAL(urlsDropped(const KUrl::List&, const QModelIndex&)),
                this, SLOT(dropUrls(const KUrl::List&, const QModelIndex&)));

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->addWidget(m_treeView);
    }

    loadTree(url());
    SidebarPage::showEvent(event);
}

void TreeViewSidebarPage::contextMenuEvent(QContextMenuEvent* event)
{
    SidebarPage::contextMenuEvent(event);

    const QModelIndex index = m_treeView->indexAt(event->pos());
    if (!index.isValid()) {
        // only open a context menu above a directory item
        return;
    }

    const QModelIndex dirModelIndex = m_proxyModel->mapToSource(index);
    KFileItem item = m_dirModel->itemForIndex(dirModelIndex);

    emit changeSelection(QList<KFileItem>());
    TreeViewContextMenu contextMenu(this, item);
    contextMenu.open();
}

void TreeViewSidebarPage::expandSelectionParent()
{
    disconnect(m_dirLister, SIGNAL(completed()),
               this, SLOT(expandSelectionParent()));

    // expand the parent folder of the selected item
    KUrl parentUrl = url().upUrl();
    if (!m_dirLister->url().isParentOf(parentUrl)) {
        return;
    }

    QModelIndex index = m_dirModel->indexForUrl(parentUrl);
    if (index.isValid()) {
        QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);
        m_treeView->setExpanded(proxyIndex, true);

        // select the item and assure that the item is visible
        index = m_dirModel->indexForUrl(url());
        if (index.isValid()) {
            proxyIndex = m_proxyModel->mapFromSource(index);
            m_treeView->scrollTo(proxyIndex);

            QItemSelectionModel* selModel = m_treeView->selectionModel();
            selModel->setCurrentIndex(proxyIndex, QItemSelectionModel::Select);
        }
    }
}

void TreeViewSidebarPage::updateActiveView(const QModelIndex& index)
{
    const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
    const KFileItem item = m_dirModel->itemForIndex(dirIndex);
    if (!item.isNull()) {
        emit changeUrl(item.url());
    }
}

void TreeViewSidebarPage::dropUrls(const KUrl::List& urls,
                                   const QModelIndex& index)
{
    if (index.isValid()) {
        const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
        KFileItem item = m_dirModel->itemForIndex(dirIndex);
        Q_ASSERT(!item.isNull());
        if (item.isDir()) {
            emit urlsDropped(urls, item.url());
        }
    }
}

void TreeViewSidebarPage::loadTree(const KUrl& url)
{
    Q_ASSERT(m_dirLister != 0);

    // adjust the root of the tree to the base bookmark
    KFilePlacesModel* placesModel = DolphinSettings::instance().placesModel();
    KUrl baseUrl = placesModel->url(placesModel->closestItem(url));
    if (!baseUrl.isValid()) {
        // it's possible that no closest item is available and hence an
        // empty URL is returned
        baseUrl = url;
    }

    if (m_dirLister->url() != baseUrl) {
        m_dirLister->stop();
        m_dirLister->openUrl(baseUrl);
    }

    // select the folder which contains the given URL
    QItemSelectionModel* selModel = m_treeView->selectionModel();
    selModel->clearSelection();

    const QModelIndex index = m_dirModel->indexForUrl(url);
    if (index.isValid()) {
        // the item with the given URL is already part of the model
        const QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);
        m_treeView->scrollTo(proxyIndex);
        selModel->setCurrentIndex(proxyIndex, QItemSelectionModel::Select);
    } else {
        // The item with the given URL is not loaded by the model yet. Iterate
        // backward to the base URL and trigger the loading of the items for
        // each hierarchy level.
        connect(m_dirLister, SIGNAL(completed()),
                this, SLOT(expandSelectionParent()));

        // Implementation note: It is important to remove the trailing slash from
        // the parent URL, as the directories from the dir lister (KDirLister::directories())
        // don't have a trailing slash and hence KUrl::List::contains() would fail...
        KUrl parentUrl = url.upUrl();
        parentUrl.adjustPath(KUrl::RemoveTrailingSlash);
        while (!parentUrl.isParentOf(baseUrl)) {
            if (m_dirLister->directories().contains(parentUrl)) {
                m_dirLister->updateDirectory(parentUrl);
            } else {
                m_dirLister->openUrl(parentUrl, true, false);
            }
            parentUrl = parentUrl.upUrl();
            parentUrl.adjustPath(KUrl::RemoveTrailingSlash);
        }
    }
}

#include "treeviewsidebarpage.moc"
