/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000 David Faure <faure@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "KonquerorIface.h"
#include "konq_misc.h"
#include "KonqMainWindowIface.h"
#include "konq_mainwindow.h"
#include "konq_viewmgr.h"
#include "konq_view.h"
#include <konq_settings.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kdebug.h>
#include <qfile.h>
#include "konq_settingsxt.h"

// these DCOP calls come from outside, so any windows created by these
// calls would have old user timestamps (for KWin's no-focus-stealing),
// it's better to reset the timestamp and rely on other means
// of detecting the time when the user action that triggered all this
// happened
// TODO a valid timestamp should be passed in the DCOP calls that
// are not for user scripting
#include <X11/Xlib.h>
extern Time qt_x_user_time;

KonquerorIface::KonquerorIface()
 : DCOPObject( "KonquerorIface" )
{
}

KonquerorIface::~KonquerorIface()
{
}

DCOPRef KonquerorIface::openBrowserWindow( const QString &url )
{
    qt_x_user_time = 0;
    KonqMainWindow *res = KonqMisc::createSimpleWindow( KURL(url) );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::openBrowserWindowASN( const QString &url, const QCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return openBrowserWindow( url );
}

DCOPRef KonquerorIface::createNewWindow( const QString &url )
{
    return createNewWindow( url, QString::null, false );
}

DCOPRef KonquerorIface::createNewWindowASN( const QString &url, const QCString& startup_id, bool tempFile )
{
    kapp->setStartupId( startup_id );
    return createNewWindow( url, QString::null, tempFile );
}

DCOPRef KonquerorIface::createNewWindowWithSelection( const QString &url, QStringList filesToSelect )
{
    qt_x_user_time = 0;
    KonqMainWindow *res = KonqMisc::createNewWindow( KURL(url), KParts::URLArgs(), false, filesToSelect );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createNewWindowWithSelectionASN( const QString &url, QStringList filesToSelect, const QCString &startup_id )
{
    kapp->setStartupId( startup_id );
    return createNewWindowWithSelection( url, filesToSelect );
}

DCOPRef KonquerorIface::createNewWindow( const QString &url, const QString &mimetype, bool tempFile )
{
    qt_x_user_time = 0;
    KParts::URLArgs args;
    args.serviceType = mimetype;
    // Filter the URL, so that "kfmclient openURL gg:foo" works also when konq is already running
    KURL finalURL = KonqMisc::konqFilteredURL( 0, url );
    KonqMainWindow *res = KonqMisc::createNewWindow( finalURL, args, false, QStringList(), tempFile );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createNewWindowASN( const QString &url, const QString &mimetype,
    const QCString& startup_id, bool tempFile )
{
    kapp->setStartupId( startup_id );
    return createNewWindow( url, mimetype, tempFile );
}

DCOPRef KonquerorIface::createBrowserWindowFromProfile( const QString &path )
{
    qt_x_user_time = 0;
    kdDebug(1202) << "void KonquerorIface::createBrowserWindowFromProfile( const QString &path ) " << endl;
    kdDebug(1202) << path << endl;
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, QString::null );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileASN( const QString &path, const QCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return createBrowserWindowFromProfile( path );
}

DCOPRef KonquerorIface::createBrowserWindowFromProfile( const QString & path, const QString &filename )
{
    qt_x_user_time = 0;
    kdDebug(1202) << "void KonquerorIface::createBrowserWindowFromProfile( path, filename ) " << endl;
    kdDebug(1202) << path << "," << filename << endl;
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, filename );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileASN( const QString &path, const QString &filename,
    const QCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return createBrowserWindowFromProfile( path, filename );
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileAndURL( const QString & path, const QString &filename, const QString &url )
{
    qt_x_user_time = 0;
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, filename, KURL(url) );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileAndURLASN( const QString & path, const QString &filename, const QString &url,
    const QCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return createBrowserWindowFromProfileAndURL( path, filename, url );
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileAndURL( const QString &path, const QString &filename, const QString &url, const QString &mimetype )
{
    qt_x_user_time = 0;
    KParts::URLArgs args;
    args.serviceType = mimetype;
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, filename, KURL(url), args );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileAndURLASN( const QString & path, const QString &filename, const QString &url, const QString &mimetype,
    const QCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return createBrowserWindowFromProfileAndURL( path, filename, url, mimetype );
}


void KonquerorIface::reparseConfiguration()
{
  KGlobal::config()->reparseConfiguration();
  KonqFMSettings::reparseConfiguration();

  QPtrList<KonqMainWindow> *mainWindows = KonqMainWindow::mainWindowList();
  if ( mainWindows )
  {
    QPtrListIterator<KonqMainWindow> it( *mainWindows );
    for (; it.current(); ++it )
        it.current()->reparseConfiguration();
  }
}

void KonquerorIface::updateProfileList()
{
  QPtrList<KonqMainWindow> *mainWindows = KonqMainWindow::mainWindowList();
  if ( !mainWindows )
    return;

  QPtrListIterator<KonqMainWindow> it( *mainWindows );
  for (; it.current(); ++it )
    it.current()->viewManager()->profileListDirty( false );
}

QString KonquerorIface::crashLogFile()
{
  return KonqMainWindow::s_crashlog_file->name();
}

QValueList<DCOPRef> KonquerorIface::getWindows()
{
    QValueList<DCOPRef> lst;
    QPtrList<KonqMainWindow> *mainWindows = KonqMainWindow::mainWindowList();
    if ( mainWindows )
    {
      QPtrListIterator<KonqMainWindow> it( *mainWindows );
      for (; it.current(); ++it )
        lst.append( DCOPRef( kapp->dcopClient()->appId(), it.current()->dcopObject()->objId() ) );
    }
    return lst;
}

void KonquerorIface::addToCombo( QString url, QCString objId )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboAdd, url, objId );
}

void KonquerorIface::removeFromCombo( QString url, QCString objId )
{
  KonqMainWindow::comboAction( KonqMainWindow::ComboRemove, url, objId );
}

void KonquerorIface::comboCleared( QCString objId )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboClear,
				 QString::null, objId );
}

bool KonquerorIface::processCanBeReused( int screen )
{
    if( qt_xscreen() != screen )
        return false; // this instance run on different screen, and Qt apps can't migrate
    if( KonqMainWindow::isPreloaded())
        return false; // will be handled by preloading related code instead
    QPtrList<KonqMainWindow>* windows = KonqMainWindow::mainWindowList();
    if( windows == NULL )
        return true;
    QStringList allowed_parts = KonqSettings::safeParts();
    bool all_parts_allowed = false;
    
    if( allowed_parts.count() == 1 && allowed_parts.first() == QString::fromLatin1( "SAFE" ))
    {
        allowed_parts.clear();
        // is duplicated in client/kfmclient.cc
        allowed_parts << QString::fromLatin1( "konq_iconview.desktop" )
                      << QString::fromLatin1( "konq_multicolumnview.desktop" )
                      << QString::fromLatin1( "konq_sidebartng.desktop" )
                      << QString::fromLatin1( "konq_infolistview.desktop" )
                      << QString::fromLatin1( "konq_treeview.desktop" )
                      << QString::fromLatin1( "konq_detailedlistview.desktop" );
    }
    else if( allowed_parts.count() == 1 && allowed_parts.first() == QString::fromLatin1( "ALL" ))
    {
        allowed_parts.clear();
        all_parts_allowed = true;
    }
    if( all_parts_allowed )
        return true;
    for( QPtrListIterator<KonqMainWindow> it1( *windows );
         it1 != NULL;
         ++it1 )
    {
        kdDebug(1202) << "processCanBeReused: count=" << (*it1)->viewCount() << endl;
        const KonqMainWindow::MapViews& views = (*it1)->viewMap();
        for( KonqMainWindow::MapViews::ConstIterator it2 = views.begin();
             it2 != views.end();
             ++it2 )
        {
            kdDebug(1202) << "processCanBeReused: part=" << (*it2)->service()->desktopEntryPath() << ", URL=" << (*it2)->url().prettyURL() << endl;
            if( !allowed_parts.contains( (*it2)->service()->desktopEntryPath()))
                return false;
        }
    }
    return true;
}

void KonquerorIface::terminatePreloaded()
{
    if( KonqMainWindow::isPreloaded())
        kapp->exit();
}
