/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include "konq_mainwindow.h"
#include "konq_guiclients.h"
#include "KonqMainWindowIface.h"
#include "konq_view.h"
#include "konq_run.h"
#include "konq_misc.h"
#include "konq_viewmgr.h"
#include "konq_frame.h"
#include "konq_tabs.h"
#include "konq_events.h"
#include "konq_actions.h"
#include "konq_extensionmanager.h"
#include "delayedinitializer.h"
#include <konq_pixmapprovider.h>
#include <konq_operations.h>
#include <konqbookmarkmanager.h>
#include <kinputdialog.h>
#include <kshortcut.h>
#include <kzip.h>
#include <config.h>
#include <pwd.h>
// we define STRICT_ANSI to get rid of some warnings in glibc
#ifndef __STRICT_ANSI__
#define __STRICT_ANSI__
#define _WE_DEFINED_IT_
#endif
#include <netdb.h>
#ifdef _WE_DEFINED_IT_
#undef __STRICT_ANSI__
#undef _WE_DEFINED_IT_
#endif
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <klargefile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <qfile.h>
#include <qclipboard.h>
#include <qmetaobject.h>
#include <qvbox.h>
#include <qlayout.h>
#include <qfileinfo.h>
#include <qwhatsthis.h>

#include <dcopclient.h>
#include <kaboutdata.h>
#include <kbookmarkbar.h>
#include <kbookmarkmenu.h>
#include <kcmultidialog.h>
#include <kdebug.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <knewmenu.h>
#include <konq_defaults.h>
#include <konq_dirpart.h>
#include <konq_popupmenu.h>
#include <konq_settings.h>
#include "konq_main.h"
#include <konq_undo.h>
#include <kprotocolinfo.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <ksycoca.h>
#include <ktempfile.h>
#include <kurlrequesterdlg.h>
#include <kurlrequester.h>
#include <kuserprofile.h>
#include <kwin.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kpopupmenu.h>
#include <kprocess.h>
#include <kio/scheduler.h>
#include <kaccelmanager.h>
#include <kuser.h>
#include <netwm.h>

#ifdef KDE_MALLINFO_STDLIB
#include <stdlib.h>
#endif
#ifdef KDE_MALLINFO_MALLOC
#include <malloc.h>
#endif

#include <X11/Xlib.h>
#include <sys/time.h>
#include <X11/Xatom.h>
#include <fixx11h.h>

template class QPtrList<QPixmap>;
template class QPtrList<KToggleAction>;

QPtrList<KonqMainWindow> *KonqMainWindow::s_lstViews = 0;
KConfig * KonqMainWindow::s_comboConfig = 0;
KCompletion * KonqMainWindow::s_pCompletion = 0;
QFile * KonqMainWindow::s_crashlog_file = 0;
bool KonqMainWindow::s_preloaded = false;
KonqMainWindow* KonqMainWindow::s_preloadedWindow = 0;
int KonqMainWindow::s_initialMemoryUsage = -1;
time_t KonqMainWindow::s_startupTime;
int KonqMainWindow::s_preloadUsageCount;

KonqOpenURLRequest KonqOpenURLRequest::null;

static int current_memory_usage();

#include "konq_mainwindow_p.h"

KonqExtendedBookmarkOwner::KonqExtendedBookmarkOwner(KonqMainWindow *w)
{
   m_pKonqMainWindow = w;
}

KonqMainWindow::KonqMainWindow( const KURL &initialURL, bool openInitialURL, const char *name, const QString& xmluiFile)
 : KParts::MainWindow( NoDCOPObject, 0L, name, WDestructiveClose | WStyle_ContextHelp )
{
  setPreloadedFlag( false );

  if ( !s_lstViews )
    s_lstViews = new QPtrList<KonqMainWindow>;

  s_lstViews->append( this );

  m_urlCompletionStarted = false;

  m_currentView = 0L;
  m_pChildFrame = 0L;
  m_pActiveChild = 0L;
  m_pWorkingTab = 0L;
  m_initialKonqRun = 0L;
  m_pBookmarkMenu = 0L;
  m_dcopObject = new KonqMainWindowIface( this );
  m_combo = 0L;
  m_bURLEnterLock = false;
  m_bLocationBarConnected = false;
  m_paBookmarkBar = 0L;
  m_pURLCompletion = 0L;
  m_goBuffer = 0;
  m_configureDialog = 0L;

  m_bViewModeToggled = false;

  m_pViewManager = new KonqViewManager( this );

  m_toggleViewGUIClient = new ToggleViewGUIClient( this );

  m_openWithActions.setAutoDelete( true );
  m_viewModeActions.setAutoDelete( true );
  m_deleteActions.setAutoDelete( true );
  m_toolBarViewModeActions.setAutoDelete( true );
  m_viewModeMenu = 0;
  m_paCopyFiles = 0;
  m_paMoveFiles = 0;
  m_paDelete = 0;
  m_paNewDir = 0;
  m_bookmarkBarActionCollection = 0L;
  KonqExtendedBookmarkOwner *extOwner = new KonqExtendedBookmarkOwner( this );
  m_pBookmarksOwner = extOwner;
  connect( extOwner,
           SIGNAL( signalFillBookmarksList(KExtendedBookmarkOwner::QStringPairList &) ),
           extOwner,
           SLOT( slotFillBookmarksList(KExtendedBookmarkOwner::QStringPairList &) ) );

  KConfig *config = KGlobal::config();

  // init history-manager, load history, get completion object
  if ( !s_pCompletion ) {
    KonqHistoryManager *mgr = new KonqHistoryManager( kapp, "history mgr" );
    s_pCompletion = mgr->completionObject();


    // setup the completion object before createGUI(), so that the combo
    // picks up the correct mode from the HistoryManager (in slotComboPlugged)
    KConfigGroupSaver cs( config, QString::fromLatin1("Settings") );
    int mode = config->readNumEntry( "CompletionMode", KGlobalSettings::completionMode() );
    s_pCompletion->setCompletionMode( (KGlobalSettings::Completion) mode );
  }
  connect(KParts::HistoryProvider::self(), SIGNAL(cleared()), SLOT(slotClearComboHistory()));

  KonqPixmapProvider *prov = KonqPixmapProvider::self();
  if ( !s_comboConfig ) {
      s_comboConfig = new KConfig( "konq_history", false, false );
      KonqCombo::setConfig( s_comboConfig );
      s_comboConfig->setGroup( "Location Bar" );
      prov->load( s_comboConfig, "ComboIconCache" );
  }
  connect( prov, SIGNAL( changed() ), SLOT( slotIconsChanged() ) );

  initCombo();
  initActions();

  setInstance( KGlobal::instance() );

  connect( KSycoca::self(), SIGNAL( databaseChanged() ),
           this, SLOT( slotDatabaseChanged() ) );

  connect( kapp, SIGNAL( kdisplayFontChanged()), SLOT(slotReconfigure()));

  //load the xmlui file specified in the profile or the default konqueror.rc
  setXMLFile( xmluiFile );

  setStandardToolBarMenuEnabled( true );

  createGUI( 0L );

  connect(toolBarMenuAction(),SIGNAL(activated()),this,SLOT(slotForceSaveMainWindowSettings()) );

  if ( m_paDelete )
  {
    m_deleteActions.append( m_paDelete );
    plugActionList( "delactions", m_deleteActions );
  }

  if ( !m_toggleViewGUIClient->empty() )
    plugActionList( QString::fromLatin1( "toggleview" ), m_toggleViewGUIClient->actions() );
  else
  {
    delete m_toggleViewGUIClient;
    m_toggleViewGUIClient = 0;
  }

  // Those menus are created by konqueror.rc so their address will never change
  QPopupMenu *popup = static_cast<QPopupMenu*>(factory()->container("edit",this));
  if (popup)
    KAcceleratorManager::manage(popup);
  popup = static_cast<QPopupMenu*>(factory()->container("tools",this));
  if (popup)
    KAcceleratorManager::manage(popup);

  KConfigGroupSaver cgs(config,"MainView Settings");
  m_bSaveViewPropertiesLocally = config->readBoolEntry( "SaveViewPropertiesLocally", false );
  m_bHTMLAllowed = config->readBoolEntry( "HTMLAllowed", false );
  m_sViewModeForDirectory = config->readEntry( "ViewMode" );
  m_bBackRightClick = config->readBoolEntry( "BackRightClick", false );

  m_ptaUseHTML->setChecked( m_bHTMLAllowed );
  m_paSaveViewPropertiesLocally->setChecked( m_bSaveViewPropertiesLocally );

  KonqUndoManager::incRef();

  connect( KonqUndoManager::self(), SIGNAL( undoAvailable( bool ) ),
           this, SLOT( slotUndoAvailable( bool ) ) );
  m_bNeedApplyKonqMainWindowSettings = true;

  if ( !initialURL.isEmpty() )
  {
    openFilteredURL( initialURL.url() );
  }
  else if ( openInitialURL )
  {
    KURL homeURL;
    homeURL.setPath( QDir::homeDirPath() );
    openURL( 0L, homeURL );
  }
  else
      // silent
      m_bNeedApplyKonqMainWindowSettings = false;

  // Read basic main-view settings, and set to autosave
  setAutoSaveSettings( "KonqMainWindow", false );

  if ( !initialGeometrySet() )
      resize( 700, 480 );
  //kdDebug(1202) << "KonqMainWindow::KonqMainWindow " << this << " done" << endl;

  if( s_initialMemoryUsage == -1 )
  {
      s_initialMemoryUsage = current_memory_usage();
      s_startupTime = time( NULL );
      s_preloadUsageCount = 0;
  }
}

KonqMainWindow::~KonqMainWindow()
{
  kdDebug(1202) << "KonqMainWindow::~KonqMainWindow " << this << endl;

  delete m_pViewManager;

  if ( s_lstViews )
  {
    s_lstViews->removeRef( this );
    if ( s_lstViews->count() == 0 )
    {
      delete s_lstViews;
      s_lstViews = 0;
    }
  }

  disconnectActionCollection( actionCollection() );

  saveToolBarServicesMap();

  //  createShellGUI( false );

  delete m_pBookmarkMenu;
  delete m_paBookmarkBar;
  delete m_pURLCompletion;

  m_viewModeActions.clear();

  KonqUndoManager::decRef();

  if ( s_lstViews == 0 ) {
      delete KonqPixmapProvider::self();
      delete s_comboConfig;
      s_comboConfig = 0L;
  }

  delete m_configureDialog;
  m_configureDialog = 0L;
  delete m_dcopObject;
  m_dcopObject = 0L;
  delete m_combo;
  m_combo = 0L;
  delete m_locationLabel;
  m_locationLabel = 0L;

  kdDebug(1202) << "KonqMainWindow::~KonqMainWindow " << this << " done" << endl;
}

QWidget * KonqMainWindow::createContainer( QWidget *parent, int index, const QDomElement &element, int &id )
{
  static QString nameBookmarkBar = QString::fromLatin1( "bookmarkToolBar" );
  static QString tagToolBar = QString::fromLatin1( "ToolBar" );

  QWidget *res = KParts::MainWindow::createContainer( parent, index, element, id );

  if ( res && (element.tagName() == tagToolBar) && (element.attribute( "name" ) == nameBookmarkBar) )
  {
    assert( res->inherits( "KToolBar" ) );
    if (!kapp->authorizeKAction("bookmarks"))
    {
        delete res;
        return 0;
    }

    if ( !m_bookmarkBarActionCollection )
    {
        // The actual menu needs a different action collection, so that the bookmarks
        // don't appear in kedittoolbar
        m_bookmarkBarActionCollection = new KActionCollection( this );
        m_bookmarkBarActionCollection->setHighlightingEnabled( true );
        connectActionCollection( m_bookmarkBarActionCollection );
        DelayedInitializer *initializer = new DelayedInitializer( QEvent::Show, res );
        connect( initializer, SIGNAL( initialize() ), this, SLOT(initBookmarkBar()) );
    }
  }

  return res;
}

void KonqMainWindow::initBookmarkBar()
{
  KToolBar * bar = static_cast<KToolBar *>( child( "bookmarkToolBar", "KToolBar" ) );

  if (!bar) return;

  delete m_paBookmarkBar;
  m_paBookmarkBar = new KBookmarkBar( KonqBookmarkManager::self(), m_pBookmarksOwner, bar, m_bookmarkBarActionCollection, this );
  connect( m_paBookmarkBar,
           SIGNAL( aboutToShowContextMenu(const KBookmark &, QPopupMenu*) ),
           this, SLOT( slotFillContextMenu(const KBookmark &, QPopupMenu*) ));

  // hide if empty
  if (bar->count() == 0 )
     bar->hide();
}

void KonqMainWindow::removeContainer( QWidget *container, QWidget *parent, QDomElement &element, int id )
{
  static QString nameBookmarkBar = QString::fromLatin1( "bookmarkToolBar" );
  static QString tagToolBar = QString::fromLatin1( "ToolBar" );

  if ( element.tagName() == tagToolBar && element.attribute( "name" ) == nameBookmarkBar )
  {
    assert( container->inherits( "KToolBar" ) );
    if (m_paBookmarkBar)
      m_paBookmarkBar->clear();
  }

  KParts::MainWindow::removeContainer( container, parent, element, id );
}

// Note: this removes the filter from the URL.
QString KonqMainWindow::detectNameFilter( QString & url )
{
    // Look for wildcard selection
    QString nameFilter;

    int pos = url.findRev( '/' );

    if ( pos > -1 )
    {
      QString lastbit = url.mid( pos + 1 );

      if ( lastbit.find( '*' ) != -1 )
      {
        nameFilter = lastbit;
        url = url.left( pos + 1 );
        kdDebug(1202) << "Found wildcard. nameFilter=" << nameFilter << "  New url=" << url << endl;
      }
    }

    return nameFilter;
}

void KonqMainWindow::openFilteredURL( const QString & _url, bool inNewTab, bool tempFile )
{
    QString url = _url;
    QString nameFilter = detectNameFilter( url );

    // Filter URL to build a correct one
    if (m_currentDir.isEmpty() && m_currentView)
       m_currentDir = m_currentView->url().path(1);

    KURL filteredURL ( KonqMisc::konqFilteredURL( this, url, m_currentDir ) );
    kdDebug(1202) << "_url " << _url << " filtered into " << filteredURL.prettyURL() << endl;

    if ( filteredURL.isEmpty() ) // initially empty, or error (e.g. ~unknown_user)
        return;

    if ( !nameFilter.isEmpty() )
    {
        if (!KProtocolInfo::supportsListing(filteredURL))
        {
            // Protocol doesn't support listing. Ouch. Revert to full URL, no name-filtering.
            url = _url;
            nameFilter = QString::null;
            filteredURL = KonqMisc::konqFilteredURL( this, url, m_currentDir );
        }
    }
    m_currentDir = QString::null;

    // Remember the initial (typed) URL
    KonqOpenURLRequest req( _url );
    req.nameFilter = nameFilter;
    req.newTab = inNewTab;
    req.newTabInFront = true;
    req.tempFile = tempFile;

    openURL( 0L, filteredURL, QString::null, req );

    // #4070: Give focus to view after URL was entered manually
    // Note: we do it here if the view mode (i.e. part) wasn't changed
    // If it is changed, then it's done in KonqView::changeViewMode
    if ( m_currentView && m_currentView->part() )
      m_currentView->part()->widget()->setFocus();

}

void KonqMainWindow::openURL( KonqView *_view, const KURL &_url,
                              const QString &_serviceType, KonqOpenURLRequest& req,
                              bool trustedSource )
{
  kdDebug(1202) << "KonqMainWindow::openURL : url = '" << _url << "'  "
                << "serviceType='" << _serviceType << "' view=" << _view << endl;

  KURL url( _url );
  QString serviceType( _serviceType );
  if ( url.url() == "about:blank" )
  {
    serviceType = "text/html";
  }
  else if ( !url.isValid() )
  {
      KMessageBox::error(0, i18n("Malformed URL\n%1").arg(url.url()));
      return;
  }
  else if ( !KProtocolInfo::isKnownProtocol( url ) && url.protocol() != "about" )
  {
      KMessageBox::error(0, i18n("Protocol not supported\n%1").arg(url.protocol()));
      return;
  }

  KonqView *view = _view;

  // When clicking a 'follow active' view (e.g. view is the sidebar),
  // open the URL in the active view
  if ( view && view->isFollowActive() )
    view = m_currentView;

  if ( !view  && !req.newTab )
    view = m_currentView; /* Note, this can be 0L, e.g. on startup */
  else if ( !view && req.newTab ) {
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, QString::fromLatin1("FMSettings") );
    bool openAfterCurrentPage = config->readBoolEntry( "OpenAfterCurrentPage", false );
    view = m_pViewManager->addTab(QString::null,
                                  QString::null,
                                  false,
                                  openAfterCurrentPage);
    if (view) {
      view->setCaption( _url.host() );
      view->setLocationBarURL( _url );

      if ( req.newTabInFront )
        m_pViewManager->showTab( view );

        updateViewActions(); //A new tab created -- we may need to enable the "remove tab" button (#56318)
    }
    else
      req.newTab = false;
  }

  const QString oldLocationBarURL = m_combo->currentText();
  if ( view )
  {
    if ( view == m_currentView )
    {
      //will do all the stuff below plus GUI stuff
      abortLoading();
    }
    else
    {
      view->stop();
      // Don't change location bar if not current view
    }
  }

  // Fast mode for local files: do the stat ourselves instead of letting KRun do it.
  if ( serviceType.isEmpty() && url.isLocalFile() )
  {
    QCString _path( QFile::encodeName(url.path()));
    KDE_struct_stat buff;
    if ( KDE_stat( _path.data(), &buff ) != -1 )
        serviceType = KMimeType::findByURL( url, buff.st_mode )->name();
  }

  kdDebug(1202) << "trying openView for " << url << " (serviceType " << serviceType << ")" << endl;
  if ( ( !serviceType.isEmpty() && serviceType != "application/octet-stream") ||
         url.url() == "about:konqueror" || url.url() == "about:plugins" )
  {
    KService::Ptr offer = KServiceTypeProfile::preferredService(serviceType, "Application");
    // If the associated app is konqueror itself, then make sure we try to embed before bailing out.
    if ( isMimeTypeAssociatedWithSelf( serviceType, offer ) )
      req.forceAutoEmbed = true;

    // Built-in view ?
    if ( !openView( serviceType, url, view /* can be 0L */, req ) )
    {
        //kdDebug(1202) << "KonqMainWindow::openURL : openView returned false" << endl;
        // Are we following another view ? Then forget about this URL. Otherwise fire app.
        if ( !req.followMode )
        {
            //kdDebug(1202) << "KonqMainWindow::openURL : we were not following. Fire app." << endl;
            // We know the servicetype, let's try its preferred service
            if ( isMimeTypeAssociatedWithSelf( serviceType, offer ) ) {
                KMessageBox::error( this, i18n("There appears to be a configuration error. You have associated Konqueror with %1, but it cannot handle this file type.").arg(serviceType));
                return;
            }
            if ( !url.isLocalFile() && KonqRun::isTextExecutable( serviceType ) )
                serviceType = "text/plain"; // view, don't execute
            // Remote URL: save or open ?
            QString protClass = KProtocolInfo::protocolClass(url.protocol());
            bool open = url.isLocalFile() || protClass==":local";
            if ( !open ) {
                KParts::BrowserRun::AskSaveResult res = KonqRun::askSave( url, offer, serviceType );
                if ( res == KParts::BrowserRun::Save )
                    KParts::BrowserRun::simpleSave( url, QString::null, this );
                open = ( res == KParts::BrowserRun::Open );
            }
            if ( open )
            {
                KURL::List lst;
                lst.append(url);
                //kdDebug(1202) << "Got offer " << (offer ? offer->name().latin1() : "0") << endl;
                if ( ( trustedSource || KonqRun::allowExecution( serviceType, url ) ) &&
                     ( KonqRun::isExecutable( serviceType ) || !offer || !KRun::run( *offer, lst ) ) )
                {
                    setLocationBarURL( oldLocationBarURL ); // Revert to previous locationbar URL
                    (void)new KRun( url, this );
                }
            }
        }
    }
  }
  else // no known serviceType, use KonqRun
  {
      if ( ( view && view == m_currentView ) ||
              ( !view && !req.newTab ) ) // startup with argument
      {
          // Show it for now in the location bar, but we'll need to store it in the view
          // later on (can't do it yet since either view == 0 or updateHistoryEntry will be called).
          kdDebug(1202) << "setLocationBarURL : url = " << url << endl;
          setLocationBarURL( url );
      }

      kdDebug(1202) << "Creating new konqrun for " << url << " req.typedURL=" << req.typedURL << endl;

      KonqRun * run = new KonqRun( this, view /* can be 0L */, url, req, trustedSource );

      // Never start in external browser
      run->setEnableExternalBrowser(false);

      if ( view )
        view->setRun( run );
      else if ( !req.newTab )
      {
        // there can be only one :) (when not a new tab)
          delete m_initialKonqRun;
          m_initialKonqRun = run;
      }

      if ( view == m_currentView )
        startAnimation();

      connect( run, SIGNAL( finished() ), this, SLOT( slotRunFinished() ) );
  }
}

