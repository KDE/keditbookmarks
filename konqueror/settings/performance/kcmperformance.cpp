/*
 *  Copyright (c) 2003 Lubos Lunak <l.lunak@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// Own
#include "kcmperformance.h"

// Qt
#include <QtGui/QLayout>
#include <QtGui/QTabWidget>
#include <QtGui/QBoxLayout>

// KDE
#include <klocale.h>
#include <kdialog.h>
#include <KDebug>
// Local
#include "konqueror.h"
#include "system.h"
#include <KPluginFactory>
#include <KPluginLoader>
K_PLUGIN_FACTORY(KCMPerformanceConfigFactory,
        registerPlugin<KCMPerformance::Config>("performance");
        registerPlugin<KCMPerformance::KonquerorConfig>("konqueror");
        )
K_EXPORT_PLUGIN(KCMPerformanceConfigFactory("kcmperformance"))

namespace KCMPerformance
{

Config::Config( QWidget* parent_P, const QVariantList & )
    : KCModule( KCMPerformanceConfigFactory::componentData(), parent_P )
    {
    setQuickHelp( i18n( "<h1>KDE Performance</h1>"
        " You can configure settings that improve KDE performance here." ));
    
    QVBoxLayout *topLayout = new QVBoxLayout( this );
    QTabWidget* tabs = new QTabWidget( this );
    konqueror_widget = new Konqueror;
    konqueror_widget->layout()->setMargin( KDialog::marginHint() );
    connect( konqueror_widget, SIGNAL( changed()), SLOT( changed()));
    tabs->addTab( konqueror_widget, i18n( "Konqueror" ));
    system_widget = new SystemWidget;
    system_widget->layout()->setMargin( KDialog::marginHint() );
    connect( system_widget, SIGNAL( changed()), SLOT( changed()));
    tabs->addTab( system_widget, i18n( "System" ));
    topLayout->addWidget( tabs );
    }

void Config::load()
    {
    konqueror_widget->load();
    system_widget->load();
    }

void Config::save()
    {
    konqueror_widget->save();
    system_widget->save();
    }

void Config::defaults()
    {
    konqueror_widget->defaults();
    system_widget->defaults();
    }

KonquerorConfig::KonquerorConfig( QWidget* parent_P, const QVariantList & )
    : KCModule( KCMPerformanceConfigFactory::componentData(), parent_P )
    {
    setQuickHelp( i18n( "<h1>Konqueror Performance</h1>"
        " You can configure several settings that improve Konqueror performance here."
        " These include options for reusing already running instances"
        " and for keeping instances preloaded." ));

    QVBoxLayout *topLayout = new QVBoxLayout( this );
    topLayout->setMargin( 0 );
    widget = new Konqueror( this );
    connect( widget, SIGNAL( changed()), SLOT( changed()));
    topLayout->addWidget( widget );
    }

void KonquerorConfig::load()
    {
    widget->load();
    }

void KonquerorConfig::save()
    {
    widget->save();
    }

void KonquerorConfig::defaults()
    {
    widget->defaults();
    }

} // namespace

#include "kcmperformance.moc"
