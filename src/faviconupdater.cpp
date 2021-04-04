// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

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

#include "faviconupdater.h"

#include "bookmarkiterator.h"
#include "toplevel.h"

#include "keditbookmarks_debug.h"
#include <KLocalizedString>

#include <KIO/FavIconRequestJob>
#include <kio/job.h>

#include <KParts/BrowserExtension>
#include <KParts/PartLoader>

FavIconUpdater::FavIconUpdater(QObject *parent)
    : QObject(parent)
{
    m_part = nullptr;
    m_webGrabber = nullptr;
}

void FavIconUpdater::downloadIcon(const KBookmark &bk)
{
    m_bk = bk;
    const QUrl url = bk.url();
    const QString favicon = KIO::favIconForUrl(url);
    if (!favicon.isEmpty()) {
        // qCDebug(KEDITBOOKMARKS_LOG) << "got favicon" << favicon;
        m_bk.setIcon(favicon);
        KEBApp::self()->notifyCommandExecuted();
        // //qCDebug(KEDITBOOKMARKS_LOG) << "emit done(true)";
        Q_EMIT done(true, QString());

    } else {
        // qCDebug(KEDITBOOKMARKS_LOG) << "no favicon found";
        webupdate = false;
        KIO::FavIconRequestJob *job = new KIO::FavIconRequestJob(url, KIO::Reload);
        connect(job, &KIO::FavIconRequestJob::result, this, &FavIconUpdater::slotResult);
    }
}

FavIconUpdater::~FavIconUpdater()
{
    delete m_webGrabber;
    delete m_part;
}

void FavIconUpdater::downloadIconUsingWebBrowser(const KBookmark &bk, const QString &currentError)
{
    // qCDebug(KEDITBOOKMARKS_LOG);
    m_bk = bk;
    webupdate = true;

    if (!m_part) {
        QString partLoadingError;
        KParts::ReadOnlyPart *part =
            KParts::PartLoader::createPartInstanceForMimeType<KParts::ReadOnlyPart>(QStringLiteral("text/html"), nullptr, this, &partLoadingError);
        if (!part) {
            Q_EMIT done(false, i18n("%1; no HTML component found (%2)", currentError, partLoadingError));
            return;
        }

        part->setProperty("pluginsEnabled", QVariant(false));
        part->setProperty("javaScriptEnabled", QVariant(false));
        part->setProperty("javaEnabled", QVariant(false));
        part->setProperty("autoloadImages", QVariant(false));

        KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(part);
        Q_ASSERT(ext);

        connect(ext, &KParts::BrowserExtension::setIconUrl, this, &FavIconUpdater::setIconUrl);

        m_part = part;
    }

    // The part isn't created by the webgrabber so that we can create the part
    // only once.
    delete m_webGrabber;
    m_webGrabber = new FavIconWebGrabber(m_part, bk.url());
    connect(m_webGrabber, &FavIconWebGrabber::done, this, &FavIconUpdater::done);
}

// khtml callback
void FavIconUpdater::setIconUrl(const QUrl &iconURL)
{
    KIO::FavIconRequestJob *job = new KIO::FavIconRequestJob(m_bk.url());
    job->setIconUrl(iconURL);
    connect(job, &KIO::FavIconRequestJob::result, this, &FavIconUpdater::slotResult);

    delete m_webGrabber;
    m_webGrabber = nullptr;
}

void FavIconUpdater::slotResult(KJob *job)
{
    KIO::FavIconRequestJob *requestJob = static_cast<KIO::FavIconRequestJob *>(job);
    if (job->error()) {
        if (!webupdate) {
            qCDebug(KEDITBOOKMARKS_LOG) << "favicon job failed, going to downloadIconUsingWebBrowser";
            // no icon found, try webupdater
            downloadIconUsingWebBrowser(m_bk, job->errorString());
        } else {
            qCDebug(KEDITBOOKMARKS_LOG) << "favicon job failed, emit done";
            // already tried webupdater
            Q_EMIT done(false, job->errorString());
        }
        return;
    }
    m_bk.setIcon(requestJob->iconFile());
    Q_EMIT done(true, QString());
}

/* -------------------------- */

FavIconWebGrabber::FavIconWebGrabber(KParts::ReadOnlyPart *part, const QUrl &url)
    : m_part(part)
    , m_url(url)
{
    // FIXME only connect to result?
    //  connect(part, SIGNAL(result(KIO::Job*job)),
    //          this, SLOT(slotCompleted()));
    connect(part, &KParts::ReadOnlyPart::canceled, this, &FavIconWebGrabber::slotCanceled);
    // clang-format off
    connect(part, SIGNAL(completed(bool)), this, SLOT(slotCompleted()));
    // clang-format on

    // the use of KIO rather than directly using KHTML is to allow silently abort on error
    // TODO: an alternative would be to derive from KHTMLPart and reimplement showError(KJob*).

    // qCDebug(KEDITBOOKMARKS_LOG) << "starting KIO::get() on" << m_url;
    KIO::Job *job = KIO::get(m_url, KIO::NoReload, KIO::HideProgressInfo);
    job->addMetaData(QStringLiteral("cookies"), QStringLiteral("none"));
    job->addMetaData(QStringLiteral("errorPage"), QStringLiteral("false"));
    connect(job, &KJob::result, this, &FavIconWebGrabber::slotFinished);
    // clang-format off
    connect(job, SIGNAL(mimetype(KIO::Job*,QString)), this, SLOT(slotMimetype(KIO::Job*,QString)));
    // clang-format on
}

void FavIconWebGrabber::slotMimetype(KIO::Job *job, const QString &type)
{
    Q_ASSERT(!job->error()); // can't be set already, surely?

    KIO::SimpleJob *sjob = static_cast<KIO::SimpleJob *>(job);
    m_url = sjob->url(); // allow for redirection
    sjob->putOnHold();

    // QString typeLocal = typeUncopied; // local copy
    qCDebug(KEDITBOOKMARKS_LOG) << "slotMimetype " << type << "calling openUrl on" << m_url;
    // TODO - what to do if typeLocal is not text/html ??

    m_part->openUrl(m_url);
}

void FavIconWebGrabber::slotFinished(KJob *job)
{
    if (job->error()) {
        qCDebug(KEDITBOOKMARKS_LOG) << job->errorString();
        Q_EMIT done(false, job->errorString());
        return;
    }
    // On success mimetype was emitted, so no need to do anything.
}

void FavIconWebGrabber::slotCompleted()
{
    qCDebug(KEDITBOOKMARKS_LOG);
    Q_EMIT done(true, QString());
}

void FavIconWebGrabber::slotCanceled(const QString &errorString)
{
    // qCDebug(KEDITBOOKMARKS_LOG) << errorString;
    Q_EMIT done(false, errorString);
}
