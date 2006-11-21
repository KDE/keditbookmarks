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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "progressindicator.h"
#include "dolphin.h"
#include "dolphinstatusbar.h"

ProgressIndicator::ProgressIndicator(const QString& progressText,
                                     const QString& finishedText,
                                     int operationsCount)
 :  m_showProgress(false),
    m_operationsCount(operationsCount),
    m_operationsIndex(0),
    m_startTime(QTime::currentTime()),
    m_finishedText(finishedText)
{
    DolphinStatusBar* statusBar = Dolphin::mainWin().activeView()->statusBar();
    statusBar->clear();
    statusBar->setProgressText(progressText);
    statusBar->setProgress(0);
}


ProgressIndicator::~ProgressIndicator()
{
    DolphinStatusBar* statusBar = Dolphin::mainWin().activeView()->statusBar();
    statusBar->setProgressText(QString::null);
    statusBar->setProgress(100);
    statusBar->setMessage(m_finishedText, DolphinStatusBar::OperationCompleted);

    if (m_showProgress) {
        Dolphin::mainWin().setEnabled(true);
    }
}

void ProgressIndicator::execOperation()
{
    ++m_operationsIndex;

    if (!m_showProgress) {
        const int elapsed = m_startTime.msecsTo(QTime::currentTime());
        if (elapsed > 500) {
            // the operations took already more than 500 milliseconds,
            // therefore show a progress indication
            Dolphin::mainWin().setEnabled(false);
            m_showProgress = true;
        }
    }

    if (m_showProgress) {
        const QTime currentTime = QTime::currentTime();
        if (m_startTime.msecsTo(currentTime) > 100) {
            m_startTime = currentTime;

            DolphinStatusBar* statusBar = Dolphin::mainWin().activeView()->statusBar();
            statusBar->setProgress((m_operationsIndex * 100) / m_operationsCount);
            kapp->processEvents();
            statusBar->repaint();
        }
    }
}


