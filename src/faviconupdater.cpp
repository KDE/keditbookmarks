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

#include "moc_faviconupdater.cpp"
