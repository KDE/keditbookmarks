/* This file is part of the KDE project
   Copyright (C) 2000, 2010 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "bookmarkiterator.h"
#include "kbookmarkmodel/model.h"
#include <KBookmarkManager>

#include <QTimer>

BookmarkIterator::BookmarkIterator(BookmarkIteratorHolder *holder, const QList<KBookmark> &bks)
    : QObject(holder)
    , m_bookmarkList(bks)
    , m_holder(holder)
{
    delayedEmitNextOne();
}

BookmarkIterator::~BookmarkIterator()
{
}

void BookmarkIterator::delayedEmitNextOne()
{
    QTimer::singleShot(1, this, &BookmarkIterator::nextOne);
}

KBookmark BookmarkIterator::currentBookmark()
{
    return m_bk;
}

void BookmarkIterator::nextOne()
{
    // //qCDebug(KEDITBOOKMARKS_LOG) << "BookmarkIterator::nextOne";

    // Look for an interesting bookmark
    while (!m_bookmarkList.isEmpty()) {
        KBookmark bk = m_bookmarkList.takeFirst();
        if (bk.hasParent() && isApplicable(bk)) {
            m_bk = bk;
            doAction();
            // Async action started, we'll have to come back later
            return;
        }
    }
    if (m_bookmarkList.isEmpty()) {
        holder()->removeIterator(this); // deletes "this"
        return;
    }
}

KBookmarkModel *BookmarkIterator::model()
{
    return m_holder->model();
}

/* --------------------------- */

BookmarkIteratorHolder::BookmarkIteratorHolder(QObject *parent, KBookmarkModel *model)
    : QObject(parent)
    , m_model(model)
{
    Q_ASSERT(m_model);
}

void BookmarkIteratorHolder::insertIterator(BookmarkIterator *itr)
{
    m_iterators.prepend(itr);
    doIteratorListChanged();
}

void BookmarkIteratorHolder::removeIterator(BookmarkIterator *itr)
{
    m_iterators.removeAll(itr);
    itr->deleteLater();
    doIteratorListChanged();
}

void BookmarkIteratorHolder::cancelAllItrs()
{
    const auto iterList = m_iterators;
    for (BookmarkIterator *iterator : iterList) {
        iterator->cancel();
    }
    qDeleteAll(m_iterators);
    m_iterators.clear();
    doIteratorListChanged();
}

void BookmarkIteratorHolder::addAffectedBookmark(const QString &address)
{
    // qCDebug(KEDITBOOKMARKS_LOG) << address;
    if (m_affectedBookmark.isNull())
        m_affectedBookmark = address;
    else
        m_affectedBookmark = KBookmark::commonParent(m_affectedBookmark, address);
    // qCDebug(KEDITBOOKMARKS_LOG) << "m_affectedBookmark is now" << m_affectedBookmark;
}

void BookmarkIteratorHolder::doIteratorListChanged()
{
    // qCDebug(KEDITBOOKMARKS_LOG) << count() << "iterators";
    Q_EMIT setCancelEnabled(count() > 0);
    if (count() == 0) {
        // qCDebug(KEDITBOOKMARKS_LOG) << "Notifying managers" << m_affectedBookmark;
        KBookmarkManager *mgr = m_model->bookmarkManager();
        model()->notifyManagers(mgr->findByAddress(m_affectedBookmark).toGroup());
        m_affectedBookmark.clear();
    }
}

#include "moc_bookmarkiterator.cpp"
