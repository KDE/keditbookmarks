/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#include <qdir.h>

#include "kfmrun.h"
#include "knewmenu.h"
#include "konq_mainview.h"
#include "kbookmarkmenu.h"
#include "konq_defaults.h"
#include "konq_childview.h"
#include "konq_mainwindow.h"
#include "konq_iconview.h"
#include "konq_treeview.h"
#include "konq_plugins.h"
#include "konq_propsmainview.h"
#include "konq_propsview.h"
#include "knewmenu.h"
#include "kpopupmenu.h"
#include "konq_frame.h"

#include <opUIUtils.h>
#include <opMenu.h>
#include <opMenuIf.h>
#include <opToolBar.h>
#include <opStatusBar.h>

#include <qkeycode.h>
#include <qlayout.h>
#include <qlist.h>
#include <qmsgbox.h>
#include <qpixmap.h>
#include <qpopmenu.h>
#include <qstring.h>
#include <qtimer.h>
#include <qpoint.h>
#include <qregexp.h>
#include <qsplitter.h>

#include <kaccel.h>
#include <kapp.h>
#include <kbookmark.h>
#include <kconfig.h>
#include <kio_cache.h>
#include <kio_paste.h>
#include <kkeydialog.h>
#include <klineeditdlg.h>
#include <kpixmapcache.h>
#include <kprocess.h>
#include <kpropsdlg.h>
#include <kstdaccel.h>
#include <kstddirs.h>
#include <kwm.h>
#include <kglobal.h>

#include <assert.h>
#include <pwd.h>

enum _ids {
/////  toolbar gear and lineedit /////
    TOOLBAR_GEAR_ID, TOOLBAR_URL_ID,

////// menu items //////
    MFILE_NEW_ID, MFILE_NEWWINDOW_ID, MFILE_RUN_ID, MFILE_OPENTERMINAL_ID,
    MFILE_OPENLOCATION_ID, MFILE_FIND_ID, MFILE_PRINT_ID,

    MEDIT_COPY_ID, MEDIT_PASTE_ID, MEDIT_TRASH_ID, MEDIT_DELETE_ID, MEDIT_SELECT_ID,
    MEDIT_SELECTALL_ID, // MEDIT_FINDINPAGE_ID, MEDIT_FINDNEXT_ID,
    MEDIT_MIMETYPES_ID, MEDIT_APPLICATIONS_ID, 

    MVIEW_SPLITHORIZONTALLY_ID, MVIEW_SPLITVERTICALLY_ID, MVIEW_REMOVEVIEW_ID, 
    MVIEW_SHOWDOT_ID, MVIEW_SHOWHTML_ID,
    MVIEW_LARGEICONS_ID, MVIEW_SMALLICONS_ID, MVIEW_TREEVIEW_ID, 
    MVIEW_RELOAD_ID, MVIEW_STOP_ID,
    // TODO : view frame source, view document source, document encoding

    MGO_UP_ID, MGO_BACK_ID, MGO_FORWARD_ID, MGO_HOME_ID,
    MGO_CACHE_ID, MGO_HISTORY_ID, MGO_MIMETYPES_ID, MGO_APPLICATIONS_ID,
    // TODO : add global mimetypes and apps here

    MOPTIONS_SHOWMENUBAR_ID, MOPTIONS_SHOWSTATUSBAR_ID, 
    MOPTIONS_SHOWTOOLBAR_ID, MOPTIONS_SHOWLOCATIONBAR_ID,
    MOPTIONS_SAVESETTINGS_ID,
    MOPTIONS_SAVELOCALSETTINGS_ID,
    // TODO : "Cache" submenu (clear cache, ...)
    MOPTIONS_CONFIGUREFILEMANAGER_ID,
    MOPTIONS_CONFIGUREBROWSER_ID,
    MOPTIONS_CONFIGUREKEYS_ID,
    MOPTIONS_RELOADPLUGINS_ID,
    MOPTIONS_SAVEVIEWCONFIGURATION_ID,
    
    MHELP_CONTENTS_ID,
    MHELP_ABOUT_ID
};

QList<KonqMainView>* KonqMainView::s_lstWindows = 0L;
QList<OpenPartsUI::Pixmap>* KonqMainView::s_lstAnimatedLogo = 0L;

KonqMainView::KonqMainView( const char *url, QWidget *parent ) : QWidget( parent )
{
  ADD_INTERFACE( "IDL:Konqueror/MainView:1.0" );

  setWidget( this );

  OPPartIf::setFocusPolicy( OpenParts::Part::ClickFocus );
  
  m_vMenuFile = 0L;
  m_vMenuFileNew = 0L;
  m_vMenuEdit = 0L;
  m_vMenuView = 0L;
  m_vMenuGo = 0L;
  m_vMenuBookmarks = 0L;
  m_vMenuOptions = 0L;

  m_vToolBar = 0L;
  m_vLocationBar = 0L;
  m_vMenuBar = 0L;
  m_vStatusBar = 0L;
  
  m_currentView = 0L;
  m_currentId = 0;

  m_sInitialURL = (url) ? url : QDir::homeDirPath().ascii();

  m_pAccel = new KAccel( this );
  m_pAccel->insertItem( i18n("Switch to left View"), "LeftView", CTRL+Key_1 );
  m_pAccel->insertItem( i18n("Switch to right View"), "RightView", CTRL+Key_2 );
  m_pAccel->insertItem( i18n("Directory up"), "DirUp", ALT+Key_Up );
  m_pAccel->insertItem( i18n("History Back"), "Back", ALT+Key_Left );
  m_pAccel->insertItem( i18n("History Forward"), "Forward", ALT+Key_Right );
  m_pAccel->connectItem( "LeftView", this, SLOT( slotFocusLeftView() ) );
  m_pAccel->connectItem( "RightView", this, SLOT( slotFocusRightView() ) );
  m_pAccel->connectItem( "DirUp", this, SLOT( slotUp() ) );
  m_pAccel->connectItem( "Back", this, SLOT( slotBack() ) );
  m_pAccel->connectItem( "Forward", this, SLOT( slotForward() ) );

  m_pAccel->readSettings();

  m_bInit = true;

  if ( !s_lstAnimatedLogo )
    s_lstAnimatedLogo = new QList<OpenPartsUI::Pixmap>;
  if ( !s_lstWindows )
    s_lstWindows = new QList<KonqMainView>;

  s_lstWindows->setAutoDelete( false );
  s_lstWindows->append( this );

  m_pMainSplitter = 0L;
  m_lstRows.setAutoDelete( true );

  initConfig();
}

KonqMainView::~KonqMainView()
{
  kdebug(0,1202,"KonqMainView::~KonqMainView()");
  cleanUp();
  kdebug(0,1202,"KonqMainView::~KonqMainView() done");
}

void KonqMainView::init()
{
  OpenParts::MenuBarManager_var menuBarManager = m_vMainWindow->menuBarManager();
  if ( !CORBA::is_nil( menuBarManager ) )
    menuBarManager->registerClient( id(), this );

  OpenParts::ToolBarManager_var toolBarManager = m_vMainWindow->toolBarManager();
  if ( !CORBA::is_nil( toolBarManager ) )
    toolBarManager->registerClient( id(), this );

  OpenParts::StatusBarManager_var statusBarManager = m_vMainWindow->statusBarManager();
  if ( !CORBA::is_nil( statusBarManager ) )
    m_vStatusBar = statusBarManager->registerClient( id() );

  CORBA::WString_var item = Q2C( i18n("Konqueror :-)") );
  m_vStatusBar->insertItem( item, 1 );

  // will KTM do it for us ?
  //  m_vStatusBar->enable( m_Props->m_bShowStatusBar ? OpenPartsUI::Show : OpenPartsUI::Hide );

  initGui();

  KonqPlugins::installKOMPlugins( this );
  m_bInit = false;
}

void KonqMainView::cleanUp()
{
  if ( m_bIsClean )
    return;

  kdebug(0,1202,"void KonqMainView::cleanUp()");

  m_vStatusBar = 0L;

  OpenParts::MenuBarManager_var menuBarManager = m_vMainWindow->menuBarManager();
  if ( !CORBA::is_nil( menuBarManager ) )
    menuBarManager->unregisterClient( id() );

  OpenParts::ToolBarManager_var toolBarManager = m_vMainWindow->toolBarManager();
  if ( !CORBA::is_nil( toolBarManager ) )
    toolBarManager->unregisterClient( id() );

  OpenParts::StatusBarManager_var statusBarManager = m_vMainWindow->statusBarManager();
  if ( !CORBA::is_nil( statusBarManager ) )
    statusBarManager->unregisterClient( id() );

  delete m_pAccel;

  clearMainView();

  if ( m_pMenuNew )
    delete m_pMenuNew;

  if ( m_pBookmarkMenu )
    delete m_pBookmarkMenu;

  delete m_pMainSplitter;

  m_animatedLogoTimer.stop();
  s_lstWindows->removeRef( this );

  OPPartIf::cleanUp();

  kdebug(0,1202,"void KonqMainView::cleanUp() done");
}

