/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Own
#include "bookmark_module.h"
#include "bookmark_item.h"

// Qt
#include <QtCore/QList>
#include <QtGui/QClipboard>
#include <QtGui/QCursor>
#include <QtGui/QGridLayout>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QMenu>

// KDE
#include <k3bookmarkdrag.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kdebug.h>
#include <kicon.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kstandardaction.h>

KBookmarkManager* s_bookmarkManager = 0;

KonqSidebarBookmarkModule::KonqSidebarBookmarkModule( KonqSidebarTree * parentTree )
    : QObject( 0L ), KonqSidebarTreeModule( parentTree ),
      m_topLevelItem( 0L ), m_ignoreOpenChange(true)
{
    if (!s_bookmarkManager)
        s_bookmarkManager = KBookmarkManager::userBookmarksManager();

    // formats handled by K3BookmarkDrag:
    QStringList formats;
    formats << "text/uri-list" << "application/x-xbel" << "text/plain";
    tree()->setDropFormats(formats);

    connect(tree(), SIGNAL(moved(Q3ListViewItem*,Q3ListViewItem*,Q3ListViewItem*)),
            this,  SLOT(slotMoved(Q3ListViewItem*,Q3ListViewItem*,Q3ListViewItem*)));
    connect(tree(), SIGNAL(dropped(K3ListView*,QDropEvent*,Q3ListViewItem*,Q3ListViewItem*)),
            this,  SLOT(slotDropped(K3ListView*,QDropEvent*,Q3ListViewItem*,Q3ListViewItem*)));

    connect(tree(), SIGNAL(expanded(Q3ListViewItem*)),
            this,  SLOT(slotOpenChange(Q3ListViewItem*)));
    connect(tree(), SIGNAL(collapsed(Q3ListViewItem*)),
            this,  SLOT(slotOpenChange(Q3ListViewItem*)));

    m_collection = new KActionCollection( this );
    QAction *action = m_collection->addAction("create_folder");
    action->setIcon( KIcon("folder-new") );
    action->setText( i18n("&Create New Folder") );
    connect(action, SIGNAL(triggered(bool)), SLOT( slotCreateFolder() ));
    action = m_collection->addAction("delete_folder");
    action->setIcon( KIcon("edit-delete") );
    action->setText( i18n("Delete Folder") );
    connect(action, SIGNAL(triggered(bool)), SLOT( slotDelete() ));
    action = m_collection->addAction("delete_bookmark");
    action->setIcon( KIcon("edit-delete") );
    action->setText( i18n("Delete Bookmark") );
    connect(action, SIGNAL(triggered(bool)), SLOT( slotDelete() ));
    action = m_collection->addAction("item_properties");
    action->setIcon( KIcon("document-properties") );
    action->setText( i18n("Properties") );
    connect(action, SIGNAL(triggered(bool)), SLOT( slotProperties() ));
    action = m_collection->addAction("open_window");
    action->setIcon( KIcon("window-new") );
    action->setText( i18n("Open in New Window") );
    connect(action, SIGNAL(triggered(bool)), SLOT( slotOpenNewWindow() ));
    action = m_collection->addAction("open_tab");
    action->setIcon( KIcon("tab-new") );
    action->setText( i18n("Open in New Tab") );
    connect(action, SIGNAL(triggered(bool)), SLOT( slotOpenTab() ));
    action = m_collection->addAction("folder_open_tabs");
    action->setIcon( KIcon("tab-new") );
    action->setText( i18n("Open Folder in Tabs") );
    connect(action, SIGNAL(triggered(bool)), SLOT( slotOpenTab() ));
    action = m_collection->addAction("copy_location");
    action->setIcon( KIcon("edit-copy") );
    action->setText( i18n("Copy Link Address") );
    connect(action, SIGNAL(triggered(bool)), SLOT( slotCopyLocation() ));

    m_collection->addAction( "edit_bookmarks",
                             KStandardAction::editBookmarks( s_bookmarkManager,
                                                             SLOT( slotEditBookmarks() ), this ) );

    connect( s_bookmarkManager, SIGNAL(changed(const QString &, const QString &) ),
             SLOT( slotBookmarksChanged(const QString &) ) );
}

KonqSidebarBookmarkModule::~KonqSidebarBookmarkModule()
{
}

