/* This file is part of the KDE project
   Copyright (C) 1999 Malte Starostik <malte@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_faviconmgr_h__
#define __konq_faviconmgr_h__

#include <kurl.h>
#include <libkonq_export.h>
class QDBusInterface;

/**
 * Maintains a list of custom icons per URL. This is only a stub
 * for the "favicons" KDED Module
 */
class LIBKONQ_EXPORT KonqFavIconMgr : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    KonqFavIconMgr(QObject *parent);

    ~KonqFavIconMgr();

    /**
     * Downloads an icon from @p iconURL and associates @p url with it.
     */
    static void setIconForURL(const KUrl &url, const KUrl &iconURL);

    /**
     * Downloads /favicon.ico from the host of @p url and associates all
     * URLs on that host with it
     * (unless a more specific entry for a URL exists)
     */
    static void downloadHostIcon(const KUrl &url);

    /**
     * Looks up an icon for @p url and returns its name if found
     * or QString() otherwise
     */
    static QString iconForURL(const KUrl &url);

private slots:
    /**
     * an icon changed, updates the combo box
     */
    virtual void notifyChange( bool isHost, QString hostOrURL, QString iconName ) = 0;

signals:
    void changed();

private:
    QDBusInterface *m_favIconsModule;
};

#endif