void KonqMainView::initConfig()
{
  // Read application config file if not already done
  if (!KonqPropsMainView::m_pDefaultProps)
  {
    kdebug(0,1202,"Reading global config");
    KConfig *config = kapp->getConfig();
    config->setGroup( "Settings" );
    KonqPropsMainView::m_pDefaultProps = new KonqPropsMainView(config);
  }

  // For the moment, no local properties
  // Copy the default properties
  m_Props = new KonqPropsMainView( *KonqPropsMainView::m_pDefaultProps );

  if ( !m_bInit )
  {
    // views will set their mode by themselves - to be checked
    //    m_rightView.m_pView->setViewMode( m_Props->rightViewMode() );
    //    m_leftView.m_pView->setViewMode( m_Props->leftViewMode() );
  }
  else
    this->resize(m_Props->m_width,m_Props->m_height);
}

void KonqMainView::initGui()
{
  openURL( m_sInitialURL );

  if ( s_lstAnimatedLogo->count() == 0 )
  {
    s_lstAnimatedLogo->setAutoDelete( true );
    for ( int i = 1; i < 9; i++ )
    {
      QString n;
      n.sprintf( "kde%i.xpm", i );
      s_lstAnimatedLogo->append( OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( n ) ) );
    }
  }			

  m_animatedLogoCounter = 0;
  QObject::connect( &m_animatedLogoTimer, SIGNAL( timeout() ), this, SLOT( slotAnimatedLogoTimeout() ) );
}

void KonqMainView::checkPrintingExtension()
{
  if ( m_currentView )
    setItemEnabled( m_vMenuFile, MFILE_PRINT_ID, m_currentView->view()->supportsInterface( "IDL:Browser/PrintingExtension:1.0" ) );
}

bool KonqMainView::event( const char* event, const CORBA::Any& value )
{
  EVENT_MAPPER( event, value );

  MAPPING( OpenPartsUI::eventCreateMenuBar, OpenPartsUI::typeCreateMenuBar_ptr, mappingCreateMenubar );
  MAPPING( OpenPartsUI::eventCreateToolBar, OpenPartsUI::typeCreateToolBar_ptr, mappingCreateToolbar );
  MAPPING( OpenParts::eventChildGotFocus, OpenParts::Part_ptr, mappingChildGotFocus );
  MAPPING( OpenParts::eventParentGotFocus, OpenParts::Part_ptr, mappingParentGotFocus );
  MAPPING( Browser::eventOpenURL, Browser::EventOpenURL, mappingOpenURL );
  MAPPING( Browser::eventNewTransfer, Browser::EventNewTransfer, mappingNewTransfer );

  END_EVENT_MAPPER;

  return false;
}

bool KonqMainView::mappingCreateMenubar( OpenPartsUI::MenuBar_ptr menuBar )
{
  m_vMenuBar = OpenPartsUI::MenuBar::_duplicate( menuBar );

  if ( CORBA::is_nil( menuBar ) )
  {
    m_vMenuFileNew->disconnect("activated", this, "slotFileNewActivated");
    m_vMenuFileNew->disconnect("aboutToShow", this, "slotFileNewAboutToShow");
    m_vMenuEdit->disconnect("aboutToShow", this, "slotMenuEditAboutToShow");
    m_vMenuView->disconnect("aboutToShow", this, "slotMenuViewAboutToShow");
    m_vMenuOptionsProfiles->disconnect( "activated", this, "slotViewProfileActivated" );

    if ( m_pMenuNew )
    {
      delete m_pMenuNew;
      m_pMenuNew = 0L;
    }

    if ( m_pBookmarkMenu )
    {
      delete m_pBookmarkMenu;
      m_pBookmarkMenu = 0L;
    }

    m_vMenuFile = 0L;
    m_vMenuFileNew = 0L;
    m_vMenuEdit = 0L;
    m_vMenuView = 0L;
    m_vMenuGo = 0L;
    m_vMenuBookmarks = 0L;
    m_vMenuOptions = 0L;
    m_vMenuHelp = 0L;

    return true;
  }

  OpenPartsUI::Pixmap_var pix;

  KStdAccel stdAccel;

  CORBA::WString_var text = Q2C( i18n("&File") );
  
  CORBA::Long m_idMenuFile = menuBar->insertMenu( text, m_vMenuFile, -1, -1 );

  text = Q2C( i18n("&New") );
  m_vMenuFile->insertItem8( text, m_vMenuFileNew, MFILE_NEW_ID, -1 );

  m_vMenuFileNew->connect("activated", this, "slotFileNewActivated");
  m_vMenuFileNew->connect("aboutToShow", this, "slotFileNewAboutToShow");
  
  m_pMenuNew = new KNewMenu( m_vMenuFileNew );

  text = Q2C( i18n("New &Window") );
  m_vMenuFile->insertItem4( text, this, "slotNewWindow", stdAccel.openNew(), MFILE_NEWWINDOW_ID, -1 );
  m_vMenuFile->insertSeparator( -1 );
  text = Q2C( i18n("&Run...") );
  m_vMenuFile->insertItem4( text, this, "slotRun", 0, MFILE_RUN_ID, -1  );
  text = Q2C( i18n("Open &Terminal") );
  m_vMenuFile->insertItem4( text, this, "slotTerminal", CTRL+Key_T, MFILE_OPENTERMINAL_ID, -1 );
  m_vMenuFile->insertSeparator( -1 );

  text = Q2C( i18n("&Open Location...") );
  m_vMenuFile->insertItem4( text, this, "slotOpenLocation", stdAccel.open(), MFILE_OPENLOCATION_ID, -1 );
  text = Q2C( i18n("&Find") );
  m_vMenuFile->insertItem4( text, this, "slotToolFind", stdAccel.find(), MFILE_FIND_ID, -1 );
  m_vMenuFile->insertSeparator( -1 );
  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "fileprint.xpm" ) );
  text = Q2C( i18n("&Print...") );
  m_vMenuFile->insertItem6( pix, text, this, "slotPrint", stdAccel.print(), MFILE_PRINT_ID, -1 );

  menuBar->setFileMenu( m_idMenuFile );

  text = Q2C( i18n("&Edit") );
  menuBar->insertMenu( text, m_vMenuEdit, -1, -1 );

  m_vMenuEdit->connect("aboutToShow", this, "slotMenuEditAboutToShow");
  m_bEditMenuDirty = true;
  
  createEditMenu();

  text = Q2C( i18n("&View") );
  menuBar->insertMenu( text, m_vMenuView, -1, -1 );  

  m_vMenuView->connect("aboutToShow", this, "slotMenuViewAboutToShow");
  m_bViewMenuDirty = true;
  
  createViewMenu();
  
  text = Q2C( i18n("&Go") );
  menuBar->insertMenu( text, m_vMenuGo, -1, -1 );

  text = Q2C( i18n("&Up") );
  m_vMenuGo->insertItem4( text, this, "slotUp", 0, MGO_UP_ID, -1 );
  text = Q2C( i18n("&Back") );
  m_vMenuGo->insertItem4( text, this, "slotBack", 0, MGO_BACK_ID, -1 );
  text = Q2C( i18n("&Forward") );
  m_vMenuGo->insertItem4( text, this, "slotForward", 0, MGO_FORWARD_ID, -1 );
  text = Q2C( i18n("&Home") );
  m_vMenuGo->insertItem4( text, this, "slotHome", 0, MGO_HOME_ID, -1 );
  m_vMenuGo->insertSeparator( -1 );
  text = Q2C( i18n("&Cache") );
  m_vMenuGo->insertItem4( text, this, "slotShowCache", 0, MGO_CACHE_ID, -1 );
  text = Q2C( i18n("&History") );
  m_vMenuGo->insertItem4( text, this, "slotShowHistory", 0, MGO_HISTORY_ID, -1 );
  text = Q2C( i18n("Mime &Types") );
  m_vMenuGo->insertItem4( text, this, "slotEditMimeTypes", 0, MGO_MIMETYPES_ID, -1 );
  text = Q2C( i18n("App&lications") );
  m_vMenuGo->insertItem4( text, this, "slotEditApplications", 0, MGO_APPLICATIONS_ID, -1 );
  //TODO: Global mime types and applications for root

  text = Q2C( i18n("&Bookmarks") );
  menuBar->insertMenu( text, m_vMenuBookmarks, -1, -1 );
  m_pBookmarkMenu = new KBookmarkMenu( this, m_vMenuBookmarks, this, true );

  text = Q2C( i18n("&Options") );
  menuBar->insertMenu( text, m_vMenuOptions, -1, -1 );

  text = Q2C( i18n("Show &Menubar") );
  m_vMenuOptions->insertItem4( text, this, "slotShowMenubar", 0, MOPTIONS_SHOWMENUBAR_ID, -1 );
  text = Q2C( i18n("Show &Statusbar") );
  m_vMenuOptions->insertItem4( text, this, "slotShowStatusbar", 0, MOPTIONS_SHOWSTATUSBAR_ID, -1 );
  text = Q2C( i18n("Show &Toolbar") );
  m_vMenuOptions->insertItem4( text, this, "slotShowToolbar", 0, MOPTIONS_SHOWTOOLBAR_ID, -1 );
  text = Q2C( i18n("Show &Locationbar") );
  m_vMenuOptions->insertItem4( text, this, "slotShowLocationbar", 0, MOPTIONS_SHOWLOCATIONBAR_ID, -1 );
  m_vMenuOptions->insertSeparator( -1 );
  text = Q2C( i18n("Sa&ve Settings") );
  m_vMenuOptions->insertItem4( text, this, "slotSaveSettings", 0, MOPTIONS_SAVESETTINGS_ID, -1 );
  text = Q2C( i18n("Save Settings for this &URL") );
  m_vMenuOptions->insertItem4( text, this, "slotSaveLocalSettings", 0, MOPTIONS_SAVELOCALSETTINGS_ID, -1 );
  m_vMenuOptions->insertSeparator( -1 );
  // TODO : cache submenu
  text = Q2C( i18n("&Configure File Manager...") );
  m_vMenuOptions->insertItem4( text, this, "slotConfigureFileManager", 0, MOPTIONS_CONFIGUREFILEMANAGER_ID, -1 );
  text = Q2C( i18n("Configure &Browser...") );
  m_vMenuOptions->insertItem4( text, this, "slotConfigureBrowser", 0, MOPTIONS_CONFIGUREBROWSER_ID, -1 );
  text = Q2C( i18n("Configure &keys") );
  m_vMenuOptions->insertItem4( text, this, "slotConfigureKeys", 0, MOPTIONS_CONFIGUREKEYS_ID, -1 );
  text = Q2C( i18n("Reload Plugins") );
  m_vMenuOptions->insertItem4( text, this, "slotReloadPlugins", 0, MOPTIONS_RELOADPLUGINS_ID, -1 );
  text = Q2C( i18n( "Save Current View Profile..." ) );
  m_vMenuOptions->insertItem4( text, this, "slotSaveViewProfile", 0, MOPTIONS_SAVEVIEWCONFIGURATION_ID, -1 );

  text = Q2C( i18n( "Load View Profile" ) );
  m_vMenuOptions->insertItem8( text, m_vMenuOptionsProfiles, -1, -1 );
  
  m_vMenuOptionsProfiles->connect( "activated", this, "slotViewProfileActivated" );
  
  fillProfileMenu();
  
