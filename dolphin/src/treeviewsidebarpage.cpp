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

#include "bookmarkselector.h"
#include "dolphinmainwindow.h"
#include "dolphinview.h"

#include "kdirlister.h"
#include "kdirmodel.h"

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QTreeView>
#include <QVBoxLayout>

TreeViewSidebarPage::TreeViewSidebarPage(DolphinMainWindow* mainWindow,
                                         QWidget* parent) :
    SidebarPage(mainWindow, parent),
    m_dirLister(0),
    m_dirModel(0),
    m_treeView(0)
{
    Q_ASSERT(mainWindow != 0);

    m_dirLister = new KDirLister();
    m_dirLister->setDirOnlyMode(true);
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(this);
    m_dirLister->setDelayedMimeTypes(true);
    m_dirLister->setAutoErrorHandlingEnabled(false, this);

    m_dirModel = new KDirModel();
    m_dirModel->setDirLister(m_dirLister);

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_dirModel);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);

    // hide all columns except of the 'Name' column
    m_treeView->hideColumn(KDirModel::Size);
    m_treeView->hideColumn(KDirModel::ModifiedTime);
    m_treeView->hideColumn(KDirModel::Permissions);
    m_treeView->hideColumn(KDirModel::Owner);
    m_treeView->hideColumn(KDirModel::Group);
    m_treeView->header()->hide();

    connect(m_treeView, SIGNAL(clicked(const QModelIndex&)),
            this, SLOT(updateViewUrl(const QModelIndex&)));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_treeView);
}

TreeViewSidebarPage::~TreeViewSidebarPage()
{
    delete m_dirLister;
    m_dirLister = 0;
}

void TreeViewSidebarPage::activeViewChanged()
{
    connectToActiveView();
}

void TreeViewSidebarPage::showEvent(QShowEvent* event)
{
    SidebarPage::showEvent(event);
    connectToActiveView();
}

void TreeViewSidebarPage::updateSelection(const KUrl& url)
{
    // adjust the root of the tree to the base bookmark
    KUrl baseUrl = BookmarkSelector::baseBookmark(url).url();
    if (m_dirLister->url() != baseUrl) {
        m_dirLister->stop();
        m_dirLister->openUrl(baseUrl);
    }

    // select the folder which contains the given url

    // TODO: check how Konqi does it before reinventing the wheel. The directory
    // must already be loaded _before_ the index can be retrieved by
    // KDirModel::indexForItem().
    QItemSelectionModel* selModel = m_treeView->selectionModel();
    selModel->clearSelection();

    KFileItem item(S_IFDIR, KFileItem::Unknown, url);
    const QModelIndex index = m_dirModel->indexForItem(item);
    if (index.isValid()) {
        m_treeView->scrollTo(index);
        m_treeView->setExpanded(index, true);

        selModel->setCurrentIndex(index, QItemSelectionModel::Select);
    }
}

void TreeViewSidebarPage::updateViewUrl(const QModelIndex& index)
{
    KFileItem* item = m_dirModel->itemForIndex(index);
    if (item != 0) {
        const KUrl& url = item->url();
        mainWindow()->activeView()->setUrl(url);
    }
}

void TreeViewSidebarPage::connectToActiveView()
{
    const QWidget* parent = parentWidget();
    if ((parent == 0) || parent->isHidden()) {
        return;
    }

    DolphinView* view = mainWindow()->activeView();
    const KUrl& url = view->url();

    m_dirLister->stop();
    m_dirLister->openUrl(url);
    connect(view, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(updateSelection(const KUrl&)));

    updateSelection(url);
}

#include "treeviewsidebarpage.moc"
