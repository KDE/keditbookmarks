/* This file is part of the KDE project
   Copyright (C) 2002 Lubos Lunak <l.lunak@kde.org>

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

#include "preloader.h"
#include "konqsettingsxt.h"
#include "preloaderadaptor.h"

#include <kdebug.h>
#include <ktoolinvocation.h>

#include <kpluginfactory.h>
#include <kpluginloader.h>

K_PLUGIN_FACTORY(KonqyPreloaderFactory,
                 registerPlugin<KonqyPreloader>();
    )
K_EXPORT_PLUGIN(KonqyPreloaderFactory("konqypreloader"))

KonqyPreloader::KonqyPreloader(QObject* parent, const QList<QVariant>&)
    : KDEDModule(parent)
    {
    reconfigure();

    (void)new PreloaderAdaptor(this);

    connect( QDBusConnection::sessionBus().interface(),
            SIGNAL( serviceOwnerChanged( const QString&, const QString&, const QString& )),
            SLOT  ( appChanged( const QString&, const QString&, const QString& )));
    check_always_preloaded_timer.setSingleShot( true );
    connect( &check_always_preloaded_timer, SIGNAL( timeout()),
	SLOT( checkAlwaysPreloaded()));
    }

KonqyPreloader::~KonqyPreloader()
    {
    updateCount();
    }

bool KonqyPreloader::registerPreloadedKonqy( const QString &id, int screen )
    {
    if( instances.count() >= KonqSettings::maxPreloadCount() )
        return false;
    instances.append( KonqyData( id, screen ));
    return true;
    }

QString KonqyPreloader::getPreloadedKonqy( int screen )
    {
    if( instances.count() == 0 )
        return "";
    for( InstancesList::Iterator it = instances.begin();
         it != instances.end();
         ++it )
        {
        if( (*it).screen == screen )
            {
           QString ret = (*it).id;
            instances.erase( it );
            check_always_preloaded_timer.start( 5000 );
            return ret;
            }
        }
    return "";
    }

void KonqyPreloader::unregisterPreloadedKonqy( const QString &id_P )
    {
    for( InstancesList::Iterator it = instances.begin();
         it != instances.end();
         ++it )
        if( (*it).id == id_P )
            {
            instances.erase( it );
            return;
            }
    }

void KonqyPreloader::appChanged( const QString & /*id*/,  const QString &oldOwner, const QString &newOwner )
    {
    if ( oldOwner.isEmpty() || !newOwner.isEmpty() )
        return;

    unregisterPreloadedKonqy( oldOwner );
    }

void KonqyPreloader::reconfigure()
    {
    KonqSettings::self()->readConfig();
    updateCount();
    // Ignore "PreloadOnStartup" here, it's used by the .desktop file
    // in the autostart folder, which will do 'konqueror --preload' in autostart
    // phase 2. This will also cause activation of this kded module.
    }

void KonqyPreloader::updateCount()
    {
    while( instances.count() > KonqSettings::maxPreloadCount() )
        {
        KonqyData konqy = instances.first();
        instances.pop_front();
        QDBusInterface ref( konqy.id, "/", "org.kde.Konqueror.Main" );
        ref.call( "terminatePreloaded" );
        }
    if( KonqSettings::alwaysHavePreloaded() &&
        KonqSettings::maxPreloadCount() > 0 &&
        instances.count() == 0 )
	{
	if( !check_always_preloaded_timer.isActive())
	    {
	    if( KToolInvocation::kdeinitExec( QLatin1String( "konqueror" ),
		QStringList() << QLatin1String( "--preload" ), NULL, NULL, "0" ) == 0 )
		{
		kDebug( 1202 ) << "Preloading Konqueror instance";
	        check_always_preloaded_timer.start( 5000 );
		}
	    // else do nothing, the launching failed
	    }
	}
    }

// have 5s interval between attempts to preload a new konqy
// in order not to start many of them at the same time
void KonqyPreloader::checkAlwaysPreloaded()
    {
    // TODO here should be detection whether the system is too busy,
    // and delaying preloading another konqy in such case
    // but I have no idea how to do it
    updateCount();
    }

void KonqyPreloader::unloadAllPreloaded()
    {
    while( instances.count() > 0 )
        {
        KonqyData konqy = instances.first();
        instances.pop_front();
        QDBusInterface ref( konqy.id, "/", "org.kde.Konqueror.Main" );
        ref.call( "terminatePreloaded" );
        }
    // ignore 'always_have_preloaded' here
    }

#include "preloader.moc"
