/*
   Copyright (C) 2001 Dawit Alemayehu <adawit@kde.org>

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

#ifndef KSAVEIO_CONFIG_H_
#define KSAVEIO_CONFIG_H_

#include <kprotocolmanager.h>

class QWidget;

class KSaveIOConfig
{
public:
  /** Timeout Settings */
  static void setReadTimeout( int );

  static void setConnectTimeout( int );

  static void setProxyConnectTimeout( int );

  static void setResponseTimeout( int );
  

  /** Cache Settings */
  static void setMaxCacheAge( int );

  static void setUseCache( bool );

  static void setMaxCacheSize( int );

  static void setCacheControl( KIO::CacheControl );


  /** Proxy Settings */
  static void setUseProxy( bool );

  static void setUseReverseProxy( bool );

  static void setProxyType( KProtocolManager::ProxyType );

  static void setProxyAuthMode( KProtocolManager::ProxyAuthMode );

  static void setProxyConfigScript( const QString&  );

  static void setProxyFor( const QString&, const QString&  );

  static void setNoProxyFor( const QString& );


  /** Miscelaneous Settings */
  static void setMarkPartial( bool );

  static void setMinimumKeepSize( int );

  static void setAutoResume( bool );

  static void setPersistentConnections( bool );
  
  /** Update all running io-slaves */
  static void updateRunningIOSlaves (QWidget * parent = 0L);
};
#endif