bool KonqMainWindow::openView( QString serviceType, const KURL &_url, KonqView *childView, KonqOpenURLRequest& req )
{
  // TODO: Replace KURL() with referring URL. DF: done, please check.
  if ( !kapp->authorizeURLAction("open", childView ? childView->url() : KURL(), _url) )
  {
     QString msg = KIO::buildErrorString(KIO::ERR_ACCESS_DENIED, _url.prettyURL());
     KMessageBox::queuedMessageBox( this, KMessageBox::Error, msg );
     return true; // Nothing else to do.
  }

  if ( KonqRun::isExecutable( serviceType ) )
     return false; // execute, don't open
  // Contract: the caller of this method should ensure the view is stopped first.

#ifndef NDEBUG
  kdDebug(1202) << "KonqMainWindow::openView " << serviceType << " " << _url << " " << childView << " req:" << req.debug() << endl;
#endif
  bool bOthersFollowed = false;

  if ( childView )
  {
    // If we're not already following another view (and if we are not reloading)
    if ( !req.followMode && !req.args.reload && !m_pViewManager->isLoadingProfile() )
    {
      // When clicking a 'follow active' view (e.g. childView is the sidebar),
      // open the URL in the active view
      // (it won't do anything itself, since it's locked to its location)
      if ( childView->isFollowActive() && childView != m_currentView )
      {
        abortLoading();
        setLocationBarURL( _url );
        KonqOpenURLRequest newreq;
        newreq.followMode = true;
        newreq.args = req.args;
        bOthersFollowed = openView( serviceType, _url, m_currentView, newreq );
      }
      // "link views" feature, and "sidebar follows active view" feature
      bOthersFollowed = makeViewsFollow(_url, req.args, serviceType, childView) || bOthersFollowed;
    }
    if ( childView->isLockedLocation() && !req.args.reload /* allow to reload a locked view*/ )
      return bOthersFollowed;
  }

  QString indexFile;

  KURL url( _url );

  // Generic mechanism for redirecting to tar:/<path>/ when clicking on a tar file,
  // zip:/<path>/ when clicking on a zip file, etc.
  // The name of the protocol to redirect to, is read from the mimetype's .desktop file
  if ( url.isLocalFile() )
  {
    KServiceType::Ptr ptr = KServiceType::serviceType( serviceType );
    if ( ptr )
    {
      const QString protocol = ptr->property("X-KDE-LocalProtocol").toString();
      if ( !protocol.isEmpty() )
      {
        url.setProtocol( protocol );
        if ( serviceType == "application/x-webarchive" )
        {
          url.setPath( url.path() + "/index.html" );
          serviceType = "text/html";
        }
        else
        {
          url.setPath( url.path() + '/' );
          serviceType = "inode/directory";
        }
      }
    }
  }

  ///////////

  // In case we open an index.html, we want the location bar
  // to still display the original URL (so that 'up' uses that URL,
  // and since that's what the user entered).
  // changeViewMode will take care of setting and storing that url.
  QString originalURL = url.prettyURL(0, KURL::StripFileProtocol);
  if ( !req.nameFilter.isEmpty() ) // keep filter in location bar
  {
    if (originalURL.right(1) != "/")
        originalURL += '/';
    originalURL += req.nameFilter;
  }

  QString serviceName; // default: none provided

  if ( url.url() == "about:konqueror" || url.url() == "about:plugins" )
  {
      serviceType = "KonqAboutPage"; // not KParts/ReadOnlyPart, it fills the Location menu ! :)
      serviceName = "konq_aboutpage";
      originalURL = req.typedURL.isEmpty() ? QString::null : url.url();
      // empty if from profile, about:konqueror if the user typed it (not req.typedURL, it could be "about:")
  }
  else if ( url.url() == "about:blank" && req.typedURL.isEmpty() )
  {
      originalURL = QString::null;
  }

  // Look for which view mode to use, if a directory - not if view locked
  if ( ( !childView || (!childView->isLockedViewMode()) )
       && serviceType == "inode/directory" )
    { // Phew !

      // Set view mode if necessary (current view doesn't support directories)
      if ( !childView || !childView->supportsServiceType( serviceType ) )
        serviceName = m_sViewModeForDirectory;
      //kdDebug(1202) << "serviceName=" << serviceName << " m_sViewModeForDirectory=" << m_sViewModeForDirectory << endl;

      if ( url.isLocalFile() ) // local, we can do better (.directory)
        {
          // Read it in the .directory file, default to m_bHTMLAllowed
          KURL urlDotDir( url );
          urlDotDir.addPath(".directory");
          bool HTMLAllowed = m_bHTMLAllowed;
          QFile f( urlDotDir.path() );
          if ( f.open(IO_ReadOnly) )
            {
              f.close();
              KSimpleConfig config( urlDotDir.path(), true );
              config.setGroup( "URL properties" );
              HTMLAllowed = config.readBoolEntry( "HTMLAllowed", m_bHTMLAllowed );
              serviceName = config.readEntry( "ViewMode", serviceName );
              kdDebug(1202) << "serviceName=" << serviceName << endl;
            }
          if ( HTMLAllowed &&
               ( ( indexFile = findIndexFile( url.path() ) ) != QString::null ) )
            {
              serviceType = "text/html";
              url = KURL();
              url.setPath( indexFile );
              serviceName = QString::null; // cancel what we just set, this is not a dir finally
            }

          // Reflect this setting in the menu
          m_ptaUseHTML->setChecked( HTMLAllowed );
        }
    }

  bool ok = true;
  if ( !childView )
  {
      if (req.newTab)
      {
          KonqFrameTabs* tabContainer = 0L;
          int index = 0;
          if ( m_pViewManager->docContainer() && m_pViewManager->docContainer()->frameType() == "Tabs")
          {
              tabContainer = static_cast<KonqFrameTabs*>(m_pViewManager->docContainer());
              index = tabContainer->currentPageIndex();
          }
          childView = m_pViewManager->addTab( serviceType, serviceName, false, req.openAfterCurrentPage );

          if (req.newTabInFront && childView)
          {
              if ( !tabContainer )
                  tabContainer = static_cast<KonqFrameTabs*>(m_pViewManager->docContainer());
              if ( req.openAfterCurrentPage )
                  tabContainer->setCurrentPage( index + 1 );
              else
                  tabContainer->setCurrentPage( tabContainer->count()-1 );
          }
      }

      else
      {
        // Create a new view
        // Initialize always uses force auto-embed even if user setting is "separate viewer",
        // since this window has no view yet - we don't want to keep an empty mainwindow.
        // This can happen with e.g. application/pdf from a target="_blank" link, or window.open.
        childView = m_pViewManager->Initialize( serviceType, serviceName );

        if ( childView )
        {
            enableAllActions( true );

            m_pViewManager->setActivePart( childView->part() );

            childView->setViewName( m_initialFrameName.isEmpty() ? req.args.frameName : m_initialFrameName );
            m_initialFrameName = QString::null;
            m_currentView = childView;
        }
      }

      if ( !childView )
          return false; // It didn't work out.
  }
  else // We know the child view
  {
      if ( !childView->isLockedViewMode() )
      {
          bool forceAutoEmbed = req.forceAutoEmbed || req.newTab;
          if ( !req.typedURL.isEmpty() ) // the user _typed_ the URL, he wants it in Konq.
              forceAutoEmbed = true;
          if ( url.protocol() == "about" )
              forceAutoEmbed = true;
          // Related to KonqFactory::createView
          if ( !forceAutoEmbed && !KonqFMSettings::settings()->shouldEmbed( serviceType ) )
          {
              kdDebug(1202) << "openView: KonqFMSettings says: don't embed this servicetype" << endl;
              ok = false;
          }

          // If the protocol doesn't support writing (e.g. HTTP) then we might want to save instead of just embedding.
          // So (if embedding would succeed, hence the checks above) we ask the user
          // Otherwise the user will get asked 'open or save' in openURL anyway.
          if ( ok && !forceAutoEmbed && !KProtocolInfo::supportsWriting( url ) ) {
              QString suggestedFilename;

              KonqRun* run = childView->run();
              if (run)
                  suggestedFilename = run->suggestedFilename();

              KParts::BrowserRun::AskSaveResult res = KParts::BrowserRun::askEmbedOrSave(
                  url, serviceType, suggestedFilename );
              if ( res == KParts::BrowserRun::Open )
                  forceAutoEmbed = true;
              else if ( res == KParts::BrowserRun::Cancel )
                  return true; // handled, don't do anything else
              else { // Save
                  KParts::BrowserRun::simpleSave( url, suggestedFilename, this );
                  return true; // handled
              }
          }
          if ( ok )
              ok = childView->changeViewMode( serviceType, serviceName, forceAutoEmbed );
      }
  }

  if (ok)
  {
      //kdDebug(1202) << "req.nameFilter= " << req.nameFilter << endl;
      //kdDebug(1202) << "req.typedURL= " << req.typedURL << endl;
      //kdDebug(1202) << "Browser extension? " << (childView->browserExtension() ? "YES" : "NO") << endl;
      //kdDebug(1202) << "Referrer: " << req.args.metaData()["referrer"] << endl;
      childView->setTypedURL( req.typedURL );
      if ( childView->browserExtension() )
          childView->browserExtension()->setURLArgs( req.args );
      if ( childView->part()->inherits("KonqDirPart") )
          static_cast<KonqDirPart *>(childView->part())->setFilesToSelect( req.filesToSelect );
      if ( !url.isEmpty() )
          childView->openURL( url, originalURL, req.nameFilter, req.tempFile );
  }
  kdDebug(1202) << "KonqMainWindow::openView ok=" << ok << " bOthersFollowed=" << bOthersFollowed << " returning "
                << (ok || bOthersFollowed)
                << endl;
  return ok || bOthersFollowed;
}

void KonqMainWindow::slotOpenURLRequest( const KURL &url, const KParts::URLArgs &args )
{
  kdDebug(1202) << "KonqMainWindow::slotOpenURLRequest frameName=" << args.frameName << endl;

  KParts::ReadOnlyPart *callingPart = static_cast<KParts::ReadOnlyPart *>( sender()->parent() );
  QString frameName = args.frameName;

  if ( !frameName.isEmpty() )
  {
    static QString _top = QString::fromLatin1( "_top" );
    static QString _self = QString::fromLatin1( "_self" );
    static QString _parent = QString::fromLatin1( "_parent" );
    static QString _blank = QString::fromLatin1( "_blank" );

    if ( frameName.lower() == _blank )
    {
      slotCreateNewWindow( url, args );
      return;
    }

    if ( frameName.lower() != _top &&
         frameName.lower() != _self &&
         frameName.lower() != _parent )
    {
      KParts::BrowserHostExtension *hostExtension = 0;
      KonqView *view = childView( callingPart, frameName, &hostExtension, 0 );
      if ( !view )
      {
        KonqMainWindow *mainWindow = 0;
        view = findChildView( callingPart, frameName, &mainWindow, &hostExtension, 0 );

        if ( !view || !mainWindow )
        {
          slotCreateNewWindow( url, args );
          return;
        }

        if ( hostExtension )
          hostExtension->openURLInFrame( url, args );
        else
          mainWindow->openURL( view, url, args );
        return;
      }

      if ( hostExtension )
        hostExtension->openURLInFrame( url, args );
      else
        openURL( view, url, args );
      return;
    }
  }

  KonqView *view = childView( callingPart );
  openURL( view, url, args );
}

//Called by slotOpenURLRequest
void KonqMainWindow::openURL( KonqView *childView, const KURL &url, const KParts::URLArgs &args )
{
  kdDebug(1202) << "KonqMainWindow::openURL (from slotOpenURLRequest) url=" << url.prettyURL() << endl;
  KonqOpenURLRequest req;
  req.args = args;

  // Clicking on a link that points to the page itself (e.g. anchor)
  if ( !args.doPost() && !args.reload &&
          childView && urlcmp( url.url(), childView->url().url(), true, true ) )
  {
    QString serviceType = args.serviceType;
    if ( serviceType.isEmpty() )
      serviceType = childView->serviceType();

    childView->stop();
    req.forceAutoEmbed = true;
    openView( serviceType, url, childView, req );
    return;
  }

  openURL( childView, url, args.serviceType, req, args.trustedSource );
}

QObject *KonqMainWindow::lastFrame( KonqView *view )
{
  QObject *nextFrame, *viewFrame;
  nextFrame = view->frame();
  viewFrame = 0;
  while ( nextFrame != 0 && ! nextFrame->inherits( "QWidgetStack" ) ) {
    viewFrame = nextFrame;
    nextFrame = nextFrame->parent();
  }
  return nextFrame ? viewFrame : 0L;
}

// Linked-views feature, plus "sidebar follows URL opened in the active view" feature
bool KonqMainWindow::makeViewsFollow( const KURL & url, const KParts::URLArgs &args,
                                      const QString & serviceType, KonqView * senderView )
{
  if ( !senderView->isLinkedView() && senderView != m_currentView )
    return false; // none of those features apply -> return

  bool res = false;
  //kdDebug(1202) << "makeViewsFollow " << senderView->className() << " url=" << url << " serviceType=" << serviceType << endl;
  KonqOpenURLRequest req;
  req.followMode = true;
  req.args = args;
  // We can't iterate over the map here, and openURL for each, because the map can get modified
  // (e.g. by part changes). Better copy the views into a list.
  QPtrList<KonqView> listViews;
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    listViews.append( it.data() );

  QObject *senderFrame = lastFrame( senderView );

  for ( KonqView * view = listViews.first() ; view ; view = listViews.next())
  {
    bool followed = false;
    // Views that should follow this URL as both views are linked
    if ( (view != senderView) && view->isLinkedView() && senderView->isLinkedView() )
    {
      QObject *viewFrame = lastFrame( view );

      // Only views in the same tab of the sender will follow
      if ( senderFrame && viewFrame && viewFrame != senderFrame )
        continue;

      kdDebug(1202) << "makeViewsFollow: Sending openURL to view " << view->part()->className() << " url=" << url << endl;

      // XXX duplicate code from ::openURL
      if ( view == m_currentView )
      {
        abortLoading();
        setLocationBarURL( url );
      }
      else
        view->stop();

      followed = openView( serviceType, url, view, req );
    }
    else
    {
      // Make the sidebar follow the URLs opened in the active view
      if ((view!=senderView) && view->isFollowActive() && senderView == m_currentView)
      {
        followed = openView(serviceType, url, view, req);
      }
    }

    // Ignore return value if the view followed but doesn't really
    // show the file contents. We still want to see that
    // file, e.g. in a separate viewer.
    // This happens in views locked to a directory mode,
    // like sidebar and konsolepart (#52161).
    bool ignore = view->isLockedViewMode() && view->supportsServiceType("inode/directory");
    //kdDebug(1202) << "View " << view->service()->name()
    //              << " supports dirs: " << view->supportsServiceType( "inode/directory" )
    //              << " is locked-view-mode:" << view->isLockedViewMode()
    //              << " ignore=" << ignore << endl;
    if ( !ignore )
      res = followed || res;
  }

  return res;
}

void KonqMainWindow::abortLoading()
{
  //kdDebug(1202) << "KonqMainWindow::abortLoading()" << endl;
  if ( m_currentView )
  {
    m_currentView->stop(); // will take care of the statusbar
    stopAnimation();
  }
}

void KonqMainWindow::slotCreateNewWindow( const KURL &url, const KParts::URLArgs &args )
{
    kdDebug(1202) << "KonqMainWindow::slotCreateNewWindow url=" << url.prettyURL() << endl;

    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, QString::fromLatin1("FMSettings") );
    if ( args.newTab() || (config->readBoolEntry( "MMBOpensTab", false ) &&
         (const_cast<KParts::URLArgs*>(&args)->metaData()["forcenewwindow"]).isEmpty()) ) {
      KonqOpenURLRequest req;
      req.newTab = true;
      req.newTabInFront = config->readBoolEntry( "NewTabsInFront", false );
      if (KApplication::keyboardMouseState() & Qt::ShiftButton)
        req.newTabInFront = !req.newTabInFront;
      req.args = args;
      openURL( 0L, url, QString::null, req );
    }
    else
    {
      KonqMisc::createNewWindow( url, args );
    }
}

// This is mostly for the JS window.open call
void KonqMainWindow::slotCreateNewWindow( const KURL &url, const KParts::URLArgs &args,
                                          const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart *&part )
{
    kdDebug(1202) << "KonqMainWindow::slotCreateNewWindow(4 args) url=" << url.prettyURL()
                  << " args.serviceType=" << args.serviceType
                  << " args.frameName=" << args.frameName << endl;

    part = 0; // Make sure to be initialized in case of failure...

    KonqMainWindow *mainWindow = 0L;
    if ( !args.frameName.isEmpty() && args.frameName.lower() != "_blank" )
    {
        KParts::BrowserHostExtension *hostExtension = 0;
        KParts::ReadOnlyPart *ro_part = 0L;
        KParts::BrowserExtension *be = ::qt_cast<KParts::BrowserExtension *>(sender());
        if (be)
            ro_part = ::qt_cast<KParts::ReadOnlyPart *>(be->parent());
        if ( findChildView( ro_part, args.frameName, &mainWindow, &hostExtension, &part ) )
        {
            // Found a view. If url isn't empty, we should open it - but this never happens currently
            // findChildView put the resulting part in 'part', so we can just return now
            //kdDebug() << " frame=" << args.frameName << " -> found part=" << part << " " << part->name() << endl;
            return;
        }
    }

    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, QString::fromLatin1("FMSettings") );
    if ( config->readBoolEntry( "PopupsWithinTabs", false ) || (config->readBoolEntry( "MMBOpensTab", false ) && windowArgs.lowerWindow ) ) {
        bool aftercurrentpage = config->readBoolEntry( "OpenAfterCurrentPage", false );
        bool newtabsinfront = config->readBoolEntry( "NewTabsInFront", false );
        if ( windowArgs.lowerWindow )
           newtabsinfront =! newtabsinfront;

        KonqView* newView = m_pViewManager->addTab(QString::null, QString::null, false, aftercurrentpage);
        if (newView == 0L) return;

        if (newtabsinfront)
          m_pViewManager->showTab( newView );

        openURL( newView, url.isEmpty() ? KURL("about:blank") : url, QString::null);
        newView->setViewName( args.frameName );
        part=newView->part();

        return;
    }

    mainWindow = new KonqMainWindow( KURL(), false );
    mainWindow->setInitialFrameName( args.frameName );
    mainWindow->resetAutoSaveSettings(); // Don't autosave

    KonqOpenURLRequest req;
    req.args = args;

    if ( args.serviceType.isEmpty() )
      mainWindow->openURL( 0L, url, QString::null, req );
    else if ( !mainWindow->openView( args.serviceType, url, 0L, req ) )
    {
      // we have problems. abort.
      delete mainWindow;
      part = 0;
      return;
    }

    KonqView * view = 0L;
    // cannot use activePart/currentView, because the activation through the partmanager
    // is delayed by a singleshot timer (see KonqViewManager::setActivePart)
    if ( mainWindow->viewMap().count() )
    {
      MapViews::ConstIterator it = mainWindow->viewMap().begin();
      view = it.data();
      part = it.key();
    }

    // activate the view _now_ in order to make the menuBar() hide call work
    if ( part ) {
       mainWindow->viewManager()->setActivePart( part, true );
    }

    QString profileName = QString::fromLatin1( url.isLocalFile() ? "konqueror/profiles/filemanagement" : "konqueror/profiles/webbrowsing" );
    KSimpleConfig cfg( locate( "data", profileName ), true );
    cfg.setGroup( "Profile" );

    if ( windowArgs.x != -1 )
        mainWindow->move( windowArgs.x, mainWindow->y() );
    if ( windowArgs.y != -1 )
        mainWindow->move( mainWindow->x(), windowArgs.y );

    QSize size = KonqViewManager::readConfigSize( cfg, mainWindow );

    int width;
    if ( windowArgs.width != -1 )
        width = windowArgs.width;
    else
        width = size.isValid() ? size.width() : mainWindow->width();

    int height;
    if ( windowArgs.height != -1 )
        height = windowArgs.height;
    else
        height = size.isValid() ? size.height() : mainWindow->height();

    mainWindow->resize( width, height );

    // process the window args

    if ( !windowArgs.menuBarVisible )
    {
        mainWindow->menuBar()->hide();
        mainWindow->m_paShowMenuBar->setChecked( false );
    }

    if ( !windowArgs.toolBarsVisible )
    {
      for ( QPtrListIterator<KToolBar> it( mainWindow->toolBarIterator() ); it.current(); ++it )
      {
        (*it)->hide();
      }
    }

    if ( view && !windowArgs.statusBarVisible )
        view->frame()->statusbar()->hide();

    if ( !windowArgs.resizable )
        // ### this doesn't seem to work :-(
        mainWindow->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );

// Trying to show the window initially behind the current window is a bit tricky,
// as this involves the window manager, which may see things differently.
// Many WMs raise and activate new windows, which means without WM support this won't work very
// well. If the WM has support for _NET_WM_USER_TIME, it will be just set to 0 (=don't focus on show),
// and the WM should take care of it itself.
    bool wm_usertime_support = false;
    extern Time qt_x_user_time;
    Time saved_last_input_time = qt_x_user_time;
    if ( windowArgs.lowerWindow )
    {
        NETRootInfo wm_info( qt_xdisplay(), NET::Supported );
        wm_usertime_support = wm_info.isSupported( NET::WM2UserTime );
        if( wm_usertime_support )
        {
        // *sigh*, and I thought nobody would need QWidget::dontFocusOnShow().
        // Avoid Qt's support for user time by setting it to 0, and
        // set the property ourselves.
            qt_x_user_time = 0;
            KWin::setUserTime( mainWindow->winId(), 0 );
        }
        // Put below the current window before showing, in case that actually works with the WM.
        // First do complete lower(), then stackUnder(), because the latter may not work with many WMs.
        mainWindow->lower();
        mainWindow->stackUnder( this );
    }

    mainWindow->show();

    if ( windowArgs.lowerWindow )
    {
        qt_x_user_time = saved_last_input_time;
        if( !wm_usertime_support )
        { // No WM support. Let's try ugly tricks.
            mainWindow->lower();
            mainWindow->stackUnder( this );
            if( this->isActiveWindow())
                this->setActiveWindow();
        }
    }

    if ( windowArgs.fullscreen )
        mainWindow->action( "fullscreen" )->activate();
}

void KonqMainWindow::slotNewWindow()
{
  // Use profile from current window, if set
  QString profile = m_pViewManager->currentProfile();
  if ( profile.isEmpty() )
  {
    if ( m_currentView && m_currentView->url().protocol().startsWith( "http" ) )
       profile = QString::fromLatin1("webbrowsing");
    else
       profile = QString::fromLatin1("filemanagement");
  }
  KonqMisc::createBrowserWindowFromProfile(
    locate( "data", QString::fromLatin1("konqueror/profiles/")+profile ),
    profile );
}

void KonqMainWindow::slotDuplicateWindow()
{
  KTempFile tempFile;
  tempFile.setAutoDelete( true );
  KConfig config( tempFile.name() );
  config.setGroup( "View Profile" );
  m_pViewManager->saveViewProfile( config, true, true );

  KonqMainWindow *mainWindow = new KonqMainWindow( KURL(), false, 0, xmlFile());
  mainWindow->viewManager()->loadViewProfile( config, m_pViewManager->currentProfile() );
  if (mainWindow->currentView())
  {
      mainWindow->copyHistory( childFrame() );
  }
  mainWindow->activateChild();
  mainWindow->show();
#ifndef NDEBUG
  mainWindow->viewManager()->printFullHierarchy( this );
#endif
}

void KonqMainWindow::slotSendURL()
{
  KURL::List lst = currentURLs();
  QString body;
  QString fileNameList;
  for ( KURL::List::Iterator it = lst.begin() ; it != lst.end() ; ++it )
  {
    if ( !body.isEmpty() ) body += '\n';
    body += (*it).prettyURL();
    if ( !fileNameList.isEmpty() ) fileNameList += ", ";
    fileNameList += (*it).fileName();
  }
  QString subject;
  if ( m_currentView && !m_currentView->part()->inherits("KonqDirPart") )
    subject = m_currentView->caption();
  else
    subject = fileNameList;
  kapp->invokeMailer(QString::null,QString::null,QString::null,
                     subject, body);
}

void KonqMainWindow::slotSendFile()
{
  KURL::List lst = currentURLs();
  QStringList urls;
  QString fileNameList;
  for ( KURL::List::Iterator it = lst.begin() ; it != lst.end() ; ++it )
  {
    if ( !fileNameList.isEmpty() ) fileNameList += ", ";
    if ( (*it).isLocalFile() && QFileInfo((*it).path()).isDir() )
    {
        // Create a temp dir, so that we can put the ZIP file in it with a proper name
        KTempFile zipFile;
        QString zipFileName = zipFile.name();
        zipFile.unlink();

        QDir().mkdir(zipFileName,true);
        zipFileName = zipFileName+"/"+(*it).fileName()+".zip";
        KZip zip( zipFileName );
        if ( !zip.open( IO_WriteOnly ) )
            continue; // TODO error message
        zip.addLocalDirectory( (*it).path(), QString::null );
        zip.close();
        fileNameList += (*it).fileName()+".zip";
        urls.append( zipFileName );
    }
    else
    {
        fileNameList += (*it).fileName();
        urls.append( (*it).url() );
    }
  }
  QString subject;
  if ( m_currentView && !m_currentView->part()->inherits("KonqDirPart") )
    subject = m_currentView->caption();
  else
    subject = fileNameList;
  kapp->invokeMailer(QString::null, QString::null, QString::null, subject,
                     QString::null, //body
                     QString::null,
                     urls); // attachments
}

void KonqMainWindow::slotRun()
{
  // HACK: The command is not executed in the directory
  // we are in currently. Minicli does not support that yet
  QByteArray data;
  kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "popupExecuteCommand()", data );
}

void KonqMainWindow::slotOpenTerminal()
{
  KConfig *config = KGlobal::config();
  config->setGroup( "General" );
  QString term = config->readPathEntry( "TerminalApplication", DEFAULT_TERMINAL );

  QString dir ( QDir::homeDirPath() );

  if ( m_currentView )
  {
      KURL u( m_currentView->url() );
      if ( u.isLocalFile() )
      {
          if ( m_currentView->serviceType() == "inode/directory" )
              dir = u.path();
          else
              dir = u.directory();
  }
  }

  KProcess cmd;
  cmd.setWorkingDirectory(dir);
  cmd << term;
  kdDebug(1202) << "slotOpenTerminal: directory " << dir
		<< ", terminal:" << term << endl;
  cmd.start(KProcess::DontCare);
}

void KonqMainWindow::slotOpenLocation()
{
  // Don't pre-fill the url, as it is auto-selected and thus overwrites the
  // X clipboard, making it impossible to paste in the url you really wanted.
  // Another example of why the X clipboard sux
  KURLRequesterDlg dlg( QString::null, this, 0, true);
  dlg.setCaption( i18n("Open Location") );
  // Set current directory for relative paths.
  // Testcase: konqueror www.kde.org; Ctrl+O; file in $HOME; would open http://$file
  QString currentDir;
  if (m_currentView && m_currentView->url().isLocalFile())
      currentDir = m_currentView->url().path(1);
  dlg.urlRequester()->completionObject()->setDir( currentDir );
  dlg.exec();
  const KURL& url = dlg.selectedURL();
  if (!url.isEmpty())
     openFilteredURL( url.url().stripWhiteSpace() );
}

void KonqMainWindow::slotToolFind()
{
  kdDebug() << "KonqMainWindow::slotToolFind sender:" << sender()->className() << endl;

  if ( m_currentView && m_currentView->part()->inherits("KonqDirPart") )
  {
    KonqDirPart* dirPart = static_cast<KonqDirPart *>(m_currentView->part());

    if (!m_paFindFiles->isChecked())
    {
        dirPart->slotFindClosed();
        return;
    }

    KonqViewFactory factory = KonqFactory::createView( "Konqueror/FindPart" );
    if ( factory.isNull() )
    {
        KMessageBox::error( this, i18n("Cannot create the find part, check your installation.") );
        m_paFindFiles->setChecked(false);
        return;
    }

    KParts::ReadOnlyPart* findPart = factory.create( m_currentView->frame(), "findPartWidget", dirPart, "findPart" );
    dirPart->setFindPart( findPart );

    m_currentView->frame()->insertTopWidget( findPart->widget() );
    findPart->widget()->show();
    findPart->widget()->setFocus();

    connect( dirPart, SIGNAL( findClosed(KonqDirPart *) ),
             this, SLOT( slotFindClosed(KonqDirPart *) ) );
  }
  else if ( sender()->inherits( "KAction" ) ) // don't go there if called by the singleShot below
  {
      KURL url;
      if ( m_currentView && m_currentView->url().isLocalFile() )
          url = m_currentView->locationBarURL();
      else
          url.setPath( QDir::homeDirPath() );
      KonqMainWindow * mw = KonqMisc::createBrowserWindowFromProfile(
          locate( "data", QString::fromLatin1("konqueror/profiles/filemanagement") ),
          "filemanagement", url, KParts::URLArgs(), true /* forbid "use html"*/ );
      mw->m_paFindFiles->setChecked(true);
      // Delay it after the openURL call (hacky!)
      QTimer::singleShot( 1, mw, SLOT(slotToolFind()));
      m_paFindFiles->setChecked(false);
  }
}

void KonqMainWindow::slotFindOpen( KonqDirPart * dirPart )
{
    kdDebug(1202) << "KonqMainWindow::slotFindOpen " << dirPart << endl;
    Q_ASSERT( m_currentView );
    Q_ASSERT( m_currentView->part() == dirPart );
    slotToolFind(); // lazy me
}

void KonqMainWindow::slotFindClosed( KonqDirPart * dirPart )
{
    kdDebug(1202) << "KonqMainWindow::slotFindClosed " << dirPart << endl;
    KonqView * dirView = m_mapViews.find( dirPart ).data();
    Q_ASSERT(dirView);
    kdDebug(1202) << "dirView=" << dirView << endl;
    if ( dirView && dirView == m_currentView )
        m_paFindFiles->setEnabled( true );
    m_paFindFiles->setChecked(false);
}

