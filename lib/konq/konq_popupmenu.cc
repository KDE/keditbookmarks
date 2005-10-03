/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
   Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <qdir.h>
//Added by qt3to4:
#include <QPixmap>
#include <Q3CString>

#include <klocale.h>
#include <kapplication.h>
#include <kbookmarkmanager.h>
#include <kdebug.h>
#include <krun.h>
#include <kprotocolinfo.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <kxmlguifactory.h>
#include <kxmlguibuilder.h>
#include <kparts/componentfactory.h>

#include <assert.h>

#include <kfileshare.h>
#include <kprocess.h>

#include "kpropertiesdialog.h"
#include "knewmenu.h"
#include "konq_popupmenu.h"
#include "konq_operations.h"
#include <dcopclient.h>
#include <kauthorized.h>

/*
 Test cases:
  iconview file: background
  iconview file: file (with and without servicemenus)
  iconview file: directory
  iconview remote protocol (e.g. ftp: or fish:)
  iconview trash:/
  sidebar directory tree
  sidebar Devices / Hard Disc
  khtml background
  khtml link
  khtml image (www.kde.org RMB on K logo)
  khtmlimage (same as above, then choose View image, then RMB)
  selected text in khtml
  embedded katepart
  kdesktop folder
  trash link on desktop
  trashed file or directory
  application .desktop file
 Then the same after uninstalling kdeaddons/konq-plugins (kuick and arkplugin in particular)
*/

class KonqPopupMenuGUIBuilder : public KXMLGUIBuilder
{
public:
  KonqPopupMenuGUIBuilder( Q3PopupMenu *menu )
  : KXMLGUIBuilder( 0 )
  {
    m_menu = menu;
  }
  virtual ~KonqPopupMenuGUIBuilder()
  {
  }

  virtual QWidget *createContainer( QWidget *parent, int index,
          const QDomElement &element,
          int &id )
  {
    if ( !parent && element.attribute( "name" ) == "popupmenu" )
      return m_menu;

    return KXMLGUIBuilder::createContainer( parent, index, element, id );
  }

  Q3PopupMenu *m_menu;
};

class KonqPopupMenu::KonqPopupMenuPrivate
{
public:
  KonqPopupMenuPrivate() : m_parentWidget( 0 ),
                           m_itemFlags( KParts::BrowserExtension::DefaultPopupItems )
  {
  }
  QString m_urlTitle;
  QWidget *m_parentWidget;
  KParts::BrowserExtension::PopupFlags m_itemFlags;
};

KonqPopupMenu::ProtocolInfo::ProtocolInfo()
{
  m_Reading = false;
  m_Writing = false;
  m_Deleting = false;
  m_Moving = false;
  m_TrashIncluded = false;
}

bool KonqPopupMenu::ProtocolInfo::supportsReading() const
{
  return m_Reading;
}

bool KonqPopupMenu::ProtocolInfo::supportsWriting() const
{
  return m_Writing;
}

bool KonqPopupMenu::ProtocolInfo::supportsDeleting() const
{
  return m_Deleting;
}

bool KonqPopupMenu::ProtocolInfo::supportsMoving() const
{
  return m_Moving;
}

bool KonqPopupMenu::ProtocolInfo::trashIncluded() const
{
  return m_TrashIncluded;
}

// This helper class stores the .desktop-file actions and the servicemenus
// in order to support X-KDE-Priority and X-KDE-Submenu.
class PopupServices
{
public:
    ServiceList* selectList( const QString& priority, const QString& submenuName );

    ServiceList builtin;
    ServiceList user, userToplevel, userPriority;
    QMap<QString, ServiceList> userSubmenus, userToplevelSubmenus, userPrioritySubmenus;
};

ServiceList* PopupServices::selectList( const QString& priority, const QString& submenuName )
{
    // we use the categories .desktop entry to define submenus
    // if none is defined, we just pop it in the main menu
    if (submenuName.isEmpty())
    {
        if (priority == "TopLevel")
        {
            return &userToplevel;
        }
        else if (priority == "Important")
        {
            return &userPriority;
        }
    }
    else if (priority == "TopLevel")
    {
        return &(userToplevelSubmenus[submenuName]);
    }
    else if (priority == "Important")
    {
        return &(userPrioritySubmenus[submenuName]);
    }
    else
    {
        return &(userSubmenus[submenuName]);
    }
    return &user;
}

//////////////////

KonqPopupMenu::KonqPopupMenu( KBookmarkManager *mgr, const KFileItemList &items,
                              KURL viewURL,
                              KActionCollection & actions,
                              KNewMenu * newMenu,
                              bool showProperties )
    : Q3PopupMenu( 0L, "konq_popupmenu" ),
      m_actions( actions ), m_ownActions( static_cast<QWidget *>( 0 ), "KonqPopupMenu::m_ownActions" ),
		  m_pMenuNew( newMenu ), m_sViewURL(viewURL), m_lstItems(items), m_pManager(mgr)
{
    KonqPopupFlags kpf = ( showProperties ? ShowProperties : IsLink ) | ShowNewWindow;
    init(0, kpf, KParts::BrowserExtension::DefaultPopupItems);
}

