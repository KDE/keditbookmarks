/*  This file is part of the KDE project

    Copyright (C) 2002-2003 Konqueror Developers
                  2002-2003 Douglas Hanley <douglash@caltech.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston,
    MA  02111-1307, USA.
*/

#include "konq_tabs.h"

#include <qapplication.h>
#include <qclipboard.h>
#include <qptrlist.h>
#include <qpopupmenu.h>
#include <qtoolbutton.h>
#include <qtooltip.h>

#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kurldrag.h>
#include <kstringhandler.h>

#include "konq_frame.h"
#include "konq_view.h"
#include "konq_viewmgr.h"
#include "konq_misc.h"

#include <konq_pixmapprovider.h>
#include <kstdaccel.h>
#include <qtabbar.h>
#include <qwhatsthis.h>
#include <qstyle.h>

#define BREAKOFF_ID 5
#define CLOSETAB_ID 6
#define RELOAD_ALL_ID 7
#define OTHERTABS_ID 8

//###################################################################

KonqFrameTabs::KonqFrameTabs(QWidget* parent, KonqFrameContainerBase* parentContainer,
                             KonqViewManager* viewManager, const char * name)
  : KTabWidget(parent, name), m_rightWidget(0), m_leftWidget(0), m_alwaysTabBar(false),
    m_closeOtherTabsId(0)
{
  QWhatsThis::add( tabBar(), i18n( "This bar contains list of currently open tabs. Click on a tab to make it "
			  "active. The option to show a close button instead of website icon in the left "
			  "corner of the tab is configurable. You can also use keyboard shortcuts to "
			  "navigate through tabs. The text on the tab is the title of the website "
			  "currently open in it, put your mouse over the tab too see the full title in "
			  "case it was truncated to fit the tab size." ) );
  //kdDebug(1202) << "KonqFrameTabs::KonqFrameTabs()" << endl;

  m_pParentContainer = parentContainer;
  m_pChildFrameList = new QPtrList<KonqFrameBase>;
  m_pChildFrameList->setAutoDelete(false);
  m_pActiveChild = 0L;
  m_pViewManager = viewManager;

  connect( this, SIGNAL( currentChanged ( QWidget * ) ),
           this, SLOT( slotCurrentChanged( QWidget* ) ) );

  m_pPopupMenu = new QPopupMenu( this );
  m_pPopupMenu->insertItem( SmallIcon( "tab_new" ),
                            i18n("&New Tab"),
                            m_pViewManager->mainWindow(),
                            SLOT( slotAddTab() ),
                            m_pViewManager->mainWindow()->action("newtab")->shortcut() );
  m_pPopupMenu->insertItem( SmallIcon( "tab_duplicate" ),
                            i18n("&Duplicate Tab"),
                            m_pViewManager->mainWindow(),
                            SLOT( slotDuplicateTabPopup() ),
                            m_pViewManager->mainWindow()->action("duplicatecurrenttab")->shortcut() );
  m_pPopupMenu->insertItem( SmallIcon( "reload" ),
                            i18n( "&Reload Tab" ),
                            m_pViewManager->mainWindow(),
                            SLOT( slotReloadPopup() ),
                            m_pViewManager->mainWindow()->action("reload")->shortcut() );
  m_pPopupMenu->insertSeparator();
  m_pSubPopupMenuTab = new QPopupMenu( this );
  m_pPopupMenu->insertItem( i18n("Other Tabs" ), m_pSubPopupMenuTab, OTHERTABS_ID );
  connect( m_pSubPopupMenuTab, SIGNAL( activated ( int ) ),
           this, SLOT( slotSubPopupMenuTabActivated( int ) ) );
  m_pPopupMenu->insertSeparator();
  m_pPopupMenu->insertItem( SmallIcon( "tab_breakoff" ),
                            i18n("D&etach Tab"),
                            m_pViewManager->mainWindow(),
                            SLOT( slotBreakOffTabPopup() ),
                            m_pViewManager->mainWindow()->action("breakoffcurrenttab")->shortcut(),
                            BREAKOFF_ID );
  m_pPopupMenu->insertSeparator();
  m_pPopupMenu->insertItem( SmallIcon( "tab_remove" ),
                            i18n("&Close Tab"),
                            m_pViewManager->mainWindow(),
                            SLOT( slotRemoveTabPopup() ),
                            m_pViewManager->mainWindow()->action("removecurrenttab")->shortcut(),
                            CLOSETAB_ID );
  connect( this, SIGNAL( contextMenu( QWidget *, const QPoint & ) ),
           SLOT(slotContextMenu( QWidget *, const QPoint & ) ) );

  KConfig *config = KGlobal::config();
  KConfigGroupSaver cs( config, QString::fromLatin1("FMSettings") );

  m_MouseMiddleClickClosesTab = config->readBoolEntry( "MouseMiddleClickClosesTab", false );

  m_permanentCloseButtons = config->readBoolEntry( "PermanentCloseButton", false );
  if (m_permanentCloseButtons) {
    setHoverCloseButton( true );
    setHoverCloseButtonDelayed( false );
  }
  else
    setHoverCloseButton( config->readBoolEntry( "HoverCloseButton", false ) );
  setTabCloseActivatePrevious( config->readBoolEntry("TabCloseActivatePrevious", false) );
  if (config->readEntry("TabPosition")=="Bottom")
    setTabPosition(QTabWidget::Bottom);
  connect( this, SIGNAL( closeRequest( QWidget * )), SLOT(slotCloseRequest( QWidget * )));
  connect( this, SIGNAL( removeTabPopup() ),
           m_pViewManager->mainWindow(), SLOT( slotRemoveTabPopup() ) );

  if ( config->readBoolEntry( "AddTabButton", true ) ) {
    m_leftWidget = new QToolButton( this );
    connect( m_leftWidget, SIGNAL( clicked() ),
             m_pViewManager->mainWindow(), SLOT( slotAddTab() ) );
    m_leftWidget->setIconSet( SmallIcon( "tab_new" ) );
    m_leftWidget->adjustSize();
    QToolTip::add(m_leftWidget, i18n("Open a new tab"));
    setCornerWidget( m_leftWidget, TopLeft );
  }
  if ( config->readBoolEntry( "CloseTabButton", true ) ) {
    m_rightWidget = new QToolButton( this );
    connect( m_rightWidget, SIGNAL( clicked() ),
             m_pViewManager->mainWindow(), SLOT( slotRemoveTab() ) );
    m_rightWidget->setIconSet( SmallIcon( "tab_remove" ) );
    m_rightWidget->adjustSize();
    QToolTip::add(m_rightWidget, i18n("Close the current tab"));
    setCornerWidget( m_rightWidget, TopRight );
  }

  setAutomaticResizeTabs( true );
  setTabReorderingEnabled( true );
  connect( this, SIGNAL( movedTab( int, int ) ),
           SLOT( slotMovedTab( int, int ) ) );
  connect( this, SIGNAL( mouseMiddleClick() ),
           SLOT( slotMouseMiddleClick() ) );
  connect( this, SIGNAL( mouseMiddleClick( QWidget * ) ),
           SLOT( slotMouseMiddleClick( QWidget * ) ) );
  connect( this, SIGNAL( mouseDoubleClick() ),
           m_pViewManager->mainWindow(), SLOT( slotAddTab() ) );

  connect( this, SIGNAL( testCanDecode(const QDragMoveEvent *, bool & )),
           SLOT( slotTestCanDecode(const QDragMoveEvent *, bool & ) ) );
  connect( this, SIGNAL( receivedDropEvent( QDropEvent * )),
           SLOT( slotReceivedDropEvent( QDropEvent * ) ) );
  connect( this, SIGNAL( receivedDropEvent( QWidget *, QDropEvent * )),
           SLOT( slotReceivedDropEvent( QWidget *, QDropEvent * ) ) );
  connect( this, SIGNAL( initiateDrag( QWidget * )),
           SLOT( slotInitiateDrag( QWidget * ) ) );
}

