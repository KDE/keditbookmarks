/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at) and              *
 *   and Patrice Tremblay                                                  *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "generalsettingspage.h"

#include <qlayout.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <kdialog.h>
#include <qlabel.h>
#include <qlineedit.h>

#include <q3grid.h>
#include <q3groupbox.h>
#include <klocale.h>
#include <qcheckbox.h>
#include <q3buttongroup.h>
#include <qpushbutton.h>
#include <kfiledialog.h>
#include <qradiobutton.h>
#include <kvbox.h>

#include "dolphinsettings.h"
#include "dolphin.h"
#include "dolphinview.h"
#include "generalsettings.h"

GeneralSettingsPage::GeneralSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_homeUrl(0),
    m_startSplit(0),
    m_startEditable(0)
{
    Q3VBoxLayout* topLayout = new Q3VBoxLayout(parent, 2, KDialog::spacingHint());

    const int spacing = KDialog::spacingHint();
    const int margin = KDialog::marginHint();
    const QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    GeneralSettings* settings = DolphinSettings::instance().generalSettings();

    KVBox* vBox = new KVBox(parent);
    vBox->setSizePolicy(sizePolicy);
    vBox->setSpacing(spacing);
    vBox->setMargin(margin);
    vBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);

    // create 'Home Url' editor
    Q3GroupBox* homeGroup = new Q3GroupBox(1, Qt::Horizontal, i18n("Home Url"), vBox);
    homeGroup->setSizePolicy(sizePolicy);
    homeGroup->setMargin(margin);

    KHBox* homeUrlBox = new KHBox(homeGroup);
    homeUrlBox->setSizePolicy(sizePolicy);
    homeUrlBox->setSpacing(spacing);

    new QLabel(i18n("Location:"), homeUrlBox);
    m_homeUrl = new QLineEdit(settings->homeUrl(), homeUrlBox);

    QPushButton* selectHomeUrlButton = new QPushButton(SmallIcon("folder"), QString::null, homeUrlBox);
    connect(selectHomeUrlButton, SIGNAL(clicked()),
            this, SLOT(selectHomeUrl()));

    KHBox* buttonBox = new KHBox(homeGroup);
    buttonBox->setSizePolicy(sizePolicy);
    buttonBox->setSpacing(spacing);
    QPushButton* useCurrentButton = new QPushButton(i18n("Use current location"), buttonBox);
    connect(useCurrentButton, SIGNAL(clicked()),
            this, SLOT(useCurrentLocation()));
    QPushButton* useDefaultButton = new QPushButton(i18n("Use default location"), buttonBox);
    connect(useDefaultButton, SIGNAL(clicked()),
            this, SLOT(useDefaulLocation()));

    // create 'Default View Mode' group
    Q3ButtonGroup* buttonGroup = new Q3ButtonGroup(3, Qt::Vertical, i18n("Default View Mode"), vBox);
    buttonGroup->setSizePolicy(sizePolicy);
    buttonGroup->setMargin(margin);

    m_iconsView = new QRadioButton(i18n("Icons"), buttonGroup);
    m_detailsView = new QRadioButton(i18n("Details"), buttonGroup);
    m_previewsView = new QRadioButton(i18n("Previews"), buttonGroup);

    switch (settings->defaultViewMode()) {
        case DolphinView::IconsView:    m_iconsView->setChecked(true); break;
        case DolphinView::DetailsView:  m_detailsView->setChecked(true); break;
        case DolphinView::PreviewsView: m_previewsView->setChecked(true); break;
    }

    // create 'Start with split view' checkbox
    m_startSplit = new QCheckBox(i18n("Start with split view"), vBox);
    m_startSplit->setChecked(settings->splitView());

    // create 'Start with editable navigation bar' checkbox
    m_startEditable = new QCheckBox(i18n("Start with editable navigation bar"), vBox);
    m_startEditable->setChecked(settings->editableUrl());

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(vBox);

    topLayout->addWidget(vBox);
}


GeneralSettingsPage::~GeneralSettingsPage()
{
}

void GeneralSettingsPage::applySettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();

    const KUrl url(m_homeUrl->text());
    KFileItem fileItem(S_IFDIR, KFileItem::Unknown, url);
    if (url.isValid() && fileItem.isDir()) {
        settings->setHomeUrl(url.prettyUrl());
    }

    DolphinView::Mode viewMode = DolphinView::IconsView;
    if (m_detailsView->isChecked()) {
        viewMode = DolphinView::DetailsView;
    }
    else if (m_previewsView->isChecked()) {
        viewMode = DolphinView::PreviewsView;
    }
    settings->setDefaultViewMode(viewMode);

    settings->setSplitView(m_startSplit->isChecked());
    settings->setEditableUrl(m_startEditable->isChecked());
}

void GeneralSettingsPage::selectHomeUrl()
{
    const QString homeUrl(m_homeUrl->text());
    KUrl url(KFileDialog::getExistingUrl(homeUrl));
    if (!url.isEmpty()) {
        m_homeUrl->setText(url.prettyUrl());
    }
}

void GeneralSettingsPage::useCurrentLocation()
{
    const DolphinView* view = Dolphin::mainWin().activeView();
    m_homeUrl->setText(view->url().prettyUrl());
}

void GeneralSettingsPage::useDefaulLocation()
{
    m_homeUrl->setText("file://" + QDir::homePath());
}

#include "generalsettingspage.moc"