//  text = Q2C( i18n("Load View Profile" ) );
//  m_vMenuOptions->insertItem4( text, this, "slotLoadViewConfiguration", 0, -1, -1 );

  text = Q2C( i18n( "&Help" ) );
  CORBA::Long helpId = m_vMenuBar->insertMenu( text, m_vMenuHelp, -1, -1 );
  m_vMenuBar->setHelpMenu( helpId );
  
  text = Q2C( i18n("&Contents" ) );
  m_vMenuHelp->insertItem4( text, this, "slotHelpContents", 0, MHELP_CONTENTS_ID, -1 );

  m_vMenuHelp->insertSeparator( -1 );
  
  text = Q2C( i18n("&About Konqueror" ) );
  m_vMenuHelp->insertItem4( text, this, "slotHelpAbout", 0, MHELP_ABOUT_ID, -1 );

  // Ok, this is wrong. But I don't see where to do it properly
  // (i.e. checking for m_v*Bar->isVisible())
  // We might need a call from KonqMainWindow::readProperties ...
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWMENUBAR_ID, true );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWSTATUSBAR_ID, true );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWTOOLBAR_ID, true );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWLOCATIONBAR_ID, true );

  checkPrintingExtension();

  return true;
}

bool KonqMainView::mappingCreateToolbar( OpenPartsUI::ToolBarFactory_ptr factory )
{
  kdebug(0,1202,"KonqMainView::mappingCreateToolbar");
  OpenPartsUI::Pixmap_var pix;

  if ( CORBA::is_nil(factory) )
     {
       m_vToolBar = 0L;
       m_vLocationBar = 0L;

       return true;
     }

  m_vToolBar = factory->create( OpenPartsUI::ToolBarFactory::Transient );

  m_vToolBar->setFullWidth( true ); // was false (why?). Changed by David so
                                    // that alignItemRight works

  CORBA::WString_var toolTip;
  
  toolTip = Q2C( i18n("Up") );
  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "up.xpm" ) );
  m_vToolBar->insertButton2( pix, MGO_UP_ID, SIGNAL(clicked()),
                             this, "slotUp", false, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "back.xpm" ) );
  toolTip = Q2C( i18n("Back") );
  m_vToolBar->insertButton2( pix, MGO_BACK_ID, SIGNAL(clicked()),
                             this, "slotBack", false, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "forward.xpm" ) );
  toolTip = Q2C( i18n("Forward") );
  m_vToolBar->insertButton2( pix, MGO_FORWARD_ID, SIGNAL(clicked()),
                             this, "slotForward", false, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "home.xpm" ) );
  toolTip = Q2C( i18n("Home") );
  m_vToolBar->insertButton2( pix, MGO_HOME_ID, SIGNAL(clicked()),
                             this, "slotHome", true, toolTip, -1);

  m_vToolBar->insertSeparator( -1 );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "reload.xpm" ) );
  toolTip = Q2C( i18n("Reload") );
  m_vToolBar->insertButton2( pix, MVIEW_RELOAD_ID, SIGNAL(clicked()),
                             this, "slotReload", true, toolTip, -1);

  m_vToolBar->insertSeparator( -1 );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "editcopy.xpm" ) );
  toolTip = Q2C( i18n("Copy") );
  m_vToolBar->insertButton2( pix, MEDIT_COPY_ID, SIGNAL(clicked()),
                             this, "slotCopy", true, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "editpaste.xpm" ) );
  toolTip = Q2C( i18n("Paste") );
  m_vToolBar->insertButton2( pix, MEDIT_PASTE_ID, SIGNAL(clicked()),
                             this, "slotPaste", true, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "fileprint.xpm" ) );
  toolTip = Q2C( i18n( "Print" ) );
  m_vToolBar->insertButton2( pix, MFILE_PRINT_ID, SIGNAL(clicked()),
                             this, "slotPrint", true, toolTip, -1 );
 				
  m_vToolBar->insertSeparator( -1 );				

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "help.xpm" ) );
  toolTip = Q2C( i18n("Help") );
  m_vToolBar->insertButton2( pix, MHELP_CONTENTS_ID, SIGNAL(clicked()),
                             this, "slotHelpContents", true, toolTip, -1);
				
  m_vToolBar->insertSeparator( -1 );				

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "stop.xpm" ) );
  toolTip = Q2C( i18n("Stop") );
  m_vToolBar->insertButton2( pix, MVIEW_STOP_ID, SIGNAL(clicked()),
                             this, "slotStop", false, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "kde1.xpm" ) );
  CORBA::Long gearIndex = m_vToolBar->insertButton2( pix, TOOLBAR_GEAR_ID, SIGNAL(clicked()),
                                                     this, "slotNewWindow", true, 0L, -1 );
  m_vToolBar->alignItemRight( TOOLBAR_GEAR_ID, true );

  //all items of the views should be inserted between the second-last and the
  //last item.
  m_lToolBarViewStartIndex = gearIndex;
  
  if ( m_currentView )
    createViewToolBar( m_currentView );

  /* will KTM do it for us ?
  m_vToolBar->enable( m_Props->m_bShowToolBar ? OpenPartsUI::Show : OpenPartsUI::Hide );
  m_vToolBar->setBarPos( (OpenPartsUI::BarPosition)(m_Props->m_toolBarPos) );
  */

  m_vLocationBar = factory->create( OpenPartsUI::ToolBarFactory::Transient );

  m_vLocationBar->setFullWidth( true );

  CORBA::WString_var text = Q2C( i18n("Location : ") );
  m_vLocationBar->insertTextLabel( text, -1, -1 );

  toolTip = Q2C( i18n("Current Location") );
  
  text = Q2C( QString::null );
  OpenPartsUI::WStrList items;
  items.length( 0 );
  if ( m_currentView )
  {
    items.length( 1 );
    items[ 0 ] = Q2C( m_currentView->locationBarURL() );
  }
  
  
  m_vLocationBar->insertCombo3( items, TOOLBAR_URL_ID, true, SIGNAL( activated(const QString &) ),
                                this, "slotURLEntered", true, toolTip, 70, -1,
				OpenPartsUI::AfterCurrent );

  m_vLocationBar->setComboAutoCompletion( TOOLBAR_URL_ID, true );
  m_vLocationBar->setItemAutoSized( TOOLBAR_URL_ID, true );

  /* will KTM do it for us ?
  m_vLocationBar->setBarPos( (OpenPartsUI::BarPosition)(m_Props->m_locationBarPos) );
  m_vLocationBar->enable( m_Props->m_bShowLocationBar ? OpenPartsUI::Show : OpenPartsUI::Hide );
  */

  checkPrintingExtension();

  kdebug(0,1202,"KonqMainView::mappingCreateToolbar : done !");
  return true;
}

