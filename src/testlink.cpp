/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000, 2010 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

// Own
#include "testlink.h"

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
    , m_job(nullptr)
{
}

TestLinkItr::~TestLinkItr()
{
    if (m_job) {
        // //qCDebug(KEDITBOOKMARKS_LOG) << "JOB kill\n";
        m_job->disconnect(this);
        m_job->kill();
    }
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
    // qCDebug(KEDITBOOKMARKS_LOG);
    m_job = KIO::get(currentBookmark().url(), KIO::Reload, KIO::HideProgressInfo);
    m_job->addMetaData(QStringLiteral("cookies"), QStringLiteral("none"));
    m_job->addMetaData(QStringLiteral("errorPage"), QStringLiteral("false"));

    connect(m_job, &KIO::TransferJob::result, this, &TestLinkItr::slotJobResult);

    m_oldStatus = currentBookmark().metaDataItem(QStringLiteral("linkstate"));
    setStatus(i18n("Checking..."));
}

void TestLinkItr::slotJobResult(KJob *job)
{
    // qCDebug(KEDITBOOKMARKS_LOG);
    m_job = nullptr;

    KIO::TransferJob *transfer = static_cast<KIO::TransferJob *>(job);
    const QString modDate = transfer->queryMetaData(QStringLiteral("modified"));

    if (transfer->error()) {
        // qCDebug(KEDITBOOKMARKS_LOG)<<"***********"<<transfer->error()<<"  "<<transfer->isErrorPage();
        // can we assume that errorString will contain no entities?
        QString err = transfer->errorString();
        err.replace(QLatin1String("\n"), QLatin1String(" "));
        setStatus(err);
    } else {
        if (!modDate.isEmpty())
            setStatus(modDate);
        else
            setStatus(i18n("OK"));
    }

    holder()->addAffectedBookmark(KBookmark::parentAddress(currentBookmark().address()));
    delayedEmitNextOne();
}

void TestLinkItr::cancel()
{
    setStatus(m_oldStatus);
}

#include "moc_testlink.cpp"