void KonqSidebarBookmarkModule::addTopLevelItem( KonqSidebarTreeTopLevelItem * item )
{
    m_ignoreOpenChange = true;

    m_topLevelItem = item;
    fillListView();

    m_ignoreOpenChange = false;
}

bool KonqSidebarBookmarkModule::handleTopLevelContextMenu( KonqSidebarTreeTopLevelItem *, const QPoint& )
{
    QMenu *menu = new QMenu;

    if (tree()->tabSupport()) {
	menu->addAction( m_collection->action("folder_open_tabs") );
	menu->addSeparator();
    }
    menu->addAction( m_collection->action("create_folder") );

    menu->addSeparator();
    menu->addAction( m_collection->action("edit_bookmarks") );

    menu->exec( QCursor::pos() );
    delete menu;

    return true;
}

void KonqSidebarBookmarkModule::showPopupMenu()
{
    KonqSidebarBookmarkItem *bi = dynamic_cast<KonqSidebarBookmarkItem*>( tree()->selectedItem() );
    if (!bi)
        return;

    bool tabSupported = tree()->tabSupport();
    QMenu *menu = new QMenu;

    if (bi->bookmark().isGroup()) {
        if (tabSupported) {
            menu->addAction( m_collection->action("folder_open_tabs") );
            menu->addSeparator();
        }
        menu->addAction( m_collection->action("create_folder") );
        menu->addAction( m_collection->action("item_properties") );
        menu->addSeparator();
        menu->addAction( m_collection->action("delete_folder") );
    } else {
        menu->addAction( m_collection->action("open_window") );
        if (tabSupported)
            menu->addAction( m_collection->action("open_tab") );
        menu->addAction( m_collection->action("copy_location") );
        menu->addSeparator();
        menu->addAction( m_collection->action("create_folder") );
        menu->addAction( m_collection->action("item_properties") );
        menu->addSeparator();
        menu->addAction( m_collection->action("delete_bookmark") );
    }

    menu->exec( QCursor::pos() );
    delete menu;
}

void KonqSidebarBookmarkModule::slotMoved(Q3ListViewItem *i, Q3ListViewItem*, Q3ListViewItem *after)
{
    KonqSidebarBookmarkItem *item = dynamic_cast<KonqSidebarBookmarkItem*>( i );
    if (!item)
        return;
    KBookmark bookmark = item->bookmark();

    KBookmark afterBookmark;
    KonqSidebarBookmarkItem *afterItem = dynamic_cast<KonqSidebarBookmarkItem*>(after);
    if (afterItem)
        afterBookmark = afterItem->bookmark();

    KBookmarkGroup oldParentGroup = bookmark.parentGroup();
    KBookmarkGroup parentGroup;
    // try to get the parent group (assume that the QListViewItem has been reparented by K3ListView)...
    // if anything goes wrong, use the root.
    if (item->parent()) {
        bool error = false;

        KonqSidebarBookmarkItem *parent = dynamic_cast<KonqSidebarBookmarkItem*>( (item->parent()) );
        if (!parent) {
            error = true;
        } else {
            if (parent->bookmark().isGroup())
                parentGroup = parent->bookmark().toGroup();
            else
                error = true;
        }

        if (error)
            parentGroup = s_bookmarkManager->root();
    } else {
        // No parent! This means the user dropped it before the top level item
        // And K3ListView has moved the item there, we need to correct it
        tree()->moveItem(item, m_topLevelItem, 0L);
        parentGroup = s_bookmarkManager->root();
    }

    // remove the old reference.
    oldParentGroup.deleteBookmark( bookmark );

    // insert the new item.
    parentGroup.moveItem(bookmark, afterBookmark);

    // inform others about the changed groups. quite expensive, so do
    // our best to update them in only one emitChanged call.
    QString oldAddress = oldParentGroup.address();
    QString newAddress = parentGroup.address();
    if (oldAddress == newAddress) {
        s_bookmarkManager->emitChanged( parentGroup );
    } else {
        int i = 0;
        while (true) {
            QChar c1 = oldAddress[i];
            QChar c2 = newAddress[i];
            if (c1 == QChar::null) {
                // oldParentGroup is probably parent of parentGroup.
                s_bookmarkManager->emitChanged( oldParentGroup );
                break;
            } else if (c2 == QChar::null) {
                // parentGroup is probably parent of oldParentGroup.
                s_bookmarkManager->emitChanged( parentGroup );
                break;
            } else {
                if (c1 == c2) {
                    // step to the next character.
                    ++i;
                } else {
                    // ugh... need to update both groups separately.
                    s_bookmarkManager->emitChanged( oldParentGroup );
                    s_bookmarkManager->emitChanged( parentGroup );
                    break;
                }
            }
        }
    }
}

