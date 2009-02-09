/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqpixmapprovider.h"

#include <QBitmap>
#include <QPainter>

#include <kiconloader.h>
#include <kmimetype.h>
#include <kprotocolinfo.h>

#include <kconfiggroup.h>

class KonqPixmapProviderSingleton
{
public:
    KonqPixmapProvider self;
};
K_GLOBAL_STATIC( KonqPixmapProviderSingleton, globalPixmapProvider )

KonqPixmapProvider * KonqPixmapProvider::self()
{
    return &globalPixmapProvider->self;
}

KonqPixmapProvider::KonqPixmapProvider()
    : KPixmapProvider(),
    org::kde::FavIcon("org.kde.kded", "/modules/favicons", QDBusConnection::sessionBus())
{
    QObject::connect(this, SIGNAL(iconChanged(bool,QString,QString)),
                     this, SLOT(notifyChange(bool,QString,QString)) );
}

KonqPixmapProvider::~KonqPixmapProvider()
{
}

// at first, tries to find the iconname in the cache
// if not available, tries to find the pixmap for the mimetype of url
// if that fails, gets the icon for the protocol
// finally, inserts the url/icon pair into the cache
QString KonqPixmapProvider::iconNameFor( const KUrl& url )
{
    QMap<KUrl,QString>::iterator it = iconMap.find( url );
    QString icon;
    if ( it != iconMap.end() ) {
        icon = it.value();
        if ( !icon.isEmpty() )
	    return icon;
    }

    if ( url.url().isEmpty() ) {
        // Use the folder icon for the empty URL
        const KMimeType::Ptr directoryType = KMimeType::mimeType( "inode/directory" );
        if( directoryType.isNull() ) // no mimetypes installed!
            return QString();
        icon = directoryType->iconName();
        Q_ASSERT( !icon.isEmpty() );
    }
    else
    {
        icon = KMimeType::iconNameForUrl( url );
        Q_ASSERT( !icon.isEmpty() );
    }

    // cache the icon found for url
    iconMap.insert( url, icon );

    return icon;
}

QPixmap KonqPixmapProvider::pixmapFor( const QString& url, int size )
{
    return loadIcon( iconNameFor( KUrl( url ) ), size );
}

void KonqPixmapProvider::load( KConfigGroup& kc, const QString& key )
{
    iconMap.clear();
    const QStringList list = kc.readPathEntry( key, QStringList() );
    QStringList::ConstIterator it = list.begin();
    QString url, icon;
    while ( it != list.end() ) {
	url = (*it);
	if ( ++it == list.end() )
	    break;
	icon = (*it);
	iconMap.insert( KUrl( url ), icon );

	++it;
    }
}

// only saves the cache for the given list of items to prevent the cache
// from growing forever.
void KonqPixmapProvider::save( KConfigGroup& kc, const QString& key,
			       const QStringList& items )
{
    QStringList list;
    QStringList::ConstIterator it = items.begin();
    QMap<KUrl,QString>::const_iterator mit;
    while ( it != items.end() ) {
	mit = iconMap.constFind( KUrl(*it) );
	if ( mit != iconMap.constEnd() ) {
	    list.append( mit.key().url() );
	    list.append( mit.value() );
	}

	++it;
    }
    kc.writePathEntry( key, list );
}

void KonqPixmapProvider::notifyChange( bool isHost, const QString& hostOrURL,
    const QString& iconName )
{
    KUrl u;
    if ( !isHost ) u = hostOrURL;

    for ( QMap<KUrl,QString>::iterator it = iconMap.begin();
          it != iconMap.end();
          ++it )
    {
        KUrl url( it.key() );
        if ( !url.protocol().startsWith( "http" ) ) continue;
        if ( ( isHost && url.host() == hostOrURL ) ||
             ( !isHost && url.host() == u.host() && url.path() == u.path() ) )
        {
            // For host default-icons still query the favicon manager to get
            // the correct icon for pages that have an own one.
            QString icon = isHost ? KMimeType::favIconForUrl( url ) : iconName;
            if ( !icon.isEmpty() )
                *it = icon;
        }
    }

    emit changed();
}

void KonqPixmapProvider::clear()
{
    iconMap.clear();
}

QPixmap KonqPixmapProvider::loadIcon( const QString& icon, int size )
{
    if ( size <= KIconLoader::SizeSmall )
	return SmallIcon( icon, size );

    return KIconLoader::global()->loadIcon( icon, KIconLoader::Panel, size );
}

#include "konqpixmapprovider.moc"