void KonqMainWindow::slotIconsChanged()
{
    m_combo->updatePixmaps();
    m_pViewManager->updatePixmaps();
    setIcon( KonqPixmapProvider::self()->pixmapFor( m_combo->currentText() ));
}

void KonqMainWindow::slotOpenWith()
{
  KURL::List lst;
  lst.append( m_currentView->url() );

  QString serviceName = sender()->name();

  KTrader::OfferList offers = m_currentView->appServiceOffers();
  KTrader::OfferList::ConstIterator it = offers.begin();
  KTrader::OfferList::ConstIterator end = offers.end();
  for (; it != end; ++it )
    if ( (*it)->desktopEntryName() == serviceName )
    {
      KRun::run( **it, lst );
      return;
    }
}

void KonqMainWindow::slotViewModeToggle( bool toggle )
{
  if ( !toggle )
    return;

  QString modeName = sender()->name();

  if ( m_currentView->service()->desktopEntryName() == modeName )
    return;

  m_bViewModeToggled = true;

  m_currentView->stop();
  m_currentView->lockHistory();

  // Save those, because changeViewMode will lose them
  KURL url = m_currentView->url();
  QString locationBarURL = m_currentView->locationBarURL();
  QStringList filesToSelect;
  if( m_currentView->part()->inherits( "KonqDirPart" ) ) {
     KFileItemList fileItemsToSelect = static_cast<KonqDirPart*>(m_currentView->part())->selectedFileItems();
     KFileItemListIterator it( fileItemsToSelect );
     while( it.current() ){
         filesToSelect += it.current()->name();
         ++it;
     }
  }


  bool bQuickViewModeChange = false;

  // iterate over all services, update the toolbar service map
  // and check if we can do a quick property-based viewmode change
  KTrader::OfferList offers = m_currentView->partServiceOffers();
  KTrader::OfferList::ConstIterator oIt = offers.begin();
  KTrader::OfferList::ConstIterator oEnd = offers.end();
  for (; oIt != oEnd; ++oIt )
  {
      KService::Ptr service = *oIt;

      if ( service->desktopEntryName() == modeName )
      {
          // we changed the viewmode of either iconview or listview
          // -> update the service in the corresponding map, so that
          // we can set the correct text, icon, etc. properties to the
          // KonqViewModeAction when rebuilding the view-mode actions in
          // updateViewModeActions
          // (I'm saying iconview/listview here, but theoretically it could be
          //  any view :)
          m_viewModeToolBarServices[ service->library() ] = service;

          if (  service->library() == m_currentView->service()->library() )
          {
              QVariant modeProp = service->property( "X-KDE-BrowserView-ModeProperty" );
              QVariant modePropValue = service->property( "X-KDE-BrowserView-ModePropertyValue" );
              if ( !modeProp.isValid() || !modePropValue.isValid() )
                  break;

              m_currentView->part()->setProperty( modeProp.toString().latin1(), modePropValue );

              KService::Ptr oldService = m_currentView->service();

              // we aren't going to re-build the viewmode actions but instead of a
              // quick viewmode change (iconview) -> find the iconview-konqviewmode
              // action and set new text,icon,etc. properties, to show the new
              // current viewmode
              QPtrListIterator<KAction> it( m_toolBarViewModeActions );
              for (; it.current(); ++it )
                  if ( QString::fromLatin1( it.current()->name() ) == oldService->desktopEntryName() )
                  {
                      assert( it.current()->inherits( "KonqViewModeAction" ) );

                      KonqViewModeAction *action = static_cast<KonqViewModeAction *>( it.current() );

                      action->setChecked( true );
		      QString servicename = service->genericName();
		      if (servicename.isEmpty())
		          servicename = service->name();
                      action->setText( servicename );
                      action->setIcon( service->icon() );
                      action->setName( service->desktopEntryName().ascii() );

                      break;
                  }

              m_currentView->setService( service );

              bQuickViewModeChange = true;
              break;
          }
      }
  }

  if ( !bQuickViewModeChange )
  {
    m_currentView->changeViewMode( m_currentView->serviceType(), modeName );
    QString locURL( locationBarURL );
    QString nameFilter = detectNameFilter( locURL );
    if( m_currentView->part()->inherits( "KonqDirPart" ) )
       static_cast<KonqDirPart*>( m_currentView->part() )->setFilesToSelect( filesToSelect );
    m_currentView->openURL( KURL( locURL ), locationBarURL, nameFilter );
  }

  // Now save this setting, either locally or globally (for directories only)
  // (We don't have views with viewmodes other than for dirs currently;
  // once we do, we might want to implement per-mimetype global-saving)
  if ( m_bSaveViewPropertiesLocally && m_currentView->supportsServiceType( "inode/directory" ) )
  {
      KURL u ( m_currentView->url() );
      u.addPath(".directory");
      if ( u.isLocalFile() )
      {
          KSimpleConfig config( u.path() ); // if we have no write access, just drop it
          config.setGroup( "URL properties" );
          config.writeEntry( "ViewMode", modeName );
          config.sync();
      }
  } else
  {
      // We save the global view mode only if the view is a built-in view
      if ( m_currentView->isBuiltinView() )
      {
          KConfig *config = KGlobal::config();
          KConfigGroupSaver cgs( config, "MainView Settings" );
          config->writeEntry( "ViewMode", modeName );
          config->sync();
          m_sViewModeForDirectory = modeName;
          //kdDebug(1202) << "m_sViewModeForDirectory=" << m_sViewModeForDirectory << endl;
      }
  }
}

void KonqMainWindow::showHTML( KonqView * _view, bool b, bool _activateView )
{
  // Save this setting, either locally or globally
  // This has to be done before calling openView since it relies on it
  if ( m_bSaveViewPropertiesLocally )
  {
      KURL u ( b ? _view->url() : KURL( _view->url().directory() ) );
      u.addPath(".directory");
      if ( u.isLocalFile() )
      {
          KSimpleConfig config( u.path() ); // No checks for access
          config.setGroup( "URL properties" );
          config.writeEntry( "HTMLAllowed", b );
          config.sync();
      }
  } else
  {
      KConfig *config = KGlobal::config();
      KConfigGroupSaver cgs( config, "MainView Settings" );
      config->writeEntry( "HTMLAllowed", b );
      config->sync();
      if ( _activateView )
          m_bHTMLAllowed = b;
  }

  if ( b && _view->supportsServiceType( "inode/directory" ) )
  {
    _view->lockHistory();
    openView( "inode/directory", _view->url(), _view );
  }
  else if ( !b && _view->supportsServiceType( "text/html" ) )
  {
    KURL u( _view->url() );
    QString fileName = u.fileName().lower();
    if ( KProtocolInfo::supportsListing( u ) && fileName.startsWith("index.htm") ) {
        _view->lockHistory();
        u.setPath( u.directory() );
        openView( "inode/directory", u, _view );
    }
  }
}

void KonqMainWindow::slotShowHTML()
{
  bool b = !m_currentView->allowHTML();

  m_currentView->stop();
  m_currentView->setAllowHTML( b );
  showHTML( m_currentView, b, true ); //current view
  m_pViewManager->showHTML(b );

}

void KonqMainWindow::setShowHTML( bool b )
{
    m_bHTMLAllowed = b;
    if ( m_currentView )
        m_currentView->setAllowHTML( b );
    m_ptaUseHTML->setChecked( b );
}

void KonqMainWindow::slotLockView()
{
  m_currentView->setLockedLocation( m_paLockView->isChecked() );
}

void KonqMainWindow::slotStop()
{
  abortLoading();
  if ( m_currentView )
  {
    m_currentView->frame()->statusbar()->message( i18n("Canceled.") );
  }
}

void KonqMainWindow::slotLinkView()
{
  // Can't access this action in passive mode anyway
  assert(!m_currentView->isPassiveMode());
  bool mode = !m_currentView->isLinkedView();

  if (linkableViewsCount() == 2)
  {
    // Exactly two linkable views : link both
    KonqMainWindow::MapViews::ConstIterator it = viewMap().begin();
    if( (*it)->isFollowActive() ) // skip sidebar
        ++it;
    (*it)->setLinkedView( mode );
    ++it;
    if( (*it)->isFollowActive() ) // skip sidebar
        ++it;
    (*it)->setLinkedView( mode );
  }
  else // Normal case : just this view
    m_currentView->setLinkedView( mode );
}

void KonqMainWindow::slotReload( KonqView* reloadView )
{
  if ( !reloadView )
    reloadView = m_currentView;

  if ( !reloadView || reloadView->url().isEmpty() )
    return;

  if ( reloadView->part() ) {
    QVariant prop = reloadView->part()->property("modified");
    if (prop.isValid() && prop.toBool())
      if ( KMessageBox::warningContinueCancel( this,
           i18n("This page contains changes that have not been submitted.\nClosing the page will discard these changes."),
	   i18n("Discard Changes?"), i18n("&Discard Changes"), "discardchangesclose") != KMessageBox::Continue )
        return;
  }

  KonqOpenURLRequest req( reloadView->typedURL() );
  if ( reloadView->prepareReload( req.args ) )
  {
      reloadView->lockHistory();
      // Reuse current servicetype for local files, but not for remote files (it could have changed, e.g. over HTTP)
      QString serviceType = reloadView->url().isLocalFile() ? reloadView->serviceType() : QString::null;
      openURL( reloadView, reloadView->url(), serviceType, req );
  }
}

void KonqMainWindow::slotReloadPopup()
{
  if (m_pWorkingTab)
    slotReload( m_pWorkingTab->activeChildView() );
}

void KonqMainWindow::slotHome()
{
  QString homeURL = m_pViewManager->profileHomeURL();

  if (homeURL.isEmpty())
    homeURL = KonqFMSettings::settings()->homeURL();

  openURL( 0L, KURL( KonqMisc::konqFilteredURL( this, homeURL ) ) );
}

void KonqMainWindow::slotGoApplications()
{
  openURL( 0L, KURL( "programs:/" ) );
}

void KonqMainWindow::slotGoDevices()
{
  openURL( 0L, KURL( "devices:/" ) );
}

void KonqMainWindow::slotGoSettings()
{
  openURL( 0L, KURL( "settings:/" ) );
}

void KonqMainWindow::slotGoDirTree()
{
  KURL u;
  u.setPath( locateLocal( "data", "konqueror/dirtree/" ) );
  openURL( 0L, u );
}

void KonqMainWindow::slotGoTrash()
{
  openURL( 0L, KURL( "trash:/" ) );
}

void KonqMainWindow::slotGoTemplates()
{
  KURL u;
  u.setPath( KGlobal::dirs()->resourceDirs( "templates" ).last() );
  openURL( 0L, u );
}

void KonqMainWindow::slotGoAutostart()
{
  KURL u;
  u.setPath( KGlobalSettings::autostartPath() );
  openURL( 0L, u );
}

QStringList KonqMainWindow::configModules() const
{
    return m_configureModules;
}

void KonqMainWindow::slotConfigureExtensions()
{
	KonqExtensionManager extensionManager(0L, "extensionmanager", this);
	extensionManager.exec();
}

void KonqMainWindow::slotConfigure()
{
    if( !m_configureDialog )
    {
        m_configureDialog = new KCMultiDialog( this, "configureDialog" );

        QStringList modules = configModules();
        QStringList::ConstIterator end( modules.end() );

        for( QStringList::ConstIterator it = modules.begin();
                it != end; ++it )
            m_configureDialog->addModule( *it );

    }

    m_configureDialog->show();

}

void KonqMainWindow::slotConfigureSpellChecking()
{
    KApplication::startServiceByDesktopName("spellchecking");
}

void KonqMainWindow::slotConfigureToolbars()
{
  if ( autoSaveSettings() )
    saveMainWindowSettings( KGlobal::config(), "KonqMainWindow" );
  KEditToolbar dlg(factory());
  connect(&dlg,SIGNAL(newToolbarConfig()),this,SLOT(slotNewToolbarConfig()));
  connect(&dlg,SIGNAL(newToolbarConfig()),this,SLOT(initBookmarkBar()));
  dlg.exec();
}

void KonqMainWindow::slotNewToolbarConfig() // This is called when OK or Apply is clicked
{
    if ( m_toggleViewGUIClient )
      plugActionList( QString::fromLatin1( "toggleview" ), m_toggleViewGUIClient->actions() );
    if ( m_currentView && m_currentView->appServiceOffers().count() > 0 )
      plugActionList( "openwith", m_openWithActions );

    plugViewModeActions();

    applyMainWindowSettings( KGlobal::config(), "KonqMainWindow" );
}

void KonqMainWindow::slotUndoAvailable( bool avail )
{
  bool enable = false;

  if ( avail && m_currentView && m_currentView->part() )
  {
    // Avoid qWarning from QObject::property if it doesn't exist
    if ( m_currentView->part()->metaObject()->findProperty( "supportsUndo" ) != -1 )
    {
      QVariant prop = m_currentView->part()->property( "supportsUndo" );
      if ( prop.isValid() && prop.toBool() )
        enable = true;
    }
  }

  m_paUndo->setEnabled( enable );
}

void KonqMainWindow::slotPartChanged( KonqView *childView, KParts::ReadOnlyPart *oldPart, KParts::ReadOnlyPart *newPart )
{
  kdDebug(1202) << "KonqMainWindow::slotPartChanged" << endl;
  m_mapViews.remove( oldPart );
  m_mapViews.insert( newPart, childView );

  // Remove the old part, and add the new part to the manager
  // Note: this makes the new part active... so it calls slotPartActivated

  m_pViewManager->replacePart( oldPart, newPart, false );
  // Set active immediately
  m_pViewManager->setActivePart( newPart, true );

  viewsChanged();
}


void KonqMainWindow::slotRunFinished()
{
  kdDebug(1202) << "KonqMainWindow::slotRunFinished()" << endl;
  const KonqRun *run = static_cast<const KonqRun *>( sender() );

  if ( run == m_initialKonqRun )
      m_initialKonqRun = 0L;

  if ( !run->mailtoURL().isEmpty() )
  {
      kapp->invokeMailer( run->mailtoURL() );
  }

  if ( run->hasError() ) { // we had an error
      QByteArray data;
      QDataStream s( data, IO_WriteOnly );
      s << run->url().prettyURL() << kapp->dcopClient()->defaultObject();
      kapp->dcopClient()->send( "konqueror*", "KonquerorIface",
				"removeFromCombo(QString,QCString)", data);
  }

  KonqView *childView = run->childView();

  // Check if we found a mimetype _and_ we got no error (example: cancel in openwith dialog)
  if ( run->foundMimeType() && !run->hasError() )
  {

    // We do this here and not in the constructor, because
    // we are waiting for the first view to be set up before doing this...
    // Note: this is only used when konqueror is started from command line.....
    if ( m_bNeedApplyKonqMainWindowSettings )
    {
      m_bNeedApplyKonqMainWindowSettings = false; // only once
      applyKonqMainWindowSettings();
    }

    return;
  }

  if ( childView )
  {
    childView->setLoading( false );

    if ( childView == m_currentView )
    {
      stopAnimation();

      // Revert to working URL - unless the URL was typed manually
      kdDebug(1202) << " typed URL = " << run->typedURL() << endl;
      if ( run->typedURL().isEmpty() && childView->history().current() ) // not typed
        childView->setLocationBarURL( childView->history().current()->locationBarURL );
    }
  }
  else // No view, e.g. empty webbrowsing profile
    stopAnimation();
}

void KonqMainWindow::applyKonqMainWindowSettings()
{
  KConfig *config = KGlobal::config();
  KConfigGroupSaver cgs( config, "MainView Settings" );
  QStringList toggableViewsShown = config->readListEntry( "ToggableViewsShown" );
  QStringList::ConstIterator togIt = toggableViewsShown.begin();
  QStringList::ConstIterator togEnd = toggableViewsShown.end();
  for ( ; togIt != togEnd ; ++togIt )
  {
    // Find the action by name
  //    KAction * act = m_toggleViewGUIClient->actionCollection()->action( (*togIt).latin1() );
    KAction *act = m_toggleViewGUIClient->action( *togIt );
    if ( act )
      act->activate();
    else
      kdWarning(1202) << "Unknown toggable view in ToggableViewsShown " << *togIt << endl;
  }
}

void KonqMainWindow::slotSetStatusBarText( const QString & )
{
   // Reimplemented to disable KParts::MainWindow default behaviour
   // Does nothing here, see konq_frame.cc
}

void KonqMainWindow::slotViewCompleted( KonqView * view )
{
  assert( view );

  // Need to update the current working directory
  // of the completion object every time the user
  // changes the directory!! (DA)
  if( m_pURLCompletion )
  {
    KURL u( view->locationBarURL() );
    if( u.isLocalFile() )
      m_pURLCompletion->setDir( u.path() );
    else
      m_pURLCompletion->setDir( u.url() );  //needs work!! (DA)
  }
}

void KonqMainWindow::slotPartActivated( KParts::Part *part )
{
  kdDebug(1202) << "KonqMainWindow::slotPartActivated " << part << " "
                <<  ( part && part->instance() && part->instance()->aboutData() ? part->instance()->aboutData()->appName() : "" ) << endl;

  KonqView *newView = 0;
  KonqView *oldView = m_currentView;

  if ( part )
  {
    newView = m_mapViews.find( static_cast<KParts::ReadOnlyPart *>( part ) ).data();

    if ( newView->isPassiveMode() )
    {
      // Passive view. Don't connect anything, don't change m_currentView
      // Another view will become the current view very soon
      kdDebug(1202) << "Passive mode - return" << endl;
      return;
    }
  }

  KParts::BrowserExtension *ext = 0;

  if ( oldView )
  {
    ext = oldView->browserExtension();
    if ( ext )
    {
      //kdDebug(1202) << "Disconnecting extension for view " << oldView << endl;
      disconnectExtension( ext );
    }

    if ( oldView->part() )
    {
      KActionCollection *coll = oldView->part()->actionCollection();
      if ( coll )
          disconnectActionCollection( coll );
    }
  }

  kdDebug(1202) << "New current view " << newView << endl;
  m_currentView = newView;
  if ( !part )
  {
    kdDebug(1202) << "No part activated - returning" << endl;
    unplugViewModeActions();
    createGUI( 0L );
    KParts::MainWindow::setCaption( "" );
    KParts::MainWindow::setIcon( kapp->icon());
    return;
  }

  ext = m_currentView->browserExtension();

  if ( ext )
  {
    connectExtension( ext );
  }
  else
  {
    kdDebug(1202) << "No Browser Extension for the new part" << endl;
    // Disable all browser-extension actions

    KParts::BrowserExtension::ActionSlotMap * actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
    KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->begin();
    KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->end();

    for ( ; it != itEnd ; ++it )
    {
      KAction * act = actionCollection()->action( it.key() );
      Q_ASSERT(act);
      if (act)
        act->setEnabled( false );
    }

    if ( m_paCopyFiles )
      m_paCopyFiles->setEnabled( false );
    if ( m_paMoveFiles )
      m_paMoveFiles->setEnabled( false );
    if ( m_paNewDir )
      m_paNewDir->setEnabled( false );
  }
  createGUI( part );

  KActionCollection *coll = m_currentView->part()->actionCollection();
  if ( coll )
      connectActionCollection( coll );

  // View-dependent GUI

  KParts::MainWindow::setCaption( m_currentView->caption() );
  m_currentView->frame()->setTitle( m_currentView->caption() , 0L);
  updateOpenWithActions();
  updateLocalPropsActions();
  updateViewActions(); // undo, lock, link and other view-dependent actions

  if ( m_bViewModeToggled )
  {
      // if we just toggled the view mode via the view mode actions, then
      // we don't need to do all the time-taking stuff below (Simon)
      QString currentServiceDesktopEntryName = m_currentView->service()->desktopEntryName();
      QPtrListIterator<KRadioAction> it( m_viewModeActions );
      for (; it.current(); ++it ) {
          if ( it.current()->name() == currentServiceDesktopEntryName ) {
              it.current()->setChecked( true );
              break;
          }
      }
      QString currentServiceLibrary = m_currentView->service()->library();
      QPtrListIterator<KAction> ittb( m_toolBarViewModeActions );
      for (; ittb.current(); ++ittb ) {
          KService::Ptr serv = KService::serviceByDesktopName( ittb.current()->name() );
          if ( serv && serv->library() == currentServiceLibrary ) {
              KToggleAction* ta = static_cast<KToggleAction*>( ittb.current() );
              ta->setChecked( true );
	      QString servicename = m_currentView->service()->genericName();
	      if (servicename.isEmpty())
	          servicename = m_currentView->service()->name();
              ta->setText( servicename );
              ta->setIcon( m_currentView->service()->icon() );
              ta->setName( m_currentView->service()->desktopEntryName().ascii() ) ;
              break;
          }
      }
  }
  else
  {
      updateViewModeActions();

      m_pMenuNew->setEnabled( m_currentView->serviceType() == QString::fromLatin1( "inode/directory" ) );
  }

  m_bViewModeToggled = false;


  m_pMenuNew->setEnabled( m_currentView->serviceType() == QString::fromLatin1( "inode/directory" ) );

  m_currentView->frame()->statusbar()->updateActiveStatus();

  if ( oldView && oldView->frame() )
    oldView->frame()->statusbar()->updateActiveStatus();

  //kdDebug(1202) << "slotPartActivated: setting location bar url to "
  //              << m_currentView->locationBarURL() << " m_currentView=" << m_currentView << endl;
  m_currentView->setLocationBarURL( m_currentView->locationBarURL() );

  updateToolBarActions();

  m_currentView->setActiveInstance();
}

void KonqMainWindow::insertChildView( KonqView *childView )
{
    kdDebug() << "KonqMainWindow::insertChildView " << childView << endl;
  m_mapViews.insert( childView->part(), childView );

  connect( childView, SIGNAL( viewCompleted( KonqView * ) ),
           this, SLOT( slotViewCompleted( KonqView * ) ) );

  if ( !m_pViewManager->isLoadingProfile() ) // see KonqViewManager::loadViewProfile
      viewCountChanged();
  emit viewAdded( childView );
}

// Called by KonqViewManager, internal
void KonqMainWindow::removeChildView( KonqView *childView )
{
  kdDebug(1202) << "KonqMainWindow::removeChildView childView " << childView << endl;

  disconnect( childView, SIGNAL( viewCompleted( KonqView * ) ),
              this, SLOT( slotViewCompleted( KonqView * ) ) );

#ifndef NDEBUG
  dumpViewList();
#endif

  MapViews::Iterator it = m_mapViews.begin();
  MapViews::Iterator end = m_mapViews.end();

  // find it in the map - can't use the key since childView->part() might be 0L

  kdDebug(1202) << "Searching map" << endl;

  while ( it != end && it.data() != childView )
      ++it;

  kdDebug(1202) << "Verifying search results" << endl;

  if ( it == m_mapViews.end() )
  {
      kdWarning(1202) << "KonqMainWindow::removeChildView childView " << childView << " not in map !" << endl;
      return;
  }

  kdDebug(1202) << "Removing view " << childView << endl;

  m_mapViews.remove( it );

  kdDebug(1202) << "View " << childView << " removed from map" << endl;

  viewCountChanged();
  emit viewRemoved( childView );

#ifndef NDEBUG
  dumpViewList();
#endif

  // KonqViewManager takes care of m_currentView
}

void KonqMainWindow::viewCountChanged()
{
  // This is called when the number of views changes.
  kdDebug(1202) << "KonqMainWindow::viewCountChanged" << endl;

  int lvc = linkableViewsCount();
  m_paLinkView->setEnabled( lvc > 1 );

  // Only one view (or one view + sidebar) -> make it/them unlinked
  if ( lvc == 1 ) {
      MapViews::Iterator it = m_mapViews.begin();
      MapViews::Iterator end = m_mapViews.end();
      for (; it != end; ++it )
          it.data()->setLinkedView( false );
  }

  viewsChanged();

  m_pViewManager->viewCountChanged();
}

void KonqMainWindow::viewsChanged()
{
  // This is called when the number of views changes OR when
  // the type of some view changes.

  // Nothing here anymore, but don't cleanup, some might come back later.

  updateViewActions(); // undo, lock, link and other view-dependent actions
}

KonqView * KonqMainWindow::childView( KParts::ReadOnlyPart *view )
{
  MapViews::ConstIterator it = m_mapViews.find( view );
  if ( it != m_mapViews.end() )
    return it.data();
  else
    return 0L;
}

KonqView * KonqMainWindow::childView( KParts::ReadOnlyPart *callingPart, const QString &name, KParts::BrowserHostExtension **hostExtension, KParts::ReadOnlyPart **part )
{
  kdDebug() << "KonqMainWindow::childView this=" << this << " looking for " << name << endl;

  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
  {
    KonqView* view = it.data();
    QString viewName = view->viewName();
    kdDebug() << "       - viewName=" << viewName << "   "
              << "frame names:" << view->frameNames().join( "," ) << endl;
    if ( !viewName.isEmpty() && viewName == name )
    {
      kdDebug() << "found existing view by name: " << view << endl;
      if ( hostExtension )
        *hostExtension = 0;
      if ( part )
        *part = view->part();
      return view;
    }

    // First look for a hostextension containing this frame name
    KParts::BrowserHostExtension *ext = KParts::BrowserHostExtension::childObject( view->part() );
    if ( ext )
    {
      ext = ext->findFrameParent(callingPart, name);
    }

//    KParts::BrowserHostExtension* ext = KonqView::hostExtension( view->part(), name );

    if ( ext )
    {
      QPtrList<KParts::ReadOnlyPart> frames = ext->frames();
      QPtrListIterator<KParts::ReadOnlyPart> frameIt( frames );
      for ( ; frameIt.current() ; ++frameIt )
      {
        if ( frameIt.current()->name() == name )
        {
          kdDebug() << "found a frame of name " << name << " : " << frameIt.current() << endl;
          if ( hostExtension )
            *hostExtension = ext;
          if ( part )
            *part = frameIt.current();
          return view;
        }
      }
    }
  }

  return 0;
}

