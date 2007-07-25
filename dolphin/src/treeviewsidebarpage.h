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

#ifndef TREEVIEWSIDEBARPAGE_H
#define TREEVIEWSIDEBARPAGE_H

#include <kurl.h>
#include <sidebarpage.h>

class KDirLister;
class KDirModel;

class DolphinSortFilterProxyModel;
class SidebarTreeView;
class QModelIndex;

/**
 * @brief Shows a tree view of the directories starting from
 *        the currently selected bookmark.
 *
 * The tree view is always synchronized with the currently active view
 * from the main window.
 */
class TreeViewSidebarPage : public SidebarPage
{
    Q_OBJECT

public:
    TreeViewSidebarPage(QWidget* parent = 0);
    virtual ~TreeViewSidebarPage();

    /** @see QWidget::sizeHint() */
    virtual QSize sizeHint() const;

public slots:
    /**
     * Changes the current selection inside the tree to \a url.
     */
    virtual void setUrl(const KUrl& url);

protected:
    /** @see QWidget::showEvent() */
    virtual void showEvent(QShowEvent* event);

    /** @see QWidget::contextMenuEvent() */
    virtual void contextMenuEvent(QContextMenuEvent* event);

private slots:
    /**
     * Expands the tree in a way that the item with the URL m_selectedUrl
     * gets visible. Is called by TreeViewSidebarPage::updateSelection()
     * if the dir lister has been completed.
     */
    void expandSelectionParent();

    /**
     * Updates the active view to the URL
     * which is given by the item with the index \a index.
     */
    void updateActiveView(const QModelIndex& index);

    /**
     * Is emitted if the URLs \a urls have been dropped
     * to the index \a index. */
    void dropUrls(const KUrl::List& urls,
                  const QModelIndex& index);

private:
    void loadTree(const KUrl& url);

private:
    KDirLister* m_dirLister;
    KDirModel* m_dirModel;
    DolphinSortFilterProxyModel* m_proxyModel;
    SidebarTreeView* m_treeView;
};

#endif // TREEVIEWSIDEBARPAGE_H
