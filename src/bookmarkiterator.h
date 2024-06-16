/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2010 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef __bookmarkiterator_h
#define __bookmarkiterator_h

#include <KBookmark>
#include <QList>
#include <QObject>

class KBookmarkModel;
class BookmarkIteratorHolder;

/**
 * A bookmark iterator goes through every bookmark and performs an asynchronous
 * action (e.g. downloading the favicon or testing whether the url exists).
 */
class BookmarkIterator : public QObject
{
    Q_OBJECT

public:
    BookmarkIterator(BookmarkIteratorHolder *holder, const QList<KBookmark> &bks);
    ~BookmarkIterator() override;
    BookmarkIteratorHolder *holder() const
    {
        return m_holder;
    }
    KBookmarkModel *model();
    void delayedEmitNextOne();
    virtual void cancel() = 0;

public Q_SLOTS:
    void nextOne();

protected:
    virtual void doAction() = 0;
    virtual bool isApplicable(const KBookmark &bk) const = 0;
    KBookmark currentBookmark();

private:
    KBookmark m_bk;
    QList<KBookmark> m_bookmarkList;
    BookmarkIteratorHolder *m_holder;
};

/**
 * The "bookmark iterator holder" handles all concurrent iterators for a given
 * functionality: e.g. all favicon iterators.
 *
 * BookmarkIteratorHolder is the base class for the favicon and testlink holders.
 */
class BookmarkIteratorHolder : public QObject
{
    Q_OBJECT
public:
    void cancelAllItrs();
    void removeIterator(BookmarkIterator *);
    void insertIterator(BookmarkIterator *);
    void addAffectedBookmark(const QString &address);
    KBookmarkModel *model()
    {
        return m_model;
    }

Q_SIGNALS:
    void setCancelEnabled(bool canCancel);

protected:
    BookmarkIteratorHolder(QObject *parent, KBookmarkModel *model);
    ~BookmarkIteratorHolder() override
    {
    }
    void doIteratorListChanged();
    int count() const
    {
        return m_iterators.count();
    }
    KBookmarkModel *m_model;

private:
    QString m_affectedBookmark;
    QList<BookmarkIterator *> m_iterators;
};

#endif
