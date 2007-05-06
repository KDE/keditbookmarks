/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "columnviewsettingspage.h"

#include "dolphinsettings.h"
#include "dolphin_columnmodesettings.h"

#include <kdialog.h>
#include <kfontrequester.h>
#include <klocale.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>

ColumnViewSettingsPage::ColumnViewSettingsPage(DolphinMainWindow* mainWindow,
                                               QWidget* parent) :
    KVBox(parent),
    m_mainWindow(mainWindow),
    m_smallIconSize(0),
    m_mediumIconSize(0),
    m_largeIconSize(0),
    m_fontRequester(0)
{
    const int spacing = KDialog::spacingHint();
    const int margin = KDialog::marginHint();
    const QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    setSpacing(spacing);
    setMargin(margin);

    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    Q_ASSERT(settings != 0);

    // Create "Icon" properties
    QGroupBox* iconSizeBox = new QGroupBox(i18n("Icon Size"), this);
    iconSizeBox->setSizePolicy(sizePolicy);

    m_smallIconSize  = new QRadioButton(i18n("Small"), this);
    m_mediumIconSize = new QRadioButton(i18n("Medium"), this);
    m_largeIconSize  = new QRadioButton(i18n("Large"), this);
    switch (settings->iconSize()) {
    case K3Icon::SizeLarge:
        m_largeIconSize->setChecked(true);
        break;

    case K3Icon::SizeMedium:
        m_mediumIconSize->setChecked(true);
        break;

    case K3Icon::SizeSmall:
    default:
        m_smallIconSize->setChecked(true);
    }

    QButtonGroup* iconSizeGroup = new QButtonGroup(this);
    iconSizeGroup->addButton(m_smallIconSize);
    iconSizeGroup->addButton(m_mediumIconSize);
    iconSizeGroup->addButton(m_largeIconSize);

    QHBoxLayout* iconSizeLayout = new QHBoxLayout(iconSizeBox);
    iconSizeLayout->addWidget(m_smallIconSize);
    iconSizeLayout->addWidget(m_mediumIconSize);
    iconSizeLayout->addWidget(m_largeIconSize);

    // create "Text" properties
    QGroupBox* textBox = new QGroupBox(i18n("Text"), this);
    textBox->setSizePolicy(sizePolicy);

    QLabel* fontLabel = new QLabel(i18n("Font:"), textBox);
    m_fontRequester = new KFontRequester(textBox);
    QFont font(settings->fontFamily(),
               settings->fontSize());
    font.setItalic(settings->italicFont());
    font.setBold(settings->boldFont());
    m_fontRequester->setFont(font);

    QHBoxLayout* textLayout = new QHBoxLayout(textBox);
    textLayout->addWidget(fontLabel);
    textLayout->addWidget(m_fontRequester);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(this);
}

ColumnViewSettingsPage::~ColumnViewSettingsPage()
{
}

void ColumnViewSettingsPage::applySettings()
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    Q_ASSERT(settings != 0);

    int iconSize = K3Icon::SizeSmall;
    if (m_mediumIconSize->isChecked()) {
        iconSize = K3Icon::SizeMedium;
    } else if (m_largeIconSize->isChecked()) {
        iconSize = K3Icon::SizeLarge;
    }
    settings->setIconSize(iconSize);

    const QFont font = m_fontRequester->font();
    settings->setFontFamily(font.family());
    settings->setFontSize(font.pointSize());
    settings->setItalicFont(font.italic());
    settings->setBoldFont(font.bold());
}

#include "columnviewsettingspage.moc"
