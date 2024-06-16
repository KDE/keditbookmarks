/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef __testlink_h
#define __testlink_h

#include <QObject>

#include <KBookmark>
#include <KIO/TransferJob>

#include "bookmarkiterator.h"
class KBookmarkModel;

class TestLinkItrHolder : public BookmarkIteratorHolder
{
public:
    TestLinkItrHolder(QObject *parent, KBookmarkModel *model);
};

class TestLinkItr : public BookmarkIterator
{
    Q_OBJECT

public:
    TestLinkItr(BookmarkIteratorHolder *holder, const QList<KBookmark> &bks);
    ~TestLinkItr() override;

    void cancel() override;

public Q_SLOTS:
    void slotJobResult(KJob *job);

private:
    void setStatus(const QString &text);
    void doAction() override;
    bool isApplicable(const KBookmark &bk) const override;

    KIO::TransferJob *m_job;
    QString m_oldStatus;
};

#endif
