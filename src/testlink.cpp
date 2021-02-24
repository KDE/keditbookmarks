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

// Own
#include "testlink.h"

// KDE
#include "keditbookmarks_debug.h"
#include <KLocalizedString>

// Local
#include "kbookmarkmodel/commands.h"
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

    if (transfer->error() || transfer->isErrorPage()) {
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
