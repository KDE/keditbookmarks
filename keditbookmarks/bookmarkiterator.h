// -*- mode:cperl; cperl-indent-level:4; cperl-continued-statement-offset:4; indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __bookmarkiterator_h
#define __bookmarkiterator_h

#include <qobject.h>
#include <qptrlist.h>
#include <kbookmark.h>

class KEBListViewItem;
class BookmarkIteratorHolder;

class BookmarkIterator : public QObject
{
   Q_OBJECT

public:
   BookmarkIterator(QValueList<KBookmark> bks);
   virtual ~BookmarkIterator();
   virtual BookmarkIteratorHolder* holder() const = 0;

public slots:
   void nextOne();
   void delayedEmitNextOne();
   void slotCancelTest(BookmarkIterator *t);

signals:
   void deleteSelf(BookmarkIterator *);

protected:
   virtual void doAction() = 0;
   virtual bool isApplicable(const KBookmark &bk) const = 0;
   KEBListViewItem* curItem() const;
   const KBookmark curBk() const;

private:
   KBookmark m_bk;
   QValueList<KBookmark> m_bklist;
};

class BookmarkIteratorHolder
{
public:
   void cancelAllItrs();
   void removeItr(BookmarkIterator*);
   void insertItr(BookmarkIterator*);
protected:
   BookmarkIteratorHolder();
   virtual ~BookmarkIteratorHolder() {};
   virtual void doItrListChanged() = 0;
   int count() const { return m_itrs.count(); }
private:
   QPtrList<BookmarkIterator> m_itrs;
};

#endif