// static
KonqView * KonqMainWindow::findChildView( KParts::ReadOnlyPart *callingPart, const QString &name, KonqMainWindow **mainWindow, KParts::BrowserHostExtension **hostExtension, KParts::ReadOnlyPart **part )
{
  if ( !s_lstViews )
    return 0;

  QPtrListIterator<KonqMainWindow> it( *s_lstViews );
  for (; it.current(); ++it )
  {
    KonqView *res = it.current()->childView( callingPart, name, hostExtension, part );
    if ( res )
    {
      if ( mainWindow )
        *mainWindow = it.current();
      return res;
    }
  }

  return 0;
}

int KonqMainWindow::activeViewsCount() const
{
  int res = 0;
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    if ( !it.data()->isPassiveMode() )
      ++res;

  return res;
}

int KonqMainWindow::linkableViewsCount() const
{
  int res = 0;
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    if ( !it.data()->isFollowActive() )
      ++res;

  return res;
}

int KonqMainWindow::mainViewsCount() const
{
  int res = 0;
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    if ( !it.data()->isPassiveMode() && !it.data()->isToggleView() )
    {
      //kdDebug(1202) << "KonqMainWindow::mainViewsCount " << res << " " << it.data() << " " << it.data()->part()->widget() << endl;
      ++res;
    }

  return res;
}

KParts::ReadOnlyPart * KonqMainWindow::currentPart() const
{
  /// ### This is currently unused. Check in the final version (!) if still unused.
  if ( m_currentView )
    return m_currentView->part();
  else
    return 0L;
}

void KonqMainWindow::customEvent( QCustomEvent *event )
{
  KParts::MainWindow::customEvent( event );

  if ( KonqFileSelectionEvent::test( event ) ||
       KonqFileMouseOverEvent::test( event ) )
  {
    // Forward the event to all views
    MapViews::ConstIterator it = m_mapViews.begin();
    MapViews::ConstIterator end = m_mapViews.end();
    for (; it != end; ++it )
      QApplication::sendEvent( (*it)->part(), event );
    return;
  }
  if ( KParts::OpenURLEvent::test( event ) )
  {
    KParts::OpenURLEvent * ev = static_cast<KParts::OpenURLEvent*>(event);
    KonqView * senderChildView = childView(ev->part());

    // Enable/disable local properties actions if current view
    if ( senderChildView == m_currentView )
        updateLocalPropsActions();

    // Forward the event to all views
    MapViews::ConstIterator it = m_mapViews.begin();
    MapViews::ConstIterator end = m_mapViews.end();
    for (; it != end; ++it )
    {
      // Don't resend to sender
      if (it.key() != ev->part())
      {
        //kdDebug(1202) << "Sending event to view " << it.key()->className() << endl;
        QApplication::sendEvent( it.key(), event );

      }
    }
  }
}

void KonqMainWindow::updateLocalPropsActions()
{
    bool canWrite = false;
    if ( m_currentView && m_currentView->url().isLocalFile() )
    {
        // Can we write ?
        QFileInfo info( m_currentView->url().path() );
        canWrite = info.isDir() && info.isWritable();
    }
    m_paSaveViewPropertiesLocally->setEnabled( canWrite );
    m_paRemoveLocalProperties->setEnabled( canWrite );
}

void KonqMainWindow::slotURLEntered( const QString &text, int state )
{
  if ( m_bURLEnterLock || text.isEmpty() )
    return;

  m_bURLEnterLock = true;

  if (state & ControlButton)
  {
      m_combo->setURL( m_currentView ? m_currentView->url().prettyURL() : QString::null );
      openFilteredURL( text.stripWhiteSpace(), true );
  }
  else
      openFilteredURL( text.stripWhiteSpace() );

  m_bURLEnterLock = false;
}

void KonqMainWindow::slotFileNewAboutToShow()
{
  // As requested by KNewMenu :
  m_pMenuNew->slotCheckUpToDate();
  // And set the files that the menu apply on :
  m_pMenuNew->setPopupFiles( KURL( m_currentView->url().url() ) );
}

void KonqMainWindow::slotSplitViewHorizontal()
{
  KonqView * newView = m_pViewManager->splitView( Qt::Horizontal );
  if (newView == 0L) return;
  newView->openURL( m_currentView->url(), m_currentView->locationBarURL() );
}

void KonqMainWindow::slotSplitViewVertical()
{
  KonqView * newView = m_pViewManager->splitView( Qt::Vertical );
  if (newView == 0L) return;
  newView->openURL( m_currentView->url(), m_currentView->locationBarURL() );
}

void KonqMainWindow::slotAddTab()
{
  KConfig *config = KGlobal::config();
  KConfigGroupSaver cs( config, QString::fromLatin1("FMSettings") );
  bool openAfterCurrentPage = config->readBoolEntry( "OpenAfterCurrentPage", false );
  KonqView* newView = m_pViewManager->addTab(QString::null,
                                             QString::null,
                                             false,
                                             openAfterCurrentPage);
  if (newView == 0L) return;
  openURL( newView, KURL("about:blank"),QString::null);
  m_pViewManager->showTab( newView );
  focusLocationBar();
  m_pWorkingTab = 0L;
}

void KonqMainWindow::slotDuplicateTab()
{
  KConfig *config = KGlobal::config();
  KConfigGroupSaver cs( config, QString::fromLatin1("FMSettings") );
  m_pViewManager->duplicateTab( 0, config->readBoolEntry( "OpenAfterCurrentPage", false ) );
}

void KonqMainWindow::slotDuplicateTabPopup()
{
  KConfig *config = KGlobal::config();
  KConfigGroupSaver cs( config, QString::fromLatin1("FMSettings") );
  m_pViewManager->duplicateTab( m_pWorkingTab, config->readBoolEntry( "OpenAfterCurrentPage", false ) );
}

void KonqMainWindow::slotBreakOffTab()
{
  if (m_currentView && m_currentView->part()) {
    QVariant prop = m_currentView->part()->property("modified");
    if (prop.isValid() && prop.toBool())
      if ( KMessageBox::warningContinueCancel( this,
           i18n("This tab contains changes that have not been submitted.\nDetaching the tab will discard these changes."),
	   i18n("Discard Changes?"), i18n("&Discard Changes"), "discardchangesdetach") != KMessageBox::Continue )
        return;
  }

  m_pViewManager->breakOffTab();
  updateViewActions();
}

void KonqMainWindow::slotBreakOffTabPopup()
{
  KonqView* originalView = m_currentView;
  KonqView *view = m_pWorkingTab->activeChildView();
  if (view && view->part()) {
    QVariant prop = view->part()->property("modified");
    if (prop.isValid() && prop.toBool()) {
      m_pViewManager->showTab( view );
      if ( KMessageBox::warningContinueCancel( this,
           i18n("This tab contains changes that have not been submitted.\nDetaching the tab will discard these changes."),
	   i18n("Discard Changes?"), i18n("&Discard Changes"), "discardchangesdetach") != KMessageBox::Continue )
      {
        m_pViewManager->showTab( originalView );
        return;
      }
    }
  }
  m_pViewManager->showTab( originalView );

  //Can't do this safely here as the tabbar may disappear and we're
  //hanging off here.
  QTimer::singleShot(0, this, SLOT( slotBreakOffTabPopupDelayed() ) );
}

void KonqMainWindow::slotBreakOffTabPopupDelayed()
{
  m_pViewManager->breakOffTab( m_pWorkingTab );
  updateViewActions();
}

void KonqMainWindow::slotPopupNewWindow()
{
    kdDebug(1202) << "KonqMainWindow::popupNewWindow()" << endl;

    KFileItemListIterator it ( popupItems );
    for ( ; it.current(); ++it )
    {
        KonqMisc::createNewWindow( (*it)->url(), popupUrlArgs );
    }
}

void KonqMainWindow::slotPopupNewTab()
{
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, QString::fromLatin1("FMSettings") );
    bool openAfterCurrentPage = config->readBoolEntry( "OpenAfterCurrentPage", false );
    bool newTabsInFront = config->readBoolEntry( "NewTabsInFront", false );

    if (KApplication::keyboardMouseState() & Qt::ShiftButton)
      newTabsInFront = !newTabsInFront;

    popupNewTab(newTabsInFront, openAfterCurrentPage);
}

void KonqMainWindow::popupNewTab(bool infront, bool openAfterCurrentPage)
{
  kdDebug(1202) << "KonqMainWindow::popupNewTab()" << endl;

  KFileItemListIterator it ( popupItems );
  KonqOpenURLRequest req;
  req.newTab = true;
  req.newTabInFront = false;
  req.openAfterCurrentPage = openAfterCurrentPage;
  req.args = popupUrlArgs;

  for ( ; it.current(); ++it )
  {
    if ( infront && it.atLast() )
    {
      req.newTabInFront = true;
    }
    openURL( 0L, (*it)->url(), QString::null, req );
  }
}

void KonqMainWindow::openMultiURL( KURL::List url )
{
    KURL::List::ConstIterator it = url.begin();
    KURL::List::ConstIterator end = url.end();
    for (; it != end; ++it )
    {
        KonqView* newView = m_pViewManager->addTab();
        Q_ASSERT( newView );
        if (newView == 0L) continue;
        openURL( newView, *it,QString::null);
        m_pViewManager->showTab( newView );
        focusLocationBar();
        m_pWorkingTab = 0L;
    }
}

void KonqMainWindow::slotRemoveView()
{
  if (m_currentView && m_currentView->part()) {
    QVariant prop = m_currentView->part()->property("modified");
    if (prop.isValid() && prop.toBool())
      if ( KMessageBox::warningContinueCancel( this,
           i18n("This view contains changes that have not been submitted.\nClosing the view will discard these changes."),
	   i18n("Discard Changes?"), i18n("&Discard Changes"), "discardchangesclose") != KMessageBox::Continue )
        return;
  }

  // takes care of choosing the new active view
  m_pViewManager->removeView( m_currentView );
}

void KonqMainWindow::slotRemoveTab()
{
  if (m_currentView && m_currentView->part()) {
    QVariant prop = m_currentView->part()->property("modified");
    if (prop.isValid() && prop.toBool())
      if ( KMessageBox::warningContinueCancel( this,
           i18n("This tab contains changes that have not been submitted.\nClosing the tab will discard these changes."),
	   i18n("Discard Changes?"), i18n("&Discard Changes"), "discardchangesclose") != KMessageBox::Continue )
        return;
  }

  m_pViewManager->removeTab();
}

void KonqMainWindow::slotRemoveTabPopup()
{
  KonqView *originalView = m_currentView;
  KonqView *view = m_pWorkingTab->activeChildView();
  if (view && view->part()) {
    QVariant prop = view->part()->property("modified");
    if (prop.isValid() && prop.toBool()) {
      m_pViewManager->showTab( view );
      if ( KMessageBox::warningContinueCancel( this,
           i18n("This tab contains changes that have not been submitted.\nClosing the tab will discard these changes."),
	   i18n("Discard Changes?"), i18n("&Discard Changes"), "discardchangesclose") != KMessageBox::Continue )
      {
        m_pViewManager->showTab( originalView );
        return;
      }
    }
    m_pViewManager->showTab( originalView );
  }

  //Can't do immediately - may kill the tabbar, and we're in an event path down from it
  QTimer::singleShot( 0, this, SLOT( slotRemoveTabPopupDelayed() ) );
}

void KonqMainWindow::slotRemoveTabPopupDelayed()
{
  m_pViewManager->removeTab( m_pWorkingTab );
}

void KonqMainWindow::slotRemoveOtherTabsPopup()
{
  if ( KMessageBox::warningContinueCancel( this,
       i18n("Do you really want to close all other tabs?"),
       i18n("Close Other Tabs Confirmation"), KGuiItem(i18n("Close &Other Tabs"),"tab_remove"),
       "CloseOtherTabConfirm") != KMessageBox::Continue )
    return;

  KonqView *originalView = m_currentView;
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it ) {
    KonqView *view = it.data();
    if ( view != originalView && view && view->part()) {
      QVariant prop = view->part()->property("modified");
      if (prop.isValid() && prop.toBool()) {
        m_pViewManager->showTab( view );
        if ( KMessageBox::warningContinueCancel( this,
           i18n("This tab contains changes that have not been submitted.\nClosing other tabs will discard these changes."),
	   i18n("Discard Changes?"), i18n("&Discard Changes"), "discardchangescloseother") != KMessageBox::Continue )
        {
           m_pViewManager->showTab( originalView );
           return;
        }
      }
    }
  }
  m_pViewManager->showTab( originalView );

  //Can't do immediately - kills the tabbar, and we're in an event path down from it
  QTimer::singleShot( 0, this, SLOT( slotRemoveOtherTabsPopupDelayed() ) );
}

void KonqMainWindow::slotRemoveOtherTabsPopupDelayed()
{
  m_pViewManager->removeOtherTabs( m_pWorkingTab );
  updateViewActions();
}

void KonqMainWindow::slotReloadAllTabs()
{
  KonqView *originalView = m_currentView;
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it ) {
    KonqView *view = it.data();
    if (view && view->part()) {
      QVariant prop = view->part()->property("modified");
      if (prop.isValid() && prop.toBool()) {
        m_pViewManager->showTab( view );
        if ( KMessageBox::warningContinueCancel( this,
           i18n("This tab contains changes that have not been submitted.\nReloading all tabs will discard these changes."),
	   i18n("Discard Changes?"), i18n("&Discard Changes"), "discardchangesreload") != KMessageBox::Continue )
        {
            m_pViewManager->showTab( originalView );
            return;
        }
      }
    }
  }
  m_pViewManager->showTab( originalView );

  m_pViewManager->reloadAllTabs();
  updateViewActions();
}


void KonqMainWindow::slotActivateNextTab()
{
  m_pViewManager->activateNextTab();
}

void KonqMainWindow::slotActivatePrevTab()
{
  m_pViewManager->activatePrevTab();
}

void KonqMainWindow::slotActivateTab()
{
  m_pViewManager->activateTab( QString( sender()->name() ).right( 2 ).toInt() -1 );
}

void KonqMainWindow::slotDumpDebugInfo()
{
#ifndef NDEBUG
  dumpViewList();
  m_pViewManager->printFullHierarchy( 0L );
#endif
}

void KonqMainWindow::slotSaveViewPropertiesLocally()
{
  m_bSaveViewPropertiesLocally = !m_bSaveViewPropertiesLocally;
  // And this is a main-view setting, so save it
  KConfig *config = KGlobal::config();
  KConfigGroupSaver cgs( config, "MainView Settings" );
  config->writeEntry( "SaveViewPropertiesLocally", m_bSaveViewPropertiesLocally );
  config->sync();
  // Now tell the views
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
    (*it)->callExtensionBoolMethod( "setSaveViewPropertiesLocally(bool)", m_bSaveViewPropertiesLocally );
}

void KonqMainWindow::slotRemoveLocalProperties()
{
  assert( m_currentView );
  KURL u ( m_currentView->url() );
  u.addPath(".directory");
  if ( u.isLocalFile() )
  {
      QFile f( u.path() );
      if ( f.open(IO_ReadWrite) )
      {
          f.close();
          KSimpleConfig config( u.path() );
          config.deleteGroup( "URL properties" ); // Bye bye
          config.sync();
          // TODO: Notify the view...
          // Or the hard way: (and hoping it doesn't cache the values!)
          slotReload();
      } else
      {
         Q_ASSERT( QFile::exists(u.path()) ); // The action shouldn't be enabled, otherwise.
         KMessageBox::sorry( this, i18n("No permissions to write to %1").arg(u.path()) );
      }
  }
}

bool KonqMainWindow::askForTarget(const QString& text, KURL& url)
{
   const KURL initialUrl = (viewCount()==2) ? otherView(m_currentView)->url() : m_currentView->url();
   QString label = text.arg( m_currentView->url().prettyURL( 0, KURL::StripFileProtocol ) );
   KURLRequesterDlg dlg(initialUrl.prettyURL( 0, KURL::StripFileProtocol ), label, this, "urlrequester", true);
   dlg.setCaption(i18n("Enter Target"));
   dlg.urlRequester()->setMode( KFile::File | KFile::ExistingOnly | KFile::Directory );
   if (dlg.exec())
   {
      url = dlg.selectedURL();
      if ( url.isValid() )
        return true;
      else
      {
        KMessageBox::error( this, i18n("<qt><b>%1</b> is not valid</qt>").arg(url.url()));
        return false;
      }
   }
   return false;
}

void KonqMainWindow::slotRequesterClicked( KURLRequester *req )
{
    req->fileDialog()->setMode(KFile::Directory|KFile::ExistingOnly);
}

void KonqMainWindow::slotCopyFiles()
{
  //kdDebug(1202) << "KonqMainWindow::slotCopyFiles()" << endl;
  KURL dest;
  if (!askForTarget(i18n("Copy selected files from %1 to:"),dest))
     return;

  KonqOperations::copy(this,KonqOperations::COPY,currentURLs(),dest);
}

void KonqMainWindow::slotMoveFiles()
{
  //kdDebug(1202) << "KonqMainWindow::slotMoveFiles()" << endl;
  KURL dest;
  if (!askForTarget(i18n("Move selected files from %1 to:"),dest))
     return;

  KonqOperations::copy(this,KonqOperations::MOVE,currentURLs(),dest);
}

void KonqMainWindow::slotNewDir()
{
    Q_ASSERT( m_currentView );
    if ( m_currentView )
        KonqOperations::newDir(this, m_currentView->url());
}

KURL::List KonqMainWindow::currentURLs() const
{
  KURL::List urls;
  if ( m_currentView )
  {
    urls.append( m_currentView->url() );
    if ( m_currentView->part()->inherits("KonqDirPart") )
    {
      KFileItemList tmpList= static_cast<KonqDirPart *>(m_currentView->part())->selectedFileItems();
      KFileItem *item=tmpList.first();
      if (item) // Return list of selected items only if we have a selection
      {
        urls.clear();
        for (; item!=0; item=tmpList.next())
          urls.append(item->url());
      }
    }
  }
  return urls;
}

// Only valid if there are one or two views
KonqView * KonqMainWindow::otherView( KonqView * view ) const
{
  assert( viewCount() <= 2 );
  MapViews::ConstIterator it = m_mapViews.begin();
  if ( (*it) == view )
    ++it;
  if ( it != m_mapViews.end() )
    return (*it);
  return 0L;
}

void KonqMainWindow::slotSaveViewProfile()
{
#if 0
    if ( m_pViewManager->currentProfile().isEmpty() )
    {
        // The action should be disabled...........
        kdWarning(1202) << "No known profile. Use the Save Profile dialog box" << endl;
    } else {

        m_pViewManager->saveViewProfile( m_pViewManager->currentProfile(),
                                         m_pViewManager->currentProfileText(),
                                         false /* URLs */, true /* size */ );

    }
#endif
    m_pViewManager->showProfileDlg( m_pViewManager->currentProfile() );
}

void KonqMainWindow::slotUpAboutToShow()
{
  QPopupMenu *popup = m_paUp->popupMenu();

  popup->clear();

  uint i = 0;

  // Use the location bar URL, because in case we display a index.html
  // we want to go up from the dir, not from the index.html
  KURL u( m_currentView->locationBarURL() );
  u = u.upURL();
  while ( u.hasPath() )
  {
    popup->insertItem( KonqPixmapProvider::self()->pixmapFor( u.url() ),
                       u.prettyURL( 0, KURL::StripFileProtocol ) );

    if ( u.path() == "/" )
      break;

    if ( ++i > 10 )
      break;

    u = u.upURL();
  }
}

void KonqMainWindow::slotUp()
{
  QTimer::singleShot( 0, this, SLOT( slotUpDelayed() ) );
}

void KonqMainWindow::slotUpDelayed()
{
  openURL( 0L, m_currentView->upURL() );
}

void KonqMainWindow::slotUpActivated( int id )
{
  KURL u( m_currentView->locationBarURL() );
  kdDebug(1202) << "slotUpActivated. Start URL is " << u << endl;
  for ( int i = 0 ; i < m_paUp->popupMenu()->indexOf( id ) + 1 ; i ++ )
      u = u.upURL();
  openURL( 0L, u );
}

void KonqMainWindow::slotGoMenuAboutToShow()
{
  kdDebug(1202) << "KonqMainWindow::slotGoMenuAboutToShow" << endl;
  if ( m_paHistory && m_currentView ) // (maybe this is before initialisation)
      m_paHistory->fillGoMenu( m_currentView->history() );
}

void KonqMainWindow::slotGoHistoryActivated( int steps )
{
  if (!m_goBuffer)
  {
    // Only start 1 timer.
    m_goBuffer = steps;
    QTimer::singleShot( 0, this, SLOT(slotGoHistoryDelayed()));
  }
}

void KonqMainWindow::slotGoHistoryDelayed()
{
  if (!m_currentView) return;
  int steps = m_goBuffer;
  m_goBuffer = 0;
  m_currentView->go( steps );
  makeViewsFollow(m_currentView->url(), KParts::URLArgs(),m_currentView->serviceType(),m_currentView);
}

void KonqMainWindow::slotBackAboutToShow()
{
  m_paBack->popupMenu()->clear();
  if ( m_currentView )
      KonqBidiHistoryAction::fillHistoryPopup( m_currentView->history(), m_paBack->popupMenu(), true, false );
}

void KonqMainWindow::slotBack()
{
  slotGoHistoryActivated(-1);
}

void KonqMainWindow::slotBackActivated( int id )
{
  slotGoHistoryActivated( -(m_paBack->popupMenu()->indexOf( id ) + 1) );
}

void KonqMainWindow::slotForwardAboutToShow()
{
  m_paForward->popupMenu()->clear();
  if ( m_currentView )
      KonqBidiHistoryAction::fillHistoryPopup( m_currentView->history(), m_paForward->popupMenu(), false, true );
}

void KonqMainWindow::slotForward()
{
  slotGoHistoryActivated( 1 );
}

void KonqMainWindow::slotForwardActivated( int id )
{
  slotGoHistoryActivated( m_paForward->popupMenu()->indexOf( id ) + 1 );
}

void KonqMainWindow::initCombo()
{
  m_combo = new KonqCombo( 0L, "history combo");

  m_combo->init( s_pCompletion );

  connect( m_combo, SIGNAL(activated(const QString&,int)),
           this, SLOT(slotURLEntered(const QString&,int)) );

  m_pURLCompletion = new KURLCompletion();
  m_pURLCompletion->setCompletionMode( s_pCompletion->completionMode() );

  // This only turns completion off. ~ is still there in the result
  // We do want completion of user names, right?
  //m_pURLCompletion->setReplaceHome( false );  // Leave ~ alone! Will be taken care of by filters!!

  connect( m_combo, SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
           SLOT( slotCompletionModeChanged( KGlobalSettings::Completion )));
  connect( m_combo, SIGNAL( completion( const QString& )),
           SLOT( slotMakeCompletion( const QString& )));
  connect( m_combo, SIGNAL( substringCompletion( const QString& )),
           SLOT( slotSubstringcompletion( const QString& )));
  connect( m_combo, SIGNAL( textRotation( KCompletionBase::KeyBindingType) ),
           SLOT( slotRotation( KCompletionBase::KeyBindingType )));
  connect( m_combo, SIGNAL( cleared() ),
           SLOT ( slotClearHistory() ) );
  connect( m_pURLCompletion, SIGNAL( match(const QString&) ),
           SLOT( slotMatch(const QString&) ));

  m_combo->lineEdit()->installEventFilter(this);

  static bool bookmarkCompletionInitialized = false;
  if ( !bookmarkCompletionInitialized )
  {
      bookmarkCompletionInitialized = true;
      DelayedInitializer *initializer = new DelayedInitializer( QEvent::KeyPress, m_combo->lineEdit() );
      connect( initializer, SIGNAL( initialize() ), this, SLOT( bookmarksIntoCompletion() ) );
  }
}

void KonqMainWindow::bookmarksIntoCompletion()
{
    // add all bookmarks to the completion list for easy access
    bookmarksIntoCompletion( KonqBookmarkManager::self()->root() );
}

// the user changed the completion mode in the combo
void KonqMainWindow::slotCompletionModeChanged( KGlobalSettings::Completion m )
{
  s_pCompletion->setCompletionMode( m );
  KConfig *config = KGlobal::config();
  config->setGroup( "Settings" );

  if (m_combo->completionMode() == KGlobalSettings::completionMode())
    config->deleteEntry("CompletionMode");
  else
  config->writeEntry( "CompletionMode", (int)m_combo->completionMode() );

  config->sync();

  // tell the other windows too (only this instance currently)
  KonqMainWindow *window = s_lstViews->first();
  while ( window ) {
    if ( window->m_combo ) {
      window->m_combo->setCompletionMode( m );
      window->m_pURLCompletion->setCompletionMode( m );
    }
    window = s_lstViews->next();
  }
}