void KonqSidebarBookmarkModule::slotDropped(K3ListView *, QDropEvent *e, Q3ListViewItem *parent, Q3ListViewItem *after)
{
    if (!KBookmark::List::canDecode(e->mimeData()))
        return;

    KBookmark afterBookmark;
    KonqSidebarBookmarkItem *afterItem = dynamic_cast<KonqSidebarBookmarkItem*>(after);
    if (afterItem)
        afterBookmark = afterItem->bookmark();

    KBookmarkGroup parentGroup;
    // try to get the parent group...
    if (after) {
        parentGroup = afterBookmark.parentGroup();
    } else if (parent) {
        if(KonqSidebarBookmarkItem *p = dynamic_cast<KonqSidebarBookmarkItem*>(parent))
        {
            if (!p)
                return;
            KBookmark bm = p->bookmark();
            if (bm.isGroup())
                parentGroup = bm.toGroup();
            else
                return;
        }
        else if(parent == m_topLevelItem)
        {
            parentGroup = s_bookmarkManager->root();
        }
    } else {
        // it's most probably the root...
        parentGroup = s_bookmarkManager->root();
    }

    const KBookmark::List bookmarks = KBookmark::List::fromMimeData(e->mimeData());

    // copy
    KBookmark::List::const_iterator it = bookmarks.constBegin();
    for (;it != bookmarks.constEnd(); ++it) {
        // insert new item.
        parentGroup.moveItem(*it, afterBookmark);
    }

    s_bookmarkManager->emitChanged( parentGroup );
}

void KonqSidebarBookmarkModule::slotCreateFolder()
{
    KonqSidebarBookmarkItem *bi = dynamic_cast<KonqSidebarBookmarkItem*>( tree()->selectedItem() );
    KBookmarkGroup parentGroup;
    if (bi)
    {
	if (bi->bookmark().isGroup())
	    parentGroup = bi->bookmark().toGroup();
	else
	    parentGroup = bi->bookmark().parentGroup();
    }
    else if(tree()->selectedItem() == m_topLevelItem)
    {
	parentGroup = s_bookmarkManager->root();
    }
    else
	return;

    KBookmark bookmark = parentGroup.createNewFolder("");//TODO use Dialog
    if(bi && !(bi->bookmark().isGroup()))
	parentGroup.moveItem(bookmark, bi->bookmark());

    s_bookmarkManager->emitChanged( parentGroup );
}

void KonqSidebarBookmarkModule::slotDelete()
{
    KonqSidebarBookmarkItem *bi = dynamic_cast<KonqSidebarBookmarkItem*>( tree()->selectedItem() );
    if (!bi)
        return;

    KBookmark bookmark = bi->bookmark();
    bool folder = bookmark.isGroup();

    if (KMessageBox::warningYesNo(
            tree(),
            folder ? i18n("Are you sure you wish to remove the bookmark folder\n\"%1\"?", bookmark.text())
                    : i18n("Are you sure you wish to remove the bookmark\n\"%1\"?", bookmark.text()),
            folder ? i18n("Bookmark Folder Deletion")
                    : i18n("Bookmark Deletion"),
            KStandardGuiItem::del(), KStandardGuiItem::cancel())
            != KMessageBox::Yes
        )
        return;

    KBookmarkGroup parentBookmark = bookmark.parentGroup();
    parentBookmark.deleteBookmark( bookmark );

    s_bookmarkManager->emitChanged( parentBookmark );
}