KonqPopupMenu::KonqPopupMenu( KBookmarkManager *mgr, const KFileItemList &items,
                              KURL viewURL,
                              KActionCollection & actions,
                              KNewMenu * newMenu,
                              QWidget * parentWidget,
                              bool showProperties )
    : Q3PopupMenu( parentWidget, "konq_popupmenu" ), m_actions( actions ), m_ownActions( static_cast<QWidget *>( 0 ), "KonqPopupMenu::m_ownActions" ), m_pMenuNew( newMenu ), m_sViewURL(viewURL), m_lstItems(items), m_pManager(mgr)
{
    KonqPopupFlags kpf = ( showProperties ? ShowProperties : IsLink ) | ShowNewWindow;
    init(parentWidget, kpf, KParts::BrowserExtension::DefaultPopupItems);
}

KonqPopupMenu::KonqPopupMenu( KBookmarkManager *mgr, const KFileItemList &items,
                              const KURL& viewURL,
                              KActionCollection & actions,
                              KNewMenu * newMenu,
                              QWidget * parentWidget,
                              KonqPopupFlags kpf,
                              KParts::BrowserExtension::PopupFlags flags)
  : Q3PopupMenu( parentWidget, "konq_popupmenu" ), m_actions( actions ), m_ownActions( static_cast<QWidget *>( 0 ), "KonqPopupMenu::m_ownActions" ), m_pMenuNew( newMenu ), m_sViewURL(viewURL), m_lstItems(items), m_pManager(mgr)
{
    init(parentWidget, kpf, flags);
}

void KonqPopupMenu::init (QWidget * parentWidget, KonqPopupFlags kpf, KParts::BrowserExtension::PopupFlags flags)
{
    d = new KonqPopupMenuPrivate;
    d->m_parentWidget = parentWidget;
    d->m_itemFlags = flags;
    setup(kpf);
}

int KonqPopupMenu::insertServicesSubmenus(const QMap<QString, ServiceList>& submenus,
                                          QDomElement& menu,
                                          bool isBuiltin)
{
    int count = 0;
    QMap<QString, ServiceList>::ConstIterator it;

    for (it = submenus.begin(); it != submenus.end(); ++it)
    {
        if (it.data().isEmpty())
        {
            //avoid empty sub-menus
            continue;
        }

        QDomElement actionSubmenu = domDocument().createElement( "menu" );
        actionSubmenu.setAttribute( "name", "actions " + it.key() );
        menu.appendChild( actionSubmenu );
        QDomElement subtext = domDocument().createElement( "text" );
        actionSubmenu.appendChild( subtext );
        subtext.appendChild( domDocument().createTextNode( it.key() ) );
        count += insertServices(it.data(), actionSubmenu, isBuiltin);
    }

    return count;
}

int KonqPopupMenu::insertServices(const ServiceList& list,
                                  QDomElement& menu,
                                  bool isBuiltin)
{
    static int id = 1000;
    int count = 0;

    ServiceList::const_iterator it = list.begin();
    for( ; it != list.end(); ++it )
    {
        if ((*it).isEmpty())
        {
            if (!menu.firstChild().isNull() &&
                menu.lastChild().toElement().tagName().lower() != "separator")
            {
                QDomElement separator = domDocument().createElement( "separator" );
                menu.appendChild(separator);
            }
            continue;
        }

        if (isBuiltin || (*it).m_display == true)
        {
            QString name;
            name.setNum( id );
            name.prepend( isBuiltin ? "builtinservice_" : "userservice_" );
            KAction * act = new KAction( QString((*it).m_strName).replace('&',"&&"), 0,
                                         this, SLOT( slotRunService() ),
                                         &m_ownActions, name.latin1() );

            if ( !(*it).m_strIcon.isEmpty() )
            {
                QPixmap pix = SmallIcon( (*it).m_strIcon );
                act->setIconSet( pix );
            }

            KonqXMLGUIClient::addAction( act, menu ); // Add to toplevel menu

            m_mapPopupServices[ id++ ] = *it;
            ++count;
        }
    }

    return count;
}

bool KonqPopupMenu::KIOSKAuthorizedAction(KConfig& cfg)
{
    if ( !cfg.hasKey( "X-KDE-AuthorizeAction") )
    {
        return true;
    }

    QStringList list = cfg.readListEntry("X-KDE-AuthorizeAction");
    if (kapp && !list.isEmpty())
    {
        for(QStringList::ConstIterator it = list.begin();
            it != list.end();
            ++it)
        {
            if (!KAuthorized::authorize((*it).stripWhiteSpace()))
            {
                return false;
            }
        }
    }

    return true;
}


