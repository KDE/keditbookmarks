/* This file is part of the KDE project
   Copyright (C) 2006 David Faure <faure@kde.org>

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

#include "konqapplication.h"
#include "konqsettings.h"
#include <QtDBus/QtDBus>
#include "konqmainwindow.h"
#include "KonquerorAdaptor.h"
#include "konqviewmanager.h"

KonquerorApplication::KonquerorApplication()
    : KApplication(),
      closed_by_sm( false )
{
    new KonquerorAdaptor; // not really an adaptor
    const QString dbusInterface = "org.kde.Konqueror.Main";
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.connect(QString(), KONQ_MAIN_PATH, dbusInterface, "reparseConfiguration", this, SLOT(slotReparseConfiguration()));
    dbus.connect(QString(), KONQ_MAIN_PATH, dbusInterface, "updateAllProfileList", this, SLOT(slotUpdateProfileList()));
    dbus.connect(QString(), KONQ_MAIN_PATH, dbusInterface, "addToCombo", this,
                 SLOT(slotAddToCombo(const QString&, const QDBusMessage&)));
    dbus.connect(QString(), KONQ_MAIN_PATH, dbusInterface, "removeFromCombo", this,
                 SLOT(slotRemoveFromCombo(const QString&, const QDBusMessage&)));
    dbus.connect(QString(), KONQ_MAIN_PATH, dbusInterface, "comboCleared", this, SLOT(slotComboCleared(const QDBusMessage&)));
}

void KonquerorApplication::slotReparseConfiguration()
{
    KGlobal::config()->reparseConfiguration();
    KonqFMSettings::reparseConfiguration();

    QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
    if ( mainWindows )
    {
        foreach ( KonqMainWindow* window, *mainWindows )
            window->reparseConfiguration();
    }
}

void KonquerorApplication::slotUpdateProfileList()
{
    QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
    if ( !mainWindows )
        return;

    foreach ( KonqMainWindow* window, *mainWindows )
        window->viewManager()->profileListDirty( false );
}

void KonquerorApplication::slotAddToCombo( const QString& url, const QDBusMessage& msg )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboAdd, url, msg.service() );
}

void KonquerorApplication::slotRemoveFromCombo( const QString& url, const QDBusMessage& msg )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboRemove, url, msg.service() );
}

void KonquerorApplication::slotComboCleared( const QDBusMessage& msg )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboClear, QString(), msg.service() );
}

#include "konqapplication.moc"
