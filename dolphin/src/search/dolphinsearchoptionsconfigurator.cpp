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

#include "dolphinsearchoptionsconfigurator.h"

#include "dolphin_searchsettings.h"
#include "searchcriterionselector.h"

#include <kcombobox.h>
#include <kdialog.h>
#include <kicon.h>
#include <klineedit.h>
#include <klocale.h>
#include <kseparator.h>

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>

struct SettingsItem
{
    const char* settingsName;
    const char* text;
};

// Contains the settings names and translated texts
// for each item of the location-combo-box.
static const SettingsItem g_locationItems[] = {
    {"Everywhere", I18N_NOOP2("@label", "Everywhere")},
    {"From Here",  I18N_NOOP2("@label", "From Here")}
};

// Contains the settings names and translated texts
// for each item of the what-combobox.
static const SettingsItem g_whatItems[] = {
    {"All",       I18N_NOOP2("@label", "All")},
    {"Images",    I18N_NOOP2("@label", "Images")},
    {"Text",      I18N_NOOP2("@label", "Text")},
    {"Filenames", I18N_NOOP2("@label", "Filenames")}
};

struct CriterionItem
{
    const char* settingsName;
    SearchCriterionSelector::Type type;
};

// Contains the settings names for type
// of availabe search criterion.
static const CriterionItem g_criterionItems[] = {
    {"Date", SearchCriterionSelector::Date},
    {"Size", SearchCriterionSelector::Size},
    {"Tag", SearchCriterionSelector::Tag},
    {"Raging", SearchCriterionSelector::Rating}
};

DolphinSearchOptionsConfigurator::DolphinSearchOptionsConfigurator(QWidget* parent) :
    QWidget(parent),
    m_initialized(false),
    m_locationBox(0),
    m_whatBox(0),
    m_addSelectorButton(0),
    m_searchButton(0),
    m_saveButton(0),
    m_vBoxLayout(0),
    m_criterions(),
    m_customSearchQuery()
{
    m_vBoxLayout = new QVBoxLayout(this);

    // add "search" configuration
    QLabel* searchLabel = new QLabel(i18nc("@label", "Search:"));

    m_locationBox = new KComboBox(this);
    for (unsigned int i = 0; i < sizeof(g_locationItems) / sizeof(SettingsItem); ++i) {
        m_locationBox->addItem(g_locationItems[i].text);
    }

    // add "what" configuration
    QLabel* whatLabel = new QLabel(i18nc("@label", "What:"));

    m_whatBox = new KComboBox(this);
    for (unsigned int i = 0; i < sizeof(g_whatItems) / sizeof(SettingsItem); ++i) {
        m_whatBox->addItem(g_whatItems[i].text);
    }

    // add "Add selector" button
    m_addSelectorButton = new QPushButton(this);
    m_addSelectorButton->setIcon(KIcon("list-add"));
    m_addSelectorButton->setToolTip(i18nc("@info", "Add search option"));
    m_addSelectorButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_addSelectorButton, SIGNAL(clicked()), this, SLOT(slotAddSelectorButtonClicked()));

    // add button "Search"
    m_searchButton = new QPushButton(this);
    m_searchButton->setIcon(KIcon("edit-find"));
    m_searchButton->setText(i18nc("@action:button", "Search"));
    m_searchButton->setToolTip(i18nc("@info", "Start searching"));
    m_searchButton->setEnabled(false);
    connect(m_searchButton, SIGNAL(clicked()), this, SIGNAL(searchOptionsChanged()));

    // add button "Save"
    m_saveButton = new QPushButton(this);
    m_saveButton->setIcon(KIcon("document-save"));
    m_saveButton->setText(i18nc("@action:button", "Save"));
    m_saveButton->setToolTip(i18nc("@info", "Save search options"));
    m_saveButton->setEnabled(false);
    connect(m_saveButton, SIGNAL(clicked()), this, SLOT(saveQuery()));

    // add button "Close"
    QPushButton* closeButton = new QPushButton(this);
    closeButton->setIcon(KIcon("dialog-close"));
    closeButton->setText(i18nc("@action:button", "Close"));
    closeButton->setToolTip(i18nc("@info", "Close search options"));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(hide()));

    QHBoxLayout* topLineLayout = new QHBoxLayout();
    topLineLayout->addWidget(m_addSelectorButton);
    topLineLayout->addWidget(searchLabel);
    topLineLayout->addWidget(m_locationBox);
    topLineLayout->addWidget(whatLabel);
    topLineLayout->addWidget(m_whatBox);
    topLineLayout->addWidget(new QWidget(this), 1); // filler
    topLineLayout->addWidget(m_searchButton);
    topLineLayout->addWidget(m_saveButton);
    topLineLayout->addWidget(closeButton);

    m_vBoxLayout->addWidget(new KSeparator(this));
    m_vBoxLayout->addLayout(topLineLayout);
    m_vBoxLayout->addWidget(new KSeparator(this));
}

DolphinSearchOptionsConfigurator::~DolphinSearchOptionsConfigurator()
{
    // store the UI configuration
    const int locationIndex = m_locationBox->currentIndex();
    SearchSettings::setLocation(g_locationItems[locationIndex].settingsName);

    const int whatIndex = m_whatBox->currentIndex();
    SearchSettings::setWhat(g_whatItems[whatIndex].settingsName);

    QString criterionsString;
    foreach(const SearchCriterionSelector* criterion, m_criterions) {
        if (!criterionsString.isEmpty()) {
            criterionsString += ',';
        }
        const int index = static_cast<int>(criterion->type());
        criterionsString += g_criterionItems[index].settingsName;
    }
    SearchSettings::setCriterions(criterionsString);

    SearchSettings::self()->writeConfig();
}

