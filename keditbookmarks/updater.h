// -*- mode:cperl; cperl-indent-level:4; cperl-continued-statement-offset:4; indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __updater_h
#define __updater_h

#include <kbookmark.h>
#include <konq_faviconmgr.h>

#include <kparts/part.h>
#include <kparts/browserinterface.h>

class FavIconWebGrabber : public QObject
{
Q_OBJECT
public:
    FavIconWebGrabber(KParts::ReadOnlyPart *part, const KURL &url);
    ~FavIconWebGrabber() {}

protected slots:
    void slotMimetype(KIO::Job *job, const QString &_type);
    void slotFinished(KIO::Job *job);

private:
    KParts::ReadOnlyPart *m_part;
    KURL m_url;
};

class FavIconBrowserInterface;

class FavIconUpdater : public KonqFavIconMgr 
{
    Q_OBJECT

public:   
    FavIconUpdater(QObject *parent, const char *name);
    ~FavIconUpdater();
    void downloadIcon(const KBookmark &bk);
    void downloadIconActual(const KBookmark &bk);

    virtual void notifyChange(bool isHost, QString hostOrURL, 
                              QString iconName);

protected slots:
    void setIconURL(const KURL &iconURL);
    void slotCompleted();

signals:
    void done(bool succeeded);

private:
    KParts::ReadOnlyPart *m_part;
    FavIconBrowserInterface *m_browserIface;
    KBookmark m_bk;
};

class FavIconBrowserInterface : public KParts::BrowserInterface
{
    Q_OBJECT
public:
    FavIconBrowserInterface(FavIconUpdater *view, const char *name)
        : KParts::BrowserInterface(view, name), m_view(view) {
        ;
    }
private:
    FavIconUpdater *m_view;
};

#endif

