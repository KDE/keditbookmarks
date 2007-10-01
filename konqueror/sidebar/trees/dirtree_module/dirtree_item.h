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

#ifndef dirtree_item_h
#define dirtree_item_h

#include "konq_sidebartreeitem.h"
#include <kfileitem.h>
#include <kurl.h>
#include <QtCore/QStringList>
#include <konq_operations.h>

class QDropEvent;

class KonqSidebarDirTreeItem : public KonqSidebarTreeItem
{
public:
    KonqSidebarDirTreeItem( KonqSidebarTreeItem *parentItem, KonqSidebarTreeTopLevelItem *topLevelItem, const KFileItem &fileItem );
    KonqSidebarDirTreeItem( KonqSidebarTree *parent, KonqSidebarTreeTopLevelItem *topLevelItem, const KFileItem &fileItem );
    ~KonqSidebarDirTreeItem();

    KFileItem fileItem() const { return m_fileItem; }

    virtual void setOpen( bool open );

    virtual void paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment );

    virtual bool acceptsDrops( const Q3StrList & formats );
    virtual void drop( QDropEvent * ev );
    virtual bool populateMimeData( QMimeData* mimeData, bool move );

    virtual void middleButtonClicked();
    virtual void rightButtonPressed();

    virtual void paste();
    virtual void trash();
    virtual void del();
    virtual void rename(); // start a rename operation
    void rename( const QString & name ); // do the actual renaming

    // The URL to open when this link is clicked
    virtual KUrl externalURL() const;
    virtual QString externalMimeType() const;
    virtual QString toolTipText() const;

    virtual void itemSelected();

    void reset();

    bool hasStandardIcon();

    QString id;

private:
    void delOperation( KonqOperations::Operation method );
    KFileItem m_fileItem;
};

#endif