bool KonqMainView::mappingChildGotFocus( OpenParts::Part_ptr child )
{
  kdebug(0, 1202, "bool KonqMainView::mappingChildGotFocus( OpenParts::Part_ptr child )");
  setActiveView( child->id() );
  return true;
}

bool KonqMainView::mappingParentGotFocus( OpenParts::Part_ptr  )
{
  kdebug(0, 1202, "bool KonqMainView::mappingParentGotFocus( OpenParts::Part_ptr child )");
  // removing view-specific menu entries (view will probably be destroyed !)
  if (m_currentView)
  {
    clearViewGUIElements( m_currentView );
    
    m_currentView->repaint();
  }

  // no more active view (even temporarily)
  setUpEnabled( "/", 0 );
  setItemEnabled( m_vMenuGo, MGO_BACK_ID, false );
  setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, false );

  return true;
}

bool KonqMainView::mappingOpenURL( Browser::EventOpenURL eventURL )
{
  openURL( eventURL );
  return true;
}

bool KonqMainView::mappingNewTransfer( Browser::EventNewTransfer transfer )
{
  //TODO: provide transfer status information somewhere (statusbar?...needs extension in OpenParts)
  
  KIOJob *job = new KIOJob;
  job->copy( transfer.source.in(), transfer.destination.in() );
  
  return true;
}

void KonqMainView::setActiveView( OpenParts::Id id )
{
  kdebug(0, 1202, "KonqMainView::setActiveView( %d )", id);
  KonqChildView* previousView = m_currentView;
  // clean view-specific part of the view/edit menus
  if ( previousView != 0L )
    clearViewGUIElements( previousView );

  MapViews::Iterator it = m_mapViews.find( id );
  assert( it != m_mapViews.end() );
  
  m_currentView = it.data();
  assert( m_currentView );
  m_currentId = id;

  setUpEnabled( m_currentView->url(), id );
  setItemEnabled( m_vMenuGo, MGO_BACK_ID, m_currentView->canGoBack() );
  setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, m_currentView->canGoForward() );

  setItemEnabled( m_vMenuFile, MFILE_PRINT_ID, m_currentView->view()->supportsInterface( "IDL:Browser/PrintingExtension:1.0" ) );

  if ( !CORBA::is_nil( m_vLocationBar ) )
  {
    CORBA::WString_var text = Q2C( m_currentView->locationBarURL() );
    m_vLocationBar->changeComboItem( TOOLBAR_URL_ID, text, 0 );
  }    

  m_bEditMenuDirty = true;
  m_bViewMenuDirty = true;
  createViewToolBar( m_currentView );
  if ( isVisible() )
  {
    if (previousView != 0L) // might be 0L e.g. if we just removed the current view
      previousView->repaint();
    m_currentView->repaint();
  }
}

Browser::View_ptr KonqMainView::activeView()
{
  if ( m_currentView )
    //KonqChildView::view() does *not* call _duplicate, so we have to do it here
    return Browser::View::_duplicate( m_currentView->view() );
  else
    return Browser::View::_nil();
}

OpenParts::Id KonqMainView::activeViewId()
{
  return m_currentId;
}

Browser::ViewList *KonqMainView::viewList()
{
  Browser::ViewList *seq = new Browser::ViewList;
  int i = 0;
  seq->length( i );

  MapViews::Iterator it = m_mapViews.begin();

  for (; it != m_mapViews.end(); it++ )
  {
    seq->length( i++ );
    (*seq)[ i ] = Browser::View::_duplicate( it.data()->view() );
  }

  return seq;
}

void KonqMainView::slotIdChanged( KonqChildView * childView, OpenParts::Id oldId, OpenParts::Id newId )
{
  m_mapViews.remove( oldId );
  m_mapViews.insert( newId, childView );
  if ( oldId == m_currentId )
    m_currentId = newId;
}

void KonqMainView::openURL( const Browser::URLRequest &_urlreq )
{
  openURL( _urlreq.url.in(), (bool)_urlreq.reload, (int)_urlreq.xOffset,
          (int)_urlreq.yOffset );
}

void KonqMainView::openURL( const char * _url, bool reload, int xOffset, int yOffset, KonqChildView *_view )
{
  /////////// First, modify the URL if necessary (adding protocol, ...) //////
  QString url = _url;

  // Root directory?
  if ( url[0] == '/' )
  {
    KURL::encode( url );
  }
  // Home directory?
  else if ( url.find( QRegExp( "^~.*" ) ) == 0 )
  {
    QString user;
    QString path;
    int index;
    
    index = url.find( "/" );
    if ( index != -1 )
    {
      user = url.mid( 1, index-1 );
      path = url.mid( index+1 );
    }
    else
      user = url.mid( 1 );
      
    if ( user.isEmpty() )
      user = QDir::homeDirPath();
    else
    {
      struct passwd *pwe = getpwnam( user.latin1() );
      if ( !pwe )
      {
	QMessageBox::warning( this, i18n( "Konqueror: Error" ),
	                      i18n( "User %1 doesn't exist" ).arg( user ),
			      i18n( "&OK" ) );
	return;
      }
      user = QString::fromLatin1( pwe->pw_dir );
    }
    
    KURL u( user + '/' + path );
    url = u.url();
  }
  else if ( strncmp( url, "www.", 4 ) == 0 )
  {
    QString tmp = "http://";
    KURL::encode( url );
    tmp += url;
    url = tmp;
  }
  else if ( strncmp( url, "ftp.", 4 ) == 0 )
  {
    QString tmp = "ftp://";
    KURL::encode( url );
    tmp += url;
    url = tmp;
  }

  KURL u( url );
  if ( u.isMalformed() )
  {
    QString tmp = i18n("Malformed URL\n%1").arg(_url);
    QMessageBox::critical( (QWidget*)0L, i18n( "Error" ), tmp, i18n( "OK" ) );
    return;
  }

  KonqChildView *view = _view;
  if ( !view )
    view = m_currentView;

  if ( view )
  {
    view->stop();
    if ( view->kfmRun() )
      delete view->kfmRun();
  }
    
  KfmRun *run = new KfmRun( this, view, url, 0, false, false );
  
  if ( view )
  {
    view->setKfmRun( run );
    view->setMiscURLData( reload, xOffset, yOffset );
  }
}

void KonqMainView::setStatusBarText( const CORBA::WChar *_text )
{
  if ( !CORBA::is_nil( m_vStatusBar ) )
    m_vStatusBar->changeItem( _text, 1 );
}

void KonqMainView::setLocationBarURL( OpenParts::Id id, const char *_url )
{
  MapViews::Iterator it = m_mapViews.find( id );
  assert( it != m_mapViews.end() );

  CORBA::WString_var wurl = Q2C( QString( _url ) );
    
  it.data()->setLocationBarURL( _url );
  
  if ( ( id == m_currentId ) && (!CORBA::is_nil( m_vLocationBar ) ) )
    m_vLocationBar->changeComboItem( TOOLBAR_URL_ID, wurl, 0 );
}

void KonqMainView::setItemEnabled( OpenPartsUI::Menu_ptr menu, int id, bool enable )
{
  // enable menu item, and if present in toolbar, related button
  // this will allow configurable toolbar buttons later
  if ( !CORBA::is_nil( menu ) ) 
    menu->setItemEnabled( id, enable );
 
  if ( !CORBA::is_nil( m_vToolBar ) )
    m_vToolBar->setItemEnabled( id, enable );
} 

void KonqMainView::setUpEnabled( QString _url, OpenParts::Id _id )
{
  if ( _id != m_currentId )
    return;

  bool bHasUpURL = false;
  
  if ( !_url.isNull() )
  {
    KURL u( _url );
    if ( u.hasPath() )
      bHasUpURL = ( u.path() != "/");
  }

  setItemEnabled( m_vMenuGo, MGO_UP_ID, bHasUpURL );
}

void KonqMainView::clearViewGUIElements( KonqChildView *childView )
{
  Browser::View_ptr view = childView->view();
  EMIT_EVENT( view, Browser::View::eventFillMenuEdit, 0L );
  EMIT_EVENT( view, Browser::View::eventFillMenuView, 0L );
    
  Browser::View::EventFillToolBar ev;
  ev.create = (CORBA::Boolean)false;
  ev.toolBar = OpenPartsUI::ToolBar::_duplicate( m_vToolBar );
  EMIT_EVENT( view, Browser::View::eventFillToolBar, ev );
}

