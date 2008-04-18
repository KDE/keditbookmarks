/*
    Extension Manager for Konqueror

    Copyright (c) 2003      by Martijn Klingens      <klingens@kde.org>
    Copyright (c) 2004      by Arend van Beelen jr.  <arend@auton.nl>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

// Own
#include "konqextensionmanager.h"

// Qt
#include <QtGui/QLayout>
#include <QtCore/QTimer>

// KDE
#include <kxmlguifactory.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcomponentdata.h>
#include <kparts/componentfactory.h>
#include <kparts/plugin.h>
#include <kplugininfo.h>
#include <kpluginselector.h>
#include <ksettings/dispatcher.h>
#include <kstandardguiitem.h>

// Local
#include "konqview.h"
#include "konqmainwindow.h"


class KonqExtensionManagerPrivate
{
public:
    KPluginSelector *pluginSelector;
    KonqMainWindow *mainWindow;
    KParts::ReadOnlyPart* activePart;
    bool isChanged;
};

KonqExtensionManager::KonqExtensionManager(QWidget *parent, KonqMainWindow *mainWindow, KParts::ReadOnlyPart* activePart)
    : KDialog( parent )
{
    setCaption( i18n("Configure") );
    setButtons( Default | Cancel | Apply | Ok | User1 );
    setButtonGuiItem( User1, KStandardGuiItem::reset() );
    setObjectName( "extensionmanager" );
    showButtonSeparator( true );

    d = new KonqExtensionManagerPrivate;
    showButton(User1, false);
    setChanged(false);

    setInitialSize(QSize(640, 480)); // FIXME: hard-coded values ?

    d->pluginSelector = new KPluginSelector(this);
    setMainWidget(d->pluginSelector);
    connect(d->pluginSelector, SIGNAL(changed(bool)), this, SLOT(setChanged(bool)));
    connect(d->pluginSelector, SIGNAL(configCommitted(const QByteArray &)),
            this, SLOT(reparseConfiguration(const QByteArray &)));

    d->mainWindow = mainWindow;
    d->activePart = activePart;

    d->pluginSelector->addPlugins("konqueror", i18n("Extensions"), "Extensions", KGlobal::config());
    if ( activePart ) {
        KComponentData componentData = activePart->componentData();
        d->pluginSelector->addPlugins(componentData.componentName(), i18n("Extensions"), "Tools", componentData.config());
        d->pluginSelector->addPlugins(componentData.componentName(), i18n("Extensions"), "Statusbar", componentData.config());
    }

    connect( this, SIGNAL( okClicked() ), SLOT( slotOk() ) );
    connect( this, SIGNAL( applyClicked() ), SLOT( slotApply() ) );
    connect( this, SIGNAL( defaultClicked() ), SLOT( slotDefault() ) );
    connect( this, SIGNAL( user1Clicked() ), SLOT( slotUser1() ) );
}

KonqExtensionManager::~KonqExtensionManager()
{
    delete d;
}

void KonqExtensionManager::reparseConfiguration(const QByteArray &conf)
{
    KSettings::Dispatcher::reparseConfiguration(conf);
}

void KonqExtensionManager::setChanged(bool c)
{
    d->isChanged = c;
    enableButton(Apply, c);
}

void KonqExtensionManager::apply()
{
    if(d->isChanged)
    {
        d->pluginSelector->save();
        setChanged(false);

        if( d->mainWindow )
        {
            KParts::Plugin::loadPlugins(d->mainWindow, d->mainWindow, KGlobal::mainComponent());
            QList<KParts::Plugin*> plugins = KParts::Plugin::pluginObjects(d->mainWindow);

            for (int i = 0; i < plugins.size(); ++i) {
                d->mainWindow->factory()->addClient(plugins.at(i));
            }
        }

        if ( d->activePart )
        {
            KParts::Plugin::loadPlugins( d->activePart, d->activePart, d->activePart->componentData() );
            QList<KParts::Plugin*> plugins = KParts::Plugin::pluginObjects( d->activePart );

            for (int i = 0; i < plugins.size(); ++i) {
                d->activePart->factory()->addClient(plugins.at(i));
            }
        }
    }
}

void KonqExtensionManager::slotOk()
{
    apply();
    accept();
}

void KonqExtensionManager::slotApply()
{
    apply();
}

void KonqExtensionManager::slotDefault()
{
    d->pluginSelector->defaults();
}

void KonqExtensionManager::slotUser1()
{
    d->pluginSelector->load();
    setChanged(false);
}

void KonqExtensionManager::showEvent(QShowEvent *event)
{
    d->pluginSelector->load();
    KDialog::showEvent(event);
}

#include "konqextensionmanager.moc"