KonqFrameTabs::~KonqFrameTabs()
{
  //kdDebug(1202) << "KonqFrameTabs::~KonqFrameTabs() " << this << " - " << className() << endl;
  m_pChildFrameList->setAutoDelete(true);
  delete m_pChildFrameList;
}

void KonqFrameTabs::listViews( ChildViewList *viewList ) {
  for( QPtrListIterator<KonqFrameBase> it( *m_pChildFrameList ); *it; ++it )
    it.current()->listViews(viewList);
}

void KonqFrameTabs::saveConfig( KConfig* config, const QString &prefix, bool saveURLs,
                                KonqFrameBase* docContainer, int id, int depth )
{
  //write children
  QStringList strlst;
  int i = 0;
  QString newPrefix;
  for (KonqFrameBase* it = m_pChildFrameList->first(); it; it = m_pChildFrameList->next())
    {
      newPrefix = QString::fromLatin1( it->frameType() ) + "T" + QString::number(i);
      strlst.append( newPrefix );
      newPrefix.append( '_' );
      it->saveConfig( config, newPrefix, saveURLs, docContainer, id, depth + i );
      i++;
    }

  config->writeEntry( QString::fromLatin1( "Children" ).prepend( prefix ), strlst );

  config->writeEntry( QString::fromLatin1( "activeChildIndex" ).prepend( prefix ),
                      currentPageIndex() );
}

