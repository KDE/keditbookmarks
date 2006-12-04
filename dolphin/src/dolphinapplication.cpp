/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Holger 'zecke' Freyther <freyther@kde.org>      *
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

#include "dolphinapplication.h"
#include "dolphinmainwindow.h"

DolphinApplication::DolphinApplication()
{
}

/*
 * cleanup what ever is left from the MainWindows
 */
DolphinApplication::~DolphinApplication()
{
    while( m_mainWindows.count() != 0 )
        delete m_mainWindows.takeFirst();
}

DolphinApplication* DolphinApplication::app()
{
    return qobject_cast<DolphinApplication*>(qApp);
}

DolphinMainWindow* DolphinApplication::createMainWindow()
{
    DolphinMainWindow* mainwindow = new DolphinMainWindow;
    mainwindow->init();
    
    m_mainWindows.append( mainwindow );
    return mainwindow;
}

void DolphinApplication::removeMainWindow( DolphinMainWindow *mainwindow )
{
    m_mainWindows.removeAll( mainwindow );
}

void DolphinApplication::refreshMainWindows()
{
    for( int i = 0; i < m_mainWindows.count(); ++i ) {
        m_mainWindows[i]->refreshViews();
    }
}

#include "dolphinapplication.moc"

