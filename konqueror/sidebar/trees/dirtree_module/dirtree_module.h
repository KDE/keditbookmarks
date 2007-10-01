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

#ifndef dirtree_module_h
#define dirtree_module_h

#include <konq_sidebartreemodule.h>
#include <kfileitem.h>
#include <QtGui/QPixmap>
#include <Qt3Support/Q3Dict>
#include <Qt3Support/Q3PtrDict>

class KDirLister;
class KonqSidebarTree;
class KonqSidebarTreeItem;
class KonqSidebarDirTreeItem;

class KonqSidebarDirTreeModule : public QObject, public KonqSidebarTreeModule
{
    Q_OBJECT
public:
    KonqSidebarDirTreeModule( KonqSidebarTree * parentTree, bool );
    virtual ~KonqSidebarDirTreeModule();

    virtual void addTopLevelItem( KonqSidebarTreeTopLevelItem * item );

    virtual void openTopLevelItem( KonqSidebarTreeTopLevelItem * item );

    virtual void followURL( const KUrl & url );

    // Called by KonqSidebarDirTreeItem
    void openSubFolder( KonqSidebarTreeItem *item );
    void addSubDir( KonqSidebarTreeItem *item );
    void removeSubDir( KonqSidebarTreeItem *item, bool childrenonly = false );

private Q_SLOTS:
    void slotNewItems( const KFileItemList & );
    void slotRefreshItems( const QList<QPair<KFileItem, KFileItem> >& );
    void slotDeleteItem( const KFileItem &item );
    void slotRedirection( const KUrl & oldUrl, const KUrl & newUrl );
    void slotListingStopped( const KUrl & url );

private:
    //KonqSidebarTreeItem * findDir( const KUrl &_url );
    void listDirectory( KonqSidebarTreeItem *item );
    KUrl::List selectedUrls();

    // URL -> item
    // Each KonqSidebarDirTreeItem is indexed on item->id() and
    // all item->alias'es
    Q3Dict<KonqSidebarTreeItem> m_dictSubDirs;

    // KFileItem -> item
    QHash<KFileItem, KonqSidebarTreeItem*> m_ptrdictSubDirs;

    KDirLister * m_dirLister;

    KUrl m_selectAfterOpening;

    KonqSidebarTreeTopLevelItem * m_topLevelItem;

    bool m_showArchivesAsFolders;
};


#endif