void KonqMainView::createViewToolBar( KonqChildView *childView )
{
  if ( CORBA::is_nil( m_vToolBar ) )
    return;

  Browser::View::EventFillToolBar ev;
  ev.create = (CORBA::Boolean)true;
  ev.startIndex = m_lToolBarViewStartIndex;
  ev.toolBar = OpenPartsUI::ToolBar::_duplicate( m_vToolBar );
  EMIT_EVENT( childView->view(), Browser::View::eventFillToolBar, ev );
}

void KonqMainView::createNewWindow( const char *url )
{
  KonqMainWindow *m_pShell = new KonqMainWindow( url );
  m_pShell->show();
}

bool KonqMainView::openView( const QString &serviceType, const QString &url, KonqChildView *childView )
{
  QString indexFile;
  KURL u( url );

  if ( !m_sInitialURL.isEmpty() )
  {
    Browser::View_var vView;
    QStringList serviceTypes;

    if ( ( serviceType == "inode/directory" ) &&
         ( KonqPropsView::defaultProps()->isHTMLAllowed() ) &&
         ( u.isLocalFile() ) &&
	 ( ( indexFile = findIndexFile( u.path() ) ) != QString::null ) )
    {
      KonqChildView::createView( "text/html", vView, serviceTypes, this );
      m_sInitialURL = indexFile;
    }
    else if (!KonqChildView::createView( serviceType, vView, serviceTypes, this ) )
      return false;
      
    splitView( Qt::Horizontal, vView, serviceTypes );

    MapViews::Iterator it = m_mapViews.find( vView->id() );
    it.data()->openURL( m_sInitialURL );
  
    setActiveView( vView->id() );
    
    m_sInitialURL = QString::null;
    return true;
  }
  
  //first check whether the current view can display this type directly, then
  //try to change the view mode. if this fails, too, then Konqueror cannot
  //display the data addressed by the URL
  if ( childView->supportsServiceType( serviceType ) )
  {
    if ( ( serviceType == "inode/directory" ) &&
         ( childView->allowHTML() ) &&
         ( u.isLocalFile() ) &&
	 ( ( indexFile = findIndexFile( u.path() ) ) != QString::null ) )
      childView->changeViewMode( "text/html", indexFile );
    else
    {
      childView->makeHistory( false );
      childView->openURL( url, true );
    }
      
    childView->setKfmRun( 0L );
    return true;
  }

  if ( childView->changeViewMode( serviceType, url ) )
  {
    childView->setKfmRun( 0L );
    return true;
  }
    
  return false;
}

KonqChildView *KonqMainView::childView( OpenParts::Id id )
{
  MapViews::ConstIterator it = m_mapViews.find( id );
  if ( it != m_mapViews.end() )
    return it.data();
  else
    return 0L;
}

KonqChildView *KonqMainView::chooseNextView( RowInfo *row, KonqChildView *view )
{
  int idx = row->children.findRef( view );
  
  assert( idx != -1 );
  
  if ( idx < row->children.count() - 1 )
    return row->children.at( idx + 1 );
  else if ( idx > 0 )
    return row->children.at( idx - 1 );
    
  idx = m_lstRows.findRef( row );
  if ( idx < m_lstRows.count() - 1 )
  {
    RowInfo *nextRow = m_lstRows.at( idx + 1 );
    return chooseNextView( nextRow, nextRow->children.first() );
  }
  else if ( idx > 0 )
  {
    RowInfo *prevRow = m_lstRows.at( idx - 1 );
    return chooseNextView( prevRow, prevRow->children.first() );
  }
  
  return 0L;
}

void KonqMainView::removeChildView( OpenParts::Id id )
{
  MapViews::ConstIterator it = m_mapViews.find( id );
  
  removeChildView( it.data()->rowInfo(), it.data() );
}

void KonqMainView::removeChildView( RowInfo *row, KonqChildView *view )
{
  if ( view == m_currentView )
  {
//    KonqChildView *nextView = chooseNextView( row, view );
//    if ( nextView )
//      setActiveView( nextView->id() );
//    else
    { //shouldn't be reached ;-)
      m_vMainWindow->setActivePart( id() );
      m_currentView = 0L;
      m_currentId = 0;
    }
  }

  bool deleteParentSplitter = false;
  QSplitter *parentSplitter = view->frame()->parentSplitter();

  row->children.removeRef( view );

  if ( row->children.count() == 0 )
  {  
    m_lstRows.removeRef( row );
    deleteParentSplitter = true;
  }	  

  m_mapViews.remove( view->id() );

  delete view;
  
  if ( deleteParentSplitter )
    delete parentSplitter;
}

void KonqMainView::clearRow( RowInfo *row )
{
  QListIterator<KonqChildView> it( row->children );
  
  for (; it.current(); ++it )
    removeChildView( row, it.current() );
}

void KonqMainView::clearMainView()
{
  QListIterator<RowInfo> it( m_lstRows );
  
  for (; it.current(); ++it )
    clearRow( it.current() );
}

void KonqMainView::saveViewProfile( KConfig &cfg )
{
  QList<KonqChildView> viewList;
  
  cfg.setGroup( "MainSplitter" );
  cfg.writeEntry( "SplitterSizes", QProperty( m_pMainSplitter->sizes() ) );
 
  QListIterator<RowInfo> rowIt( m_lstRows );
  for ( int i = 0; rowIt.current(); ++rowIt, ++i )
  {
    cfg.setGroup( QString::fromLatin1( "Row %1" ).arg( i ) );
    
    cfg.writeEntry( "SplitterSizes", QProperty( rowIt.current()->splitter->sizes() ) );

    QStringList strlst;
    QListIterator<KonqChildView> viewIt( rowIt.current()->children );
    for (; viewIt.current(); ++viewIt )
    {
      strlst.append( QString().setNum( viewList.count() ) );
      viewList.append( viewIt.current() );
    }
    
    cfg.writeEntry( "ChildViews", strlst );
  }
  
  QListIterator<KonqChildView> viewIt( viewList );
  for (int i = 0; viewIt.current(); ++viewIt, ++i )
  {
    cfg.setGroup( QString::fromLatin1( "View %1" ).arg( i ) );
    
    cfg.writeEntry( "URL", viewIt.current()->url() );
    cfg.writeEntry( "ServiceType", viewIt.current()->serviceTypes().first() );
    
    //HACK
    if ( viewIt.current()->viewName() == "KonquerorKfmTreeView" )
      cfg.writeEntry( "IsBuiltinTreeView", true );
  }
  
  cfg.sync();
}

void KonqMainView::loadViewProfile( KConfig &cfg )
{
  clearMainView();
  
  QStringList groupList = cfg.groupList();
  
  QStringList::ConstIterator sIt = groupList.begin();
  QStringList::ConstIterator sEnd = groupList.end();
  
  QValueList<int> rowList;
  
  for (; sIt != sEnd; ++sIt )
   if ( strcmp( (*sIt).left( 4 ).latin1(), "Row " ) == 0 )
   {
     QString rowNr = *sIt;
     rowNr.remove( 0, 4 );
     rowList.append( rowNr.toInt() );
   };

  cfg.setGroup( "MainSplitter" );
  QValueList<int> mainSplitterSizes = 
    QProperty( cfg.readPropertyEntry( "SplitterSizes", QProperty::IntListType ) )
    .intListValue();

  QValueList<int>::ConstIterator rIt = rowList.begin();
  QValueList<int>::ConstIterator rEnd = rowList.end();
  
  for (; rIt != rEnd; ++rIt )
  {
    cfg.setGroup( QString::fromLatin1( "Row %1" ).arg( *rIt ) );
    
    RowInfo *rowInfo = new RowInfo;
    
    QSplitter *rowSplitter = new QSplitter( Qt::Horizontal, m_pMainSplitter );
    rowSplitter->setOpaqueResize();
    rowSplitter->show();
    
    rowInfo->splitter = rowSplitter;
    
    QValueList<int> splitterSizes = 
      QProperty( cfg.readPropertyEntry( "SplitterSizes", QProperty::IntListType ) )
      .intListValue();
    
    QStringList childList = cfg.readListEntry( "ChildViews" );
    
    QStringList::ConstIterator cIt = childList.begin();
    QStringList::ConstIterator cEnd = childList.end();
    
    for (; cIt != cEnd; ++cIt )
    {
      cfg.setGroup( QString::fromLatin1( "View " ) + (*cIt ) );
      
      QString serviceType = cfg.readEntry( "ServiceType" );
      QString url = cfg.readEntry( "URL" );
      
      //HACK
      bool treeView = ( cfg.hasKey( "IsBuiltinTreeView" ) &&
                        cfg.readBoolEntry( "IsBuiltinTreeView" ) &&
			serviceType == "inode/directory" );
      
      Browser::View_var vView;
      QStringList serviceTypes;
      
      if ( treeView )
      {
        //HACK
        vView = Browser::View::_duplicate( new KonqKfmTreeView( this ) );
	serviceTypes.append( "inode/directory" );
      }
      else
      {
        //Simon TODO: error handling
        KonqChildView::createView( serviceType, vView, serviceTypes, this );
      }
      
      setupView( rowInfo, vView, serviceTypes );
      
      MapViews::ConstIterator vIt = m_mapViews.find( vView->id() );
      vIt.data()->openURL( url );
    }

    rowInfo->splitter->setSizes( splitterSizes );
    
    m_lstRows.append( rowInfo );
  }

  m_pMainSplitter->setSizes( mainSplitterSizes );  
  
  setActiveView( m_lstRows.first()->children.first()->id() );
}

