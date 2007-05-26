/* This file is part of the KDE project
   Copyright (C) 2005 Daniel Teske <teske@squorn.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "treeitem.h"
#include <kdebug.h>
#include <QtCore/QVector>

TreeItem::TreeItem(const KBookmark& bk, TreeItem * parent)
    : mparent(parent), mbk(bk)
{
    init = false;
#ifdef DEBUG_STUPID_QT
    deleted = false;
#endif
}

TreeItem::~TreeItem()
{
    qDeleteAll(children);
    children.clear();
}

TreeItem * TreeItem::child(int row)
{
#ifdef DEBUG_STUPID_QT
    if(deleted)
        kFatal()<<"child for deleted "<<endl;
#endif
    if(!init)
        initChildren();
    return children[row];
}

int TreeItem::childCount()
{
    if(!init)
        initChildren();
    return children.count();
}

TreeItem * TreeItem::parent()
{
#ifdef DEBUG_STUPID_QT
    if(deleted)
        kFatal()<<"parent for deleted "<<endl;
#endif
    return mparent;
}

#ifdef DEBUG_STUPID_QT
void TreeItem::markDelete()
{
    deleted = true;
    QList<TreeItem *>::iterator it, end;
    end = children.end();
    for(it = children.begin(); it != end; ++it)
    {
        (*it)->markDelete();
    }
}
#endif

void TreeItem::insertChildren(int first, int last)
{
    // Find child number last
    KBookmarkGroup parent = bookmark().toGroup();
    KBookmark child = parent.first();
    for(int j=0; j < last; ++j)
        child = parent.next(child);
    
    //insert children
    int i = last;
    do
    {
        children.insert(i, new TreeItem(child, this));
        child = parent.previous(child);
        --i;
    } while(i >= first);

}

void TreeItem::deleteChildren(int first, int last)
{
    kDebug()<<"deleteChildren of "<<bookmark().address()<<" "<<first<<" "<<last<<endl;
    QList<TreeItem *>::iterator firstIt, lastIt, it;
    firstIt = children.begin() + first;
    lastIt = children.begin() + last + 1;
    for( it = firstIt; it != lastIt; ++it)
    {
        kDebug()<<"removing child "<<(*it)->bookmark().address()<<endl;
#ifndef DEBUG_STUPID_QT
        delete *it;
#else
        (*it)->markDelete();
#endif
    }
    children.erase(firstIt, lastIt);
}

void TreeItem::moveChildren(int first, int last, TreeItem * newParent, int position)
{
    if(newParent != this)
    {
        for(int i = last; i>=first; --i)
        {
            TreeItem * item = children[i];
            item->mparent = newParent;
            newParent->children.insert(position, item);

            QList<TreeItem *>::iterator firstIt, lastIt;
            firstIt = children.begin() + first;
            lastIt = children.begin() + last + 1;
            children.erase(firstIt, lastIt);
        }
    }
    else
    {
        if(first > position)
        {
            // swap around 
            int tempPos = position;
            position = last + 1;
            last = first - 1;
            first = tempPos;
        }
        // Invariant first > position
        QVector<TreeItem *> temp;
        for(int i=first; i<=last; ++i)
            temp.append(children[i]);
        
        int count = (last-first + 1);
        for(int i=first; i+count<position; ++i)
            children[i] = children[i+count];

        for(int i = position - count; i < position; ++i)
            children[i] = temp[ i - position + count];

    }
}

KBookmark TreeItem::bookmark()
{
#ifdef DEBUG_STUPID_QT
    if(deleted)
        kFatal()<<"child for deleted "<<endl;
#endif
    return mbk;
}

void TreeItem::initChildren()
{
    init = true;
    if(mbk.isGroup())
    {
        KBookmarkGroup parent = mbk.toGroup();
        for(KBookmark child = parent.first(); child.hasParent(); child = parent.next(child) )
        {
            TreeItem * item = new TreeItem(child, this);
            children.append(item);
        }
    }
}

TreeItem * TreeItem::treeItemForBookmark(const KBookmark& bk)
{
    if(bk.address() == mbk.address())
        return this;
    QString commonParent = KBookmark::commonParent(bk.address(), mbk.address());
    if(commonParent == mbk.address()) //mbk is a parent of bk
    {
        QList<TreeItem *>::const_iterator it, end;
        end = children.constEnd();
        for( it = children.constBegin(); it != end; ++it)
        {
            KBookmark child = (*it)->bookmark();
            if( KBookmark::commonParent(child.address(), bk.address()) == child.address())
                    return (*it)->treeItemForBookmark(bk);
        }
        return 0;
    }
    else
    {
        if(parent() == 0)
            return 0;
        return parent()->treeItemForBookmark(bk);
    }
}