void KonqPopupMenu::setup(KonqPopupFlags kpf)
{
    assert( m_lstItems.count() >= 1 );

    m_ownActions.setWidget( this );

    const bool bIsLink  = (kpf & IsLink);
    bool currentDir     = false;
    bool sReading       = true;
    bool sDeleting      = ( d->m_itemFlags & KParts::BrowserExtension::NoDeletion ) == 0;
    bool sMoving        = sDeleting;
    bool sWriting       = sDeleting && m_lstItems.first()->isWritable();
    m_sMimeType         = m_lstItems.first()->mimetype();
    QString mimeGroup   = m_sMimeType.left(m_sMimeType.find('/'));
    mode_t mode         = m_lstItems.first()->mode();
    bool isDirectory    = S_ISDIR(mode);
    bool bTrashIncluded = false;
    bool mediaFiles     = false;
    bool isLocal        = m_lstItems.first()->isLocalFile()
                       || m_lstItems.first()->url().protocol()=="media"
                       || m_lstItems.first()->url().protocol()=="system";
    bool isTrashLink     = false;
    m_lstPopupURLs.clear();
    int id = 0;
    setFont(KGlobalSettings::menuFont());
    m_pluginList.setAutoDelete( true );
    m_ownActions.setHighlightingEnabled( true );

    attrName = QLatin1String( "name" );

    prepareXMLGUIStuff();
    m_builder = new KonqPopupMenuGUIBuilder( this );
    m_factory = new KXMLGUIFactory( m_builder );

    KURL url;
    KFileItemListIterator it ( m_lstItems );
    QStringList mimeTypeList;
    // Check whether all URLs are correct
    for ( ; it.current(); ++it )
    {
        url = (*it)->url();

        // Build the list of URLs
        m_lstPopupURLs.append( url );

        // Determine if common mode among all URLs
        if ( mode != (*it)->mode() )
            mode = 0; // modes are different => reset to 0

        // Determine if common mimetype among all URLs
        if ( m_sMimeType != (*it)->mimetype() )
        {
            m_sMimeType = QString::null; // mimetypes are different => null

            if ( mimeGroup != (*it)->mimetype().left((*it)->mimetype().find('/')))
                mimeGroup = QString::null; // mimetype groups are different as well!
        }

        if ( mimeTypeList.findIndex( (*it)->mimetype() ) == -1 )
            mimeTypeList << (*it)->mimetype();

        if ( isLocal && !url.isLocalFile() && url.protocol() != "media" && url.protocol() != "system" )
            isLocal = false;

        if ( !bTrashIncluded && (
             ( url.protocol() == "trash" && url.path().length() <= 1 )
             || url.url() == "system:/trash" || url.url() == "system:/trash/" ) ) {
            bTrashIncluded = true;
            isLocal = false;
        }

        if ( sReading )
            sReading = KProtocolInfo::supportsReading( url );

        if ( sWriting )
            sWriting = KProtocolInfo::supportsWriting( url ) && (*it)->isWritable();

        if ( sDeleting )
            sDeleting = KProtocolInfo::supportsDeleting( url );

        if ( sMoving )
            sMoving = KProtocolInfo::supportsMoving( url );
        if ( (*it)->mimetype().startsWith("media/") )
            mediaFiles = true;
    }
    url = m_sViewURL;
    url.cleanPath();

    //check if url is current directory
    if ( m_lstItems.count() == 1 )
    {
        KURL firstPopupURL( m_lstItems.first()->url() );
        firstPopupURL.cleanPath();
        //kdDebug(1203) << "View path is " << url.url() << endl;
        //kdDebug(1203) << "First popup path is " << firstPopupURL.url() << endl;
        currentDir = firstPopupURL.equals( url, true /* ignore_trailing */ );
        if ( isLocal && m_sMimeType == "application/x-desktop" ) {
            KSimpleConfig cfg( firstPopupURL.path(), true );
            cfg.setDesktopGroup();
            isTrashLink = ( cfg.readEntry("Type") == "Link" && cfg.readEntry("URL") == "trash:/" );
        }

        if ( isTrashLink ) {
            sDeleting = false;
        }
    }
    const bool isSingleLocal = m_lstItems.count() == 1 && isLocal;

    m_info.m_Reading = sReading;
    m_info.m_Writing = sWriting;
    m_info.m_Deleting = sDeleting;
    m_info.m_Moving = sMoving;
    m_info.m_TrashIncluded = bTrashIncluded;

    // isCurrentTrash: popup on trash:/ itself, or on the trash.desktop link
    bool isCurrentTrash = ( m_lstItems.count() == 1 && bTrashIncluded ) || isTrashLink;
    bool isIntoTrash = ( url.protocol() == "trash" || url.url().startsWith( "system:/trash" ) ) && !isCurrentTrash; // trashed file, not trash:/ itself
    //kdDebug() << "isLocal=" << isLocal << " url=" << url << " isCurrentTrash=" << isCurrentTrash << " isIntoTrash=" << isIntoTrash << " bTrashIncluded=" << bTrashIncluded << endl;
    bool isSingleMedium = m_lstItems.count() == 1 && mediaFiles;
    clear();

    //////////////////////////////////////////////////////////////////////////

    KAction * act;

    if (!isCurrentTrash)
        addMerge( "konqueror" );

    bool isKDesktop = Q3CString( kapp->name() ) == "kdesktop";
    KAction *actNewWindow = 0;

    if (( kpf & ShowProperties ) && isKDesktop &&
        !KAuthorized::authorizeKAction("editable_desktop_icons"))
    {
        kpf &= ~ShowProperties; // remove flag
    }

    // Either 'newview' is in the actions we're given (probably in the tabhandling group)
    // or we need to insert it ourselves (e.g. for kdesktop). In the first case, actNewWindow must remain 0.
    if ( ((kpf & ShowNewWindow) != 0) && sReading )
    {
        QString openStr = isKDesktop ? i18n( "&Open" ) : i18n( "Open in New &Window" );
        actNewWindow = new KAction( openStr, "window_new", 0, this, SLOT( slotPopupNewView() ), &m_ownActions, "newview" );
    }

    if ( actNewWindow && !isKDesktop )
    {
        if (isCurrentTrash)
            actNewWindow->setToolTip( i18n( "Open the trash in a new window" ) );
        else if (isSingleMedium)
            actNewWindow->setToolTip( i18n( "Open the medium in a new window") );
        else
            actNewWindow->setToolTip( i18n( "Open the document in a new window" ) );
    }

    if ( S_ISDIR(mode) && sWriting && !isCurrentTrash ) // A dir, and we can create things into it
    {
        if ( currentDir && m_pMenuNew ) // Current dir -> add the "new" menu
        {
            // As requested by KNewMenu :
            m_pMenuNew->slotCheckUpToDate();
            m_pMenuNew->setPopupFiles( m_lstPopupURLs );

            KonqXMLGUIClient::addAction( m_pMenuNew );

            KonqXMLGUIClient::addSeparator();
        }
        else
        {
            if (d->m_itemFlags & KParts::BrowserExtension::ShowCreateDirectory)
            {
                KAction *actNewDir = new KAction( i18n( "Create &Folder..." ), "folder_new", 0, this, SLOT( slotPopupNewDir() ), &m_ownActions, "newdir" );
                KonqXMLGUIClient::addAction( actNewDir );
                KonqXMLGUIClient::addSeparator();
            }
        }
    } else if ( isIntoTrash ) {
        // Trashed item, offer restoring
        act = new KAction( i18n( "&Restore" ), 0, this, SLOT( slotPopupRestoreTrashedItems() ), &m_ownActions, "restore" );
        KonqXMLGUIClient::addAction( act );
    }

    if (d->m_itemFlags & KParts::BrowserExtension::ShowNavigationItems)
    {
        if (d->m_itemFlags & KParts::BrowserExtension::ShowUp)
            KonqXMLGUIClient::addAction( "up" );
        KonqXMLGUIClient::addAction( "back" );
        KonqXMLGUIClient::addAction( "forward" );
        if (d->m_itemFlags & KParts::BrowserExtension::ShowReload)
            KonqXMLGUIClient::addAction( "reload" );
        KonqXMLGUIClient::addSeparator();
    }

    // "open in new window" is either provided by us, or by the tabhandling group
    if (actNewWindow)
    {
        KonqXMLGUIClient::addAction( actNewWindow );
        KonqXMLGUIClient::addSeparator();
    }
    KonqXMLGUIClient::addGroup( "tabhandling" ); // includes a separator

    if ( !bIsLink )
    {
        if ( !currentDir && sReading ) {
            if ( sDeleting ) {
                KonqXMLGUIClient::addAction( "cut" );
            }
            KonqXMLGUIClient::addAction( "copy" );
        }

        if ( S_ISDIR(mode) && sWriting ) {
            if ( currentDir )
                KonqXMLGUIClient::addAction( "paste" );
            else
                KonqXMLGUIClient::addAction( "pasteto" );
        }
        if ( !currentDir )
        {
            if ( m_lstItems.count() == 1 && sMoving )
                KonqXMLGUIClient::addAction( "rename" );

            bool addTrash = false;
            bool addDel = false;

            if ( sMoving && !isIntoTrash && !isTrashLink )
                addTrash = true;

            if ( sDeleting ) {
                if ( !isLocal )
                    addDel = true;
                else if (KApplication::keyboardMouseState() & Qt::ShiftModifier) {
                    addTrash = false;
                    addDel = true;
                }
                else {
                    KConfigGroup configGroup( kapp->config(), "KDE" );
                    if ( configGroup.readBoolEntry( "ShowDeleteCommand", false ) )
                        addDel = true;
                }
            }

            if ( addTrash )
                KonqXMLGUIClient::addAction( "trash" );
            if ( addDel )
                KonqXMLGUIClient::addAction( "del" );
        }
    }
    if ( isCurrentTrash )
    {
        act = new KAction( i18n( "&Empty Trash Bin" ), "emptytrash", 0, this, SLOT( slotPopupEmptyTrashBin() ), &m_ownActions, "empytrash" );
        KonqXMLGUIClient::addAction( act );
    }
    KonqXMLGUIClient::addGroup( "editactions" );

    if (d->m_itemFlags & KParts::BrowserExtension::ShowTextSelectionItems) {
      KonqXMLGUIClient::addMerge( 0 );
      m_factory->addClient( this );
      return;
    }

    if ( !isCurrentTrash && !isIntoTrash && (d->m_itemFlags & KParts::BrowserExtension::ShowBookmark))
    {
        KonqXMLGUIClient::addSeparator();
        QString caption;
        if (currentDir)
        {
           bool httpPage = (m_sViewURL.protocol().find("http", 0, false) == 0);
           if (httpPage)
              caption = i18n("&Bookmark This Page");
           else
              caption = i18n("&Bookmark This Location");
        }
        else if (S_ISDIR(mode))
           caption = i18n("&Bookmark This Folder");
        else if (bIsLink)
           caption = i18n("&Bookmark This Link");
        else
           caption = i18n("&Bookmark This File");

        act = new KAction( caption, "bookmark_add", 0, this, SLOT( slotPopupAddToBookmark() ), &m_ownActions, "bookmark_add" );
        if (m_lstItems.count() > 1)
            act->setEnabled(false);
        if (KAuthorized::authorizeKAction("bookmarks"))
            KonqXMLGUIClient::addAction( act );
        if (bIsLink)
            KonqXMLGUIClient::addGroup( "linkactions" );
    }

    //////////////////////////////////////////////////////

    PopupServices s;
    KURL urlForServiceMenu( m_lstItems.first()->url() );

    // 1 - Look for builtin and user-defined services
    if ( m_sMimeType == "application/x-desktop" && isSingleLocal ) // .desktop file
    {
        // get builtin services, like mount/unmount
        s.builtin = KDEDesktopMimeType::builtinServices( m_lstItems.first()->url() );
        const QString path = m_lstItems.first()->url().path();
        KSimpleConfig cfg( path, true );
        cfg.setDesktopGroup();
        const QString priority = cfg.readEntry("X-KDE-Priority");
        const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );
        if ( cfg.readEntry("Type") == "Link" ) {
           urlForServiceMenu = cfg.readEntry("URL");
           // TODO: Do we want to make all the actions apply on the target
           // of the .desktop file instead of the .desktop file itself?
        }
        ServiceList* list = s.selectList( priority, submenuName );
        (*list) = KDEDesktopMimeType::userDefinedServices( path, cfg, url.isLocalFile() );
    }

    if ( sReading )
    {

        // 2 - Look for "servicesmenus" bindings (konqueror-specific user-defined services)

        // first check the .directory if this is a directory
        if (isDirectory && isSingleLocal)
        {
            QString dotDirectoryFile = m_lstItems.first()->url().path(1).append(".directory");
            KSimpleConfig cfg( dotDirectoryFile, true );
            cfg.setDesktopGroup();

            if (KIOSKAuthorizedAction(cfg))
            {
                const QString priority = cfg.readEntry("X-KDE-Priority");
                const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );
                ServiceList* list = s.selectList( priority, submenuName );
                (*list) += KDEDesktopMimeType::userDefinedServices( dotDirectoryFile, cfg, true );
            }
        }

        QStringList dirs = KGlobal::dirs()->findDirs( "data", "konqueror/servicemenus/" );
        QStringList::ConstIterator dIt = dirs.begin();
        QStringList::ConstIterator dEnd = dirs.end();

        for (; dIt != dEnd; ++dIt )
        {
            QDir dir( *dIt );

            QStringList entries = dir.entryList( "*.desktop", QDir::Files );
            QStringList::ConstIterator eIt = entries.begin();
            QStringList::ConstIterator eEnd = entries.end();

            for (; eIt != eEnd; ++eIt )
            {
                KSimpleConfig cfg( *dIt + *eIt, true );
                cfg.setDesktopGroup();

                if (!KIOSKAuthorizedAction(cfg))
                {
                    continue;
                }

                if ( cfg.hasKey( "X-KDE-ShowIfRunning" ) )
                {
                    const QString app = cfg.readEntry( "X-KDE-ShowIfRunning" );
                    if ( !kapp->dcopClient()->isApplicationRegistered( app.utf8() ) )
                        continue;
                }
		if ( cfg.hasKey( "X-KDE-ShowIfDcopCall" ) )
		{
		    QString dcopcall = cfg.readEntry( "X-KDE-ShowIfDcopCall" );
		    const Q3CString app = dcopcall.section(' ', 0,0).utf8();

		    //if( !kapp->dcopClient()->isApplicationRegistered( app ))
		    //	continue; //app does not exist so cannot send call

		    QByteArray dataToSend;
		    QDataStream dataStream(&dataToSend, QIODevice::WriteOnly);
		    dataStream << m_lstPopupURLs;

		    DCOPCString replyType;
		    QByteArray replyData;
		    DCOPCString object =    dcopcall.section(' ', 1,-2).utf8();
		    QString function =  dcopcall.section(' ', -1);
		    if(!function.endsWith("(KURL::List)")) {
			kdWarning() << "Desktop file " << *eIt << " contains an invalid X-KDE-ShowIfDcopCall - the function must take the exact parameter (KURL::List) and must be specified." << endl;
			continue; //Be safe.
		    }

		    if(!kapp->dcopClient()->call( app, object,
				    function.utf8(),
				    dataToSend, replyType, replyData, true, 1000))
			continue;
		    if(replyType != "bool" || !replyData[0])
			continue;

		}
                if ( cfg.hasKey( "X-KDE-Protocol" ) )
                {
                    const QString protocol = cfg.readEntry( "X-KDE-Protocol" );
                    if ( protocol != urlForServiceMenu.protocol() )
                        continue;
                }
                else if ( urlForServiceMenu.protocol() == "trash" || urlForServiceMenu.url().startsWith( "system:/trash" ) )
                {
                    // Require servicemenus for the trash to ask for protocol=trash explicitely.
                    // Trashed files aren't supposed to be available for actions.
                    // One might want a servicemenu for trash.desktop itself though.
                    continue;
                }

                if ( cfg.hasKey( "X-KDE-Require" ) )
                {
                    const QStringList capabilities = cfg.readListEntry( "X-KDE-Require" );
                    if ( capabilities.contains( "Write" ) && !sWriting )
                        continue;
                }
                if ( (cfg.hasKey( "Actions" ) || cfg.hasKey( "X-KDE-GetActionMenu") ) && cfg.hasKey( "ServiceTypes" ) )
                {
                    const QStringList types = cfg.readListEntry( "ServiceTypes" );
                    const QStringList excludeTypes = cfg.readListEntry( "ExcludeServiceTypes" );
                    bool ok = false;

                    // check for exact matches or a typeglob'd mimetype if we have a mimetype
                    for (QStringList::ConstIterator it = types.begin();
                         it != types.end() && !ok;
                         ++it)
                    {
                        // first check if we have an all mimetype
                        bool checkTheMimetypes = false;
                        if (*it == "all/all" ||
                            *it == "allfiles" /*compat with KDE up to 3.0.3*/)
                        {
                            checkTheMimetypes = true;
                        }

                        // next, do we match all files?
                        if (!ok &&
                            !isDirectory &&
                            *it == "all/allfiles")
                        {
                            checkTheMimetypes = true;
                        }

                        // if we have a mimetype, see if we have an exact or a type globbed match
                        if (!ok &&
                            (!m_sMimeType.isEmpty() &&
                              *it == m_sMimeType) ||
                            (!mimeGroup.isEmpty() &&
                             ((*it).right(1) == "*" &&
                              (*it).left((*it).find('/')) == mimeGroup)))
                        {
                            checkTheMimetypes = true;
                        }

                        if (checkTheMimetypes)
                        {
                            ok = true;
                            for (QStringList::ConstIterator itex = excludeTypes.begin(); itex != excludeTypes.end(); ++itex)
                            {
                                if( ((*itex).right(1) == "*" && (*itex).left((*itex).find('/')) == mimeGroup) ||
                                    ((*itex) == m_sMimeType) )
                                {
                                    ok = false;
                                    break;
                                }
                            }
                        }
                    }

                    if ( ok )
                    {
                        const QString priority = cfg.readEntry("X-KDE-Priority");
                        const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );

                        ServiceList* list = s.selectList( priority, submenuName );
                        (*list) += KDEDesktopMimeType::userDefinedServices( *dIt + *eIt, cfg, url.isLocalFile(), m_lstPopupURLs );
		    }

                }
            }
        }

        KTrader::OfferList offers;

        if (KAuthorized::authorizeKAction("openwith"))
        {
            QString constraint = "Type == 'Application' and DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'";
            QString subConstraint = " and '%1' in ServiceTypes";

            QStringList::ConstIterator it = mimeTypeList.begin();
            QStringList::ConstIterator end = mimeTypeList.end();
            Q_ASSERT( it != end );
            QString first = *it;
            ++it;
            while ( it != end ) {
                constraint += subConstraint.arg( *it );
                ++it;
            }

            offers = KTrader::self()->query( first, constraint );
        }

        //// Ok, we have everything, now insert

        m_mapPopup.clear();
        m_mapPopupServices.clear();
        // "Open With..." for folders is really not very useful, especially for remote folders.
        // (media:/something, or trash:/, or ftp://...)
        if ( !isDirectory || isLocal )
        {
            if ( hasAction() )
                KonqXMLGUIClient::addSeparator();

            if ( !offers.isEmpty() )
            {
                // First block, app and preview offers
                id = 1;

                QDomElement menu = domElement();

                if ( offers.count() > 1 ) // submenu 'open with'
                {
                    menu = domDocument().createElement( "menu" );
                    menu.setAttribute( "name", "openwith submenu" );
                    domElement().appendChild( menu );
                    QDomElement text = domDocument().createElement( "text" );
                    menu.appendChild( text );
                    text.appendChild( domDocument().createTextNode( i18n("&Open With") ) );
                }

                KTrader::OfferList::ConstIterator it = offers.begin();
                for( ; it != offers.end(); it++ )
                {
                    if ((*it)->noDisplay())
                        continue;

                    Q3CString nam;
                    nam.setNum( id );

                    QString actionName( (*it)->name() );
                    if ( menu == domElement() ) // no submenu -> prefix single offer
                        actionName = i18n( "Open with %1" ).arg( actionName );

                    act = new KAction( actionName, (*it)->pixmap( KIcon::Small ), 0,
                                       this, SLOT( slotRunService() ),
                                       &m_ownActions, nam.prepend( "appservice_" ) );
                    KonqXMLGUIClient::addAction( act, menu );

                    m_mapPopup[ id++ ] = *it;
                }

                QString openWithActionName;
                if ( menu != domElement() ) // submenu
                {
                    KonqXMLGUIClient::addSeparator( menu );
                    openWithActionName = i18n( "&Other..." );
                }
                else
                {
                    openWithActionName = i18n( "&Open With..." );
                }
                KAction *openWithAct = new KAction( openWithActionName, 0, this, SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
                KonqXMLGUIClient::addAction( openWithAct, menu );
            }
            else // no app offers -> Open With...
            {
                act = new KAction( i18n( "&Open With..." ), 0, this, SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
                KonqXMLGUIClient::addAction( act );
            }

        }
        KonqXMLGUIClient::addGroup( "preview" );
    }

    // Second block, builtin + user
    QDomElement actionMenu = domElement();
    int userItemCount = 0;
    if (s.user.count() + s.userSubmenus.count() +
        s.userPriority.count() + s.userPrioritySubmenus.count() > 1)
    {
        // we have more than one item, so let's make a submenu
        actionMenu = domDocument().createElement( "menu" );
        actionMenu.setAttribute( "name", "actions submenu" );
        domElement().appendChild( actionMenu );
        QDomElement text = domDocument().createElement( "text" );
        actionMenu.appendChild( text );
        text.appendChild( domDocument().createTextNode( i18n("Ac&tions") ) );
    }

    userItemCount += insertServicesSubmenus(s.userPrioritySubmenus, actionMenu, false);
    userItemCount += insertServices(s.userPriority, actionMenu, false);

    // see if we need to put a separator between our priority items and our regular items
    if (userItemCount > 0 &&
        (s.user.count() > 0 ||
         s.userSubmenus.count() > 0 ||
         s.builtin.count() > 0) &&
         actionMenu.lastChild().toElement().tagName().lower() != "separator")
    {
        QDomElement separator = domDocument().createElement( "separator" );
        actionMenu.appendChild(separator);
    }
	QDomElement element = domElement( );
    userItemCount += insertServicesSubmenus(s.userSubmenus, actionMenu, false);
    userItemCount += insertServices(s.user, actionMenu, false);
    userItemCount += insertServices(s.builtin, element, true);

    userItemCount += insertServicesSubmenus(s.userToplevelSubmenus, element, false);
    userItemCount += insertServices(s.userToplevel, element, false);

    if ( userItemCount > 0 )
    {
        addPendingSeparator();
    }

    if ( !isCurrentTrash && !isIntoTrash && !mediaFiles && sReading )
        addPlugins(); // now it's time to add plugins

    if ( KPropertiesDialog::canDisplay( m_lstItems ) && (kpf & ShowProperties) )
    {
        act = new KAction( i18n( "&Properties" ), 0, this, SLOT( slotPopupProperties() ),
                           &m_ownActions, "properties" );
        KonqXMLGUIClient::addAction( act );
    }

    while ( !domElement().lastChild().isNull() &&
            domElement().lastChild().toElement().tagName().lower() == "separator" )
        domElement().removeChild( domElement().lastChild() );

    if ( isDirectory && isLocal )
    {
        if ( KFileShare::authorization() == KFileShare::Authorized )
        {
            KonqXMLGUIClient::addSeparator();
            act = new KAction( i18n("Share"), 0, this, SLOT( slotOpenShareFileDialog() ),
                               &m_ownActions, "sharefile" );
            KonqXMLGUIClient::addAction( act );
        }
    }

    KonqXMLGUIClient::addMerge( 0 );
    //kdDebug() << k_funcinfo << domDocument().toString() << endl;

    m_factory->addClient( this );
}