void KonqFrameTabs::copyHistory( KonqFrameBase *other )
{
  if( other->frameType() != "Tabs" ) {
    kdDebug(1202) << "Frame types are not the same" << endl;
    return;
  }

  for (uint i = 0; i < m_pChildFrameList->count(); i++ )
    {
      m_pChildFrameList->at(i)->copyHistory( static_cast<KonqFrameTabs *>( other )->m_pChildFrameList->at(i) );
    }
}

void KonqFrameTabs::printFrameInfo( const QString& spaces )
{
  kdDebug(1202) << spaces << "KonqFrameTabs " << this << " visible="
                << QString("%1").arg(isVisible()) << " activeChild="
                << m_pActiveChild << endl;

  if (!m_pActiveChild)
      kdDebug(1202) << "WARNING: " << this << " has a null active child!" << endl;

  KonqFrameBase* child;
  int childFrameCount = m_pChildFrameList->count();
  for (int i = 0 ; i < childFrameCount ; i++) {
    child = m_pChildFrameList->at(i);
    if (child != 0L)
      child->printFrameInfo(spaces + "  ");
    else
      kdDebug(1202) << spaces << "  Null child" << endl;
  }
}

void KonqFrameTabs::reparentFrame( QWidget* parent, const QPoint & p, bool showIt )
{
  QWidget::reparent( parent, p, showIt );
}

void KonqFrameTabs::setTitle( const QString &title , QWidget* sender)
{
  // kdDebug(1202) << "KonqFrameTabs::setTitle( " << title << " , " << sender << " )" << endl;
  setTabLabel( sender,title );
}

void KonqFrameTabs::setTabIcon( const QString &url, QWidget* sender )
{
  //kdDebug(1202) << "KonqFrameTabs::setTabIcon( " << url << " , " << sender << " )" << endl;
  QIconSet iconSet;
  if (m_permanentCloseButtons)
    iconSet =  SmallIcon( "fileclose" );
  else
    iconSet =  QIconSet( KonqPixmapProvider::self()->pixmapFor( url ) );
  if (tabIconSet( sender ).pixmap().serialNumber() != iconSet.pixmap().serialNumber())
    setTabIconSet( sender, iconSet );
}

void KonqFrameTabs::activateChild()
{
  if (m_pActiveChild)
    {
      showPage( m_pActiveChild->widget() );
      m_pActiveChild->activateChild();
    }
}

void KonqFrameTabs::insertChildFrame( KonqFrameBase* frame, int index )
{
  //kdDebug(1202) << "KonqFrameTabs " << this << ": insertChildFrame " << frame << endl;

  if (frame)
    {
      //kdDebug(1202) << "Adding frame" << endl;
      bool showTabBar = (count() == 1);
      insertTab(frame->widget(),"", index);
      frame->setParentContainer(this);
      if (index == -1) m_pChildFrameList->append(frame);
      else m_pChildFrameList->insert(index, frame);
      if (m_rightWidget)
        m_rightWidget->setEnabled( m_pChildFrameList->count()>1 );
      KonqView* activeChildView = frame->activeChildView();
      if (activeChildView != 0L) {
        activeChildView->setCaption( activeChildView->caption() );
        activeChildView->setTabIcon( activeChildView->url().url() );
      }
      if (showTabBar)
          setTabBarHidden(false);
      else if ( count() == 1 )
          this->hideTabBar();//the first frame inserted (initialization)
    }
  else
    kdWarning(1202) << "KonqFrameTabs " << this << ": insertChildFrame(0L) !" << endl;
}

void KonqFrameTabs::removeChildFrame( KonqFrameBase * frame )
{
  //kdDebug(1202) << "KonqFrameTabs::RemoveChildFrame " << this << ". Child " << frame << " removed" << endl;
  if (frame) {
    removePage(frame->widget());
    m_pChildFrameList->remove(frame);
    if (m_rightWidget)
      m_rightWidget->setEnabled( m_pChildFrameList->count()>1 );
    if (count() == 1)
      hideTabBar();
  }
  else
    kdWarning(1202) << "KonqFrameTabs " << this << ": removeChildFrame(0L) !" << endl;

  //kdDebug(1202) << "KonqFrameTabs::RemoveChildFrame finished" << endl;
}