void makeTextNodeMod(const KBookmark &bk, const QString &m_nodename, const QString &m_newText) {
    QDomNode subnode = bk.internalElement().namedItem(m_nodename);
    if (subnode.isNull()) {
        subnode = bk.internalElement().ownerDocument().createElement(m_nodename);
        bk.internalElement().appendChild(subnode);
    }

    if (subnode.firstChild().isNull()) {
        QDomText domtext = subnode.ownerDocument().createTextNode("");
        subnode.appendChild(domtext);
    }

    QDomText domtext = subnode.firstChild().toText();

    QString m_oldText = domtext.data();
    domtext.setData(m_newText);
}

void KonqSidebarBookmarkModule::slotProperties(KonqSidebarBookmarkItem *bi)
{
    if (!bi) {
        bi = dynamic_cast<KonqSidebarBookmarkItem*>( tree()->selectedItem() );
        if (!bi)
            return;
    }

    KBookmark bookmark = bi->bookmark();

    QString folder = bookmark.isGroup() ? QString() : bookmark.url().pathOrUrl();
    BookmarkEditDialog dlg( bookmark.fullText(), folder, 0, 0,
                            i18n("Bookmark Properties") );
    if ( dlg.exec() != KDialog::Accepted )
        return;

    makeTextNodeMod(bookmark, "title", dlg.finalTitle());
    if ( !dlg.finalUrl().isNull() )
    {
        KUrl u(dlg.finalUrl());
        bookmark.internalElement().setAttribute("href", u.url());
    }

    KBookmarkGroup parentBookmark = bookmark.parentGroup();
    s_bookmarkManager->emitChanged( parentBookmark );
}

void KonqSidebarBookmarkModule::slotOpenNewWindow()
{
    KonqSidebarBookmarkItem *bi = dynamic_cast<KonqSidebarBookmarkItem*>( tree()->selectedItem() );
    if (!bi)
        return;

    emit tree()->createNewWindow( bi->bookmark().url() );
}

void KonqSidebarBookmarkModule::slotOpenTab()
{
    KonqSidebarBookmarkItem *bi = dynamic_cast<KonqSidebarBookmarkItem*>( tree()->selectedItem() );
    KBookmark bookmark;
    if (bi)
    {
	bookmark = bi->bookmark();
    }
    else if(tree()->selectedItem() == m_topLevelItem)
	bookmark = s_bookmarkManager->root();
    else
	return;

#ifdef __GNUC__
#warning newTab hack, must be done otherwise
#endif
#if 0
    DCOPRef ref(kapp->dcopClient()->appId(), tree()->topLevelWidget()->name());

    if (bookmark.isGroup()) {
        KBookmarkGroup group = bookmark.toGroup();
        bookmark = group.first();
        while (!bookmark.isNull()) {
            if (!bookmark.isGroup() && !bookmark.isSeparator())
                ref.call( "newTab(QString)", bookmark.url().url() );
            bookmark = group.next(bookmark);
        }
    } else {
        ref.call( "newTab(QString)", bookmark.url().url() );
    }
#endif
}

void KonqSidebarBookmarkModule::slotCopyLocation()
{
    KonqSidebarBookmarkItem *bi = dynamic_cast<KonqSidebarBookmarkItem*>( tree()->selectedItem() );
    if (!bi)
        return;

    KBookmark bookmark = bi->bookmark();

    if ( !bookmark.isGroup() )
    {
        qApp->clipboard()->setData( K3BookmarkDrag::newDrag(bookmark, 0),
                                    QClipboard::Selection );
        qApp->clipboard()->setData( K3BookmarkDrag::newDrag(bookmark, 0),
                                    QClipboard::Clipboard );
    }
}

void KonqSidebarBookmarkModule::slotOpenChange(Q3ListViewItem* i)
{
    if (m_ignoreOpenChange)
        return;

    KonqSidebarBookmarkItem *bi = dynamic_cast<KonqSidebarBookmarkItem*>( i );
    if (!bi)
        return;

    KBookmark bookmark = bi->bookmark();

    bool open = bi->isOpen();

    if (!open)
        m_folderOpenState.remove(bookmark.address()); // no need to store closed folders...
    else
        m_folderOpenState[bookmark.address()] = open;
}