void KonqMainView::splitView ( Orientation orientation ) 
{
  kdebug(0, 1202, "KonqMainView::splitview default" );

  QString url = m_currentView->url();
  const QString serviceType = m_currentView->serviceTypes().first();

  Browser::View_var vView;
  QStringList serviceTypes;
  
  if ( !KonqChildView::createView( serviceType, vView, serviceTypes, this ) )
    return; //do not split the view at all if we can't clone the current view

  splitView( orientation, vView, serviceTypes );

  MapViews::Iterator it = m_mapViews.find( vView->id() );
  it.data()->openURL( url );
}

void KonqMainView::splitView ( Orientation orientation, 
			       Browser::View_ptr newView,
			       const QStringList &newViewServiceTypes )
{
/*
  The splitter layout looks like this:
  
  the mainsplitter is a vertical splitter, holding only horizontal splitters
  
  the horizontal splitters, childs of the mainsplitter, actually hold the
  views (viewframes) as children
 */
 
  if ( !m_pMainSplitter )
  {
    m_pMainSplitter = new QSplitter( Qt::Vertical, this );
    m_pMainSplitter->setOpaqueResize();
    m_pMainSplitter->show();
      
    orientation = Qt::Vertical; //force this
  }

  QSplitter *parentSplitter;
  RowInfo *rowInfo;

  if ( orientation == Qt::Horizontal )
    rowInfo = m_currentView->rowInfo();
  else
  {
    parentSplitter = new QSplitter( Qt::Horizontal, m_pMainSplitter );
    parentSplitter->setOpaqueResize();
    parentSplitter->show();
    
    rowInfo = new RowInfo;
    rowInfo->splitter = parentSplitter;
    m_lstRows.append( rowInfo );
  }

  setupView( rowInfo, newView, newViewServiceTypes );

  if ( orientation == Qt::Vertical )
  {
    QValueList<int> sizes = m_pMainSplitter->sizes();
    QValueList<int>::Iterator it = sizes.fromLast();
    sizes.remove( it );
    sizes.append( 100 );
    
    m_pMainSplitter->setSizes( sizes );
  }
}

void KonqMainView::setupView( RowInfo *row, Browser::View_ptr view, const QStringList &serviceTypes )
{
  KonqFrame* newViewFrame = new KonqFrame( row->splitter );

  KonqChildView *v = new KonqChildView( view, newViewFrame, 
					this, serviceTypes );

  v->setRowInfo( row );
  row->children.append( v );

  QObject::connect( v, SIGNAL(sigIdChanged( KonqChildView *, OpenParts::Id, OpenParts::Id )), 
                    this, SLOT(slotIdChanged( KonqChildView * , OpenParts::Id, OpenParts::Id ) ));

  m_mapViews.insert( view->id(), v );

  v->lockHistory();

  if (isVisible()) v->show();

  setItemEnabled( m_vMenuView, MVIEW_REMOVEVIEW_ID, 
	(m_mapViews.count() > 1) );
}

void KonqMainView::createViewMenu()
{
  if ( !CORBA::is_nil( m_vMenuView ) && m_bViewMenuDirty )
  {
    m_vMenuView->clear();
  
    CORBA::WString_var text;
    
    m_vMenuView->setCheckable( true );
    //  m_vMenuView->insertItem4( i18n("Show Directory Tr&ee"), this, "slotShowTree" , 0 );
    text = Q2C( i18n("Split view &horizontally") );
    m_vMenuView->insertItem4( text, this, "slotSplitViewHorizontal" , 0, MVIEW_SPLITHORIZONTALLY_ID, -1 );
    text = Q2C( i18n("Split view &vertically") );
    m_vMenuView->insertItem4( text, this, "slotSplitViewVertical" , 0, MVIEW_SPLITVERTICALLY_ID, -1 );
    text = Q2C( i18n("Remove view") );
    m_vMenuView->insertItem4( text, this, "slotRemoveView" , 0, MVIEW_REMOVEVIEW_ID, -1 );
    m_vMenuView->insertSeparator( -1 );
    
    // Two namings for the same thing ! We have to decide ourselves. 
    // I prefer the second one, because of .kde.html
    //m_vMenuView->insertItem4( i18n("&Always Show index.html"), this, "slotShowHTML" , 0, MVIEW_SHOWHTML_ID, -1 );
    text = Q2C( i18n("&Use HTML") );
    m_vMenuView->insertItem4( text, this, "slotShowHTML" , 0, MVIEW_SHOWHTML_ID, -1 );
    
    m_vMenuView->insertSeparator( -1 );

    text = Q2C( i18n("&Large Icons") );
    m_vMenuView->insertItem4( text, this, "slotLargeIcons" , 0, MVIEW_LARGEICONS_ID, -1 );
    text = Q2C( i18n("&Small Icons") );
    m_vMenuView->insertItem4( text, this, "slotSmallIcons" , 0, MVIEW_SMALLICONS_ID, -1 );
    text = Q2C( i18n("&Tree View") );
    m_vMenuView->insertItem4( text, this, "slotTreeView" , 0, MVIEW_TREEVIEW_ID, -1 );
    m_vMenuView->insertSeparator( -1 );

    text = Q2C( i18n("&Reload Document") );
    m_vMenuView->insertItem4( text, this, "slotReload" , Key_F5, MVIEW_RELOAD_ID, -1 );
    text = Q2C( i18n("Sto&p loading") );
    m_vMenuView->insertItem4( text, this, "slotStop" , 0, MVIEW_STOP_ID, -1 );
    //TODO: view frame source, view document source, document encoding

    setItemEnabled( m_vMenuView, MVIEW_REMOVEVIEW_ID, 
	(m_mapViews.count() > 1) );

    if ( m_currentView )
    {
      m_vMenuView->setItemChecked( MVIEW_SHOWHTML_ID, (m_currentView->allowHTML() ) );
      
      EMIT_EVENT( m_currentView->view(), Browser::View::eventFillMenuView, m_vMenuView );
    }

    m_bViewMenuDirty = false;
  }
}

void KonqMainView::createEditMenu()
{
  if ( !CORBA::is_nil( m_vMenuEdit ) && m_bEditMenuDirty )
  {
    m_vMenuEdit->clear();

    KStdAccel stdAccel; //?????????????????????

    CORBA::WString_var text = Q2C( i18n("&Copy") );
    m_vMenuEdit->insertItem4( text, this, "slotCopy", stdAccel.copy(), MEDIT_COPY_ID, -1 );
    text = Q2C( i18n("&Paste") );
    m_vMenuEdit->insertItem4( text, this, "slotPaste", stdAccel.paste(), MEDIT_PASTE_ID, -1 );
    text = Q2C( i18n("&Move to Trash") );
    m_vMenuEdit->insertItem4( text, this, "slotTrash", stdAccel.cut(), MEDIT_TRASH_ID, -1 );
    text = Q2C( i18n("&Delete") );
    m_vMenuEdit->insertItem4( text, this, "slotDelete", CTRL+Key_Delete, MEDIT_DELETE_ID, -1 );
    m_vMenuEdit->insertSeparator( -1 );

    if ( m_currentView )
      EMIT_EVENT( m_currentView->view(), Browser::View::eventFillMenuEdit, m_vMenuEdit );

    m_bEditMenuDirty = false;
  }
}

QString KonqMainView::findIndexFile( const QString &dir )
{
  QDir d( dir );
  
  QString f = d.filePath( "index.html", false );
  if ( QFile::exists( f ) )
    return f;
  
  f = d.filePath( ".kde.html", false );
  if ( QFile::exists( f ) )
    return f; 

  return QString::null;
}

