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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qclipboard.h>
#include <qcursor.h>
#include <qpopupmenu.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kbookmarkdrag.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kmessagebox.h>

#include <dcopclient.h>
#include <dcopref.h>

#include "bookmark_module.h"
#include "bookmark_item.h"
#include <konqbookmarkmanager.h>

KonqSidebarBookmarkModule::KonqSidebarBookmarkModule( KonqSidebarTree * parentTree )
    : QObject( 0L ), KonqSidebarTreeModule( parentTree ),
      m_topLevelItem( 0L ), m_ignoreOpenChange(true)
{
    // formats handled by KBookmarkDrag:
    QStringList formats;
    formats << "text/uri-list" << "application/x-xbel" << "text/plain";
    tree()->setDropFormats(formats);

    connect(tree(), SIGNAL(moved(QListViewItem*,QListViewItem*,QListViewItem*)),
            this,   SLOT(slotMoved(QListViewItem*,QListViewItem*,QListViewItem*)));
    connect(tree(), SIGNAL(dropped(KListView*,QDropEvent*,QListViewItem*,QListViewItem*)),
            this,   SLOT(slotDropped(KListView*,QDropEvent*,QListViewItem*,QListViewItem*)));

    connect(tree(), SIGNAL(expanded(QListViewItem*)),
            this,   SLOT(slotOpenChange(QListViewItem*)));
    connect(tree(), SIGNAL(collapsed(QListViewItem*)),
            this,   SLOT(slotOpenChange(QListViewItem*)));

    m_collection = new KActionCollection( this, "bookmark actions" );
    (void) new KAction( i18n("&Create New Folder"), "folder_new", 0, this,
                        SLOT( slotCreateFolder() ), m_collection, "create_folder");
    (void) new KAction( i18n("Delete Folder"), "editdelete", 0, this,
                        SLOT( slotDelete() ), m_collection, "delete_folder");
    (void) new KAction( i18n("Delete Bookmark"), "editdelete", 0, this,
                        SLOT( slotDelete() ), m_collection, "delete_bookmark");
    (void) new KAction( i18n("Properties"), "edit", 0, this,
                        SLOT( slotProperties() ), m_collection, "item_properties");
    (void) new KAction( i18n("Open in New Window"), "window_new", 0, this,
                        SLOT( slotOpenNewWindow() ), m_collection, "open_window");
    (void) new KAction( i18n("Open in New Tab"), "tab_new", 0, this,
                        SLOT( slotOpenTab() ), m_collection, "open_tab");
    (void) new KAction( i18n("Open Folder in Tabs"), "tab_new", 0, this,
                        SLOT( slotOpenTab() ), m_collection, "folder_open_tabs");
    (void) new KAction( i18n("Copy Link Address"), "editcopy", 0, this,
                        SLOT( slotCopyLocation() ), m_collection, "copy_location");

    connect( KonqBookmarkManager::self(), SIGNAL(changed(const QString &, const QString &) ),
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

void KonqSidebarBookmarkModule::showPopupMenu()
{
    KonqSidebarBookmarkItem *bi = dynamic_cast<KonqSidebarBookmarkItem*>( tree()->selectedItem() );
    if (!bi)
        return;

    // see if the newTab() dcop function is available (i.e. the sidebar is embedded into konqueror)
    bool tabSupport = false;
    DCOPRef ref(kapp->dcopClient()->appId(), tree()->topLevelWidget()->name());
    DCOPReply reply = ref.call("functions()");
    if (reply.isValid()) {
        QCStringList funcs;
        reply.get(funcs, "QCStringList");
        for (QCStringList::ConstIterator it = funcs.begin(); it != funcs.end(); ++it) {
            if ((*it) == "void newTab(QString url)") {
                tabSupport = true;
                break;
            }
        }
    }

    QPopupMenu *menu = new QPopupMenu;

    if (bi->bookmark().isGroup()) {
        if (tabSupport) {
            m_collection->action("folder_open_tabs")->plug(menu);
            menu->insertSeparator();
        }
        m_collection->action("create_folder")->plug(menu);
        m_collection->action("delete_folder")->plug(menu);
    } else {
        if (tabSupport)
            m_collection->action("open_tab")->plug(menu);
        m_collection->action("open_window")->plug(menu);
        m_collection->action("copy_location")->plug(menu);
        menu->insertSeparator();
        m_collection->action("create_folder")->plug(menu);
        m_collection->action("delete_bookmark")->plug(menu);
    }
    menu->insertSeparator();
    m_collection->action("item_properties")->plug(menu);

    menu->exec( QCursor::pos() );
    delete menu;
}

void KonqSidebarBookmarkModule::slotMoved(QListViewItem *i, QListViewItem*, QListViewItem *after)
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
    // try to get the parent group (assume that the QListViewItem has been reparented by KListView)...
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
            parentGroup = KonqBookmarkManager::self()->root();
    } else {
        // most probably the root...
        parentGroup = KonqBookmarkManager::self()->root();
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
        KonqBookmarkManager::self()->emitChanged( parentGroup );
    } else {
        int i = 0;
        while (true) {
            QChar c1 = oldAddress[i];
            QChar c2 = newAddress[i];
            if (c1 == QChar::null) {
                // oldParentGroup is probably parent of parentGroup.
                KonqBookmarkManager::self()->emitChanged( oldParentGroup );
                break;
            } else if (c2 == QChar::null) {
                // parentGroup is probably parent of oldParentGroup.
                KonqBookmarkManager::self()->emitChanged( parentGroup );
                break;
            } else {
                if (c1 == c2) {
                    // step to the next character.
                    ++i;
                } else {
                    // ugh... need to update both groups separately.
                    KonqBookmarkManager::self()->emitChanged( oldParentGroup );
                    KonqBookmarkManager::self()->emitChanged( parentGroup );
                    break;
                }
            }
        }
    }
}

