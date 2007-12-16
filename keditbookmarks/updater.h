// -*- c-basic-offset: 4; indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef __updater_h
#define __updater_h

#include <kbookmark.h>
#include "favicon_interface.h"

#include <kparts/part.h>
#include <kparts/browserinterface.h>

class FavIconWebGrabber : public QObject
{
    Q_OBJECT
public:
    FavIconWebGrabber(KParts::ReadOnlyPart *part, const KUrl &url);
    ~FavIconWebGrabber() {}

protected Q_SLOTS:
    void slotMimetype(KIO::Job *job, const QString &_type);
    void slotFinished(KJob *job);

private:
    KParts::ReadOnlyPart *m_part;
    KUrl m_url;
};

class FavIconBrowserInterface;

class FavIconUpdater : public QObject
{
    Q_OBJECT

public:
    FavIconUpdater(QObject *parent);
    ~FavIconUpdater();
    void downloadIcon(const KBookmark &bk);
    void downloadIconActual(const KBookmark &bk);

private Q_SLOTS:
    void setIconURL(const KUrl &iconURL);
    void slotCompleted();
    void notifyChange(bool isHost, const QString& hostOrURL, const QString& iconName);

Q_SIGNALS:
    void done(bool succeeded);

private:
    KParts::ReadOnlyPart *m_part;
    FavIconBrowserInterface *m_browserIface;
    FavIconWebGrabber *m_webGrabber;
    KBookmark m_bk;
    bool webupdate;
    org::kde::FavIcon m_favIconModule;
};

class FavIconBrowserInterface : public KParts::BrowserInterface
{
    Q_OBJECT
public:
    FavIconBrowserInterface(FavIconUpdater *view)
        : KParts::BrowserInterface(view), m_view(view) {
        ;
    }
private:
    FavIconUpdater *m_view;
};

#endif