void KonqPopupMenu::slotOpenShareFileDialog()
{
    KPropertiesDialog* dlg = showPropertiesDialog();
    dlg->showFileSharingPage();
}

KonqPopupMenu::~KonqPopupMenu()
{
  m_pluginList.clear();
  delete m_factory;
  delete m_builder;
  delete d;
  //kdDebug(1203) << "~KonqPopupMenu leave" << endl;
}

void KonqPopupMenu::setURLTitle( const QString& urlTitle )
{
    d->m_urlTitle = urlTitle;
}

void KonqPopupMenu::slotPopupNewView()
{
  KURL::List::ConstIterator it = m_lstPopupURLs.begin();
  for ( ; it != m_lstPopupURLs.end(); it++ )
    (void) new KRun(*it,this);
}

void KonqPopupMenu::slotPopupNewDir()
{
  if (m_lstPopupURLs.empty())
    return;

  KonqOperations::newDir(d->m_parentWidget, m_lstPopupURLs.first());
}

void KonqPopupMenu::slotPopupEmptyTrashBin()
{
  KonqOperations::emptyTrash();
}

void KonqPopupMenu::slotPopupRestoreTrashedItems()
{
  KonqOperations::restoreTrashedItems( m_lstPopupURLs );
}

void KonqPopupMenu::slotPopupOpenWith()
{
  KRun::displayOpenWithDialog( m_lstPopupURLs );
}

