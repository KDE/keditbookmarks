/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000, 2010 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

// Own
#include "testlink.h"

// Qt
#include <QNetworkAccessManager>
#include <QNetworkReply>

// KDE
#include <KLocalizedString>

// Local
#include "kbookmarkmodel/model.h"

TestLinkItrHolder::TestLinkItrHolder(QObject *parent, KBookmarkModel *model)
    : BookmarkIteratorHolder(parent, model)
{
}

/* -------------------------- */

TestLinkItr::TestLinkItr(BookmarkIteratorHolder *holder, const QList<KBookmark> &bks)
    : BookmarkIterator(holder, bks)
{
}

TestLinkItr::~TestLinkItr()
{
}

void TestLinkItr::setStatus(const QString &text)
{
    currentBookmark().setMetaDataItem(QStringLiteral("linkstate"), text);
    model()->emitDataChanged(currentBookmark());
}

bool TestLinkItr::isApplicable(const KBookmark &bk) const
{
    return !bk.isGroup() && !bk.isSeparator();
}

void TestLinkItr::doAction()
{
    static QNetworkAccessManager nam;

    QNetworkRequest request(currentBookmark().url());
    request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);

    QNetworkReply *reply = nam.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() != QNetworkReply::NoError) {
            QString err = reply->errorString();
            err.replace(QLatin1String("\n"), QLatin1String(" "));
            setStatus(err);
        } else {
            setStatus(i18n("OK"));
        }

        holder()->addAffectedBookmark(KBookmark::parentAddress(currentBookmark().address()));
        delayedEmitNextOne();
    });

    m_oldStatus = currentBookmark().metaDataItem(QStringLiteral("linkstate"));
    setStatus(i18n("Checking..."));
}

void TestLinkItr::cancel()
{
    setStatus(m_oldStatus);
}

#include "moc_testlink.cpp"