// at first, try to find a completion in the current view, then use the global
// completion (history)
void KonqMainWindow::slotMakeCompletion( const QString& text )
{
  if( m_pURLCompletion )
  {
    m_urlCompletionStarted = true; // flag for slotMatch()

    // kdDebug(1202) << "Local Completion object found!" << endl;
    QString completion = m_pURLCompletion->makeCompletion( text );
    m_currentDir = QString::null;

    if ( completion.isNull() && !m_pURLCompletion->isRunning() )
    {
      // No match() signal will come from m_pURLCompletion
      // ask the global one
      // tell the static completion object about the current completion mode
      completion = s_pCompletion->makeCompletion( text );

      // some special handling necessary for CompletionPopup
      if ( m_combo->completionMode() == KGlobalSettings::CompletionPopup ||
           m_combo->completionMode() == KGlobalSettings::CompletionPopupAuto )
        m_combo->setCompletedItems( historyPopupCompletionItems( text ) );

      else if ( !completion.isNull() )
        m_combo->setCompletedText( completion );
    }
    else
    {
      // To be continued in slotMatch()...
      if( !m_pURLCompletion->dir().isEmpty() )
        m_currentDir = m_pURLCompletion->dir();
    }
  }
  // kdDebug(1202) << "Current dir: " << m_currentDir << "  Current text: " << text << endl;
}

void KonqMainWindow::slotSubstringcompletion( const QString& text )
{
    bool filesFirst = currentURL().startsWith( "/" ) ||
                      currentURL().startsWith( "file:/" );
    QStringList items;
    if ( filesFirst && m_pURLCompletion )
        items = m_pURLCompletion->substringCompletion( text );

    items += s_pCompletion->substringCompletion( text );
    if ( !filesFirst && m_pURLCompletion )
        items += m_pURLCompletion->substringCompletion( text );

    m_combo->setCompletedItems( items );
}

void KonqMainWindow::slotRotation( KCompletionBase::KeyBindingType type )
{
  // Tell slotMatch() to do nothing
  m_urlCompletionStarted = false;

  bool prev = (type == KCompletionBase::PrevCompletionMatch);
  if ( prev || type == KCompletionBase::NextCompletionMatch ) {
    QString completion = prev ? m_pURLCompletion->previousMatch() :
                                m_pURLCompletion->nextMatch();

    if( completion.isNull() ) { // try the history KCompletion object
        completion = prev ? s_pCompletion->previousMatch() :
                            s_pCompletion->nextMatch();
    }
    if ( completion.isEmpty() || completion == m_combo->currentText() )
      return;

    m_combo->setCompletedText( completion );
  }
}

// Handle match() from m_pURLCompletion
void KonqMainWindow::slotMatch( const QString &match )
{
  if ( match.isEmpty() ) // this case is handled directly
    return;

  // Check flag to avoid match() raised by rotation
  if ( m_urlCompletionStarted ) {
    m_urlCompletionStarted = false;

    // some special handling necessary for CompletionPopup
    if ( m_combo->completionMode() == KGlobalSettings::CompletionPopup ||
         m_combo->completionMode() == KGlobalSettings::CompletionPopupAuto ) {
      QStringList items = m_pURLCompletion->allMatches();
      items += historyPopupCompletionItems( m_combo->currentText() );
      // items.sort(); // should we?
      m_combo->setCompletedItems( items );
    }
    else if ( !match.isNull() )
       m_combo->setCompletedText( match );
  }
}

void KonqMainWindow::slotCtrlTabPressed()
{
   KonqView * view = m_pViewManager->chooseNextView( m_currentView );
   if ( view )
      m_pViewManager->setActivePart( view->part() );
}

void KonqMainWindow::slotClearHistory()
{
   KonqHistoryManager::kself()->emitClear();
}

void KonqMainWindow::slotClearComboHistory()
{
   if (m_combo && m_combo->count())
      m_combo->clearHistory();
}

bool KonqMainWindow::eventFilter(QObject*obj,QEvent *ev)
{
  if ( ( ev->type()==QEvent::FocusIn || ev->type()==QEvent::FocusOut ) &&
       m_combo && m_combo->lineEdit() == obj )
  {
    //kdDebug(1202) << "KonqMainWindow::eventFilter " << obj << " " << obj->className() << " " << obj->name() << endl;

    QFocusEvent * focusEv = static_cast<QFocusEvent*>(ev);
    if (focusEv->reason() == QFocusEvent::Popup)
    {
      return KParts::MainWindow::eventFilter( obj, ev );
    }

    KParts::BrowserExtension * ext = 0;
    if ( m_currentView )
        ext = m_currentView->browserExtension();
    QStrList slotNames;
    if (ext)
      slotNames = ext->metaObject()->slotNames();

    //for ( char * s = slotNames.first() ; s ; s = slotNames.next() )
    //{
    //    kdDebug(1202) << "slotNames=" << s << endl;
    //}


    if (ev->type()==QEvent::FocusIn)
    {
      //kdDebug(1202) << "ComboBox got the focus..." << endl;
      if (m_bLocationBarConnected)
      {
        //kdDebug(1202) << "Was already connected..." << endl;
        return KParts::MainWindow::eventFilter( obj, ev );
      }
      m_bLocationBarConnected = true;

      // Workaround for Qt issue: usually, QLineEdit reacts on Ctrl-D,
      // but the duplicate_window action also has Ctrl-D as accel and
      // prevents the lineedit from getting this event. IMHO the accel
      // should be disabled in favor of the focus-widget.
      KAction *duplicate = actionCollection()->action("duplicate_window");
      if ( duplicate->shortcut() == QKeySequence(CTRL+Key_D) )
          duplicate->setEnabled( false );

      if (slotNames.contains("cut()"))
        disconnect( m_paCut, SIGNAL( activated() ), ext, SLOT( cut() ) );
      if (slotNames.contains("copy()"))
        disconnect( m_paCopy, SIGNAL( activated() ), ext, SLOT( copy() ) );
      if (slotNames.contains("paste()"))
        disconnect( m_paPaste, SIGNAL( activated() ), ext, SLOT( paste() ) );
      if (slotNames.contains("del()"))
        disconnect( m_paDelete, SIGNAL( activated() ), ext, SLOT( del() ) );
      if (slotNames.contains("trash()"))
        disconnect( m_paTrash, SIGNAL( activated() ), ext, SLOT( trash() ) );

      connect( m_paCut, SIGNAL( activated() ), m_combo->lineEdit(), SLOT( cut() ) );
      connect( m_paCopy, SIGNAL( activated() ), m_combo->lineEdit(), SLOT( copy() ) );
      connect( m_paPaste, SIGNAL( activated() ), m_combo->lineEdit(), SLOT( paste() ) );
      connect( QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()) );
      connect( m_combo->lineEdit(), SIGNAL(textChanged(const QString &)), this, SLOT(slotCheckComboSelection()) );
      connect( m_combo->lineEdit(), SIGNAL(selectionChanged()), this, SLOT(slotCheckComboSelection()) );

      m_paTrash->setEnabled(false);
      if (m_paDelete)
        m_paDelete->setEnabled(false);

      slotClipboardDataChanged();

    }
    else if ( ev->type()==QEvent::FocusOut)
    {
      //kdDebug(1202) << "ComboBox lost focus..." << endl;
      if (!m_bLocationBarConnected)
      {
        //kdDebug(1202) << "Was already disconnected..." << endl;
        return KParts::MainWindow::eventFilter( obj, ev );
      }
      m_bLocationBarConnected = false;

      // see above in FocusIn for explanation
      // we use new_window as reference, as it's always in the same state
      // as duplicate_window
      KAction *duplicate = actionCollection()->action("duplicate_window");
      if ( duplicate->shortcut() == QKeySequence(CTRL+Key_D) )
          duplicate->setEnabled( actionCollection()->action("new_window")->isEnabled() );

      if (slotNames.contains("cut()"))
        connect( m_paCut, SIGNAL( activated() ), ext, SLOT( cut() ) );
      if (slotNames.contains("copy()"))
        connect( m_paCopy, SIGNAL( activated() ), ext, SLOT( copy() ) );
      if (slotNames.contains("paste()"))
        connect( m_paPaste, SIGNAL( activated() ), ext, SLOT( paste() ) );
      if (slotNames.contains("del()"))
        connect( m_paDelete, SIGNAL( activated() ), ext, SLOT( del() ) );
      if (slotNames.contains("trash()"))
        connect( m_paTrash, SIGNAL( activated() ), ext, SLOT( trash() ) );

      disconnect( m_paCut, SIGNAL( activated() ), m_combo->lineEdit(), SLOT( cut() ) );
      disconnect( m_paCopy, SIGNAL( activated() ), m_combo->lineEdit(), SLOT( copy() ) );
      disconnect( m_paPaste, SIGNAL( activated() ), m_combo->lineEdit(), SLOT( paste() ) );
      disconnect( QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()) );
      disconnect( m_combo->lineEdit(), SIGNAL(textChanged(const QString &)), this, SLOT(slotCheckComboSelection()) );
      disconnect( m_combo->lineEdit(), SIGNAL(selectionChanged()), this, SLOT(slotCheckComboSelection()) );

      if ( ext )
      {
          m_paCut->setEnabled( ext->isActionEnabled( "cut" ) );
          m_paCopy->setEnabled( ext->isActionEnabled( "copy" ) );
          m_paPaste->setEnabled( ext->isActionEnabled( "paste" ) );
          if (m_paDelete)
              m_paDelete->setEnabled( ext->isActionEnabled( "delete" ) );
          m_paTrash->setEnabled( ext->isActionEnabled( "trash" ) );
      }
      else
      {
          m_paCut->setEnabled( false );
          m_paCopy->setEnabled( false );
          m_paPaste->setEnabled( false );
          if (m_paDelete)
              m_paDelete->setEnabled( false );
          m_paTrash->setEnabled( false );
      }
    }
  }
  return KParts::MainWindow::eventFilter( obj, ev );
}

void KonqMainWindow::slotClipboardDataChanged()
{
  //kdDebug(1202) << "KonqMainWindow::slotClipboardDataChanged()" << endl;
  QMimeSource *data = QApplication::clipboard()->data();
  m_paPaste->setEnabled( data->provides( "text/plain" ) );
  slotCheckComboSelection();
}

void KonqMainWindow::slotCheckComboSelection()
{
  //kdDebug(1202) << "m_combo->lineEdit()->hasMarkedText() : " << hasSelection << endl;
  bool hasSelection = m_combo->lineEdit()->hasSelectedText();
  m_paCopy->setEnabled( hasSelection );
  m_paCut->setEnabled( hasSelection );
}

void KonqMainWindow::slotClearLocationBar()
{
  kdDebug(1202) << "slotClearLocationBar" << endl;
  slotStop();
  m_combo->clearTemporary();
  m_combo->setFocus();
}

void KonqMainWindow::slotForceSaveMainWindowSettings()
{
//  kdDebug(1202)<<"slotForceSaveMainWindowSettings()"<<endl;
  if ( autoSaveSettings() ) // don't do it on e.g. JS window.open windows with no toolbars!
  {
      saveMainWindowSettings( KGlobal::config(), "KonqMainWindow" );
      KGlobal::config()->sync();
  }
}

void KonqMainWindow::slotShowMenuBar()
{
  if (menuBar()->isVisible())
    menuBar()->hide();
  else
    menuBar()->show();
  slotForceSaveMainWindowSettings();
}

void KonqMainWindow::slotUpdateFullScreen( bool set )
{
  if( set )
  {
    showFullScreen();
    // Create toolbar button for exiting from full-screen mode
    // ...but only if there isn't one already...

    bool haveFullScreenButton = false;

    //Walk over the toolbars and check whether there is a show fullscreen button in any of them
    QPtrListIterator<KToolBar> barIt = toolBarIterator();
    for (; barIt.current(); ++barIt )
    {
        //Are we plugged here, in a visible toolbar?
        if (barIt.current()->isVisible() &&
            action( "fullscreen" )->isPlugged(barIt.current()))
        {
            haveFullScreenButton = true;
            break;
        }
    }

    if (!haveFullScreenButton)
    {
        QPtrList<KAction> lst;
        lst.append( m_ptaFullScreen );
        plugActionList( "fullscreen", lst );
    }


    menuBar()->hide();

    // Qt bug, the flags are lost. They know about it.
    // happens only with the hackish non-_NET_WM_STATE_FULLSCREEN way
    setWFlags( WDestructiveClose );
    // Qt bug (see below)
    setAcceptDrops( FALSE );
    topData()->dnd = 0;
    setAcceptDrops( TRUE );
  }
  else
  {
#if QT_VERSION >= 0x030300
    setWindowState( windowState() & ~WindowFullScreen );
#else
    if( isMaximized())
        {
        showNormal();
        showMaximized(); // showNormal() would reset maximize state
        }
    else
        showNormal();
#endif
    unplugActionList( "fullscreen" );

    menuBar()->show(); // maybe we should store this setting instead of forcing it

    // Qt bug, the flags aren't restored. They know about it.
    setWFlags( WType_TopLevel | WDestructiveClose );
    // Other Qt bug
    setAcceptDrops( FALSE );
    topData()->dnd = 0;
    setAcceptDrops( TRUE );
  }
}

void KonqMainWindow::setLocationBarURL( const KURL &url )
{
    setLocationBarURL( url.prettyURL( 0, KURL::StripFileProtocol ) );
}

void KonqMainWindow::setLocationBarURL( const QString &url )
{
  kdDebug(1202) << "KonqMainWindow::setLocationBarURL: url = " << url << endl;

  m_combo->setURL( url );

  if ( !url.isEmpty() ) {
      setIcon( KonqPixmapProvider::self()->pixmapFor( url ) );
  }
}

// called via DCOP from KonquerorIface
void KonqMainWindow::comboAction( int action, const QString& url, const QCString& objId )
{
    if (!s_lstViews) // this happens in "konqueror --silent"
        return;

    KonqCombo *combo = 0L;
    KonqMainWindow *window = s_lstViews->first();
    while ( window ) {
        if ( window->m_combo ) {
            combo = window->m_combo;

            switch ( action ) {
            case ComboAdd:
              combo->insertPermanent( url );
              break;
            case ComboClear:
              combo->clearHistory();
              break;
            case ComboRemove:
              combo->removeURL( url );
              break;
            default:
              break;
            }
        }
        window = s_lstViews->next();
    }

    // only one instance should save...
    if ( combo && objId == kapp->dcopClient()->defaultObject() )
      combo->saveItems();
}

QString KonqMainWindow::locationBarURL() const
{
    return m_combo->currentText();
}

void KonqMainWindow::focusLocationBar()
{
  m_combo->setFocus();
}

void KonqMainWindow::startAnimation()
{
  //kdDebug(1202) << "KonqMainWindow::startAnimation" << endl;
  m_paAnimatedLogo->start();
  m_paStop->setEnabled( true );
}

void KonqMainWindow::stopAnimation()
{
  //kdDebug(1202) << "KonqMainWindow::stopAnimation" << endl;
  m_paAnimatedLogo->stop();
  m_paStop->setEnabled( false );
}

void KonqMainWindow::setUpEnabled( const KURL &url )
{
  //kdDebug(1202) << "KonqMainWindow::setUpEnabled(" << url << ")" << endl;
  //kdDebug(1202) << "hasPath=" << url.hasPath() << endl;
  bool bHasUpURL = false;

  bHasUpURL = ( ( url.hasPath() && url.path() != "/" && ( url.path()[0]=='/' ) )
                || !url.query().isEmpty() /*e.g. lists.kde.org*/ );
  if ( !bHasUpURL )
    bHasUpURL = url.hasSubURL();

  m_paUp->setEnabled( bHasUpURL );
}

void KonqMainWindow::initActions()
{
  actionCollection()->setHighlightingEnabled( true );
  connectActionCollection( actionCollection() );


  // Note about this method : don't call setEnabled() on any of the actions.
  // They are all disabled then re-enabled with enableAllActions
  // If any one needs to be initially disabled, put that code in enableAllActions

  // File menu
  m_pMenuNew = new KNewMenu ( actionCollection(), this, "new_menu" );
  QObject::connect( m_pMenuNew->popupMenu(), SIGNAL(aboutToShow()),
                    this, SLOT(slotFileNewAboutToShow()) );

  m_paFileType = new KAction( i18n( "&Edit File Type..." ), 0, actionCollection(), "editMimeType" );
  m_paProperties = new KAction( i18n( "Properties" ), ALT+Key_Return, actionCollection(), "properties" );
  (void) new KAction( i18n( "New &Window" ), "window_new", KStdAccel::shortcut(KStdAccel::New), this, SLOT( slotNewWindow() ), actionCollection(), "new_window" );
  (void) new KAction( i18n( "&Duplicate Window" ), "window_new", CTRL+Key_D, this, SLOT( slotDuplicateWindow() ), actionCollection(), "duplicate_window" );
  (void) new KAction( i18n( "Send &Link Address..." ), "mail_generic", 0, this, SLOT( slotSendURL() ), actionCollection(), "sendURL" );
  (void) new KAction( i18n( "S&end File..." ), "mail_generic", 0, this, SLOT( slotSendFile() ), actionCollection(), "sendPage" );
  (void) new KAction( i18n( "&Run Command..." ), "run", 0/*kdesktop has a binding for it*/, this, SLOT( slotRun() ), actionCollection(), "run" );
  if (kapp->authorize("shell_access"))
  {
     (void) new KAction( i18n( "Open &Terminal" ), "openterm", Key_F4, this, SLOT( slotOpenTerminal() ), actionCollection(), "open_terminal" );
  }
  (void) new KAction( i18n( "&Open Location..." ), "fileopen", KStdAccel::shortcut(KStdAccel::Open), this, SLOT( slotOpenLocation() ), actionCollection(), "open_location" );

  m_paFindFiles = new KToggleAction( i18n( "&Find File..." ), "filefind", 0 /*not KStdAccel::find!*/, this, SLOT( slotToolFind() ), actionCollection(), "findfile" );

  m_paPrint = KStdAction::print( 0, 0, actionCollection(), "print" );
  (void) KStdAction::quit( this, SLOT( close() ), actionCollection(), "quit" );

  m_ptaUseHTML = new KToggleAction( i18n( "&Use index.html" ), 0, this, SLOT( slotShowHTML() ), actionCollection(), "usehtml" );
  m_paLockView = new KToggleAction( i18n( "Lock to Current Location"), 0, this, SLOT( slotLockView() ), actionCollection(), "lock" );
  m_paLinkView = new KToggleAction( i18n( "Lin&k View"), 0, this, SLOT( slotLinkView() ), actionCollection(), "link" );

  // Go menu
  m_paUp = new KToolBarPopupAction( i18n( "&Up" ), "up", KStdAccel::shortcut(KStdAccel::Up), this, SLOT( slotUp() ), actionCollection(), "up" );
  connect( m_paUp->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotUpAboutToShow() ) );
  connect( m_paUp->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotUpActivated( int ) ) );

  QPair< KGuiItem, KGuiItem > backForward = KStdGuiItem::backAndForward();
  m_paBack = new KToolBarPopupAction( backForward.first, KStdAccel::shortcut(KStdAccel::Back), this, SLOT( slotBack() ), actionCollection(), "back" );
  connect( m_paBack->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotBackAboutToShow() ) );
  connect( m_paBack->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotBackActivated( int ) ) );

  m_paForward = new KToolBarPopupAction( backForward.second, KStdAccel::shortcut(KStdAccel::Forward), this, SLOT( slotForward() ), actionCollection(), "forward" );
  connect( m_paForward->popupMenu(), SIGNAL( aboutToShow() ), this, SLOT( slotForwardAboutToShow() ) );
  connect( m_paForward->popupMenu(), SIGNAL( activated( int ) ), this, SLOT( slotForwardActivated( int ) ) );

  m_paHistory = new KonqBidiHistoryAction( i18n("History"), actionCollection(), "history" );
  connect( m_paHistory, SIGNAL( menuAboutToShow() ), this, SLOT( slotGoMenuAboutToShow() ) );
  connect( m_paHistory, SIGNAL( activated( int ) ), this, SLOT( slotGoHistoryActivated( int ) ) );

  m_paHome = new KAction( i18n( "Home URL" ), "gohome", KStdAccel::shortcut(KStdAccel::Home), this, SLOT( slotHome() ), actionCollection(), "home" );

  (void) new KAction( i18n( "App&lications" ), 0, this, SLOT( slotGoApplications() ), actionCollection(), "go_applications" );
  (void) new KAction( i18n( "&Devices" ), 0, this, SLOT( slotGoDevices() ), actionCollection(), "go_devices" );
  (void) new KAction( i18n( "Sett&ings" ), 0, this, SLOT( slotGoSettings() ), actionCollection(), "go_settings" );
  //(void) new KAction( i18n( "Sidebar Configuration" ), 0, this, SLOT( slotGoDirTree() ), actionCollection(), "go_dirtree" );
  (void) new KAction( i18n( "Trash" ), 0, this, SLOT( slotGoTrash() ), actionCollection(), "go_trash" );
  (void) new KAction( i18n( "Templates" ), 0, this, SLOT( slotGoTemplates() ), actionCollection(), "go_templates" );
  (void) new KAction( i18n( "Autostart" ), 0, this, SLOT( slotGoAutostart() ), actionCollection(), "go_autostart" );
  KonqMostOftenURLSAction *mostOften = new KonqMostOftenURLSAction( i18n("Most Often Visited"), actionCollection(), "go_most_often" );
  connect( mostOften, SIGNAL( activated( const KURL& )),
	   SLOT( slotOpenURL( const KURL& )));

  // Settings menu

  m_paSaveViewProfile = new KAction( i18n( "&Save View Profile..." ), 0, this, SLOT( slotSaveViewProfile() ), actionCollection(), "saveviewprofile" );
  m_paSaveViewPropertiesLocally = new KToggleAction( i18n( "View Properties Saved in &Folder" ), 0, this, SLOT( slotSaveViewPropertiesLocally() ), actionCollection(), "saveViewPropertiesLocally" );
   // "Remove" ? "Reset" ? The former is more correct, the latter is more kcontrol-like...
  m_paRemoveLocalProperties = new KAction( i18n( "Remove Folder Properties" ), 0, this, SLOT( slotRemoveLocalProperties() ), actionCollection(), "removeLocalProperties" );


  m_configureModules << "kde-filebehavior.desktop" << "kde-fileappearance.desktop" <<
      "kde-filepreviews.desktop" << "kde-filetypes.desktop" <<
      "kde-khtml_behavior.desktop" << "kde-khtml_java_js.desktop" <<
      "kde-khtml_fonts.desktop" << "kde-ebrowsing.desktop" <<
      "kde-kcmhistory.desktop" << "kde-cookies.desktop" <<
      "kde-cache.desktop" << "kde-proxy.desktop" << "kde-kcmcss.desktop" <<
      "kde-crypto.desktop" << "kde-useragent.desktop" <<
      "kde-khtml_plugins.desktop" << "kde-kcmkonqyperformance.desktop";


  if (!kapp->authorizeControlModules(configModules()).isEmpty())
     KStdAction::preferences (this, SLOT (slotConfigure()), actionCollection() );

  KStdAction::keyBindings( guiFactory(), SLOT( configureShortcuts() ), actionCollection() );
  KStdAction::configureToolbars( this, SLOT( slotConfigureToolbars() ), actionCollection() );

  m_paConfigureExtensions = new KAction( i18n("Configure Extensions..."), "extensions", 0, this, SLOT( slotConfigureExtensions()), actionCollection(), "options_configure_extensions");
  m_paConfigureSpellChecking = new KAction( i18n("Configure Spell Checking..."), "spellcheck", 0,this, SLOT( slotConfigureSpellChecking()), actionCollection(), "configurespellcheck");

  // Window menu
  m_paSplitViewHor = new KAction( i18n( "Split View &Left/Right" ), "view_left_right", CTRL+SHIFT+Key_L, this, SLOT( slotSplitViewHorizontal() ), actionCollection(), "splitviewh" );
  m_paSplitViewVer = new KAction( i18n( "Split View &Top/Bottom" ), "view_top_bottom", CTRL+SHIFT+Key_T, this, SLOT( slotSplitViewVertical() ), actionCollection(), "splitviewv" );
  m_paAddTab = new KAction( i18n( "&New Tab" ), "tab_new", "CTRL+SHIFT+N;CTRL+T", this, SLOT( slotAddTab() ), actionCollection(), "newtab" );
  m_paDuplicateTab = new KAction( i18n( "&Duplicate Current Tab" ), "tab_duplicate", CTRL+SHIFT+Key_D, this, SLOT( slotDuplicateTab() ), actionCollection(), "duplicatecurrenttab" );
  m_paBreakOffTab = new KAction( i18n( "Detach Current Tab" ), "tab_breakoff", CTRL+SHIFT+Key_B, this, SLOT( slotBreakOffTab() ), actionCollection(), "breakoffcurrenttab" );
  m_paRemoveView = new KAction( i18n( "&Close Active View" ),"view_remove", CTRL+SHIFT+Key_R, this, SLOT( slotRemoveView() ), actionCollection(), "removeview" );
  m_paRemoveTab = new KAction( i18n( "Close Current Tab" ), "tab_remove", CTRL+Key_W, this, SLOT( slotRemoveTab() ), actionCollection(), "removecurrenttab" );
  m_paRemoveOtherTabs = new KAction( i18n( "Close &Other Tabs" ), "tab_remove", 0, this, SLOT( slotRemoveOtherTabsPopup() ), actionCollection(), "removeothertabs" );

  m_paActivateNextTab = new KAction( i18n( "Activate Next Tab" ), "tab_next", QApplication::reverseLayout() ? KStdAccel::tabPrev() : KStdAccel::tabNext(), this, SLOT( slotActivateNextTab() ), actionCollection(), "activatenexttab" );
  m_paActivatePrevTab = new KAction( i18n( "Activate Previous Tab" ), "tab_previous", QApplication::reverseLayout() ? KStdAccel::tabNext() : KStdAccel::tabPrev(), this, SLOT( slotActivatePrevTab() ), actionCollection(), "activateprevtab" );

  char actionname[15];
  for (int i=1;i<13;i++) {
    sprintf(actionname,"activate_tab_%02d", i);
    new KAction(i18n("Activate Tab %1").arg(i), 0, this, SLOT(slotActivateTab()), actionCollection(), actionname);
  }

  m_paMoveTabLeft = new KAction( i18n("Move Tab Left"), 0 , CTRL+SHIFT+Key_Left,this, SLOT( slotMoveTabLeft()),actionCollection(),"tab_move_left");
  m_paMoveTabRight = new KAction( i18n("Move Tab Right"), 0 , CTRL+SHIFT+Key_Right,this, SLOT( slotMoveTabRight()),actionCollection(),"tab_move_right");

