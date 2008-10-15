/***************************************************************************
 *   Copyright (C) 2008 by Simon St James <kdedevel@etotheipiplusone.com>  *
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

#include "folderexpander.h"
#include "dolphinview.h"

#include "dolphinsettings.h"
#include "dolphin_generalsettings.h"

#include <QtCore/QTimer>
#include <QtGui/QAbstractItemView>
#include <QtGui/QTreeView>
#include <QtGui/QScrollBar>

#include <QtCore/QEvent>
#include <QtGui/QDragMoveEvent>

#include <QtGui/QSortFilterProxyModel>

#include <kdirmodel.h>
#include <kdebug.h>

FolderExpander::FolderExpander(QAbstractItemView *view, QSortFilterProxyModel *proxyModel) :
    QObject(view),
    m_enabled(true),
    m_view(view),
    m_proxyModel(proxyModel),
    m_autoExpandTriggerTimer(0),
    m_autoExpandPos()
{
    // Validation.  If these fail, the event filter is never
    // installed on the view and the FolderExpander is inactive.
    if (m_view == 0)  {
        kWarning() << "Need a view!";
        return; // Not valid.
    }
    if (m_proxyModel == 0)  {
        kWarning() << "Need a proxyModel!";
        return; // Not valid.
    }
    KDirModel *m_dirModel = qobject_cast< KDirModel* >( m_proxyModel->sourceModel() );  
    if (m_dirModel == 0) {
        kWarning() << "Expected m_proxyModel's sourceModel() to be a KDirModel!";
        return; // Not valid.
    }

    // Initialise auto-expand timer.
    m_autoExpandTriggerTimer = new QTimer(this);
    m_autoExpandTriggerTimer->setSingleShot(true);
    connect(m_autoExpandTriggerTimer, SIGNAL(timeout()), 
            this, SLOT(autoExpandTimeout()));

    // The view scrolling complicates matters, so we want to
    // be informed if they occur.
    connect(m_view->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(viewScrolled()));
    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(viewScrolled()));

    // "Dragging" events are sent to the QAbstractItemView's viewport.
    m_view->viewport()->installEventFilter(this);
}

void FolderExpander::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool FolderExpander::enabled() const
{
    return m_enabled;
}

FolderExpander::~FolderExpander()
{
}

void FolderExpander::viewScrolled()
{
    if (m_autoExpandTriggerTimer->isActive())  {
        kDebug() << "Resetting time due to scrolling!"; 
        // (Re-)set the timer while we're scrolling the view
        // (or it's being scrolled by some external mechanism).
        // TODO - experiment with this.  Cancelling the timer,
        // or adding a "penalty" on top of AUTO_EXPAND_DELAY
        // might work more nicely when drilling down through the sidebar
        // tree.
        m_autoExpandTriggerTimer->start(AUTO_EXPAND_DELAY);
    }
}

void FolderExpander::autoExpandTimeout()
{
    if (!m_enabled) {
        return;
    }

    // We want to find whether the file currently being hovered over is a
    // directory. TODO - is there a simpler way, preferably without
    // needing to pass in m_proxyModel that has a KDirModel as its sourceModel() ... ?
    QModelIndex proxyIndexToExpand = m_view->indexAt(m_autoExpandPos);
    QModelIndex indexToExpand = m_proxyModel->mapToSource(proxyIndexToExpand);
    KDirModel* m_dirModel = qobject_cast< KDirModel* >(m_proxyModel->sourceModel()); 
    Q_ASSERT(m_dirModel != 0);
    KFileItem itemToExpand = m_dirModel->itemForIndex(indexToExpand );

    if (itemToExpand.isNull()) {
        return;
    }
    
    kDebug() << "Need to expand: " << itemToExpand.targetUrl() << " isDir? = " << itemToExpand.isDir();

    if (itemToExpand.isDir()) {
        QTreeView *viewAsTreeView = qobject_cast<QTreeView*>(m_view);
        if (viewAsTreeView != 0) {
            // Toggle expanded state of this directory.
            viewAsTreeView->setExpanded(proxyIndexToExpand, !viewAsTreeView->isExpanded(proxyIndexToExpand));
        }
        else {
            // Enter this directory.
            emit enterDir(proxyIndexToExpand);
        }
    }
}

bool FolderExpander::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched);
    // We're interested in reading Drag* events, but not filtering them,
    // so always return false.
    // We just store the position of the hover, here; actually working out
    // what the hovered item is and whether it is expandable is done in
    // autoExpandTimeout.
    if (event->type() == QEvent::DragMove) {
        QDragMoveEvent *dragMoveEvent = static_cast<QDragMoveEvent*>(event);
        // (Re-)set the timer while we're still moving and dragging.
        m_autoExpandTriggerTimer->start(AUTO_EXPAND_DELAY);
        m_autoExpandPos = dragMoveEvent->pos();
    } else if (event->type() == QEvent::DragLeave || event->type() == QEvent::Drop) {
        m_autoExpandTriggerTimer->stop();
    }
    return false;
}
