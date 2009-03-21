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

#include "statusbarsettingspage.h"
#include "dolphinsettings.h"
#include "dolphin_generalsettings.h"

#include <kdialog.h>
#include <klocale.h>
#include <kvbox.h>

#include <QCheckBox>
#include <QVBoxLayout>

StatusBarSettingsPage::StatusBarSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_showZoomSlider(0),
    m_showSpaceInfo(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    KVBox* vBox = new KVBox(this);
    vBox->setSpacing(KDialog::spacingHint());

    m_showZoomSlider = new QCheckBox(i18nc("@option:check", "Show zoom slider"), vBox);
    connect(m_showZoomSlider, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    m_showSpaceInfo = new QCheckBox(i18nc("@option:check", "Show space information"), vBox);
    connect(m_showSpaceInfo, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(vBox);

    topLayout->addWidget(vBox);

    loadSettings();
}

StatusBarSettingsPage::~StatusBarSettingsPage()
{
}

void StatusBarSettingsPage::applySettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    settings->setShowZoomSlider(m_showZoomSlider->isChecked());
    settings->setShowSpaceInfo(m_showSpaceInfo->isChecked());
    settings->writeConfig();
}

void StatusBarSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void StatusBarSettingsPage::loadSettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    m_showZoomSlider->setChecked(settings->showZoomSlider());
    m_showSpaceInfo->setChecked(settings->showSpaceInfo());
}

#include "statusbarsettingspage.moc"