#ifndef NDEBUG
  m_paDumpDebugInfo = new KAction( i18n( "Dump Debug Info" ), "view_dump_debug_info", 0, this, SLOT( slotDumpDebugInfo() ), actionCollection(), "dumpdebuginfo" );
#endif

  m_paSaveRemoveViewProfile = new KAction( i18n( "C&onfigure View Profiles..." ), 0, m_pViewManager, SLOT( slotProfileDlg() ), actionCollection(), "saveremoveviewprofile" );
  m_pamLoadViewProfile = new KActionMenu( i18n( "Load &View Profile" ), actionCollection(), "loadviewprofile" );

  m_pViewManager->setProfiles( m_pamLoadViewProfile );

  m_ptaFullScreen = KStdAction::fullScreen( 0, 0, actionCollection(), this );
  KShortcut fullScreenShortcut = m_ptaFullScreen->shortcut();
  fullScreenShortcut.append( KKey( Key_F11 ) );
  m_ptaFullScreen->setShortcut( fullScreenShortcut );
  connect( m_ptaFullScreen, SIGNAL( toggled( bool )), this, SLOT( slotUpdateFullScreen( bool )));

  KShortcut reloadShortcut = KStdAccel::shortcut(KStdAccel::Reload);
  reloadShortcut.append(KKey(CTRL + Key_R));
  m_paReload = new KAction( i18n( "&Reload" ), "reload", reloadShortcut, this, SLOT( slotReload() ), actionCollection(), "reload" );
  m_paReloadAllTabs = new KAction( i18n( "&Reload All Tabs" ), "reload_all_tabs", SHIFT+Key_F5, this, SLOT( slotReloadAllTabs() ), actionCollection(), "reload_all_tabs" );

  m_paUndo = KStdAction::undo( KonqUndoManager::self(), SLOT( undo() ), actionCollection(), "undo" );
  //m_paUndo->setEnabled( KonqUndoManager::self()->undoAvailable() );
  connect( KonqUndoManager::self(), SIGNAL( undoTextChanged( const QString & ) ),
           m_paUndo, SLOT( setText( const QString & ) ) );

  // Those are connected to the browserextension directly
  m_paCut = KStdAction::cut( 0, 0, actionCollection(), "cut" );
  m_paCopy = KStdAction::copy( 0, 0, actionCollection(), "copy" );
  m_paPaste = KStdAction::paste( 0, 0, actionCollection(), "paste" );
  m_paStop = new KAction( i18n( "&Stop" ), "stop", Key_Escape, this, SLOT( slotStop() ), actionCollection(), "stop" );

  m_paRename = new KAction( i18n( "&Rename" ), /*"editrename",*/ Key_F2, actionCollection(), "rename" );
  m_paTrash = new KAction( i18n( "&Move to Trash" ), "edittrash", Key_Delete, actionCollection(), "trash" );

  KConfig *config = KGlobal::config();
  KConfigGroupSaver cgs( config, "FMSettings" );
  if ( config->readBoolEntry( "ShowDeleteCommand", true ) )
    m_paDelete = new KAction( i18n( "&Delete" ), "editdelete", SHIFT+Key_Delete, actionCollection(), "del" );

  m_paAnimatedLogo = new KonqLogoAction( i18n("Animated Logo"), 0, this, SLOT( slotDuplicateWindow() ), actionCollection(), "animated_logo" );

  // Location bar
  m_locationLabel = new KonqDraggableLabel( this, i18n("L&ocation: ") );
  (void) new KWidgetAction( m_locationLabel, i18n("L&ocation: "), Key_F6, this, SLOT( slotLocationLabelActivated() ), actionCollection(), "location_label" );
  m_locationLabel->setBuddy( m_combo );

  KWidgetAction* comboAction = new KWidgetAction( m_combo, i18n( "Location Bar" ), 0,
                  0, 0, actionCollection(), "toolbar_url_combo" );
  comboAction->setShortcutConfigurable( false );
  comboAction->setAutoSized( true );

  QWhatsThis::add( m_combo, i18n( "Location Bar<p>"
				  "Enter a web address or search term." ) );

  KAction *clearLocation = new KAction( i18n( "Clear Location Bar" ),
					QApplication::reverseLayout() ? "clear_left" : "locationbar_erase",
					CTRL+Key_L, this, SLOT( slotClearLocationBar() ), actionCollection(), "clear_location" );
  clearLocation->setWhatsThis( i18n( "Clear Location bar<p>"
				     "Clears the content of the location bar." ) );

  // Bookmarks menu
  m_pamBookmarks = new KActionMenu( i18n( "&Bookmarks" ), "bookmark", actionCollection(), "bookmarks" );
  m_pamBookmarks->setDelayed( false );

  // The actual menu needs a different action collection, so that the bookmarks
  // don't appear in kedittoolbar
  m_bookmarksActionCollection = new KActionCollection( this );
  m_bookmarksActionCollection->setHighlightingEnabled( true );
  connectActionCollection( m_bookmarksActionCollection );

  m_pBookmarkMenu = new KBookmarkMenu( KonqBookmarkManager::self(), m_pBookmarksOwner, m_pamBookmarks->popupMenu(), m_bookmarksActionCollection, true );
  connect( m_pBookmarkMenu,
           SIGNAL( aboutToShowContextMenu(const KBookmark &, QPopupMenu*) ),
           this, SLOT( slotFillContextMenu(const KBookmark &, QPopupMenu*) ));

  KAction *addBookmark = actionCollection()->action("add_bookmark");
  if (addBookmark)
     addBookmark->setText(i18n("Bookmark This Location"));

  m_paShowMenuBar = KStdAction::showMenubar( this, SLOT( slotShowMenuBar() ), actionCollection() );

  (void) new KAction( i18n( "Kon&queror Introduction" ), 0, this, SLOT( slotIntro() ), actionCollection(), "konqintro" );

  KAction *goUrl = new KonqGoURLAction( i18n( "Go" ), "key_enter", 0, this, SLOT( goURL() ), actionCollection(), "go_url" );
  goUrl->setWhatsThis( i18n( "Go<p>"
			     "Goes to the page that has been entered into the location bar." ) );

  enableAllActions( false );

  // help stuff
  m_paUp->setWhatsThis( i18n( "Enter the parent folder<p>"
                              "For instance, if the current location is file:/home/%1 clicking this "
                              "button will take you to file:/home." ).arg( KUser().loginName() ) );
  m_paUp->setStatusText( i18n( "Enter the parent folder" ) );

  m_paBack->setWhatsThis( i18n( "Move backwards one step in the browsing history<p>" ) );
  m_paBack->setStatusText( i18n( "Move backwards one step in the browsing history" ) );

  m_paForward->setWhatsThis( i18n( "Move forward one step in the browsing history<p>" ) );
  m_paForward->setStatusText( i18n( "Move forward one step in the browsing history" ) );

  m_paHome->setWhatsThis( i18n( "Navigate to your 'Home URL'<p>"
                                "You can configure the location this button takes you to in the "
                                "<b>KDE Control Center</b>, under <b>File Manager</b>/<b>Behavior</b>." ) );
  m_paHome->setStatusText( i18n( "Navigate to your 'Home URL'" ) );

  m_paReload->setWhatsThis( i18n( "Reload the currently displayed document<p>"
                                  "This may, for example, be needed to refresh webpages that have been "
                                  "modified since they were loaded, in order to make the changes visible." ) );
  m_paReload->setStatusText( i18n( "Reload the currently displayed document" ) );

  m_paReloadAllTabs->setWhatsThis( i18n( "Reload all currently displayed documents in tabs<p>"
                                  "This may, for example, be needed to refresh webpages that have been "
                                  "modified since they were loaded, in order to make the changes visible." ) );
  m_paReloadAllTabs->setStatusText( i18n( "Reload all currently displayed document in tabs" ) );

  m_paStop->setWhatsThis( i18n( "Stop loading the document<p>"
                                "All network transfers will be stopped and Konqueror will display the content "
                                "that has been received so far." ) );
  m_paStop->setStatusText( i18n( "Stop loading the document" ) );

  m_paCut->setWhatsThis( i18n( "Cut the currently selected text or item(s) and move it "
                               "to the system clipboard<p> "
                               "This makes it available to the <b>Paste</b> command in Konqueror "
                               "and other KDE applications." ) );
  m_paCut->setStatusText( i18n( "Move the selected text or item(s) to the clipboard" ) );

  m_paCopy->setWhatsThis( i18n( "Copy the currently selected text or item(s) to the "
                                "system clipboard<p>"
                                "This makes it available to the <b>Paste</b> command in Konqueror "
                                "and other KDE applications." ) );
  m_paCopy->setStatusText( i18n( "Copy the selected text or item(s) to the clipboard" ) );

  m_paPaste->setWhatsThis( i18n( "Paste the previously cut or copied clipboard "
                                 "contents<p>"
                                 "This also works for text copied or cut from other KDE applications." ) );
  m_paPaste->setStatusText( i18n( "Paste the clipboard contents" ) );

  m_paPrint->setWhatsThis( i18n( "Print the currently displayed document<p>"
                                 "You will be presented with a dialog where you can set various "
                                 "options, such as the number of copies to print and which printer "
                                 "to use.<p>"
                                 "This dialog also provides access to special KDE printing "
                                 "services such as creating a PDF file from the current document." ) );
  m_paPrint->setStatusText( i18n( "Print the current document" ) );



  // Please proof-read those (David)

  m_ptaUseHTML->setStatusText( i18n("If present, open index.html when entering a folder.") );
  m_paLockView->setStatusText( i18n("A locked view cannot change folders. Use in combination with 'link view' to explore many files from one folder") );
  m_paLinkView->setStatusText( i18n("Sets the view as 'linked'. A linked view follows folder changes made in other linked views.") );
}

void KonqMainWindow::slotFillContextMenu( const KBookmark &bk, QPopupMenu * pm )
{
  kdDebug() << "KonqMainWindow::slotFillContextMenu(bk, pm == " << pm << ")" << endl;
  popupItems.clear();
  popupUrlArgs = KParts::URLArgs();
  if ( bk.isGroup() )
  {
    KBookmarkGroup grp = bk.toGroup();
    QValueList<KURL> list = grp.groupUrlList();
    QValueList<KURL>::Iterator it = list.begin();
    for (; it != list.end(); ++it )
      popupItems.append( new KFileItem( (*it), QString::null, KFileItem::Unknown) );
    pm->insertItem( SmallIcon("tab_new"), i18n( "Open Folder in Tabs" ), this, SLOT( slotPopupNewTab() ) );
  }
  else
  {
    popupItems.append( new KFileItem( bk.url(), QString::null, KFileItem::Unknown) );
    pm->insertItem( SmallIcon("window_new"), i18n( "Open in New Window" ), this, SLOT( slotPopupNewWindow() ) );
    pm->insertItem( SmallIcon("tab_new"), i18n( "Open in New Tab" ), this, SLOT( slotPopupNewTab() ) );
  }
}

void KonqMainWindow::slotMoveTabLeft()
{
  if ( QApplication::reverseLayout() )
    m_pViewManager->moveTabForward();
  else
    m_pViewManager->moveTabBackward();
}

void KonqMainWindow::slotMoveTabRight()
{
  if ( QApplication::reverseLayout() )
    m_pViewManager->moveTabBackward();
  else
    m_pViewManager->moveTabForward();
}

void KonqMainWindow::updateToolBarActions( bool pendingAction /*=false*/)
{
  // Enables/disables actions that depend on the current view & url (mostly toolbar)
  // Up, back, forward, the edit extension, stop button, wheel
  setUpEnabled( m_currentView->url() );
  m_paBack->setEnabled( m_currentView->canGoBack() );
  m_paForward->setEnabled( m_currentView->canGoForward() );

  if ( m_currentView->isLoading() )
  {
    startAnimation(); // takes care of m_paStop
  }
  else
  {
    m_paAnimatedLogo->stop();
    m_paStop->setEnabled( pendingAction );  //enable/disable based on any pending actions...
  }

  if ( m_currentView && m_currentView->url().isLocalFile() &&
       !m_currentView->isLockedViewMode() ) {
      if ( m_currentView->serviceTypes().contains( "inode/directory" ) )
          m_ptaUseHTML->setEnabled( true );
      else if ( m_currentView->serviceTypes().contains(  "text/html" ) ) {
          // Currently viewing an index.html file via this feature (i.e. url points to a dir)
          QString locPath = KURL( m_currentView->locationBarURL() ).path();
          m_ptaUseHTML->setEnabled( QFileInfo( locPath ).isDir() );
      } else
          m_ptaUseHTML->setEnabled( false );
  }
  else
  {
      m_ptaUseHTML->setEnabled( false );
  }
}

void KonqMainWindow::updateViewActions()
{
  // Update actions that depend on the current view and its mode, or on the number of views etc.

  // Don't do things in this method that depend on m_currentView->url().
  // When going 'back' in history this will be called before opening the url.
  // Use updateToolBarActions instead.

  slotUndoAvailable( KonqUndoManager::self()->undoAvailable() );

  // Can lock a view only if there is a next view
  //m_paLockView->setEnabled( m_pViewManager->chooseNextView(m_currentView) != 0L && );
  //kdDebug(1202) << "KonqMainWindow::updateViewActions m_paLockView enabled ? " << m_paLockView->isEnabled() << endl;

  m_paLockView->setEnabled( viewCount() > 1 );
  m_paLockView->setChecked( m_currentView && m_currentView->isLockedLocation() );

  // Can remove view if we'll still have a main view after that
  m_paRemoveView->setEnabled( mainViewsCount() > 1 ||
                              ( m_currentView && m_currentView->isToggleView() ) );

  KonqFrameBase* docContainer = m_pViewManager->docContainer();

  if ( docContainer == 0L && !(currentView() && currentView()->frame()))
  {
    m_paAddTab->setEnabled( false );
    m_paDuplicateTab->setEnabled( false );
    m_paRemoveTab->setEnabled( false );
    m_paRemoveOtherTabs->setEnabled( false );
    m_paBreakOffTab->setEnabled( false );
    m_paActivateNextTab->setEnabled( false );
    m_paActivatePrevTab->setEnabled( false );
    m_paMoveTabLeft->setEnabled( false );
    m_paMoveTabRight->setEnabled( false );
  }
  else
  {
    m_paAddTab->setEnabled( true );
    m_paDuplicateTab->setEnabled( true );
    if ( docContainer && docContainer->frameType() == "Tabs" )
    {
        KonqFrameTabs* tabContainer = static_cast<KonqFrameTabs*>(docContainer);
        bool state = (tabContainer->count()>1);
        m_paRemoveTab->setEnabled( state );
	m_paRemoveOtherTabs->setEnabled( state );
        m_paBreakOffTab->setEnabled( state );
        m_paActivateNextTab->setEnabled( state );
        m_paActivatePrevTab->setEnabled( state );

        QPtrList<KonqFrameBase>* childFrameList = tabContainer->childFrameList();
        m_paMoveTabLeft->setEnabled( currentView() ? currentView()->frame()!=
	    (QApplication::reverseLayout() ? childFrameList->last() : childFrameList->first()) : false );
        m_paMoveTabRight->setEnabled( currentView() ? currentView()->frame()!=
	    (QApplication::reverseLayout() ? childFrameList->first() : childFrameList->last()) : false );
    }
    else
    {
      m_paRemoveTab->setEnabled( false );
      m_paRemoveOtherTabs->setEnabled( false );
      m_paBreakOffTab->setEnabled( false );
      m_paActivateNextTab->setEnabled( false );
      m_paActivatePrevTab->setEnabled( false );
      m_paMoveTabLeft->setEnabled( false );
      m_paMoveTabRight->setEnabled( false );

    }
  }

  // Can split a view if it's not a toggle view (because a toggle view can be here only once)
  bool isNotToggle = m_currentView && !m_currentView->isToggleView();
  m_paSplitViewHor->setEnabled( isNotToggle );
  m_paSplitViewVer->setEnabled( isNotToggle );

  m_paLinkView->setChecked( m_currentView && m_currentView->isLinkedView() );

  if ( m_currentView && m_currentView->part() &&
       m_currentView->part()->inherits("KonqDirPart") )
  {
    KonqDirPart * dirPart = static_cast<KonqDirPart *>(m_currentView->part());
    m_paFindFiles->setEnabled( dirPart->findPart() == 0 );

    // Create the copy/move options if not already done
    if ( !m_paCopyFiles )
    {
      // F5 is the default key binding for Reload.... a la Windows.
      // mc users want F5 for Copy and F6 for move, but I can't make that default.
      m_paCopyFiles = new KAction( i18n("Copy &Files..."), Key_F7, this, SLOT( slotCopyFiles() ), actionCollection(), "copyfiles" );
      m_paMoveFiles = new KAction( i18n("M&ove Files..."), Key_F8, this, SLOT( slotMoveFiles() ), actionCollection(), "movefiles" );

      // This action doesn't appear in the GUI, it's for the shortcut only.
      // KNewMenu takes care of the GUI stuff.
      m_paNewDir = new KAction( i18n("Create Folder..." ), Key_F10, this, SLOT( slotNewDir() ),
                                actionCollection(), "konq_create_dir" );

      QPtrList<KAction> lst;
      lst.append( m_paCopyFiles );
      lst.append( m_paMoveFiles );
      m_paCopyFiles->setEnabled( false );
      m_paMoveFiles->setEnabled( false );
      plugActionList( "operations", lst );
    }
  }
  else
  {
      m_paFindFiles->setEnabled( false );

      if (m_paCopyFiles)
      {
          unplugActionList( "operations" );
          delete m_paCopyFiles;
          m_paCopyFiles = 0;
          delete m_paMoveFiles;
          m_paMoveFiles = 0;
          delete m_paNewDir;
          m_paNewDir = 0;
      }
  }
}

QString KonqMainWindow::findIndexFile( const QString &dir )
{
  QDir d( dir );

  QString f = d.filePath( "index.html", false );
  if ( QFile::exists( f ) )
    return f;

  f = d.filePath( "index.htm", false );
  if ( QFile::exists( f ) )
    return f;

  f = d.filePath( "index.HTML", false );
  if ( QFile::exists( f ) )
    return f;

  return QString::null;
}

void KonqMainWindow::connectExtension( KParts::BrowserExtension *ext )
{
  //kdDebug(1202) << "Connecting extension " << ext << endl;
  KParts::BrowserExtension::ActionSlotMap * actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
  KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->begin();
  KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->end();

  QStrList slotNames = ext->metaObject()->slotNames();

  for ( ; it != itEnd ; ++it )
  {
    KAction * act = actionCollection()->action( it.key() );
    //kdDebug(1202) << it.key() << endl;
    if ( act )
    {
      // Does the extension have a slot with the name of this action ?
      if ( slotNames.contains( it.key()+"()" ) )
      {
          connect( act, SIGNAL( activated() ), ext, it.data() /* SLOT(slot name) */ );
          act->setEnabled( ext->isActionEnabled( it.key() ) );
      } else
          act->setEnabled(false);

    } else kdError(1202) << "Error in BrowserExtension::actionSlotMap(), unknown action : " << it.key() << endl;
  }

}

void KonqMainWindow::disconnectExtension( KParts::BrowserExtension *ext )
{
  //kdDebug(1202) << "Disconnecting extension " << ext << endl;
  KParts::BrowserExtension::ActionSlotMap * actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
  KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->begin();
  KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->end();

  QStrList slotNames =  ext->metaObject()->slotNames();

  for ( ; it != itEnd ; ++it )
  {
    KAction * act = actionCollection()->action( it.key() );
    //kdDebug(1202) << it.key() << endl;
    if ( act && slotNames.contains( it.key()+"()" ) )
    {
        //kdDebug(1202) << "disconnectExtension: " << act << " " << act->name() << endl;
        act->disconnect( ext );
    }
  }
}

void KonqMainWindow::enableAction( const char * name, bool enabled )
{
  KAction * act = actionCollection()->action( name );
  if (!act)
    kdWarning(1202) << "Unknown action " << name << " - can't enable" << endl;
  else
  {
    if ( m_bLocationBarConnected && (
      act==m_paCopy || act==m_paCut || act==m_paPaste || act==m_paDelete || act==m_paTrash ) )
        // Don't change action state while the location bar has focus.
        return;
    //kdDebug(1202) << "KonqMainWindow::enableAction " << name << " " << enabled << endl;
    act->setEnabled( enabled );
  }

  // Update "copy files" and "move files" accordingly
  if (m_paCopyFiles && !strcmp( name, "copy" ))
  {
    m_paCopyFiles->setEnabled( enabled );
  }
  else if (m_paMoveFiles && !strcmp( name, "cut" ))
  {
    m_paMoveFiles->setEnabled( enabled );
  }
}

void KonqMainWindow::currentProfileChanged()
{
    bool enabled = !m_pViewManager->currentProfile().isEmpty();
    m_paSaveViewProfile->setEnabled( enabled );
    m_paSaveViewProfile->setText( enabled ? i18n("&Save View Profile \"%1\"...").arg(m_pViewManager->currentProfileText())
                                          : i18n("&Save View Profile...") );
}

void KonqMainWindow::enableAllActions( bool enable )
{
  kdDebug(1202) << "KonqMainWindow::enableAllActions " << enable << endl;
  KParts::BrowserExtension::ActionSlotMap * actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();

  QValueList<KAction *> actions = actionCollection()->actions();
  QValueList<KAction *>::Iterator it = actions.begin();
  QValueList<KAction *>::Iterator end = actions.end();
  for (; it != end; ++it )
  {
    KAction *act = *it;
    if ( !QString(act->name()).startsWith("options_configure") /* do not touch the configureblah actions */
         && ( !enable || !actionSlotMap->contains( act->name() ) ) ) /* don't enable BE actions */
      act->setEnabled( enable );
  }
  // This method is called with enable=false on startup, and
  // then only once with enable=true when the first view is setup.
  // So the code below is where actions that should initially be disabled are disabled.
  if (enable)
  {
      setUpEnabled( m_currentView ? m_currentView->url() : KURL() );
      // we surely don't have any history buffers at this time
      m_paBack->setEnabled( false );
      m_paForward->setEnabled( false );

      // Load profile submenu
      m_pViewManager->profileListDirty( false );

      currentProfileChanged();

      updateViewActions(); // undo, lock, link and other view-dependent actions

      m_paStop->setEnabled( m_currentView && m_currentView->isLoading() );

      if (m_toggleViewGUIClient)
      {
          QPtrList<KAction> actions = m_toggleViewGUIClient->actions();
          for ( KAction * it = actions.first(); it ; it = actions.next() )
              it->setEnabled( true );
      }

  }
  actionCollection()->action( "quit" )->setEnabled( true );
}

void KonqMainWindow::disableActionsNoView()
{
    // No view -> there are some things we can't do
    m_paUp->setEnabled( false );
    m_paReload->setEnabled( false );
    m_paReloadAllTabs->setEnabled( false );
    m_paBack->setEnabled( false );
    m_paForward->setEnabled( false );
    m_ptaUseHTML->setEnabled( false );
    m_pMenuNew->setEnabled( false );
    m_paLockView->setEnabled( false );
    m_paLockView->setChecked( false );
    m_paSplitViewVer->setEnabled( false );
    m_paSplitViewHor->setEnabled( false );
    m_paRemoveView->setEnabled( false );
    m_paLinkView->setEnabled( false );
    if (m_toggleViewGUIClient)
    {
        QPtrList<KAction> actions = m_toggleViewGUIClient->actions();
        for ( KAction * it = actions.first(); it ; it = actions.next() )
            it->setEnabled( false );
    }
    // There are things we can do, though : bookmarks, view profile, location bar, new window,
    // settings, etc.
    m_paHome->setEnabled( true );
    m_pamBookmarks->setEnabled( true );
    static const char* const s_enActions[] = { "new_window", "duplicate_window", "open_location",
                                         "toolbar_url_combo", "clear_location", "animated_logo",
                                         "konqintro", "go_most_often", "go_applications", "go_dirtree",
                                         "go_trash", "go_templates", "go_autostart", "go_url",
                                         "options_configure_extensions", 0 };
    for ( int i = 0 ; s_enActions[i] ; ++i )
    {
        KAction * act = action(s_enActions[i]);
        if (act)
            act->setEnabled( true );
    }
    m_pamLoadViewProfile->setEnabled( true );
    m_paSaveViewProfile->setEnabled( true );
    m_paSaveRemoveViewProfile->setEnabled( true );
    m_combo->clearTemporary();
    updateLocalPropsActions();
}

void KonqExtendedBookmarkOwner::openBookmarkURL( const QString & url )
{
  kdDebug(1202) << (QString("KonqMainWindow::openBookmarkURL(%1)").arg(url)) << endl;
  m_pKonqMainWindow->openFilteredURL( url );
}

