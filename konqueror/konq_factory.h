/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 1999 David Faure <faure@kde.org>
    Copyright (C) 1999 Torben Weis <weis@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef __konq_factory_h__
#define __konq_factory_h__

#include <qstring.h>
#include <qstringlist.h>

#include <klibloader.h>
#include <ktrader.h>

#include <kparts/part.h>

class BrowserView;
class KonqMainWindow;
class KAboutData;

class KonqViewFactory
{
public:
  KonqViewFactory() : m_factory( 0L ), m_createBrowser( false ) {}

  KonqViewFactory( KLibFactory *factory, const QStringList &args, bool createBrowser );

  KonqViewFactory( const KonqViewFactory &factory )
  { (*this) = factory; }

  KonqViewFactory &operator=( const KonqViewFactory &other )
  {
    m_factory = other.m_factory;
    m_args = other.m_args;
    m_createBrowser = other.m_createBrowser;
    return *this;
  }

  KParts::ReadOnlyPart *create( QWidget *parentWidget, const char *widgetName,
                                QObject *parent, const char *name );

  bool isNull() const { return m_factory ? false : true; }

private:
  KLibFactory *m_factory;
  QStringList m_args;
  bool m_createBrowser;
};

class KonqFactory
{
public:
  static KonqViewFactory createView( const QString &serviceType,
				     const QString &serviceName = QString::null,
				     KService::Ptr *serviceImpl = 0,
				     KTrader::OfferList *partServiceOffers = 0,
				     KTrader::OfferList *appServiceOffers = 0,
				     bool forceAutoEmbed = false );

  static void getOffers( const QString & serviceType,
                         KTrader::OfferList *partServiceOffers = 0,
                         KTrader::OfferList *appServiceOffers = 0);

  static const KAboutData* aboutData();

private:
  static KAboutData *s_aboutData;
};

#endif
