/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/


#include <qapplication.h>
#include <qwhatsthis.h>
#include <qstyle.h>
#include <qdir.h>

#include <kdebug.h>
#include <kmessagebox.h>
#include <kurifilter.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kwin.h>
#include <kprotocolinfo.h>
#include <kurldrag.h>

#include "konq_misc.h"
#include "konq_mainwindow.h"
#include "konq_viewmgr.h"
#include "konq_view.h"

/**********************************************
 *
 * KonqMisc
 *
 **********************************************/

// Terminates fullscreen-mode for any full-screen window on the current desktop
void KonqMisc::abortFullScreenMode()
{
  QPtrList<KonqMainWindow> *mainWindows = KonqMainWindow::mainWindowList();
  if ( mainWindows )
  {
    int currentDesktop = KWin::currentDesktop();
    QPtrListIterator<KonqMainWindow> it( *mainWindows );
    for (; it.current(); ++it )
    {
      if ( it.current()->fullScreenMode() )
      {
	KWin::Info info = KWin::info( it.current()->winId() );
	if ( info.desktop == currentDesktop )
          it.current()->slotToggleFullScreen();
      }
    }
  }
}

KonqMainWindow * KonqMisc::createSimpleWindow( const KURL & _url, const QString &frameName )
{
  abortFullScreenMode();

  // If _url is 0L, open $HOME [this doesn't happen anymore]
  KURL url = !_url.isEmpty() ? _url : KURL(QDir::homeDirPath().prepend( "file:" ));

  KonqMainWindow *win = new KonqMainWindow( url );
  win->setInitialFrameName( frameName );
  win->show();

  return win;
}

KonqMainWindow * KonqMisc::createNewWindow( const KURL &url, const KParts::URLArgs &args, bool forbidUseHTML )
{
  kdDebug() << "KonqMisc::createNewWindow url=" << url.url() << endl;

  // For HTTP or html files, use the web browsing profile, otherwise use filemanager profile
  QString profileName = (!(KProtocolInfo::supportsListing(url)) ||
                        KMimeType::findByURL(url)->name() == "text/html")
          ? "webbrowsing" : "filemanagement";

  QString profile = locate( "data", QString::fromLatin1("konqueror/profiles/") + profileName );
  return createBrowserWindowFromProfile( profile, profileName, url, args, forbidUseHTML );
}


KonqMainWindow * KonqMisc::createBrowserWindowFromProfile( const QString &path, const QString &filename, const KURL &url, const KParts::URLArgs &args, bool forbidUseHTML )
{
  kdDebug(1202) << "void KonqMisc::createBrowserWindowFromProfile() " << endl;
  kdDebug(1202) << "path=" << path << ",filename=" << filename << ",url=" << url.prettyURL() << endl;
  abortFullScreenMode();

  KonqMainWindow * mainWindow;
  if ( path.isEmpty() )
  {
      // The profile doesn't exit -> creating a simple window
      mainWindow = createSimpleWindow( url, args.frameName );
      if ( forbidUseHTML )
          mainWindow->setShowHTML( false );
  }
  else
  {
      mainWindow = new KonqMainWindow( KURL(), false );
      if ( forbidUseHTML )
          mainWindow->setShowHTML( false );
      //FIXME: obey args (like passing post-data (to KRun), etc.)
      KonqOpenURLRequest req;
      req.args = args;
      mainWindow->viewManager()->loadViewProfile( path, filename, url, req );
  }
  mainWindow->setInitialFrameName( args.frameName );
  mainWindow->show();
  return mainWindow;
}

QString KonqMisc::konqFilteredURL( QWidget* parent, const QString& _url, const QString& _path )
{
  if ( !_url.startsWith( "about:" ) ) // Don't filter "about:" URLs
  {
    KURIFilterData data = _url;

    if( !_path.isEmpty() )
      data.setAbsolutePath(_path);

    if( KURIFilter::self()->filterURI( data ) )
    {
      if( data.uriType() == KURIFilterData::ERROR && !data.errorMsg().isEmpty() )
        KMessageBox::sorry( parent, i18n( data.errorMsg().utf8() ) );
      else
        return data.uri().url();
    }
  }
  else if ( _url.startsWith( "about:" ) && _url != "about:blank" ) {
    // We can't use "about:" as it is, KURL doesn't parse it.
    return "about:konqueror";
  }
  return _url;  // return the original url if it cannot be filtered.
}

KonqDraggableLabel::KonqDraggableLabel( KonqMainWindow* mw, const QString& text )
  : QLabel( text, 0L, "kde toolbar widget" )	// Use this name for it to be styled!
  , m_mw(mw)
{
  setAlignment( (QApplication::reverseLayout() ? Qt::AlignRight : Qt::AlignLeft) |
                 Qt::AlignVCenter | Qt::ShowPrefix );
  setAcceptDrops(true);
  adjustSize();
  validDrag = false;
}

void KonqDraggableLabel::mousePressEvent( QMouseEvent * ev )
{
  validDrag = true;
  startDragPos = ev->pos();
}

void KonqDraggableLabel::mouseMoveEvent( QMouseEvent * ev )
{
  if ((startDragPos - ev->pos()).manhattanLength() > QApplication::startDragDistance())
  {
    validDrag = false;
    if ( m_mw->currentView() )
    {
      KURL::List lst;
      lst.append( m_mw->currentView()->url() );
      QDragObject * drag = KURLDrag::newDrag( lst, m_mw );
      drag->setPixmap( KMimeType::pixmapForURL( lst.first(), 0, KIcon::Small ) );
      drag->dragCopy();
    }
  }
}

void KonqDraggableLabel::mouseReleaseEvent( QMouseEvent * )
{
  validDrag = false;
}

void KonqDraggableLabel::dragEnterEvent( QDragEnterEvent *ev )
{
  if ( QUriDrag::canDecode( ev ) )
    ev->acceptAction();
}

void KonqDraggableLabel::dropEvent( QDropEvent* ev )
{
  KURL::List lst;
  if ( KURLDrag::decode( ev, lst ) ) {
    m_mw->openURL( 0L, lst.first() );
  }
}