void KonqMainWindow::setCaption( const QString &caption )
{
  // KParts sends us empty captions when activating a brand new part
  // We can't change it there (in case of apps removing all parts altogether)
  // but here we never do that.
  if ( !caption.isEmpty() && m_currentView )
  {
    kdDebug(1202) << "KonqMainWindow::setCaption(" << caption << ")" << endl;

    // Keep an unmodified copy of the caption (before kapp->makeStdCaption is applied)
    m_currentView->setCaption( caption );
    KParts::MainWindow::setCaption( m_currentView->caption() );
  }
}

void KonqMainWindow::show()
{
  // We need to check if our toolbars are shown/hidden here, and set
  // our menu items accordingly. We can't do it in the constructor because
  // view profiles store toolbar info, and that info is read after
  // construct time.
  m_paShowMenuBar->setChecked( !menuBar()->isHidden() );
  updateBookmarkBar(); // hide if empty

  // Call parent method
  KParts::MainWindow::show();
}

QString KonqExtendedBookmarkOwner::currentURL() const
{
   return m_pKonqMainWindow->currentURL();
}

QString KonqMainWindow::currentProfile() const
{
   return m_pViewManager->currentProfile();
}

QString KonqMainWindow::currentURL() const
{
  if ( !m_currentView )
    return QString::null;
  QString url = m_currentView->url().prettyURL();
  if ( m_currentView->part() && m_currentView->part()->inherits("KonqDirPart") )
  {
      QString nameFilter = static_cast<KonqDirPart *>(m_currentView->part())->nameFilter();
      if ( !nameFilter.isEmpty() )
      {
          if (url.right(1) != "/")
              url += '/';
          url += nameFilter;
      }
  }
  return url;
}

void KonqExtendedBookmarkOwner::slotFillBookmarksList( KExtendedBookmarkOwner::QStringPairList & list )
{
  KonqFrameBase *docContainer = m_pKonqMainWindow->viewManager()->docContainer();
  if (docContainer == 0L) return;
  if (docContainer->frameType() != "Tabs") return;

  KonqFrameTabs* tabContainer = static_cast<KonqFrameTabs*>(docContainer);

  QPtrList<KonqFrameBase> frameList = *tabContainer->childFrameList();
  QPtrListIterator<KonqFrameBase> it( frameList );

  for ( it.toFirst(); it != 0L; ++it )
  {
    if ( !it.current()->activeChildView() )
      continue;
    if( it.current()->activeChildView()->locationBarURL().isEmpty() )
      continue;
    list << qMakePair( it.current()->activeChildView()->caption(),
                       it.current()->activeChildView()->url().url() );
  }
}

QString KonqExtendedBookmarkOwner::currentTitle() const
{
   return m_pKonqMainWindow->currentTitle();
}

QString KonqMainWindow::currentTitle() const
{
  return m_currentView ? m_currentView->caption() : QString::null;
}

void KonqMainWindow::slotPopupMenu( const QPoint &_global, const KURL &url, const QString &_mimeType, mode_t _mode )
{
  slotPopupMenu( 0L, _global, url, _mimeType, _mode );
}

void KonqMainWindow::slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KURL &url, const QString &_mimeType, mode_t _mode )
{
  KFileItem item( url, _mimeType, _mode );
  KFileItemList items;
  items.append( &item );
  slotPopupMenu( client, _global, items, KParts::URLArgs(), KParts::BrowserExtension::DefaultPopupItems, false ); //BE CAREFUL WITH sender() !
}

void KonqMainWindow::slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KURL &url, const KParts::URLArgs &_args, KParts::BrowserExtension::PopupFlags f, mode_t _mode )
{
  KFileItem item( url, _args.serviceType, _mode );
  KFileItemList items;
  items.append( &item );
  slotPopupMenu( client, _global, items, _args, f, false ); //BE CAREFUL WITH sender() !
}

void KonqMainWindow::slotPopupMenu( const QPoint &_global, const KFileItemList &_items )
{
  slotPopupMenu( 0L, _global, _items );
}

void KonqMainWindow::slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KFileItemList &_items )
{
  slotPopupMenu( client, _global, _items, KParts::URLArgs(), KParts::BrowserExtension::DefaultPopupItems, true );
}

void KonqMainWindow::slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KFileItemList &_items, const KParts::URLArgs &_args, KParts::BrowserExtension::PopupFlags _flags )
{
  slotPopupMenu( client, _global, _items, _args, _flags, true );
}

void KonqMainWindow::slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KFileItemList &_items, const KParts::URLArgs &_args, KParts::BrowserExtension::PopupFlags itemFlags, bool showProperties )
{
  KonqView * m_oldView = m_currentView;

  KonqView * currentView = childView( static_cast<KParts::ReadOnlyPart *>( sender()->parent() ) );

  //kdDebug() << "KonqMainWindow::slotPopupMenu m_oldView=" << m_oldView << " new currentView=" << currentView << " passive:" << currentView->isPassiveMode() << endl;

  if ( (m_oldView != currentView) && currentView->isPassiveMode() )
  {
      // Make this view active only temporarily (because it's passive)
      m_currentView = currentView;

      if ( m_oldView && m_oldView->browserExtension() )
          disconnectExtension( m_oldView->browserExtension() );
      if ( m_currentView->browserExtension() )
          connectExtension( m_currentView->browserExtension() );
  }
  // Note that if m_oldView!=currentView and currentView isn't passive,
  // then the KParts mechanism has already noticed the click in it,
  // but KonqViewManager delays the GUI-rebuilding with a single-shot timer.
  // Right after the popup shows up, currentView _will_ be m_currentView.

  //kdDebug(1202) << "KonqMainWindow::slotPopupMenu( " << client << "...)" << " current view=" << m_currentView << " " << m_currentView->part()->className() << endl;

  // This action collection is used to pass actions to KonqPopupMenu.
  // It has to be a KActionCollection instead of a KActionPtrList because we need
  // the actionStatusText signal...
  KActionCollection popupMenuCollection( (QWidget*)0 );
  popupMenuCollection.insert( m_paBack );
  popupMenuCollection.insert( m_paForward );
  popupMenuCollection.insert( m_paUp );
  popupMenuCollection.insert( m_paReload );

  popupMenuCollection.insert( m_paFindFiles );

  popupMenuCollection.insert( m_paUndo );
  popupMenuCollection.insert( m_paCut );
  popupMenuCollection.insert( m_paCopy );
  popupMenuCollection.insert( m_paPaste );
  popupMenuCollection.insert( m_paTrash );
  popupMenuCollection.insert( m_paRename );
  if (m_paDelete)
      popupMenuCollection.insert( m_paDelete );

  // The pasteto action is used when clicking on a dir, to paste into it.
  KAction *actPaste = KStdAction::paste( this, SLOT( slotPopupPasteTo() ), &popupMenuCollection, "pasteto" );
  actPaste->setEnabled( m_paPaste->isEnabled() );
  popupMenuCollection.insert( actPaste );

  bool sReading = false;
  if ( _items.count() > 0 )
  {
    m_popupURL = _items.getFirst()->url();
    sReading = KProtocolInfo::supportsReading( m_popupURL );
    if (sReading)
      m_popupServiceType = _items.getFirst()->mimetype();
  }
  else
  {
    m_popupURL = KURL();
    m_popupServiceType = QString::null;
  }

  if ( (_items.count() == 1) && !m_popupServiceType.isEmpty() ) {
      QString currentServiceName = currentView->service()->desktopEntryName();

      // List of services for the "Preview In" submenu.
      m_popupEmbeddingServices = KTrader::self()->query(
          m_popupServiceType,
          "KParts/ReadOnlyPart",
          // Obey "HideFromMenus". It defaults to false so we want "absent or true"
          // (wow, testing for 'true' if absent doesn't work, so order matters)
          "(not exist [X-KDE-BrowserView-HideFromMenus] or not [X-KDE-BrowserView-HideFromMenus]) "
          "and DesktopEntryName != '"+currentServiceName+"' "
          // I had an old local dirtree.desktop without lib, no need for invalid entries
          "and exist [Library]",
          QString::null );
  }


  // Don't set the view URL for a toggle view.
  // (This is a bit of a hack for the directory tree....)
  // ## should use the new currentView->isHierarchicalView() instead?
  // Would this be correct for the konqlistview tree view?
  KURL viewURL = currentView->isToggleView() ? KURL() : currentView->url();

  bool openedForViewURL = false;
  //bool dirsSelected = false;
  bool devicesFile = false;

  if ( _items.count() == 1 )
  {
      KURL firstURL = _items.getFirst()->url();
      if ( !viewURL.isEmpty() )
      {
	  //firstURL.cleanPath();
          openedForViewURL = firstURL.equals( viewURL, true );
      }
      devicesFile = firstURL.protocol().find("device", 0, false) == 0;
      //dirsSelected = S_ISDIR( _items.getFirst()->mode() );
  }
    //check if current url is trash
  KURL url = viewURL;
  url.cleanPath();
  bool isIntoTrash = url.protocol() == "trash";
  bool doTabHandling = !openedForViewURL && !isIntoTrash && sReading;
  bool showEmbeddingServices = !isIntoTrash && !devicesFile && (itemFlags & KParts::BrowserExtension::ShowTextSelectionItems) == 0;
  PopupMenuGUIClient *konqyMenuClient = new PopupMenuGUIClient( this, m_popupEmbeddingServices,
                                                                showEmbeddingServices, doTabHandling );

  //kdDebug(1202) << "KonqMainWindow::slotPopupMenu " << viewURL.prettyURL() << endl;


  // Those actions go into the PopupMenuGUIClient, since that's the one defining them.
  KAction *actNewWindow = 0L, *actNewTab = 0L;
  if( doTabHandling )
  {
      actNewWindow = new KAction( i18n( "Open in New &Window" ), "window_new", 0, this, SLOT( slotPopupNewWindow() ), konqyMenuClient->actionCollection(), "newview" );
      actNewWindow->setStatusText( i18n( "Open the document in a new window" ) );
      actNewTab = new KAction( i18n( "Open in &New Tab" ), "tab_new", 0, this, SLOT( slotPopupNewTab() ), konqyMenuClient->actionCollection(), "openintab" );
      actNewTab->setStatusText( i18n( "Open the document in a new tab" ) );
      doTabHandling = true;
  }

  if (currentView->isHierarchicalView())
    itemFlags |= KParts::BrowserExtension::ShowCreateDirectory;

  KonqPopupMenu::KonqPopupFlags kpf = 0;
  if ( showProperties )
      kpf |= KonqPopupMenu::ShowProperties;
  else
      kpf |= KonqPopupMenu::IsLink; // HACK

  QGuardedPtr<KonqPopupMenu> pPopupMenu = new KonqPopupMenu(
      KonqBookmarkManager::self(), _items,
      viewURL,
      popupMenuCollection,
      m_pMenuNew,
      // This parent ensures that if the part destroys itself (e.g. KHTML redirection),
      // it will close the popupmenu
      currentView->part()->widget(),
      kpf,
      itemFlags );

  if ( openedForViewURL && !viewURL.isLocalFile() )
      pPopupMenu->setURLTitle( currentView->caption() );

  // We will need these if we call the newTab slot
  popupItems = _items;
  popupUrlArgs = _args;
  popupUrlArgs.serviceType = QString::null; // Reset so that Open in New Window/Tab does mimetype detection

  connectActionCollection( pPopupMenu->actionCollection() );

  pPopupMenu->factory()->addClient( konqyMenuClient );

  if ( client )
    pPopupMenu->factory()->addClient( client );

  QObject::disconnect( m_pMenuNew->popupMenu(), SIGNAL(aboutToShow()),
                       this, SLOT(slotFileNewAboutToShow()) );

  pPopupMenu->exec( _global );

  QObject::connect( m_pMenuNew->popupMenu(), SIGNAL(aboutToShow()),
                       this, SLOT(slotFileNewAboutToShow()) );

  delete pPopupMenu;

  delete konqyMenuClient;
  m_popupEmbeddingServices.clear();
  popupItems.clear();

  // Deleted by konqyMenuClient's actioncollection
  //delete actNewTab;
  //delete actNewWindow;

  delete actPaste;

  // We're sort of misusing KActionCollection here, but we need it for the actionStatusText signal...
  // Anyway. If the action belonged to the view, and the view got deleted, we don't want ~KActionCollection
  // to iterate over those deleted actions
  KActionPtrList lst = popupMenuCollection.actions();
  KActionPtrList::iterator it = lst.begin();
  for ( ; it != lst.end() ; ++it )
      popupMenuCollection.take( *it );

  //kdDebug(1202) << "-------- KonqMainWindow::slotPopupMenu() - m_oldView = " << m_oldView << ", currentView = " << currentView
  //<< ", m_currentView = " << m_currentView << endl;

  // Restore current view if current is passive
  if ( (m_oldView != currentView) && (currentView == m_currentView) && currentView->isPassiveMode() )
  {
    //kdDebug() << "KonqMainWindow::slotPopupMenu restoring active view " << m_oldView << endl;
    if ( m_currentView->browserExtension() )
      disconnectExtension( m_currentView->browserExtension() );
    if ( m_oldView )
    {
        if ( m_oldView->browserExtension() )
        {
            connectExtension( m_oldView->browserExtension() );
            m_currentView = m_oldView;
        }
        m_oldView->part()->widget()->setFocus();
    }
  }
}

void KonqMainWindow::slotOpenEmbedded()
{
  QCString name = sender()->name();

  m_popupService = m_popupEmbeddingServices[ name.toInt() ]->desktopEntryName();

  m_popupEmbeddingServices.clear();

  QTimer::singleShot( 0, this, SLOT( slotOpenEmbeddedDoIt() ) );
}

void KonqMainWindow::slotOpenEmbeddedDoIt()
{
  m_currentView->stop();
  m_currentView->setLocationBarURL(m_popupURL);
  m_currentView->setTypedURL(QString::null);
  if ( m_currentView->changeViewMode( m_popupServiceType,
                                      m_popupService ) )
      m_currentView->openURL( m_popupURL, m_popupURL.prettyURL(0, KURL::StripFileProtocol) );
}

void KonqMainWindow::slotDatabaseChanged()
{
  if ( KSycoca::isChanged("mimetypes") )
  {
    MapViews::ConstIterator it = m_mapViews.begin();
    MapViews::ConstIterator end = m_mapViews.end();
    for (; it != end; ++it )
      (*it)->callExtensionMethod( "refreshMimeTypes()" );
  }
}

void KonqMainWindow::slotPopupPasteTo()
{
    if ( !m_currentView || m_popupURL.isEmpty() )
        return;
    m_currentView->callExtensionURLMethod( "pasteTo(const KURL&)", m_popupURL );
}

void KonqMainWindow::slotReconfigure()
{
  reparseConfiguration();
}

void KonqMainWindow::reparseConfiguration()
{
  kdDebug(1202) << "KonqMainWindow::reparseConfiguration() !" << endl;

  KConfig *config = KGlobal::config();
  KConfigGroupSaver cgs(config,"MainView Settings");
  m_bBackRightClick = config->readBoolEntry( "BackRightClick", false );
  m_bSaveViewPropertiesLocally = config->readBoolEntry( "SaveViewPropertiesLocally", false );
  m_bHTMLAllowed = config->readBoolEntry( "HTMLAllowed", false );
  m_sViewModeForDirectory = config->readEntry( "ViewMode" );

  // Update display of "Delete" command if necessary
  config->setGroup( "FMSettings" );  // cgs' destructor will reset
  if ( config->readBoolEntry( "ShowDeleteCommand", true ) )
  {
    if ( !m_paDelete )
    {
      unplugActionList( "delactions" );
      m_paDelete = new KAction( i18n( "&Delete" ), "editdelete", SHIFT+Key_Delete, actionCollection(), "del" );
      m_paDelete->setEnabled( actionCollection()->action("trash")->isEnabled() );
      m_deleteActions.clear();
      m_deleteActions.append( m_paDelete );
      plugActionList( "delactions", m_deleteActions );
    }
  }
  else
  {
    unplugActionList( "delactions" );
    m_deleteActions.clear();  // deletes m_paDelete
    m_paDelete = 0;
  }

  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
      (*it)->reparseConfiguration();
}

void KonqMainWindow::saveProperties( KConfig *config )
{
  m_pViewManager->saveViewProfile( *config, true /* save URLs */, false );
}

void KonqMainWindow::readProperties( KConfig *config )
{
  kdDebug(1202) << "KonqMainWindow::readProperties( KConfig *config )" << endl;
  m_pViewManager->loadViewProfile( *config, QString::null /*no profile name*/ );
}

void KonqMainWindow::setInitialFrameName( const QString &name )
{
  m_initialFrameName = name;
}

void KonqMainWindow::slotActionStatusText( const QString &text )
{
  if ( !m_currentView )
    return;

  KonqFrameStatusBar *statusBar = m_currentView->frame()->statusbar();

  if ( !statusBar )
    return;

  statusBar->message( text );
}

void KonqMainWindow::slotClearStatusText()
{
  if ( !m_currentView )
    return;

  KonqFrameStatusBar *statusBar = m_currentView->frame()->statusbar();

  if ( !statusBar )
    return;

  statusBar->slotClear();
}

void KonqMainWindow::updateOpenWithActions()
{
  unplugActionList( "openwith" );

  m_openWithActions.clear();

  if (!kapp->authorizeKAction("openwith"))
     return;

  const KTrader::OfferList & services = m_currentView->appServiceOffers();
  KTrader::OfferList::ConstIterator it = services.begin();
  KTrader::OfferList::ConstIterator end = services.end();
  for (; it != end; ++it )
  {
    KAction *action = new KAction( i18n( "Open with %1" ).arg( (*it)->name() ), 0, 0, (*it)->desktopEntryName().latin1() );
    action->setIcon( (*it)->icon() );

    connect( action, SIGNAL( activated() ),
             this, SLOT( slotOpenWith() ) );

    m_openWithActions.append( action );
  }
  if ( services.count() > 0 )
  {
      m_openWithActions.append( new KActionSeparator );
      plugActionList( "openwith", m_openWithActions );
  }
}

void KonqMainWindow::updateViewModeActions()
{
  unplugViewModeActions();
  if ( m_viewModeMenu )
  {
    QPtrListIterator<KRadioAction> it( m_viewModeActions );
    for (; it.current(); ++it )
      it.current()->unplugAll();
    delete m_viewModeMenu;
  }

  m_viewModeMenu = 0;
  m_toolBarViewModeActions.clear();
  m_viewModeActions.clear();

  // if we changed the viewmode to something new, then we have to
  // make sure to also clear our [libiconview,liblistview]->service-for-viewmode
  // map
  if ( m_viewModeToolBarServices.count() > 0 &&
       !m_viewModeToolBarServices.begin().data()->serviceTypes().contains( m_currentView->serviceType() ) )
  {
      // Save the current map to the config file, for later reuse
      saveToolBarServicesMap();

      m_viewModeToolBarServices.clear();
  }

  KTrader::OfferList services = m_currentView->partServiceOffers();

  if ( services.count() <= 1 )
    return;

  m_viewModeMenu = new KActionMenu( i18n( "&View Mode" ), this );

  // a temporary map, just like the m_viewModeToolBarServices map, but
  // mapping to a KonqViewModeAction object. It's just temporary as we
  // of use it to group the viewmode actions (iconview,multicolumnview,
  // treeview, etc.) into to two groups -> icon/list
  // Although I wrote this now only of icon/listview it has to work for
  // any view, that's why it's so general :)
  QMap<QString,KonqViewModeAction*> groupedServiceMap;

  // Another temporary map, the preferred service for each library (2 entries in our example)
  QMap<QString,QString> preferredServiceMap;

  KConfig * config = KGlobal::config();
  config->setGroup( "ModeToolBarServices" );

  KTrader::OfferList::ConstIterator it = services.begin();
  KTrader::OfferList::ConstIterator end = services.end();
  for (; it != end; ++it )
  {
      QVariant prop = (*it)->property( "X-KDE-BrowserView-Toggable" );
      if ( prop.isValid() && prop.toBool() ) // No toggable views in view mode
          continue;

      KRadioAction *action;

      QString itname = (*it)->genericName();
      if (itname.isEmpty())
          itname = (*it)->name();

      QString icon = (*it)->icon();
      if ( icon != QString::fromLatin1( "unknown" ) )
          // we *have* to specify a parent qobject, otherwise the exclusive group stuff doesn't work!(Simon)
          action = new KRadioAction( itname, icon, 0, this, (*it)->desktopEntryName().ascii() );
      else
          action = new KRadioAction( itname, 0, this, (*it)->desktopEntryName().ascii() );

      action->setExclusiveGroup( "KonqMainWindow_ViewModes" );

      connect( action, SIGNAL( toggled( bool ) ),
               this, SLOT( slotViewModeToggle( bool ) ) );

      m_viewModeActions.append( action );
      action->plug( m_viewModeMenu->popupMenu() );

      QString library = (*it)->library();
      // Group all non-builtin views together
      QVariant builtIntoProp = (*it)->property( "X-KDE-BrowserView-Built-Into" );
      if ( !builtIntoProp.isValid() || builtIntoProp.toString() != "konqueror" )
          library = "external";

      // look if we already have a KonqViewModeAction (in the toolbar)
      // for this component
      QMap<QString,KonqViewModeAction*>::Iterator mapIt = groupedServiceMap.find( library );

      // if we don't have -> create one
      if ( mapIt == groupedServiceMap.end() )
      {
          // default service on this action: the current one (i.e. the first one)
          QString text = itname;
          QString icon = (*it)->icon();
          QCString name = (*it)->desktopEntryName().latin1();
          //kdDebug(1202) << " Creating action for " << library << ". Default service " << itname << endl;

          // if we previously changed the viewmode (see slotViewModeToggle!)
          // then we will want to use the previously used settings (previous as
          // in before the actions got deleted)
          QMap<QString,KService::Ptr>::ConstIterator serviceIt = m_viewModeToolBarServices.find( library );
          if ( serviceIt != m_viewModeToolBarServices.end() )
          {
              //kdDebug(1202) << " Setting action for " << library << " to " << (*serviceIt)->name() << endl;
              text = (*serviceIt)->genericName();
              if (text.isEmpty())
                  text = (*serviceIt)->name();
              icon = (*serviceIt)->icon();
              name = (*serviceIt)->desktopEntryName().ascii();
          } else
          {
              // if we don't have it in the map, we should look for a setting
              // for this library in the config file.
              QString preferredService = config->readEntry( library );
              if ( !preferredService.isEmpty() && name != preferredService.latin1() )
              {
                  //kdDebug(1202) << " Inserting into preferredServiceMap(" << (*it)->library() << ") : " << preferredService << endl;
                  // The preferred service isn't the current one, so remember to set it later
                  preferredServiceMap[ library ] = preferredService;
              }
          }

          KonqViewModeAction *tbAction = new KonqViewModeAction( text,
                                                                 icon,
                                                                 this,
                                                                 name );

          tbAction->setExclusiveGroup( "KonqMainWindow_ToolBarViewModes" );

          tbAction->setChecked( action->isChecked() );

          connect( tbAction, SIGNAL( toggled( bool ) ),
                   this, SLOT( slotViewModeToggle( bool ) ) );

          m_toolBarViewModeActions.append( tbAction );

          mapIt = groupedServiceMap.insert( library, tbAction );
      }

      // Check the actions (toolbar button and menu item) if they correspond to the current view
      bool bIsCurrentView = (*it)->desktopEntryName() == m_currentView->service()->desktopEntryName();
      if ( bIsCurrentView )
      {
          (*mapIt)->setChecked( true );
          action->setChecked( true );
      }

      // Set the contents of the button from the current service, either if it's the current view
      // or if it's our preferred service for this button (library)
      if ( bIsCurrentView
           || ( preferredServiceMap.contains( library ) && (*it)->desktopEntryName() == preferredServiceMap[ library ] ) )
      {
          //kdDebug(1202) << " Changing action for " << library << " into service " << (*it)->name() << endl;

          QString mapitname = (*it)->genericName();
          if (mapitname.isEmpty())
              mapitname = (*it)->name();
          (*mapIt)->setText( mapitname );
          (*mapIt)->setIcon( (*it)->icon() );
          (*mapIt)->setName( (*it)->desktopEntryName().ascii() ); // tricky...
          preferredServiceMap.remove( library ); // The current view has priority over the saved settings
      }

      // plug action also into the delayed popupmenu of appropriate toolbar action
      action->plug( (*mapIt)->popupMenu() );
  }

#ifndef NDEBUG
  Q_ASSERT( preferredServiceMap.isEmpty() );
  QMap<QString,QString>::Iterator debugIt = preferredServiceMap.begin();
  QMap<QString,QString>::Iterator debugEnd = preferredServiceMap.end();
  for ( ; debugIt != debugEnd ; ++debugIt )
      kdDebug(1202) << " STILL IN preferredServiceMap : " << debugIt.key() << " | " << debugIt.data() << endl;
#endif

  if ( !m_currentView->isToggleView() ) // No view mode for toggable views
      // (The other way would be to enforce a better servicetype for them, than Browser/View)
      if ( /* already tested: services.count() > 1 && */ m_viewModeMenu )
          plugViewModeActions();
}

void KonqMainWindow::saveToolBarServicesMap()
{
    QMap<QString,KService::Ptr>::ConstIterator serviceIt = m_viewModeToolBarServices.begin();
    QMap<QString,KService::Ptr>::ConstIterator serviceEnd = m_viewModeToolBarServices.end();
    KConfig * config = KGlobal::config();
    config->setGroup( "ModeToolBarServices" );
    for ( ; serviceIt != serviceEnd ; ++serviceIt )
        config->writeEntry( serviceIt.key(), serviceIt.data()->desktopEntryName() );
    config->sync();
}

