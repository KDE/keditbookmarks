/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#include "faviconupdater.h"

#include "toplevel.h"

#include "keditbookmarks_debug.h"
#include <KLocalizedString>

#include <KIO/FavIconRequestJob>
#include <KIO/SimpleJob>
#include <KIO/TransferJob>
#include <KParts/NavigationExtension>
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
        auto partResult = KParts::PartLoader::instantiatePartForMimeType<KParts::ReadOnlyPart>(QStringLiteral("text/html"), nullptr, this);
        if (!partResult) {
            Q_EMIT done(false, i18n("%1; no HTML component found (%2)", currentError, partResult.errorString));
            return;
        }

        auto part = partResult.plugin;
        part->setProperty("pluginsEnabled", QVariant(false));
        part->setProperty("javaScriptEnabled", QVariant(false));
        part->setProperty("javaEnabled", QVariant(false));
        part->setProperty("autoloadImages", QVariant(false));
        KParts::NavigationExtension *ext = KParts::NavigationExtension::childObject(part);

        if (!ext) {
            Q_EMIT done(false, i18n("No browser component found"));
            return;
        }

        connect(ext, &KParts::NavigationExtension::setIconUrl, this, &FavIconUpdater::setIconUrl);
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
    connect(part, &KParts::ReadOnlyPart::completed, this, &FavIconWebGrabber::slotCompleted);

    m_part->openUrl(m_url);
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

#include "moc_faviconupdater.cpp"
