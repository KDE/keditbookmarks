/***************************************************************************
 *   Copyright (C) 2009-2010 by Peter Penz <peter.penz19@gmail.com>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/
#ifndef SERVICESSETTINGSPAGE_H
#define SERVICESSETTINGSPAGE_H

#include <settings/settingspagebase.h>

#include <QMap>
#include <QString>

class QCheckBox;
class QGroupBox;
class QListView;
class QSortFilterProxyModel;
class ServiceModel;

/**
 * @brief Page for the 'Services' settings of the Dolphin settings dialog.
 */
class ServicesSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    ServicesSettingsPage(QWidget* parent);
    virtual ~ServicesSettingsPage();

    /** @see SettingsPageBase::applySettings() */
    virtual void applySettings();

    /** @see SettingsPageBase::restoreDefaults() */
    virtual void restoreDefaults();

protected:
    virtual void showEvent(QShowEvent* event);

private slots:
    /**
     * Loads locally installed services.
     */
    void loadServices();

private:
    /**
     * Loads installed version control systems.
     */
    void loadVersionControlSystems();

    bool isInServicesList(const QString& service) const;

    /**
     * Adds a row to the model of m_listView.
     */
    void addRow(const QString& icon,
                const QString& text,
                const QString& value,
                bool checked);

private:
    bool m_initialized;
    ServiceModel* m_serviceModel;
    QSortFilterProxyModel* m_sortModel;
    QListView* m_listView;
    QStringList m_enabledVcsPlugins;
};

#endif
