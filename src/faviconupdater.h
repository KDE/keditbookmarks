/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef FAVICONUPDATER_H
#define FAVICONUPDATER_H

#include <KBookmark>

#include <KIO/Job>
#include <KParts/ReadOnlyPart>
#include <QUrl>
#include <kparts/part.h>

class FavIconWebGrabber : public QObject
{
    Q_OBJECT
public:
    FavIconWebGrabber(KParts::ReadOnlyPart *part, const QUrl &url);
    ~FavIconWebGrabber() override
    {
    }

Q_SIGNALS:
    void done(bool succeeded, const QString &errorString);

private Q_SLOTS:
    void slotCanceled(const QString &errorString);
    void slotCompleted();

private:
    KParts::ReadOnlyPart *m_part;
    QUrl m_url;
};

class FavIconUpdater : public QObject
{
    Q_OBJECT

public:
    explicit FavIconUpdater(QObject *parent);
    ~FavIconUpdater() override;
    void downloadIcon(const KBookmark &bk);
    void downloadIconUsingWebBrowser(const KBookmark &bk, const QString &currentError);

private Q_SLOTS:
    void setIconUrl(const QUrl &iconURL);
    void slotResult(KJob *job);

Q_SIGNALS:
    void done(bool succeeded, const QString &error);

private:
    KParts::ReadOnlyPart *m_part;
    FavIconWebGrabber *m_webGrabber;
    KBookmark m_bk;
    bool webupdate;
};

#endif