void KonqPopupMenu::slotPopupAddToBookmark()
{
  KBookmarkGroup root;
  if ( m_lstPopupURLs.count() == 1 ) {
    KURL url = m_lstPopupURLs.first();
    QString title = d->m_urlTitle.isEmpty() ? url.prettyURL() : d->m_urlTitle;
    root = m_pManager->addBookmarkDialog( url.prettyURL(), title );
  }
  else
  {
    root = m_pManager->root();
    KURL::List::ConstIterator it = m_lstPopupURLs.begin();
    for ( ; it != m_lstPopupURLs.end(); it++ )
      root.addBookmark( m_pManager, (*it).prettyURL(), (*it) );
  }
  m_pManager->emitChanged( root );
}

void KonqPopupMenu::slotRunService()
{
  Q3CString senderName = sender()->name();
  int id = senderName.mid( senderName.find( '_' ) + 1 ).toInt();

  // Is it a usual service (application)
  QMap<int,KService::Ptr>::Iterator it = m_mapPopup.find( id );
  if ( it != m_mapPopup.end() )
  {
    KRun::run( **it, m_lstPopupURLs );
    return;
  }

  // Is it a service specific to desktop entry files ?
  QMap<int,KDEDesktopMimeType::Service>::Iterator it2 = m_mapPopupServices.find( id );
  if ( it2 != m_mapPopupServices.end() )
  {
      KDEDesktopMimeType::executeService( m_lstPopupURLs, it2.data() );
  }

  return;
}