void KonqFrameTabs::slotCurrentChanged( QWidget* newPage )
{
  setTabColor( newPage, KGlobalSettings::textColor() );
  KonqFrameBase* currentFrame = dynamic_cast<KonqFrameBase*>(newPage);

  if (currentFrame && !m_pViewManager->isLoadingProfile()) {
    m_pActiveChild = currentFrame;
    currentFrame->activateChild();
  }
}

void KonqFrameTabs::moveTabBackward( int index )
{
  if ( index == 0 )
    return;
  moveTab( index, index-1 );
}

void KonqFrameTabs::moveTabForward( int index )
{
  if ( index == count()-1 )
    return;
  moveTab( index, index+1 );
}

void KonqFrameTabs::slotMovedTab( int from, int to )
{
  KonqFrameBase* fromFrame = m_pChildFrameList->at( from );
  m_pChildFrameList->remove( fromFrame );
  m_pChildFrameList->insert( to, fromFrame );

  KonqFrameBase* currentFrame = dynamic_cast<KonqFrameBase*>( currentPage() );
  if ( currentFrame && !m_pViewManager->isLoadingProfile() ) {
    m_pActiveChild = currentFrame;
    currentFrame->activateChild();
  }
}

void KonqFrameTabs::slotContextMenu( QWidget *w, const QPoint &p )
{
  uint tabCount = m_pChildFrameList->count();
  m_pPopupMenu->setItemEnabled( BREAKOFF_ID, tabCount>1 );
  m_pPopupMenu->setItemEnabled( CLOSETAB_ID, tabCount>1 );
  m_pPopupMenu->setItemEnabled( RELOAD_ALL_ID, tabCount>1 );
  m_pPopupMenu->setItemEnabled( m_closeOtherTabsId, tabCount>1 );
  m_pPopupMenu->setItemEnabled( OTHERTABS_ID, tabCount>1 );

  // Yes, I know this is an unchecked dynamic_cast - I'm casting sideways in a
  // class hierarchy and it could crash one day, but I haven't checked
  // setWorkingTab so I don't know if it can handle nulls.

  m_pViewManager->mainWindow()->setWorkingTab( dynamic_cast<KonqFrameBase*>(w) );
  refreshSubPopupMenuTab();
  m_pPopupMenu->exec( p );
}

void KonqFrameTabs::refreshSubPopupMenuTab()
{
    m_pSubPopupMenuTab->clear();
    int i=0;
    m_pSubPopupMenuTab->insertItem( SmallIcon( "reload_all_tabs" ),
                                    i18n( "&Reload All Tabs" ),
                                    m_pViewManager->mainWindow(),
                                    SLOT( slotReloadAllTabs() ),
                                    m_pViewManager->mainWindow()->action("reload_all_tabs")->shortcut(),
                                    RELOAD_ALL_ID );
    m_pSubPopupMenuTab->insertSeparator();
    for (KonqFrameBase* it = m_pChildFrameList->first(); it; it = m_pChildFrameList->next())
    {
        KonqFrame* frame = dynamic_cast<KonqFrame *>(it);
        if ( frame && frame->activeChildView() )
        {
            QString title = frame->title().stripWhiteSpace();
            if ( title.isEmpty() )
                title = frame->activeChildView()->url().url();
            title = KStringHandler::csqueeze( title, 50 );
            m_pSubPopupMenuTab->insertItem( QIconSet( KonqPixmapProvider::self()->pixmapFor( frame->activeChildView()->url().url() ) ), title, i );

        }
        i++;
    }
    m_pSubPopupMenuTab->insertSeparator();
    m_closeOtherTabsId =
      m_pSubPopupMenuTab->insertItem( SmallIcon( "tab_remove" ),
				      i18n( "Close &Other Tabs" ),
				      m_pViewManager->mainWindow(),
				      SLOT( slotRemoveOtherTabsPopup() ),
				      m_pViewManager->mainWindow()->action("removeothertabs")->shortcut() );
}

void KonqFrameTabs::slotCloseRequest( QWidget *w )
{
  if ( m_pChildFrameList->count() > 1 ) {
    // Yes, I know this is an unchecked dynamic_cast - I'm casting sideways in a class hierarchy and it could crash one day, but I haven't checked setWorkingTab so I don't know if it can handle nulls.
    m_pViewManager->mainWindow()->setWorkingTab( dynamic_cast<KonqFrameBase*>(w) );
    emit ( removeTabPopup() );
  }
}