void KonqMainView::fillProfileMenu()
{
  m_vMenuOptionsProfiles->clear();
  
  QStringList dirs = KGlobal::dirs()->findDirs( "data", "konqueror/profiles/" );
  QStringList::ConstIterator dIt = dirs.begin();
  QStringList::ConstIterator dEnd = dirs.end();
  
  for (; dIt != dEnd; ++dIt )
  {
    QDir dir( *dIt );
    QStringList entries = dir.entryList( QDir::Files );
    
    QStringList::ConstIterator eIt = entries.begin();
    QStringList::ConstIterator eEnd = entries.end();
    
    CORBA::WString_var text;
    
    for (; eIt != eEnd; ++eIt )
      m_vMenuOptionsProfiles->insertItem7( ( text = Q2C( *eIt ) ), -1, -1 );
    
  }
}

/////////////////////// MENUBAR AND TOOLBAR SLOTS //////////////////

void KonqMainView::slotNewWindow()
{
  QString url = m_currentView->url();
  KonqMainWindow *m_pShell = new KonqMainWindow( url );
  m_pShell->show();
}

void KonqMainView::slotRun()
{
  // HACK: The command is not executed in the directory
  // we are in currently. KWM does not support that yet
  KWM::sendKWMCommand("execute");
}

void KonqMainView::slotTerminal()
{
    KConfig *config = KApplication::getKApplication()->getConfig();
    config->setGroup( "Misc Defaults" ); // TODO : change this in kcmkonq too
    QString term = config->readEntry( "Terminal", DEFAULT_TERMINAL );
 
    QString dir ( QDir::homeDirPath() );
 
    KURL u( m_currentView->url() );
    if ( u.isLocalFile() )
      dir = u.path();

    QString cmd;
    cmd.sprintf("cd \"%s\" ; %s &", dir.data(), term.data());
    system( cmd.data() ); 
}

void KonqMainView::slotOpenLocation()
{
  QString u;
  if (m_currentView)
    u = m_currentView->url();

  KLineEditDlg l( i18n("Open Location:"), u, this, true );
  int x = l.exec();
  if ( x )
  {
    u = l.text();
    u = u.stripWhiteSpace();
    // Exit if the user did not enter an URL
    if ( u.isEmpty() )
      return;
    openURL( u.ascii() );
  }
}

void KonqMainView::slotToolFind()
{
  KShellProcess proc;
  proc << "kfind";
  
  KURL url;
  if ( m_currentView )
    url = m_currentView->url();

  if( url.isLocalFile() )
    proc << url.directory();

  proc.start(KShellProcess::DontCare);
}

void KonqMainView::slotPrint()
{
  Browser::View_ptr view = m_currentView->view();
  
  if ( view->supportsInterface( "IDL:Browser/PrintingExtension:1.0" ) )
  {
    CORBA::Object_var obj = view->getInterface( "IDL:Browser/PrintingExtension:1.0" );
    Browser::PrintingExtension_var printExt = Browser::PrintingExtension::_narrow( obj );
    printExt->print();
  }
}

void KonqMainView::slotCopy()
{
  // TODO
}

void KonqMainView::slotPaste()
{
  // TODO
}

void KonqMainView::slotTrash()
{
  // TODO
}

void KonqMainView::slotDelete()
{
  // TODO
}

void KonqMainView::slotSplitViewHorizontal()
{
  splitView( Qt::Horizontal );
}
 
void KonqMainView::slotSplitViewVertical()
{
  splitView( Qt::Vertical );
}

void KonqMainView::slotRemoveView()
{
  if ( m_mapViews.count() == 1 )
    return;

  removeChildView( m_currentId );

  setItemEnabled( m_vMenuView, MVIEW_REMOVEVIEW_ID, 
	(m_mapViews.count() > 1) );
}

void KonqMainView::slotShowHTML()
{
  assert( !CORBA::is_nil( m_vMenuView ) );
  assert( m_currentView );
  
  bool b = !m_currentView->allowHTML();
  
  m_currentView->setAllowHTML( b );
  m_vMenuView->setItemChecked( MVIEW_SHOWHTML_ID, b );

  if ( b && m_currentView->supportsServiceType( "inode/directory" ) )
  {
    m_currentView->lockHistory();
    openURL( m_currentView->url() );
  }
  else if ( !b && m_currentView->supportsServiceType( "text/html" ) )
  {
    KURL u( m_currentView->url() );
    m_currentView->lockHistory();
    openURL( u.directory() );
  }
}

void KonqMainView::slotLargeIcons()
{
  Browser::View_var v;

  if ( m_currentView->viewName() != "KonquerorKfmIconView" )
  {
    v = Browser::View::_duplicate( new KonqKfmIconView( this ) );
    QStringList serviceTypes;
    serviceTypes.append( "inode/directory" );
    m_currentView->lockHistory();
    m_currentView->changeView( v, serviceTypes );
  }
  
  v = Browser::View::_duplicate( m_currentView->view() );
  Konqueror::KfmIconView_var iv = Konqueror::KfmIconView::_narrow( v );
  
  iv->slotLargeIcons();
}

void KonqMainView::slotSmallIcons()
{
  Browser::View_var v;
  
  if ( m_currentView->viewName() != "KonquerorKfmIconView" )
  {
    v = Browser::View::_duplicate( new KonqKfmIconView( this ) );
    QStringList serviceTypes;
    serviceTypes.append( "inode/directory" );
    m_currentView->lockHistory();
    m_currentView->changeView( v, serviceTypes );
  }
  
  v = Browser::View::_duplicate( m_currentView->view() );
  Konqueror::KfmIconView_var iv = Konqueror::KfmIconView::_narrow( v );
  
  iv->slotSmallIcons();
}

void KonqMainView::slotTreeView()
{
  if ( m_currentView->viewName() != "KonquerorKfmTreeView" )
  {
    Browser::View_var v = Browser::View::_duplicate( new KonqKfmTreeView( this ) );
    QStringList serviceTypes;
    serviceTypes.append( "inode/directory" );
    m_currentView->lockHistory();
    m_currentView->changeView( v, serviceTypes );
  }
}

void KonqMainView::slotReload()
{
  m_currentView->reload();
}

void KonqMainView::slotStop()
{
  if ( m_currentView )
  {
    m_currentView->stop();
    if ( m_currentView->kfmRun() )
      delete m_currentView->kfmRun();
    m_currentView->setKfmRun( 0L );
  }    
}

void KonqMainView::slotUp()
{
  kdebug(0, 1202, "KonqMainView::slotUp()");
  QString url = m_currentView->url();
  KURL u( url );
  u.cd(".."); // KURL does it for us
  
  openURL( u.url() ); // not m_currentView->openURL since the view mode might be different
}

void KonqMainView::slotBack()
{ 
  m_currentView->goBack();

  if( !m_currentView->canGoBack() )
    setItemEnabled( m_vMenuGo, MGO_BACK_ID, false );
}

void KonqMainView::slotForward()
{
  m_currentView->goForward();
  if( !m_currentView->canGoForward() )
    setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, false );
}

void KonqMainView::slotHome()
{
  openURL("~"); // might need a view-mode change...
}

void KonqMainView::slotShowCache()
{
  QString file = KIOCache::storeIndex();
  if ( file.isEmpty() )
  {
    QMessageBox::critical( 0L, i18n("Error"), i18n( "Could not write index file" ), i18n( "OK" ) );
    return;
  }

  QString f = file;
  KURL::encode( f );
  openURL( f );
}

void KonqMainView::slotShowHistory()
{
  // TODO
}

void KonqMainView::slotEditMimeTypes()
{
    openURL( kapp->kde_mimedir() );
}

void KonqMainView::slotEditApplications()
{
    openURL( kapp->kde_appsdir() );
}

void KonqMainView::slotShowMenubar()
{
  m_vMenuBar->enable( OpenPartsUI::Toggle );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWMENUBAR_ID, m_vMenuBar->isVisible() );
}

void KonqMainView::slotShowStatusbar()
{
  m_vStatusBar->enable( OpenPartsUI::Toggle );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWSTATUSBAR_ID, m_vStatusBar->isVisible() );
}

void KonqMainView::slotShowToolbar()
{
  m_vToolBar->enable( OpenPartsUI::Toggle );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWTOOLBAR_ID, m_vToolBar->isVisible() );
}

void KonqMainView::slotShowLocationbar()
{
  m_vLocationBar->enable( OpenPartsUI::Toggle );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWLOCATIONBAR_ID, m_vLocationBar->isVisible() );
}

void KonqMainView::slotSaveSettings()
{
  KConfig *config = kapp->getConfig();
  config->setGroup( "Settings" );

  // Update the values in m_Props, if necessary :
  m_Props->m_width = this->width();
  m_Props->m_height = this->height();
//  m_Props->m_toolBarPos = m_pToolbar->barPos();
  // m_Props->m_statusBarPos = m_pStatusBar->barPos(); doesn't exist. Hum.
//  m_Props->m_menuBarPos = m_pMenu->menuBarPos();
//  m_Props->m_locationBarPos = m_pLocationBar->barPos();
  m_Props->saveProps(config);
}

