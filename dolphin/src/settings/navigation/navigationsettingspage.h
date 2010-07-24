/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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
#ifndef NAVIGATIONSETTINGSPAGE_H
#define NAVIGATIONSETTINGSPAGE_H

#include <settings/settingspagebase.h>

class QCheckBox;
class QRadioButton;

/**
 * @brief Page for the 'Navigation' settings of the Dolphin settings dialog.
 */
class NavigationSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    NavigationSettingsPage(QWidget* parent);
    virtual ~NavigationSettingsPage();

    /** @see SettingsPageBase::applySettings() */
    virtual void applySettings();

    /** @see SettingsPageBase::restoreDefaults() */
    virtual void restoreDefaults();

private:
    void loadSettings();

private:
    QRadioButton* m_singleClick;
    QRadioButton* m_doubleClick;
    QCheckBox* m_openArchivesAsFolder;
    QCheckBox* m_autoExpandFolders;
};

#endif
