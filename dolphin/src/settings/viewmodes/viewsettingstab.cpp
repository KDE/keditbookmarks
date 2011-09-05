/***************************************************************************
 *   Copyright (C) 2008-2011 by Peter Penz <peter.penz19@gmail.com>        *
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

#include "viewsettingstab.h"

#include "dolphinfontrequester.h"
#include "dolphin_compactmodesettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_iconsmodesettings.h"

#include <KComboBox>
#include <KLocale>

#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>

#include <views/zoomlevelinfo.h>

template<class T>
void apply(int iconSizeValue, int previewSizeValue, const QFont& font, bool useSystemFont)
{
    const int iconSize = ZoomLevelInfo::iconSizeForZoomLevel(iconSizeValue);
    const int previewSize = ZoomLevelInfo::iconSizeForZoomLevel(previewSizeValue);
    T::setIconSize(iconSize);
    T::setPreviewSize(previewSize);

    T::setUseSystemFont(useSystemFont);
    T::setFontFamily(font.family());
    T::setFontSize(font.pointSizeF());
    T::setItalicFont(font.italic());
    T::setFontWeight(font.weight());

    T::self()->writeConfig();
}

template<class T>
void load(int* iconSizeValue, int* previewSizeValue, QFont* font, bool* useSystemFont)
{
    const QSize iconSize(T::iconSize(), T::iconSize());
    *iconSizeValue = ZoomLevelInfo::zoomLevelForIconSize(iconSize);

    const QSize previewSize(T::previewSize(), T::previewSize());
    *previewSizeValue = ZoomLevelInfo::zoomLevelForIconSize(previewSize);

    *useSystemFont = T::useSystemFont();

    *font = QFont(T::fontFamily(), qRound(T::fontSize()));
    font->setItalic(T::italicFont());
    font->setWeight(T::fontWeight());
    font->setPointSizeF(T::fontSize());
}


ViewSettingsTab::ViewSettingsTab(Mode mode, QWidget* parent) :
    QWidget(parent),
    m_mode(mode),
    m_defaultSizeSlider(0),
    m_previewSizeSlider(0),
    m_fontRequester(0),
    m_textWidthBox(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);

    // Create "Icon Size" group
    QGroupBox* iconSizeGroup = new QGroupBox(this);
    iconSizeGroup->setTitle(i18nc("@title:group", "Icon Size"));

    const int minRange = ZoomLevelInfo::minimumLevel();
    const int maxRange = ZoomLevelInfo::maximumLevel();

    QLabel* defaultLabel = new QLabel(i18nc("@label:listbox", "Default:"), this);
    m_defaultSizeSlider = new QSlider(Qt::Horizontal, this);
    m_defaultSizeSlider->setPageStep(1);
    m_defaultSizeSlider->setTickPosition(QSlider::TicksBelow);
    m_defaultSizeSlider->setRange(minRange, maxRange);

    QLabel* previewLabel = new QLabel(i18nc("@label:listbox", "Preview:"), this);
    m_previewSizeSlider = new QSlider(Qt::Horizontal, this);
    m_previewSizeSlider->setPageStep(1);
    m_previewSizeSlider->setTickPosition(QSlider::TicksBelow);
    m_previewSizeSlider->setRange(minRange, maxRange);

    QGridLayout* layout = new QGridLayout(iconSizeGroup);
    layout->addWidget(defaultLabel, 0, 0, Qt::AlignRight);
    layout->addWidget(m_defaultSizeSlider, 0, 1);
    layout->addWidget(previewLabel, 1, 0, Qt::AlignRight);
    layout->addWidget(m_previewSizeSlider, 1, 1);

    // Create "Text" group
    QGroupBox* textGroup = new QGroupBox(i18nc("@title:group", "Text"), this);

    QLabel* fontLabel = new QLabel(i18nc("@label:listbox", "Font:"), textGroup);
    m_fontRequester = new DolphinFontRequester(textGroup);

    QGridLayout* textGroupLayout = new QGridLayout(textGroup);
    textGroupLayout->addWidget(fontLabel, 0, 0, Qt::AlignRight);
    textGroupLayout->addWidget(m_fontRequester, 0, 1);

    if (m_mode == IconsMode) {
        QLabel* textWidthLabel = new QLabel(i18nc("@label:listbox", "Text width:"), textGroup);
        m_textWidthBox = new KComboBox(textGroup);
        m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Small"));
        m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Medium"));
        m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Large"));
        m_textWidthBox->addItem(i18nc("@item:inlistbox Text width", "Huge"));

        textGroupLayout->addWidget(textWidthLabel, 2, 0, Qt::AlignRight);
        textGroupLayout->addWidget(m_textWidthBox, 2, 1);
    }

    topLayout->addWidget(iconSizeGroup);
    topLayout->addWidget(textGroup);
    topLayout->addStretch(1);

    loadSettings();

    connect(m_defaultSizeSlider, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
    connect(m_previewSizeSlider, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
    connect(m_fontRequester, SIGNAL(changed()), this, SIGNAL(changed()));
    if (m_mode == IconsMode) {
        connect(m_textWidthBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));
    }
}

ViewSettingsTab::~ViewSettingsTab()
{
}

void ViewSettingsTab::applySettings()
{
    const int defaultSize = m_defaultSizeSlider->value();
    const int previewSize = m_previewSizeSlider->value();
    const QFont font = m_fontRequester->currentFont();
    const bool useSystemFont = (m_fontRequester->mode() == DolphinFontRequester::SystemFont);

    switch (m_mode) {
    case IconsMode:
        IconsModeSettings::setTextWidthIndex(m_textWidthBox->currentIndex());
        apply<IconsModeSettings>(defaultSize, previewSize, font, useSystemFont);
        break;
    case CompactMode:
        apply<CompactModeSettings>(defaultSize, previewSize, font, useSystemFont);
        break;
    case DetailsMode:
        apply<DetailsModeSettings>(defaultSize, previewSize, font, useSystemFont);
        break;
    default:
        Q_ASSERT(false);
        break;
    }
}

void ViewSettingsTab::restoreDefaultSettings()
{
    KConfigSkeleton* settings = 0;
    switch (m_mode) {
    case IconsMode:   settings = IconsModeSettings::self(); break;
    case CompactMode: settings = CompactModeSettings::self(); break;
    case DetailsMode: settings = DetailsModeSettings::self(); break;
    default: Q_ASSERT(false); break;
    }

    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void ViewSettingsTab::loadSettings()
{
    int iconSizeValue = 0;
    int previewSizeValue = 0;
    QFont font;
    bool useSystemFont = false;

    switch (m_mode) {
    case IconsMode:
        m_textWidthBox->setCurrentIndex(IconsModeSettings::textWidthIndex());
        load<IconsModeSettings>(&iconSizeValue, &previewSizeValue, &font, &useSystemFont);
        break;
    case CompactMode:
        load<CompactModeSettings>(&iconSizeValue, &previewSizeValue, &font, &useSystemFont);
        break;
    case DetailsMode:
        load<DetailsModeSettings>(&iconSizeValue, &previewSizeValue, &font, &useSystemFont);
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    m_defaultSizeSlider->setValue(iconSizeValue);
    m_previewSizeSlider->setValue(previewSizeValue);
    m_fontRequester->setMode(useSystemFont ? DolphinFontRequester::SystemFont : DolphinFontRequester::CustomFont);
    m_fontRequester->setCustomFont(font);
}

#include "viewsettingstab.moc"