void KonqSidebarBookmarkModule::slotBookmarksChanged( const QString & groupAddress )
{
    m_ignoreOpenChange = true;

    // update the right part of the tree
    KBookmarkGroup group = s_bookmarkManager->findByAddress( groupAddress ).toGroup();
    KonqSidebarBookmarkItem * item = findByAddress( groupAddress );
    Q_ASSERT(!group.isNull());
    Q_ASSERT(item);
    if (!group.isNull() && item)
    {
        // Delete all children of item
        Q3ListViewItem * child = item->firstChild();
        while( child ) {
            Q3ListViewItem * next = child->nextSibling();
            delete child;
            child = next;
        }
        fillGroup( item, group );
    }

    m_ignoreOpenChange = false;
}

void KonqSidebarBookmarkModule::fillListView()
{
    m_ignoreOpenChange = true;

    KBookmarkGroup root = s_bookmarkManager->root();
    fillGroup( m_topLevelItem, root );

    m_ignoreOpenChange = false;
}

void KonqSidebarBookmarkModule::fillGroup( KonqSidebarTreeItem * parentItem, const KBookmarkGroup &group )
{
    int n = 0;
    for ( KBookmark bk = group.first() ; !bk.isNull() ; bk = group.next(bk), ++n )
    {
            KonqSidebarBookmarkItem * item = new KonqSidebarBookmarkItem( parentItem, m_topLevelItem, bk, n );
            if ( bk.isGroup() )
            {
                KBookmarkGroup grp = bk.toGroup();
                fillGroup( item, grp );

                QString address(grp.address());
                if (m_folderOpenState.contains(address))
                    item->setOpen(m_folderOpenState[address]);
                else
                    item->setOpen(false);
            }
            else if ( bk.isSeparator() )
                item->setVisible( false );
            else
                item->setExpandable( false );
    }
}

// Borrowed from KEditBookmarks
KonqSidebarBookmarkItem * KonqSidebarBookmarkModule::findByAddress( const QString & address ) const
{
    Q3ListViewItem * item = m_topLevelItem;
    // The address is something like /5/10/2
    const QStringList addresses = address.split('/', QString::SkipEmptyParts);
    for ( QStringList::const_iterator it = addresses.constBegin() ; it != addresses.constEnd() ; ++it )
    {
        uint number = (*it).toUInt();
        item = item->firstChild();
        for ( uint i = 0 ; i < number ; ++i )
            item = item->nextSibling();
    }
    Q_ASSERT(item);
    return static_cast<KonqSidebarBookmarkItem *>(item);
}

// Borrowed&modified from KBookmarkMenu...
BookmarkEditDialog::BookmarkEditDialog(const QString& title, const QString& url,
                                       QWidget * parent, const char * name, const QString& caption )
  : KDialog( parent ),
    m_title(0), m_location(0)
{
    setObjectName( name );
    setModal( true );
    setCaption( caption );
    setButtons( Ok|Cancel );
    showButtonSeparator( false );

    setButtonText( Ok, i18n( "&Update" ) );

    QWidget *main = new QWidget( this );
    setMainWidget( main );

    bool folder = url.isNull();
    QGridLayout *grid = new QGridLayout( main );
    grid->setSpacing( spacingHint() );

    QLabel *nameLabel = new QLabel(i18n("Name:"), main);
    nameLabel->setObjectName("title label");
    grid->addWidget(nameLabel, 0, 0);
    m_title = new KLineEdit(main);
    m_title->setText(title);
    nameLabel->setBuddy(m_title);
    grid->addWidget(m_title, 0, 1);
    if(!folder) {
        QLabel *locationLabel = new QLabel(i18n("Location:"), main);
        locationLabel->setObjectName("location label");
        grid->addWidget(locationLabel, 1, 0);
        m_location = new KLineEdit(main);
        m_location->setText(url);
        locationLabel->setBuddy(m_location);
        grid->addWidget(m_location, 1, 1);
    }
    main->setMinimumSize( 300, 0 );
}

QString BookmarkEditDialog::finalUrl() const
{
    if (m_location!=0)
        return m_location->text();
    else
        return QString();
}

QString BookmarkEditDialog::finalTitle() const
{
    return m_title->text();
}

extern "C"
{
   KDE_EXPORT KonqSidebarTreeModule* create_konq_sidebartree_bookmarks(KonqSidebarTree* par,const bool)
	{
		return new KonqSidebarBookmarkModule(par);
	}
}

#include "bookmark_module.moc"