void KonqFrameTabs::slotSubPopupMenuTabActivated( int _id)
{
    setCurrentPage( _id );
}

void KonqFrameTabs::slotMouseMiddleClick()
{
  QApplication::clipboard()->setSelectionMode( QClipboard::Selection );
  KURL filteredURL ( KonqMisc::konqFilteredURL( this, QApplication::clipboard()->text() ) );
  if ( !filteredURL.isEmpty() ) {
    KonqView* newView = m_pViewManager->addTab(QString::null, QString::null, false, false);
    if (newView == 0L) return;
    m_pViewManager->mainWindow()->openURL( newView, filteredURL, QString::null );
    m_pViewManager->showTab( newView );
    m_pViewManager->mainWindow()->focusLocationBar();
  }
}

void KonqFrameTabs::slotMouseMiddleClick( QWidget *w )
{
  if ( m_MouseMiddleClickClosesTab ) {
    if ( m_pChildFrameList->count() > 1 ) {
      // Yes, I know this is an unchecked dynamic_cast - I'm casting sideways in a class hierarchy and it could crash one day, but I haven't checked setWorkingTab so I don't know if it can handle nulls.
      m_pViewManager->mainWindow()->setWorkingTab( dynamic_cast<KonqFrameBase*>(w) );
      emit ( removeTabPopup() );
    }
  }
  else {
  QApplication::clipboard()->setSelectionMode( QClipboard::Selection );
  KURL filteredURL ( KonqMisc::konqFilteredURL( this, QApplication::clipboard()->text() ) );
  if ( !filteredURL.isEmpty() ) {
    KonqFrameBase* frame = dynamic_cast<KonqFrameBase*>(w);
    if (frame) {
      m_pViewManager->mainWindow()->openURL( frame->activeChildView(), filteredURL );
    }
  }
  }
}

void KonqFrameTabs::slotTestCanDecode(const QDragMoveEvent *e, bool &accept /* result */)
{
  accept = KURLDrag::canDecode( e );
}

void KonqFrameTabs::slotReceivedDropEvent( QDropEvent *e )
{
  KURL::List lstDragURLs;
  bool ok = KURLDrag::decode( e, lstDragURLs );
  if ( ok && lstDragURLs.first().isValid() ) {
    KonqView* newView = m_pViewManager->addTab(QString::null, QString::null, false, false);
    if (newView == 0L) return;
    m_pViewManager->mainWindow()->openURL( newView, lstDragURLs.first(), QString::null );
    m_pViewManager->showTab( newView );
    m_pViewManager->mainWindow()->focusLocationBar();
  }
}

void KonqFrameTabs::slotReceivedDropEvent( QWidget *w, QDropEvent *e )
{
  KURL::List lstDragURLs;
  bool ok = KURLDrag::decode( e, lstDragURLs );
  KonqFrameBase* frame = dynamic_cast<KonqFrameBase*>(w);
  if ( ok && lstDragURLs.first().isValid() && frame ) {
    KURL lstDragURL = lstDragURLs.first();
    if ( lstDragURL != frame->activeChildView()->url() )
      m_pViewManager->mainWindow()->openURL( frame->activeChildView(), lstDragURL );
  }
}

void KonqFrameTabs::slotInitiateDrag( QWidget *w )
{
  KonqFrameBase* frame = dynamic_cast<KonqFrameBase*>( w );
  if (frame) {
    KURL::List lst;
    lst.append( frame->activeChildView()->url() );
    KURLDrag *d = new KURLDrag( lst, this );
    d->setPixmap( KMimeType::pixmapForURL( lst.first(), 0, KIcon::Small ) );
    d->dragCopy();
  }
}

void KonqFrameTabs::hideTabBar()
{
  if ( !m_alwaysTabBar ) {
    setTabBarHidden(true);
  }
  m_pPopupMenu->setItemEnabled( BREAKOFF_ID, false );
  m_pPopupMenu->setItemEnabled( CLOSETAB_ID, false );
}

void KonqFrameTabs::setAlwaysTabbedMode( bool enable )
{
  bool update = ( enable != m_alwaysTabBar );

  m_alwaysTabBar = enable;
  if ( update ) {
    if ( m_alwaysTabBar )
      setTabBarHidden(false);
    else
      hideTabBar();
  }
}

#include "konq_tabs.moc"
