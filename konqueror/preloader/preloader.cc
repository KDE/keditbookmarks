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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "preloader.h"

#include <kconfig.h>
#include <dcopref.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kdebug.h>

KonqyPreloader::KonqyPreloader( const QCString& obj )
    : KDEDModule( obj )
    {
    reconfigure();
    connect( kapp->dcopClient(), SIGNAL( applicationRemoved( const QCString& )),
        SLOT( appRemoved( const QCString& )));
    connect( &check_always_preloaded_timer, SIGNAL( timeout()),
	SLOT( checkAlwaysPreloaded()));
    }

KonqyPreloader::~KonqyPreloader()
    {
    max_count = 0;
    always_have_preloaded = false;
    updateCount();
    }

bool KonqyPreloader::registerPreloadedKonqy( QCString id, int screen )
    {
    if( instances.count() >= max_count )
        return false;
    instances.append( KonqyData( id, screen ));
    return true;
    }

QCString KonqyPreloader::getPreloadedKonqy( int screen )
    {
    if( instances.count() == 0 )
        return "";
    for( InstancesList::ConstIterator it = instances.begin();
         it != instances.end();
         ++it )
        {
        if( (*it).screen == screen )
            {
            QCString ret = (*it).id;
            check_always_preloaded_timer.start( 5000, true );
            return ret;
            }
        }
    return "";
    }

void KonqyPreloader::unregisterPreloadedKonqy( QCString id_P )
    {
    for( InstancesList::Iterator it = instances.begin();
         it != instances.end();
         ++it )
        if( (*it).id == id_P )
            {
            instances.remove( it );
            return;
            }
    }

void KonqyPreloader::appRemoved( const QCString& id )
    {
    unregisterPreloadedKonqy( id );
    }
    
void KonqyPreloader::reconfigure()
    {
    KConfig cfg( QString::fromLatin1( "konquerorrc" ), true );
    KConfigGroupSaver group( &cfg, "Reusing" );
    max_count = cfg.readNumEntry( "MaxPreloadCount", 1 );
    always_have_preloaded = cfg.readBoolEntry( "AlwaysHavePreloaded", false )
	&& max_count > 0;
    updateCount();
    // Ignore "PreloadOnStartup" here, it's used by the .desktop file
    // in the autostart folder, which will do 'konqueror --preload' in autostart
    // phase 2. This will also cause activation of this kded module.
    }

void KonqyPreloader::updateCount()
    {
    while( instances.count() > max_count )
        {
        KonqyData konqy = instances.first();
        instances.pop_front();
        DCOPRef ref( konqy.id, "KonquerorIface" );
        ref.send( "terminatePreloaded" );
        }
    if( always_have_preloaded && instances.count() == 0 )
	{
	if( !check_always_preloaded_timer.isActive())
	    {
	    if( kapp->kdeinitExec( QString::fromLatin1( "konqueror" ),
		QStringList() << QString::fromLatin1( "--preload" )) == 0 )
		{
		kdDebug( 1202 ) << "Preloading Konqueror instance" << endl;
	        check_always_preloaded_timer.start( 5000, true );
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
        DCOPRef ref( konqy.id, "KonquerorIface" );
        ref.send( "terminatePreloaded" );
        }
    // ignore 'always_have_preloaded' here
    }
    
extern "C"
KDEDModule *create_konqy_preloader( const QCString& obj )
    {
    return new KonqyPreloader( obj );
    }

#include "preloader.moc"