void KonqMainView::slotSaveLocalSettings()
{
//TODO
}

void KonqMainView::slotConfigureFileManager()
{
  if (fork() == 0) {
    // execute 'kcmkonq' 
    execl(locate("exe", "/kcmkonq"), 0);
    warning("Error launching kcmkonq !");
    exit(1);
  }             
}

void KonqMainView::slotConfigureBrowser()
{
  if (fork() == 0) {
    // execute 'kcmkio' 
    execl(locate("exe", "kcmkio"), 0);
    warning("Error launching kcmkio !");
    exit(1);
  }                          
}

void KonqMainView::slotConfigureKeys()
{
  KKeyDialog::configureKeys( m_pAccel );
}

void KonqMainView::slotReloadPlugins()
{
  KonqPlugins::reload();

  KOM::Component::PluginInfoSeq_var plugins = describePlugins();
  for ( CORBA::ULong k = 0; k < plugins->length(); ++k )
    removePlugin( plugins[ k ].id );

  KonqPlugins::installKOMPlugins( this );

  Browser::View_ptr pView;
  
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
  {
    pView = (*it)->view();

    plugins = pView->describePlugins();
    for ( CORBA::ULong k = 0; k < plugins->length(); ++k )
      pView->removePlugin( plugins[ k ].id );

    KonqPlugins::installKOMPlugins( pView );
  }
}

void KonqMainView::slotSaveViewProfile()
{
  KLineEditDlg *dlg = new KLineEditDlg( i18n( "Enter Name for Profile" ),
                                       QString::null, this, false );
  
  if ( dlg->exec() && !dlg->text().isEmpty() )
  {
    QString fileName = locateLocal( "data", 
                       QString::fromLatin1( "konqueror/profiles/" ) + 
		       dlg->text() );
    
    if ( QFile::exists( fileName ) )
    {
      QFile f( fileName );
      f.remove();
    }
    
    KConfig cfg( fileName );
    saveViewProfile( cfg );
    
    if ( !CORBA::is_nil( m_vMenuOptionsProfiles ) )
      fillProfileMenu();
  }
}

void KonqMainView::slotViewProfileActivated( CORBA::Long id )
{
  CORBA::WString_var text = m_vMenuOptionsProfiles->text( id );
  QString name = QString::fromLatin1( "konqueror/profiles/" ) + C2Q( text );
  
  QString fileName = locate( "data", name );
  
  KConfig cfg( fileName, true );
  loadViewProfile( cfg );
}

void KonqMainView::slotHelpContents()
{
  kapp->invokeHTMLHelp( "konqueror/index.html", "" );
}

void KonqMainView::slotHelpAbout()
{
  QMessageBox::about( 0L, i18n( "About Konqueror" ), i18n(
"Konqueror Version 0.1\n"
"Author: Torben Weis <weis@kde.org>\n"
"Current maintainer: David Faure <faure@kde.org>\n\n"
"Current team:\n"
"David Faure <faure@kde.org>\n"
"Simon Hausmann <hausmann@kde.org>\n"
"Michael Reiher <michael.reiher@gmx.de>\n"
"Matthias Welk <welk@fokus.gmd.de>\n"
"Waldo Bastian <bastian@kde.org> , Lars Knoll <knoll@mpi-hd.mpg.de> (khtml library)\n"
"Matt Koss <koss@napri.sk>, Alex Zepeda <garbanzo@hooked.net> (kio library/slaves)\n"
  ));
}

void KonqMainView::slotURLEntered( const CORBA::WChar *_url )
{
  QString url = C2Q( _url );

  // Exit if the user did not enter an URL
  if ( url.isEmpty() )
    return;

  openURL( url.ascii() );
  
  m_vLocationBar->setCurrentComboItem( TOOLBAR_URL_ID, 0 );
}

void KonqMainView::slotBookmarkSelected( CORBA::Long id )
{
  if ( m_pBookmarkMenu )
    m_pBookmarkMenu->slotBookmarkSelected( id );
}

void KonqMainView::slotEditBookmarks()
{
  KBookmarkManager::self()->slotEditBookmarks();
}

void KonqMainView::slotURLStarted( OpenParts::Id id, const char *url )
{
  kdebug(0, 1202, "KonqMainView::slotURLStarted( %d, %s )", id, url);
  if ( !url )
    return;

  MapViews::Iterator it = m_mapViews.find( id );
  
  assert( it != m_mapViews.end() );
  
  if ( id == m_currentId )
    slotStartAnimation();

  (*it)->makeHistory( true );
  
  if ( id == m_currentId )
  {
    setUpEnabled( url, id );
    setItemEnabled( m_vMenuGo, MGO_BACK_ID, m_currentView->canGoBack() );
    setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, m_currentView->canGoForward() );
  }
}

void KonqMainView::slotURLCompleted( OpenParts::Id id )
{
  kdebug(0, 1202, "void KonqMainView::slotURLCompleted( OpenParts::Id id )");

  MapViews::Iterator it = m_mapViews.find( id );
  
  assert( it != m_mapViews.end() );

  if ( id == m_currentId )
    slotStopAnimation();

  (*it)->makeHistory( false );
 
  if ( id == m_currentId )
  {
    setItemEnabled( m_vMenuGo, MGO_BACK_ID, m_currentView->canGoBack() );
    setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, m_currentView->canGoForward() );
  }
}

void KonqMainView::slotAnimatedLogoTimeout()
{
  m_animatedLogoCounter++;
  if ( m_animatedLogoCounter == s_lstAnimatedLogo->count() )
    m_animatedLogoCounter = 0;

  if ( !CORBA::is_nil( m_vToolBar ) )
    m_vToolBar->setButtonPixmap( TOOLBAR_GEAR_ID, *( s_lstAnimatedLogo->at( m_animatedLogoCounter ) ) );
}

void KonqMainView::slotStartAnimation()
{
  m_animatedLogoCounter = 0;
  m_animatedLogoTimer.start( 50 );
  setItemEnabled( m_vMenuView, MVIEW_STOP_ID, true );
}

void KonqMainView::slotStopAnimation()
{
  m_animatedLogoTimer.stop();

  if ( !CORBA::is_nil( m_vToolBar ) )
  {
    m_vToolBar->setButtonPixmap( TOOLBAR_GEAR_ID, *( s_lstAnimatedLogo->at( 0 ) ) );
    setItemEnabled( m_vMenuView, MVIEW_STOP_ID, false );
  }

  CORBA::WString_var msg = Q2C( i18n("Document: Done") );
  setStatusBarText( msg );
}

void KonqMainView::popupMenu( const QPoint &_global, const QStringList &_urls, mode_t _mode )
{
  KonqPopupMenu * popupMenu = new KonqPopupMenu( _urls, 
                                                 _mode,
                                                 m_currentView->url(),
                                                 m_currentView->canGoBack(),
                                                 m_currentView->canGoForward(),
                                                 !m_vMenuBar->isVisible() ); // hidden ?

  kdebug(0, 1202, "exec()");
  int iSelected = popupMenu->exec( _global );
  kdebug(0, 1202, "deleting popupMenu object");
  delete popupMenu;
  /* Test for konqueror-specific entries. A normal signal/slot mechanism doesn't work here,
     because those slots are virtual. */
  switch (iSelected) {
    case KPOPUPMENU_UP_ID : slotUp(); break;
    case KPOPUPMENU_BACK_ID : slotBack(); break;
    case KPOPUPMENU_FORWARD_ID : slotForward(); break;
    case KPOPUPMENU_SHOWMENUBAR_ID : slotShowMenubar(); break;
  }
}

void KonqMainView::slotFileNewActivated( CORBA::Long id )
{
  if ( m_pMenuNew )
     {
       QStringList urls;
       urls.append( m_currentView->url() );

       m_pMenuNew->setPopupFiles( urls );

       m_pMenuNew->slotNewFile( (int)id );
     }
}

void KonqMainView::slotFileNewAboutToShow()
{
  if ( m_pMenuNew )
    m_pMenuNew->slotCheckUpToDate();
}

void KonqMainView::slotMenuEditAboutToShow()
{
  createEditMenu();
}

void KonqMainView::slotMenuViewAboutToShow()
{
  createViewMenu();
}

void KonqMainView::resizeEvent( QResizeEvent * )
{
  if( m_pMainSplitter )
    m_pMainSplitter->setGeometry( 0, 0, width(), height() ); 
}

void KonqMainView::openBookmarkURL( const char *url )
{
  kdebug(0,1202,"KonqMainView::openBookmarkURL(%s)",url);
  openURL( url );
}
 
QString KonqMainView::currentTitle()
{
  CORBA::WString_var t = m_vMainWindow->partCaption( m_currentId );
  QString title = C2Q( t );
  return title;
}
 
QString KonqMainView::currentURL()
{
  return m_currentView->url();
}

#include "konq_mainview.moc"