void KonqSidebarBookmarkModule::slotDropped(KListView *, QDropEvent *e, QListViewItem *parent, QListViewItem *after)
{
    if (!KBookmarkDrag::canDecode(e))
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
        KonqSidebarBookmarkItem *p = dynamic_cast<KonqSidebarBookmarkItem*>(parent);
        if (!p)
            return;
        KBookmark bm = p->bookmark();
        if (bm.isGroup())
            parentGroup = bm.toGroup();
        else
            return;
    } else {
        // it's most probably the root...
        parentGroup = KonqBookmarkManager::self()->root();
    }

    QValueList<KBookmark> bookmarks = KBookmarkDrag::decode(e);

    // copy
    QValueList<KBookmark>::iterator it = bookmarks.begin();
    for (;it != bookmarks.end(); ++it) {
        // insert new item.
        parentGroup.moveItem(*it, afterBookmark);
    }

    KonqBookmarkManager::self()->emitChanged( parentGroup );
}

void KonqSidebarBookmarkModule::slotCreateFolder()
{
    KonqSidebarBookmarkItem *bi = dynamic_cast<KonqSidebarBookmarkItem*>( tree()->selectedItem() );
    if (!bi)
        return;

    KBookmarkGroup parentGroup;
    if (bi->bookmark().isGroup())
        parentGroup = bi->bookmark().toGroup();
    else
        parentGroup = bi->bookmark().parentGroup();

    KBookmark bookmark = parentGroup.createNewFolder(KonqBookmarkManager::self());
    parentGroup.moveItem(bookmark, bi->bookmark());

    KonqBookmarkManager::self()->emitChanged( parentGroup );
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
            folder ? i18n("Are you sure you wish to remove this bookmark folder?")
                    : i18n("Are you sure you wish to remove this bookmark?"),
            folder ? i18n("Bookmark Folder Deletion")
                    : i18n("Bookmark Deletion"),
            KGuiItem( i18n("&Delete"), "editdelete"), KStdGuiItem::cancel())
            != KMessageBox::Yes
        )
        return;

    KBookmarkGroup parentBookmark = bookmark.parentGroup();
    parentBookmark.deleteBookmark( bookmark );

    KonqBookmarkManager::self()->emitChanged( parentBookmark );
}

void makeTextNodeMod(KBookmark bk, const QString &m_nodename, const QString &m_newText) {
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

    QString folder = bookmark.isGroup() ? QString::null : bookmark.url().url();
    BookmarkEditDialog dlg( bookmark.fullText(), folder, 0, 0,
                            i18n("Bookmark Properties") );
    if ( dlg.exec() != KDialogBase::Accepted )
        return;

    makeTextNodeMod(bookmark, "title", dlg.finalTitle());
    if ( !dlg.finalUrl().isNull() )
        bookmark.internalElement().setAttribute("href", dlg.finalUrl());

    KBookmarkGroup parentBookmark = bookmark.parentGroup();
    KonqBookmarkManager::self()->emitChanged( parentBookmark );
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
    if (!bi)
        return;

    KBookmark bookmark = bi->bookmark();

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
}

