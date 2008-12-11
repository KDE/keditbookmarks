/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "dolphinviewautoscroller.h"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTimer>

DolphinViewAutoScroller::DolphinViewAutoScroller(QAbstractItemView* parent) :
    QObject(parent),
    m_rubberBandSelection(false),
    m_scrollInc(0),
    m_itemView(parent),
    m_timer()
{
    m_itemView->setAutoScroll(false);
    m_itemView->viewport()->installEventFilter(this);
    m_itemView->installEventFilter(this);
    
    m_timer = new QTimer(this);
    m_timer->setSingleShot(false);
    m_timer->setInterval(1000 / 25); // 25 frames per second
    connect(m_timer, SIGNAL(timeout()), this, SLOT(scrollViewport()));
}

DolphinViewAutoScroller::~DolphinViewAutoScroller()
{
}

bool DolphinViewAutoScroller::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_itemView->viewport()) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                m_rubberBandSelection = true;
            }
            break;
            
        case QEvent::MouseMove:
            if (m_rubberBandSelection) {
                triggerAutoScroll();
            }
            break;
            
        case QEvent::MouseButtonRelease:
            m_rubberBandSelection = false;
            stopAutoScroll();
            break;
            
        case QEvent::DragEnter:
        case QEvent::DragMove:
            m_rubberBandSelection = false;
            triggerAutoScroll();
            break;
            
        case QEvent::Drop:
        case QEvent::DragLeave:
            m_rubberBandSelection = false;
            stopAutoScroll();
            break;
            
        default:
            break;
        }
    } else if ((watched == m_itemView) && (event->type() == QEvent::KeyPress)) {
        const int key = static_cast<QKeyEvent*>(event)->key();
        const bool arrowKeyPressed = (key == Qt::Key_Up)   || (key == Qt::Key_Down) ||
                                     (key == Qt::Key_Left) || (key == Qt::Key_Right);
        if (arrowKeyPressed) {
            QMetaObject::invokeMethod(this, "scrollToCurrentIndex", Qt::QueuedConnection);
        }
    } 
    

    return QObject::eventFilter(watched, event);
}

void DolphinViewAutoScroller::scrollViewport()
{
    QScrollBar* verticalScrollBar = m_itemView->verticalScrollBar();
    if (verticalScrollBar != 0) {
        const int value = verticalScrollBar->value();
        verticalScrollBar->setValue(value + m_scrollInc);
        
    }
    QScrollBar* horizontalScrollBar = m_itemView->horizontalScrollBar();
    if (horizontalScrollBar != 0) {
        const int value = horizontalScrollBar->value();
        horizontalScrollBar->setValue(value + m_scrollInc);
        
    }
    
    if (m_rubberBandSelection) {
        // The scrolling does not lead to an update of the rubberband
        // selection. Fake a mouse move event to let the QAbstractItemView
        // update the rubberband.
        QWidget* viewport = m_itemView->viewport();
        const QPoint pos = viewport->mapFromGlobal(QCursor::pos());
        QMouseEvent event(QEvent::MouseMove, pos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(viewport, &event);
    }
}

void DolphinViewAutoScroller::scrollToCurrentIndex()
{
     const QModelIndex index = m_itemView->currentIndex();
     m_itemView->scrollTo(index);
}

void DolphinViewAutoScroller::triggerAutoScroll()
{
    const bool verticalScrolling = (m_itemView->verticalScrollBar() != 0) &&
                                    m_itemView->verticalScrollBar()->isVisible();
    const bool horizontalScrolling = (m_itemView->horizontalScrollBar() != 0) &&
                                     m_itemView->horizontalScrollBar()->isVisible();
    if (!verticalScrolling && !horizontalScrolling) {
        // no scrollbars are shown at all, so no autoscrolling is necessary
        return;
    }
    
    QWidget* viewport = m_itemView->viewport();
    const QPoint pos = viewport->mapFromGlobal(QCursor::pos());
    if (verticalScrolling) {
        calculateScrollIncrement(pos.y(), viewport->height());
    }
    if (horizontalScrolling) {
        calculateScrollIncrement(pos.x(), viewport->width());
    }
    
    if (m_timer->isActive()) {
        if (m_scrollInc == 0) {
            m_timer->stop();
        }
    } else if (m_scrollInc != 0) {
        m_timer->start();
    }
}

void DolphinViewAutoScroller::stopAutoScroll()
{
    m_timer->stop();
    m_scrollInc = 0;
}

void DolphinViewAutoScroller::calculateScrollIncrement(int cursorPos, int rangeSize)
{
    const int minSpeed = 2;
    const int maxSpeed = 32;
    const int speedLimiter = 8;
    const int autoScrollBorder = 32;
    
    if (cursorPos < autoScrollBorder) {
        m_scrollInc = -minSpeed + (cursorPos - autoScrollBorder) / speedLimiter;
        if (m_scrollInc < -maxSpeed) {
            m_scrollInc = -maxSpeed;
        }
    } else if (cursorPos > rangeSize - autoScrollBorder) {
        m_scrollInc = minSpeed + (cursorPos - rangeSize + autoScrollBorder) / speedLimiter;
        if (m_scrollInc > maxSpeed) {
            m_scrollInc = maxSpeed;
        }
    } else {
        m_scrollInc = 0;
    }
}

#include "dolphinviewautoscroller.moc"