void KonqPopupMenu::slotPopupMimeType()
{
    KonqOperations::editMimeType( m_sMimeType );
}

void KonqPopupMenu::slotPopupProperties()
{
    (void)showPropertiesDialog();
}

KPropertiesDialog* KonqPopupMenu::showPropertiesDialog()
{
    // It may be that the kfileitem was created by hand
    // (see KonqKfmIconView::slotMouseButtonPressed)
    // In that case, we can get more precise info in the properties
    // (like permissions) if we stat the URL.
    if ( m_lstItems.count() == 1 )
    {
        KFileItem * item = m_lstItems.first();
        if (item->entry().count() == 0) // this item wasn't listed by a slave
        {
            // KPropertiesDialog will use stat to get more info on the file
            return new KPropertiesDialog( item->url(), d->m_parentWidget );
        }
    }
    return new KPropertiesDialog( m_lstItems, d->m_parentWidget );
}

KAction *KonqPopupMenu::action( const QDomElement &element ) const
{
  QByteArray name = element.attribute( attrName ).toLatin1();

  KAction *res = m_ownActions.action( name.data() );

  if ( !res )
    res = m_actions.action( name.data() );

  if ( !res && m_pMenuNew && strcmp( name.data(), m_pMenuNew->name() ) == 0 )
    return m_pMenuNew;

  return res;
}

