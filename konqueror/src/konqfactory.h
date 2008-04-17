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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef __konq_factory_h__
#define __konq_factory_h__

#include "konqprivate_export.h"

#include <kservice.h>

class KAboutData;
class KPluginFactory;
namespace KParts { class ReadOnlyPart; }

class KonqViewFactory // TODO rename to KonqPartLoader?
{
public:
    /**
     * Create null factory
     */
    KonqViewFactory() : m_factory(0), m_args() {}

    KonqViewFactory(const QString& libName, KPluginFactory* factory);

    // The default copy ctor and operator= can be used, this is a value class.

    void setArgs(const QVariantList &args);

    KParts::ReadOnlyPart *create(QWidget *parentWidget, QObject *parent);

    bool isNull() const { return m_factory ? false : true; }

private:
    QString m_libName;
    KPluginFactory *m_factory;
    QVariantList m_args;
};

/**
 * Factory for creating (loading) parts when creating a view.
 */
class KONQ_TESTS_EXPORT KonqFactory
{
public:
    /**
     * Return the factory that can be used to actually create the part inside a view.
     *
     * The implementation locates the part module (library), using the trader
     * and opens it (using klibfactory), which gives us a factory that can be used to
     * actually create the part (later on, when the KonqView exists).
     *
     * Not a static method so that we can define an abstract base class
     * with another implementation, for unit tests, if wanted.
     */
    KonqViewFactory createView( const QString &serviceType,
                                const QString &serviceName = QString(),
                                KService::Ptr *serviceImpl = 0,
                                KService::List *partServiceOffers = 0,
                                KService::List *appServiceOffers = 0,
                                bool forceAutoEmbed = false );

    static void getOffers( const QString & serviceType,
                           KService::List *partServiceOffers = 0,
                           KService::List *appServiceOffers = 0);

    static const KAboutData* aboutData();
};

#endif