void KonqMainWindow::plugViewModeActions()
{
  QPtrList<KAction> lst;
  lst.append( m_viewModeMenu );
  plugActionList( "viewmode", lst );
  // display the toolbar viewmode icons only for inode/directory, as here we have dedicated icons
  if ( m_currentView && m_currentView->serviceType() == "inode/directory" )
    plugActionList( "viewmode_toolbar", m_toolBarViewModeActions );
}

void KonqMainWindow::unplugViewModeActions()
{
  unplugActionList( "viewmode" );
  unplugActionList( "viewmode_toolbar" );
}

KonqMainWindowIface* KonqMainWindow::dcopObject()
{
  return m_dcopObject;
}

void KonqMainWindow::updateBookmarkBar()
{
  KToolBar * bar = static_cast<KToolBar *>( child( "bookmarkToolBar", "KToolBar" ) );

  if (!bar) return;

  // hide if empty
  if (m_paBookmarkBar && bar->count() == 0 )
        bar->hide();

}

void KonqMainWindow::closeEvent( QCloseEvent *e )
{
  kdDebug(1202) << "KonqMainWindow::closeEvent begin" << endl;
  // This breaks session management (the window is withdrawn in kwin)
  // so let's do this only when closed by the user.
  if ( static_cast<KonquerorApplication *>(kapp)->closedByUser() )
  {
    if ( viewManager()->docContainer() && viewManager()->docContainer()->frameType()=="Tabs" )
    {
      KonqFrameTabs* tabContainer = static_cast<KonqFrameTabs*>(viewManager()->docContainer());
      if ( tabContainer->count() > 1 )
      {
        KConfig *config = KGlobal::config();
        KConfigGroupSaver cs( config, QString::fromLatin1("Notification Messages") );

        if ( !config->hasKey( "MultipleTabConfirm" ) )
        {
          switch (
              KMessageBox::warningYesNoCancel(
                  this,
                  i18n("You have multiple tabs open in this window, "
                        "are you sure you want to quit?"),
                  i18n("Confirmation"),
                  KStdGuiItem::quit(),
                  KGuiItem(i18n( "C&lose Current Tab" ), "tab_remove"),
                  "MultipleTabConfirm"
              )
            ) {
            case KMessageBox::Yes :
            break;
            case KMessageBox::No :
            {
              e->ignore();
              slotRemoveTab();
              return;
            }
            break;
            case KMessageBox::Cancel :
            {
              e->ignore();
    	      return;
            }
          }
        }
      }

      KonqView *originalView = m_currentView;
      MapViews::ConstIterator it = m_mapViews.begin();
      MapViews::ConstIterator end = m_mapViews.end();
      for (; it != end; ++it ) {
        KonqView *view = it.data();
        if (view && view->part()) {
          QVariant prop = view->part()->property("modified");
          if (prop.isValid() && prop.toBool()) {
            m_pViewManager->showTab( view );
            if ( KMessageBox::warningContinueCancel( this,
              i18n("This tab contains changes that have not been submitted.\nClosing the window will discard these changes."),
              i18n("Discard Changes?"), i18n("&Discard Changes"), "discardchangesclose") != KMessageBox::Continue )
            {
              e->ignore();
              m_pViewManager->showTab( originalView );
              return;
            }
          }
        }
      }
//      m_pViewManager->showTab( originalView );
    }
    else if ( m_currentView && m_currentView->part() )
    {
      QVariant prop = m_currentView->part()->property("modified");
      if (prop.isValid() && prop.toBool())
         if ( KMessageBox::warningContinueCancel( this,
           i18n("This page contains changes that have not been submitted.\nClosing the window will discard these changes."),
	   i18n("Discard Changes?"), i18n("&Discard Changes"), "discardchangesclose") != KMessageBox::Continue )
         {
           e->ignore();
           return;
         }
    }

    hide();
    qApp->flushX();
  }
  // We're going to close - tell the parts
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
  {
      if ( (*it)->part() && (*it)->part()->widget() )
          QApplication::sendEvent( (*it)->part()->widget(), e );
  }
  KParts::MainWindow::closeEvent( e );
  if( isPreloaded() && !kapp->sessionSaving())
  { // queryExit() refused closing, hide instead
      hide();
  }
  kdDebug(1202) << "KonqMainWindow::closeEvent end" << endl;
}

bool KonqMainWindow::queryExit()
{
    if( kapp->sessionSaving()) // *sigh*
        return true;
    return !stayPreloaded();
}

void KonqMainWindow::setIcon( const QPixmap& pix )
{
  KParts::MainWindow::setIcon( pix );

  QPixmap big = pix;

  QString url = m_combo->currentText();

  if ( !url.isEmpty() )
    big = KonqPixmapProvider::self()->pixmapFor( url, KIcon::SizeMedium );

  KWin::setIcons( winId(), big, pix );
}

void KonqMainWindow::slotIntro()
{
  openURL( 0L, KURL("about:konqueror") );
}

void KonqMainWindow::goURL()
{
  QLineEdit *lineEdit = m_combo->lineEdit();
  if ( !lineEdit )
    return;

  QKeyEvent event( QEvent::KeyPress, Key_Return, '\n', 0 );
  QApplication::sendEvent( lineEdit, &event );
}

void KonqMainWindow::slotLocationLabelActivated()
{
  m_combo->setFocus();
  m_combo->lineEdit()->selectAll();
}

void KonqMainWindow::slotOpenURL( const KURL& url )
{
    openURL( 0L, url );
}

bool KonqMainWindow::sidebarVisible() const
{
KAction *a = m_toggleViewGUIClient->action("konq_sidebartng");
return (a && static_cast<KToggleAction*>(a)->isChecked());
}

void KonqMainWindow::slotAddWebSideBar(const KURL& url, const QString& name)
{
    if (url.url().isEmpty() && name.isEmpty())
        return;

    kdDebug(1202) << "Requested to add URL " << url << " [" << name << "] to the sidebar!" << endl;

    KAction *a = m_toggleViewGUIClient->action("konq_sidebartng");
    if (!a) {
        KMessageBox::sorry(0L, i18n("Your sidebar is not functional or unavailable. A new entry cannot be added."), i18n("Web Sidebar"));
        return;
    }

    int rc = KMessageBox::questionYesNo(0L,
              i18n("Add new web extension \"%1\" to your sidebar?")
                                .arg(name.isEmpty() ? name : url.prettyURL()),
              i18n("Web Sidebar"));

    if (rc == KMessageBox::Yes) {
        // Show the sidebar
        if (!static_cast<KToggleAction*>(a)->isChecked()) {
            a->activate();
        }

        // Tell it to add a new panel
        MapViews::ConstIterator it;
        for (it = viewMap().begin(); it != viewMap().end(); ++it) {
            KonqView *view = it.data();
            if (view) {
                KService::Ptr svc = view->service();
                if (svc->desktopEntryName() == "konq_sidebartng") {
                    emit view->browserExtension()->addWebSideBar(url, name);
                    break;
                }
            }
        }
    }
}

void KonqMainWindow::bookmarksIntoCompletion( const KBookmarkGroup& group )
{
    static const QString& http = KGlobal::staticQString( "http" );
    static const QString& ftp = KGlobal::staticQString( "ftp" );

    if ( group.isNull() )
        return;

    for ( KBookmark bm = group.first();
          !bm.isNull(); bm = group.next(bm) ) {
        if ( bm.isGroup() ) {
            bookmarksIntoCompletion( bm.toGroup() );
            continue;
        }

        KURL url = bm.url();
        if ( !url.isValid() )
            continue;

        QString u = url.prettyURL();
        s_pCompletion->addItem( u );

        if ( url.isLocalFile() )
            s_pCompletion->addItem( url.path() );
        else if ( url.protocol() == http )
            s_pCompletion->addItem( u.mid( 7 ));
        else if ( url.protocol() == ftp &&
                  url.host().startsWith( ftp ) )
            s_pCompletion->addItem( u.mid( 6 ) );
    }
}

void KonqMainWindow::connectActionCollection( KActionCollection *coll )
{
    connect( coll, SIGNAL( actionStatusText( const QString & ) ),
             this, SLOT( slotActionStatusText( const QString & ) ) );
    connect( coll, SIGNAL( clearStatusText() ),
             this, SLOT( slotClearStatusText() ) );
}

void KonqMainWindow::disconnectActionCollection( KActionCollection *coll )
{
    disconnect( coll, SIGNAL( actionStatusText( const QString & ) ),
                this, SLOT( slotActionStatusText( const QString & ) ) );
    disconnect( coll, SIGNAL( clearStatusText() ),
                this, SLOT( slotClearStatusText() ) );
}

//
// the smart popup completion code , <l.lunak@kde.org>
//

// prepend http://www. or http:// if there's no protocol in 's'
// used only when there are no completion matches
static QString hp_tryPrepend( const QString& s )
{
    if( s.isEmpty() || s[ 0 ] == '/' )
        return QString::null;
    for( unsigned int pos = 0;
         pos < s.length() - 2; // 4 = ://x
         ++pos )
        {
        if( s[ pos ] == ':' && s[ pos + 1 ] == '/' && s[ pos + 2 ] == '/' )
            return QString::null;
        if( !s[ pos ].isLetter() )
            break;
        }
    return ( s.startsWith( "www." ) ? "http://" : "http://www." ) + s;
}


static void hp_removeDupe( KCompletionMatches& l, const QString& dupe,
    KCompletionMatches::Iterator it_orig )
{
    for( KCompletionMatches::Iterator it = l.begin();
         it != l.end();
         ) {
        if( it == it_orig ) {
            ++it;
            continue;
        }
        if( (*it).value() == dupe ) {
            (*it_orig).first = kMax( (*it_orig).first, (*it).index());
            it = l.remove( it );
            continue;
        }
        ++it;
    }
}

// remove duplicates like 'http://www.kde.org' and 'http://www.kde.org/'
// (i.e. the trailing slash)
// some duplicates are also created by prepending protocols
static void hp_removeDuplicates( KCompletionMatches& l )
{
    QString http = "http://";
    QString ftp = "ftp://ftp.";
    QString file = "file:";
    QString file2 = "file://";
    l.removeDuplicates();
    for( KCompletionMatches::Iterator it = l.begin();
         it != l.end();
         ++it ) {
        QString str = (*it).value();
        if( str.startsWith( http )) {
            if( str.find( '/', 7 ) < 0 ) { // http://something<noslash>
                hp_removeDupe( l, str + '/', it );
                hp_removeDupe( l, str.mid( 7 ) + '/', it );
            }
            hp_removeDupe( l, str.mid( 7 ), it );
        }
        else if( str.startsWith( ftp )) // ftp://ftp.
            hp_removeDupe( l, str.mid( 6 ), it ); // remove dupes without ftp://
        else if( str.startsWith( file2 ))
            hp_removeDupe( l, str.mid( 7 ), it ); // remove dupes without file://
        else if( str.startsWith( file ))
            hp_removeDupe( l, str.mid( 5 ), it ); // remove dupes without file:
    }
}

static void hp_removeCommonPrefix( KCompletionMatches& l, const QString& prefix )
{
    for( KCompletionMatches::Iterator it = l.begin();
         it != l.end();
         ) {
        if( (*it).value().startsWith( prefix )) {
            it = l.remove( it );
            continue;
        }
        ++it;
    }
}

// don't include common prefixes like 'http://', i.e. when s == 'h', include
// http://hotmail.com but don't include everything just starting with 'http://'
static void hp_checkCommonPrefixes( KCompletionMatches& matches, const QString& s )
{
    static const char* const prefixes[] = {
        "http://",
        "https://",
        "www.",
        "ftp://",
        "http://www.",
        "https://www.",
        "ftp://ftp.",
        "file:",
        "file://",
        NULL };
    for( const char* const *pos = prefixes;
         *pos != NULL;
         ++pos ) {
        QString prefix = *pos;
        if( prefix.startsWith( s )) {
            hp_removeCommonPrefix( matches, prefix );
        }
    }
}

QStringList KonqMainWindow::historyPopupCompletionItems( const QString& s)
{
    const QString http = "http://";
    const QString https = "https://";
    const QString www = "http://www.";
    const QString wwws = "https://www.";
    const QString ftp = "ftp://";
    const QString ftpftp = "ftp://ftp.";
    const QString file = "file:"; // without /, because people enter /usr etc.
    const QString file2 = "file://";
    if( s.isEmpty())
	    return QStringList();
    KCompletionMatches matches= s_pCompletion->allWeightedMatches( s );
    hp_checkCommonPrefixes( matches, s );
    bool checkDuplicates = false;
    if ( !s.startsWith( ftp ) ) {
        matches += s_pCompletion->allWeightedMatches( ftp + s );
	if( QString( "ftp." ).startsWith( s ))
            hp_removeCommonPrefix( matches, ftpftp );
        checkDuplicates = true;
    }
    if ( !s.startsWith( https ) ) {
        matches += s_pCompletion->allWeightedMatches( https + s );
	if( QString( "www." ).startsWith( s ))
            hp_removeCommonPrefix( matches, wwws );
        checkDuplicates = true;
    }
    if ( !s.startsWith( http )) {
        matches += s_pCompletion->allWeightedMatches( http + s );
	if( QString( "www." ).startsWith( s ))
            hp_removeCommonPrefix( matches, www );
        checkDuplicates = true;
    }
    if ( !s.startsWith( www ) ) {
        matches += s_pCompletion->allWeightedMatches( www + s );
        checkDuplicates = true;
    }
    if ( !s.startsWith( wwws ) ) {
        matches += s_pCompletion->allWeightedMatches( wwws + s );
        checkDuplicates = true;
    }
    if ( !s.startsWith( ftpftp ) ) {
        matches += s_pCompletion->allWeightedMatches( ftpftp + s );
        checkDuplicates = true;
    }
    if ( !s.startsWith( file ) ) {
        matches += s_pCompletion->allWeightedMatches( file + s );
        checkDuplicates = true;
    }
    if ( !s.startsWith( file2 ) ) {
        matches += s_pCompletion->allWeightedMatches( file2 + s );
        checkDuplicates = true;
    }
    if( checkDuplicates )
        hp_removeDuplicates( matches );
    QStringList items = matches.list();
    if( items.count() == 0
	&& !s.contains( ':' ) && s[ 0 ] != '/' )
        {
        QString pre = hp_tryPrepend( s );
        if( !pre.isNull())
            items += pre;
        }
    return items;
}

#ifndef NDEBUG
void KonqMainWindow::dumpViewList()
{
  MapViews::Iterator end = m_mapViews.end();

  kdDebug(1202) << m_mapViews.count() << "Views" << endl;

  for (MapViews::Iterator it = m_mapViews.begin(); it != end; it++)
  {
    kdDebug(1202) << it.data() << endl;
  }
}
#endif

// KonqFrameContainerBase implementation BEGIN

/**
 * Call this after inserting a new frame into the splitter.
 */
void KonqMainWindow::insertChildFrame( KonqFrameBase * frame, int /*index*/ )
{
  m_pChildFrame = frame;
  m_pActiveChild = frame;
  frame->setParentContainer(this);
  setCentralWidget( frame->widget() );
}

/**
 * Call this before deleting one of our children.
 */
void KonqMainWindow::removeChildFrame( KonqFrameBase * /*frame*/ )
{
  m_pChildFrame = 0L;
  m_pActiveChild = 0L;
}

void KonqMainWindow::saveConfig( KConfig* config, const QString &prefix, bool saveURLs, KonqFrameBase* docContainer, int id, int depth ) { if( m_pChildFrame ) m_pChildFrame->saveConfig( config, prefix, saveURLs, docContainer, id, depth); }

void KonqMainWindow::copyHistory( KonqFrameBase *other ) { if( m_pChildFrame ) m_pChildFrame->copyHistory( other ); }

void KonqMainWindow::printFrameInfo( const QString &spaces ) { if( m_pChildFrame ) m_pChildFrame->printFrameInfo( spaces ); }

void KonqMainWindow::reparentFrame( QWidget* /*parent*/,
                                    const QPoint & /*p*/, bool /*showIt*/ ) { return; }

KonqFrameContainerBase* KonqMainWindow::parentContainer()const { return 0L; }
void KonqMainWindow::setParentContainer(KonqFrameContainerBase* /*parent*/) { return; }

void KonqMainWindow::setTitle( const QString &/*title*/ , QWidget* /*sender*/) { return; }
void KonqMainWindow::setTabIcon( const QString &/*url*/, QWidget* /*sender*/ ) { return; }

QWidget* KonqMainWindow::widget() { return this; }

void KonqMainWindow::listViews( ChildViewList *viewList ) { if( m_pChildFrame ) m_pChildFrame->listViews( viewList ); }

QCString KonqMainWindow::frameType() { return QCString("MainWindow"); }

KonqFrameBase* KonqMainWindow::childFrame()const { return m_pChildFrame; }

void KonqMainWindow::setActiveChild( KonqFrameBase* /*activeChild*/ ) { return; }

bool KonqMainWindow::isMimeTypeAssociatedWithSelf( const QString &mimeType )
{
    return isMimeTypeAssociatedWithSelf( mimeType, KServiceTypeProfile::preferredService( mimeType, "Application" ) );
}

bool KonqMainWindow::isMimeTypeAssociatedWithSelf( const QString &/*mimeType*/, const KService::Ptr &offer )
{
    // Prevention against user stupidity : if the associated app for this mimetype
    // is konqueror/kfmclient, then we'll loop forever. So we have to
    // 1) force embedding first, if that works we're ok
    // 2) check what KRun is going to do before calling it.
    return ( offer && ( offer->desktopEntryName() == "konqueror" ||
             offer->exec().stripWhiteSpace().startsWith( "kfmclient" ) ) );
}

// KonqFrameContainerBase implementation END

void KonqMainWindow::setPreloadedFlag( bool preloaded )
{
    if( s_preloaded == preloaded )
        return;
    s_preloaded = preloaded;
    if( s_preloaded )
    {
        kapp->disableSessionManagement(); // dont restore preloaded konqy's
        return; // was registered before calling this
    }
    delete s_preloadedWindow; // preloaded state was abandoned without reusing the window
    s_preloadedWindow = NULL;
    kapp->enableSessionManagement(); // enable SM again
    DCOPRef ref( "kded", "konqy_preloader" );
    ref.send( "unregisterPreloadedKonqy", kapp->dcopClient()->appId());
}

void KonqMainWindow::setPreloadedWindow( KonqMainWindow* window )
{
    s_preloadedWindow = window;
    if( window == NULL )
        return;
    window->viewManager()->clear();
    KIO::Scheduler::unregisterWindow( window );
}

// used by preloading - this KonqMainWindow will be reused, reset everything
// that won't be reset by loading a profile
void KonqMainWindow::resetWindow()
{
    char data[ 1 ];
    // empty append to get current X timestamp
    QWidget tmp_widget;
    XChangeProperty( qt_xdisplay(), tmp_widget.winId(), XA_WM_CLASS, XA_STRING, 8,
		    PropModeAppend, (unsigned char*) &data, 0 );
    XEvent ev;
    XWindowEvent( qt_xdisplay(), tmp_widget.winId(), PropertyChangeMask, &ev );
    long x_time = ev.xproperty.time;
    // bad hack - without updating the _KDE_NET_WM_USER_CREATION_TIME property,
    // KWin will apply don't_steal_focus to this window, and will not make it active
    // (shows mainly with 'konqueror --preload')
    static Atom atom = XInternAtom( qt_xdisplay(), "_KDE_NET_WM_USER_CREATION_TIME", False );
    XChangeProperty( qt_xdisplay(), winId(), atom, XA_CARDINAL, 32,
		     PropModeReplace, (unsigned char *) &x_time, 1);
    extern Time qt_x_user_time;   // reset also user time, so that this window
    qt_x_user_time = CurrentTime; // won't have _NET_WM_USER_TIME set
#if !KDE_IS_VERSION( 3, 2, 90 ) // _KDE_NET_USER_TIME is obsolete
    static Atom atom2 = XInternAtom( qt_xdisplay(), "_KDE_NET_USER_TIME", False );
    timeval tv;
    gettimeofday( &tv, NULL );
    unsigned long now = tv.tv_sec * 10 + tv.tv_usec / 100000;
    XChangeProperty(qt_xdisplay(), winId(), atom2, XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *)&now, 1);
#endif
    static Atom atom3 = XInternAtom( qt_xdisplay(), "_NET_WM_USER_TIME", False );
    XDeleteProperty( qt_xdisplay(), winId(), atom3 );
// Qt remembers the iconic state if the window was withdrawn while on another virtual desktop
    clearWState( WState_Minimized );
    ignoreInitialGeometry();
    kapp->setTopWidget( this ); // set again the default window icon
}

bool KonqMainWindow::event( QEvent* e )
{
    if( e->type() == QEvent::DeferredDelete )
    {
    // since the preloading code tries to reuse KonqMainWindow,
    // the last window shouldn't be really deleted, but only hidden
    // deleting WDestructiveClose windows is done using deleteLater(),
    // so catch QEvent::DefferedDelete and check if this window should stay
        if( stayPreloaded())
        {
            setWFlags(WDestructiveClose); // was reset before deleteLater()
            return true; // no deleting
        }
    }
    return KParts::MainWindow::event( e );
}

bool KonqMainWindow::stayPreloaded()
{
    // last window?
    if( mainWindowList()->count() > 1 )
        return false;
    // not running in full KDE environment?
    if( getenv( "KDE_FULL_SESSION" ) == NULL || getenv( "KDE_FULL_SESSION" )[ 0 ] == '\0' )
        return false;
    KConfigGroupSaver group( KGlobal::config(), "Reusing" );
    if( KGlobal::config()->readNumEntry( "MaxPreloadCount", 1 ) == 0 )
        return false;
    viewManager()->clear(); // reduce resource usage before checking it
    if( !checkPreloadResourceUsage())
        return false;
    DCOPRef ref( "kded", "konqy_preloader" );
    if( !ref.callExt( "registerPreloadedKonqy", DCOPRef::NoEventLoop, 5000,
        kapp->dcopClient()->appId(), qt_xscreen()))
    {
        return false;
    }
    KonqMainWindow::setPreloadedFlag( true );
    kdDebug(1202) << "Konqy kept for preloading :" << kapp->dcopClient()->appId() << endl;
    KonqMainWindow::setPreloadedWindow( this );
    return true;
}

// try to avoid staying running when leaking too much memory
// this is checked by using mallinfo() and comparing
// memory usage during konqy startup and now, if the difference
// is too large -> leaks -> quit
// also, if this process is running for too long, or has been
// already reused too many times -> quit, just in case
bool KonqMainWindow::checkPreloadResourceUsage()
{
    if(
#ifndef NDEBUG
        isatty( STDIN_FILENO ) ||
#endif
        isatty( STDOUT_FILENO ) || isatty( STDERR_FILENO ))
    {
        kdDebug(1202) << "Running from tty, not keeping for preloading" << endl;
        return false;
    }
    int usage = current_memory_usage();
    kdDebug(1202) << "Memory usage increase: " << ( usage - s_initialMemoryUsage )
        << " (" << usage << "/" << s_initialMemoryUsage << ")" << endl;
    int max_allowed_usage = s_initialMemoryUsage + 16 * 1024 * 1024;
    if( usage > max_allowed_usage ) // too much memory used?
    {
	kdDebug(1202) << "Not keeping for preloading due to high memory usage" << endl;
	return false;
    }
    // working memory usage test ( usage != 0 ) makes others less strict
    if( ++s_preloadUsageCount > ( usage != 0 ? 100 : 10 )) // reused too many times?
    {
	kdDebug(1202) << "Not keeping for preloading due to high usage count" << endl;
	return false;
    }
    if( time( NULL ) > s_startupTime + 60 * 60 * ( usage != 0 ? 4 : 1 )) // running for too long?
    {
	kdDebug(1202) << "Not keeping for preloading due to long usage time" << endl;
	return false;
    }
    return true;
}

static int current_memory_usage()
{
#ifdef __linux__
// Check whole memory usage - VmSize
    QFile f( QCString().sprintf( "/proc/%i/statm", getpid()));
    if( f.open( IO_ReadOnly ))
    {
        QString line;
        if( f.readLine( line, 1024 ) > 0 )
        {
            line = line.stripWhiteSpace();
            int usage = line.section( ' ', 0, 0 ).toInt();
            if( usage > 0 )
                return usage * 1024;
        }
    }
    kdWarning() << "Couldn't read VmSize from /proc/*/statm." << endl;
#endif

// Check malloc() usage - very imprecise, but better than nothing.
    int usage_sum = 0;
#if defined(KDE_MALLINFO_STDLIB) || defined(KDE_MALLINFO_MALLOC)
    // ugly hack for kdecore/malloc
    extern int kde_malloc_is_used;
    free( calloc( 4, 4 )); // trigger setting kde_malloc_is_used
    if( kde_malloc_is_used )
    {
	struct mallinfo m = mallinfo();
	usage_sum = m.hblkhd + m.uordblks;
    }
    else
    {
        struct mallinfo m = mallinfo();
#ifdef KDE_MALLINFO_FIELD_hblkhd
        usage_sum += m.hblkhd;
#endif
#ifdef KDE_MALLINFO_FIELD_uordblks
        usage_sum += m.uordblks;
#endif
#ifdef KDE_MALLINFO_FIELD_usmblks
        usage_sum += m.usmblks;
#endif
    }
#endif
    return usage_sum;
}

#include "konq_mainwindow.moc"
#include "konq_mainwindow_p.moc"

/* vim: et sw=4 ts=4
 */