void KonqSidebarBookmarkModule::slotCopyLocation()
{
    KonqSidebarBookmarkItem *bi = dynamic_cast<KonqSidebarBookmarkItem*>( tree()->selectedItem() );
    if (!bi)
        return;

    KBookmark bookmark = bi->bookmark();

    if ( !bookmark.isGroup() )
    {
        kapp->clipboard()->setData( KBookmarkDrag::newDrag(bookmark, 0),
                                    QClipboard::Selection );
        kapp->clipboard()->setData( KBookmarkDrag::newDrag(bookmark, 0),
                                    QClipboard::Clipboard );
    }
}

void KonqSidebarBookmarkModule::slotOpenChange(QListViewItem* i)
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
    KBookmarkGroup group = KonqBookmarkManager::self()->findByAddress( groupAddress ).toGroup();
    KonqSidebarBookmarkItem * item = findByAddress( groupAddress );
    Q_ASSERT(!group.isNull());
    Q_ASSERT(item);
    if (!group.isNull() && item)
    {
        // Delete all children of item
        QListViewItem * child = item->firstChild();
        while( child ) {
            QListViewItem * next = child->nextSibling();
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

    KBookmarkGroup root = KonqBookmarkManager::self()->root();
    fillGroup( m_topLevelItem, root );

    m_ignoreOpenChange = false;
}

void KonqSidebarBookmarkModule::fillGroup( KonqSidebarTreeItem * parentItem, KBookmarkGroup group )
{
    int n = 0;
    for ( KBookmark bk = group.first() ; !bk.isNull() ; bk = group.next(bk), ++n )
    {
        if ( !bk.isSeparator() ) // Separators don't look good in the tree IMHO
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
            else
                item->setExpandable( false );
        }
    }
}

// Borrowed from KEditBookmarks
KonqSidebarBookmarkItem * KonqSidebarBookmarkModule::findByAddress( const QString & address ) const
{
    QListViewItem * item = m_topLevelItem;
    // The address is something like /5/10/2
    QStringList addresses = QStringList::split('/',address);
    for ( QStringList::Iterator it = addresses.begin() ; it != addresses.end() ; ++it )
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
  : KDialogBase(parent, name, true, caption,
                (Ok|Cancel),
                Ok, false, KGuiItem()),
    m_title(0), m_location(0)
{
    setButtonOK( i18n( "&Update" ) );

    QWidget *main = new QWidget( this );
    setMainWidget( main );

    bool folder = url.isNull();
    QGridLayout *grid = new QGridLayout( main, 2, folder?1:2, spacingHint() );

    QLabel *nameLabel = new QLabel(i18n("Name:"), main, "title label");
    grid->addWidget(nameLabel, 0, 0);
    m_title = new KLineEdit(main, "title edit");
    m_title->setText(title);
    nameLabel->setBuddy(m_title);
    grid->addWidget(m_title, 0, 1);
    if(!folder) {
        QLabel *locationLabel = new QLabel(i18n("Location:"), main, "location label");
        grid->addWidget(locationLabel, 1, 0);
        m_location = new KLineEdit(main, "location edit");
        m_location->setText(url);
        locationLabel->setBuddy(m_location);
        grid->addWidget(m_location, 1, 1);
    }
}

void BookmarkEditDialog::slotOk()
{
    accept();
}

void BookmarkEditDialog::slotCancel()
{
    reject();
}

QString BookmarkEditDialog::finalUrl() const
{
    if (m_location!=0)
        return m_location->text();
    else
        return QString::null;
}

QString BookmarkEditDialog::finalTitle() const
{
    if (m_title!=0) 
        return m_title->text();
    else
        return QString::null;
}

extern "C"
{
   KonqSidebarTreeModule* create_konq_sidebartree_bookmarks(KonqSidebarTree* par,const bool)
	{
		return new KonqSidebarBookmarkModule(par);
	} 
}

#include "bookmark_module.moc"