KUrl DolphinSearchOptionsConfigurator::nepomukUrl() const
{
    QString searchOptions;
    foreach (const SearchCriterionSelector* criterion, m_criterions) {
        const QString criterionString = criterion->toString();
        if (!criterionString.isEmpty()) {
            if (!searchOptions.isEmpty()) {
                searchOptions += ' ';
            }
            searchOptions += criterionString;
        }
    }

    QString searchString = m_customSearchQuery;
    if (!searchString.isEmpty() && !searchOptions.isEmpty()) {
        searchString += ' ' + searchOptions;
    } else if (!searchOptions.isEmpty()) {
        searchString += searchOptions;
    }

    searchString.insert(0, QLatin1String("nepomuksearch:/"));
    return KUrl(searchString);
}

void DolphinSearchOptionsConfigurator::setCustomSearchQuery(const QString& searchQuery)
{
    m_customSearchQuery = searchQuery.simplified();

    const bool enabled = hasSearchParameters();
    m_searchButton->setEnabled(enabled);
    m_saveButton->setEnabled(enabled);
}

void DolphinSearchOptionsConfigurator::showEvent(QShowEvent* event)
{
    if (!event->spontaneous() && !m_initialized) {
        // restore the UI layout of the last session
        const QString location = SearchSettings::location();
        for (unsigned int i = 0; i < sizeof(g_locationItems) / sizeof(SettingsItem); ++i) {
            if (g_locationItems[i].settingsName == location) {
                m_locationBox->setCurrentIndex(i);
                break;
            }
        }

        const QString what = SearchSettings::what();
        for (unsigned int i = 0; i < sizeof(g_whatItems) / sizeof(SettingsItem); ++i) {
            if (g_whatItems[i].settingsName == what) {
                m_whatBox->setCurrentIndex(i);
                break;
            }
        }

        const QString criterions = SearchSettings::criterions();
        QStringList criterionsList = criterions.split(',');
        foreach (const QString& criterionName, criterionsList) {
            for (unsigned int i = 0; i < sizeof(g_criterionItems) / sizeof(CriterionItem); ++i) {
                if (g_criterionItems[i].settingsName == criterionName) {
                    const SearchCriterionSelector::Type type = g_criterionItems[i].type;
                    addCriterion(new SearchCriterionSelector(type, this));
                    break;
                }
            }
        }

        m_initialized = true;
    }
    QWidget::showEvent(event);
}

void DolphinSearchOptionsConfigurator::slotAddSelectorButtonClicked()
{
    SearchCriterionSelector* selector = new SearchCriterionSelector(SearchCriterionSelector::Date, this);
    addCriterion(selector);
}

void DolphinSearchOptionsConfigurator::slotCriterionChanged()
{
    const bool enabled = hasSearchParameters();
    m_searchButton->setEnabled(enabled);
    m_saveButton->setEnabled(enabled);
}

void DolphinSearchOptionsConfigurator::removeCriterion()
{
    SearchCriterionSelector* criterion = qobject_cast<SearchCriterionSelector*>(sender());
    Q_ASSERT(criterion != 0);
    m_vBoxLayout->removeWidget(criterion);

    const int index = m_criterions.indexOf(criterion);
    m_criterions.removeAt(index);

    criterion->deleteLater();

    updateSelectorButton();
}

void DolphinSearchOptionsConfigurator::updateSelectorButton()
{
    const int selectors = m_vBoxLayout->count() - 1;
    m_addSelectorButton->setEnabled(selectors < 10);
}

void DolphinSearchOptionsConfigurator::saveQuery()
{
    KDialog dialog(0, Qt::Dialog);

    QWidget* container = new QWidget(&dialog);

    QLabel* label = new QLabel(i18nc("@label", "Name:"), container);
    KLineEdit* lineEdit = new KLineEdit(container);
    lineEdit->setMinimumWidth(250);

    QHBoxLayout* layout = new QHBoxLayout(container);
    layout->addWidget(label, Qt::AlignRight);
    layout->addWidget(lineEdit);

    dialog.setMainWidget(container);
    dialog.setCaption(i18nc("@title:window", "Save Search Options"));
    dialog.setButtons(KDialog::Ok | KDialog::Cancel);
    dialog.setDefaultButton(KDialog::Ok);
    dialog.setButtonText(KDialog::Ok, i18nc("@action:button", "Save"));

    KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                              "SaveSearchOptionsDialog");
    dialog.restoreDialogSize(dialogConfig);
    dialog.exec(); // TODO...
}

void DolphinSearchOptionsConfigurator::addCriterion(SearchCriterionSelector* criterion)
{
    connect(criterion, SIGNAL(removeCriterion()), this, SLOT(removeCriterion()));    
    connect(criterion, SIGNAL(criterionChanged()), this, SLOT(slotCriterionChanged()));

    // insert the new selector before the KSeparator at the bottom
    const int index = m_vBoxLayout->count() - 1;
    m_vBoxLayout->insertWidget(index, criterion);
    updateSelectorButton();

    m_criterions.append(criterion);
}

bool DolphinSearchOptionsConfigurator::hasSearchParameters() const
{
    if (!m_customSearchQuery.isEmpty()) {
        // performance optimization: if a custom search query is defined,
        // there is no need to call the (quite expensive) method nepomukUrl()
        return true;
    }
    return nepomukUrl().path() != QLatin1String("/");
}

#include "dolphinsearchoptionsconfigurator.moc"
