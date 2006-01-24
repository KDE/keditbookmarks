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

#ifndef bookmark_item_h
#define bookmark_item_h

#include <konq_sidebartreeitem.h>
#include <kbookmark.h>

/**
 * A bookmark item
 */
class KonqSidebarBookmarkItem : public KonqSidebarTreeItem
{
public:
    KonqSidebarBookmarkItem( KonqSidebarTreeItem *parentItem, KonqSidebarTreeTopLevelItem *topLevelItem,
                      const KBookmark & bk, int key );

    virtual ~KonqSidebarBookmarkItem() {}

    virtual bool populateMimeData( QMimeData* mimeData, bool move );

    virtual void middleButtonClicked();
    virtual void rightButtonPressed();

    virtual void del();

    // The URL to open when this link is clicked
    virtual KUrl externalURL() const;

    // overwrite this if you want a tooltip shown on your item
    virtual QString toolTipText() const;

    // Called when this item is selected
    virtual void itemSelected();

    virtual QString key( int column, bool /*ascending*/ ) const;

    virtual KBookmark &bookmark();

private:
    KBookmark m_bk;
    int m_key;
};

#endif
