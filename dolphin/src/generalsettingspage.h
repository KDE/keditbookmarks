/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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
#ifndef GENERALSETTINGSPAGE_H
#define GENERALSETTINGSPAGE_H

#include <settingspagebase.h>

class DolphinMainWindow;
class QLineEdit;
class QCheckBox;

/**
 * @brief Page for the 'General' settings of the Dolphin settings dialog.
 */
class GeneralSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    GeneralSettingsPage(DolphinMainWindow* mainWindow, QWidget* parent);
    virtual ~GeneralSettingsPage();

    /** @see SettingsPageBase::applySettings() */
    virtual void applySettings();

    /** @see SettingsPageBase::restoreDefaults() */
    virtual void restoreDefaults();

private:
    void loadSettings();

private:
    QCheckBox* m_confirmMoveToTrash;
    QCheckBox* m_confirmDelete;

    QCheckBox* m_showDeleteCommand;
    QCheckBox* m_showCopyMoveMenu;
    
    QCheckBox* m_showZoomSlider;
    QCheckBox* m_showSpaceInfo;
    
    QCheckBox* m_browseThroughArchives;
    QCheckBox* m_renameInline;
    QCheckBox* m_autoExpandFolders;
};

#endif
