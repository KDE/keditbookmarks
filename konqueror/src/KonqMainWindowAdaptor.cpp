/* This file is part of the KDE project
   Copyright 2000 Simon Hausmann <hausmann@kde.org>
   Copyright 2000, 2006 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "KonqMainWindowAdaptor.h"
#include "KonqViewAdaptor.h"
#include "konqview.h"

#include <kdebug.h>
#include <kstartupinfo.h>

KonqMainWindowAdaptor::KonqMainWindowAdaptor( KonqMainWindow * mainWindow )
    : QDBusAbstractAdaptor( mainWindow ), m_pMainWindow( mainWindow )
{
//  m_dcopActionProxy = new KDCOPActionProxy( mainWindow->actionCollection(), this );
}

KonqMainWindowAdaptor::~KonqMainWindowAdaptor()
{
//  delete m_dcopActionProxy;
}

void KonqMainWindowAdaptor::openUrl( const QString& url, bool tempFile )
{
  m_pMainWindow->openFilteredUrl( url, false, tempFile );
}

void KonqMainWindowAdaptor::newTab( const QString& url, bool tempFile )
{
  m_pMainWindow->openFilteredUrl( url, true, tempFile );
}

void KonqMainWindowAdaptor::newTabASN( const QString& url, const QByteArray& startup_id, bool tempFile )
{
#ifdef Q_WS_X11
  KStartupInfo::setNewStartupId( m_pMainWindow, startup_id );
#endif
  m_pMainWindow->openFilteredUrl( url, true, tempFile );
}

void KonqMainWindowAdaptor::reload()
{
  m_pMainWindow->slotReload();
}

QDBusObjectPath KonqMainWindowAdaptor::currentView()
{
  kDebug() ;
  KonqView *view = m_pMainWindow->currentView();
  if ( !view )
    return QDBusObjectPath();

  return QDBusObjectPath( view->dbusObjectPath() );
}

QDBusObjectPath KonqMainWindowAdaptor::currentPart()
{
  KonqView *view = m_pMainWindow->currentView();
  if ( !view )
    return QDBusObjectPath();

  return QDBusObjectPath( view->partObjectPath() );
}

QDBusObjectPath KonqMainWindowAdaptor::view(int viewNumber)
{
  KonqMainWindow::MapViews viewMap = m_pMainWindow->viewMap();
  KonqMainWindow::MapViews::const_iterator it = viewMap.constBegin();
  for ( int i = 0; it != viewMap.constEnd() && i < viewNumber; ++i )
      ++it;
  if ( it == viewMap.constEnd() )
      return QDBusObjectPath();
  return QDBusObjectPath( (*it)->dbusObjectPath() );
}

QDBusObjectPath KonqMainWindowAdaptor::part(int partNumber)
{
  KonqMainWindow::MapViews viewMap = m_pMainWindow->viewMap();
  KonqMainWindow::MapViews::const_iterator it = viewMap.constBegin();
  for ( int i = 0; it != viewMap.constEnd() && i < partNumber; ++i )
      ++it;
  if ( it == viewMap.constEnd() )
      return QDBusObjectPath();
  return QDBusObjectPath( (*it)->partObjectPath() );
}

void KonqMainWindowAdaptor::splitViewHorizontally()
{
    m_pMainWindow->slotSplitViewHorizontal();
}

void KonqMainWindowAdaptor::splitViewVertically()
{
    m_pMainWindow->slotSplitViewVertical();
}

#include "KonqMainWindowAdaptor.moc"