KActionCollection *KonqPopupMenu::actionCollection() const
{
    return const_cast<KActionCollection *>( &m_ownActions );
}

QString KonqPopupMenu::mimeType() const
{
    return m_sMimeType;
}

KonqPopupMenu::ProtocolInfo KonqPopupMenu::protocolInfo() const
{
    return m_info;
}

void KonqPopupMenu::addPlugins()
{
    // search for Konq_PopupMenuPlugins inspired by simons kpropsdlg
    //search for a plugin with the right protocol
    KTrader::OfferList plugin_offers;
    unsigned int pluginCount = 0;
    plugin_offers = KTrader::self()->query( m_sMimeType.isNull() ? QLatin1String( "all/all" ) : m_sMimeType, "'KonqPopupMenu/Plugin' in ServiceTypes");
    if ( plugin_offers.isEmpty() )
        return; // no plugins installed do not bother about it

    KTrader::OfferList::ConstIterator iterator = plugin_offers.begin();
    KTrader::OfferList::ConstIterator end = plugin_offers.end();

    addGroup( "plugins" );
    // travers the offerlist
    for(; iterator != end; ++iterator, ++pluginCount ) {
        //kdDebug() << (*iterator)->library() << endl;
        KonqPopupMenuPlugin *plugin =
            KParts::ComponentFactory::
            createInstanceFromLibrary<KonqPopupMenuPlugin>( QFile::encodeName( (*iterator)->library() ),
                                                            this,
                                                            (*iterator)->name().latin1() );
        if ( !plugin )
            continue;
        QString pluginClientName = QString::fromLatin1( "Plugin%1" ).arg( pluginCount );
        addMerge( pluginClientName );
        plugin->domDocument().documentElement().setAttribute( "name", pluginClientName );
        m_pluginList.append( plugin );
        insertChildClient( plugin );
    }

    // ## Where is this used?
    addMerge( "plugins" );
}

KURL KonqPopupMenu::url() const // ### should be viewURL()
{
  return m_sViewURL;
}

KFileItemList KonqPopupMenu::fileItemList() const
{
  return m_lstItems;
}

KURL::List KonqPopupMenu::popupURLList() const
{
  return m_lstPopupURLs;
}

/**
        Plugin
*/

KonqPopupMenuPlugin::KonqPopupMenuPlugin( KonqPopupMenu *parent, const char *name )
    : QObject( parent, name )
{
}

KonqPopupMenuPlugin::~KonqPopupMenuPlugin()
{
}

#include "konq_popupmenu.moc"
