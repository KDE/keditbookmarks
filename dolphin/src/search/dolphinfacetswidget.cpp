/***************************************************************************
*    Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>            *
*                                                                         *
*    This program is free software; you can redistribute it and/or modify *
*    it under the terms of the GNU General Public License as published by *
*    the Free Software Foundation; either version 2 of the License, or    *
*    (at your option) any later version.                                  *
*                                                                         *
*    This program is distributed in the hope that it will be useful,      *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
*    GNU General Public License for more details.                         *
*                                                                         *
*    You should have received a copy of the GNU General Public License    *
*    along with this program; if not, write to the                        *
*    Free Software Foundation, Inc.,                                      *
*    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA           *
* **************************************************************************/

#include "dolphinfacetswidget.h"

#include <KLocale>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDate>
#include <QRadioButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

DolphinFacetsWidget::DolphinFacetsWidget(QWidget* parent) :
    QWidget(parent),
    m_documents(0),
    m_images(0),
    m_audio(0),
    m_videos(0),
    m_anytime(0),
    m_today(0),
    m_yesterday(0),
    m_thisWeek(0),
    m_thisMonth(0),
    m_thisYear(0),
    m_anyRating(0),
    m_oneOrMore(0),
    m_twoOrMore(0),
    m_threeOrMore(0),
    m_fourOrMore(0),
    m_maxRating(0)
{
    QButtonGroup* filetypeGroup = new QButtonGroup(this);
    m_anyType   = createRadioButton(i18nc("@option:check", "Any"), filetypeGroup);
    m_documents = createRadioButton(i18nc("@option:check", "Documents"), filetypeGroup);
    m_images    = createRadioButton(i18nc("@option:check", "Images"), filetypeGroup);
    m_audio     = createRadioButton(i18nc("@option:check", "Audio Files"), filetypeGroup);
    m_videos    = createRadioButton(i18nc("@option:check", "Videos"), filetypeGroup);

    QVBoxLayout* typeLayout = new QVBoxLayout();
    typeLayout->setSpacing(0);
    typeLayout->addWidget(m_anyType);
    typeLayout->addWidget(m_documents);
    typeLayout->addWidget(m_images);
    typeLayout->addWidget(m_audio);
    typeLayout->addWidget(m_videos);
    typeLayout->addStretch();

    QButtonGroup* timespanGroup = new QButtonGroup(this);
    m_anytime   = createRadioButton(i18nc("@option:option", "Anytime"), timespanGroup);
    m_today     = createRadioButton(i18nc("@option:option", "Today"), timespanGroup);
    m_yesterday = createRadioButton(i18nc("@option:option", "Yesterday"), timespanGroup);
    m_thisWeek  = createRadioButton(i18nc("@option:option", "This Week"), timespanGroup);
    m_thisMonth = createRadioButton(i18nc("@option:option", "This Month"), timespanGroup);
    m_thisYear  = createRadioButton(i18nc("@option:option", "This Year"), timespanGroup);

    QVBoxLayout* timespanLayout = new QVBoxLayout();
    timespanLayout->setSpacing(0);
    timespanLayout->addWidget(m_anytime);
    timespanLayout->addWidget(m_today);
    timespanLayout->addWidget(m_yesterday);
    timespanLayout->addWidget(m_thisWeek);
    timespanLayout->addWidget(m_thisMonth);
    timespanLayout->addWidget(m_thisYear);
    timespanLayout->addStretch();

    QButtonGroup* ratingGroup = new QButtonGroup(this);
    m_anyRating   = createRadioButton(i18nc("@option:option", "Any Rating"), ratingGroup);
    m_oneOrMore   = createRadioButton(i18nc("@option:option", "1 or more"), ratingGroup);
    m_twoOrMore   = createRadioButton(i18nc("@option:option", "2 or more"), ratingGroup);
    m_threeOrMore = createRadioButton(i18nc("@option:option", "3 or more"), ratingGroup);
    m_fourOrMore  = createRadioButton(i18nc("@option:option", "4 or more"), ratingGroup);
    m_maxRating   = createRadioButton(i18nc("@option:option", "Highest Rating"), ratingGroup);

    QVBoxLayout* ratingLayout = new QVBoxLayout();
    ratingLayout->setSpacing(0);
    ratingLayout->addWidget(m_anyRating);
    ratingLayout->addWidget(m_oneOrMore);
    ratingLayout->addWidget(m_twoOrMore);
    ratingLayout->addWidget(m_threeOrMore);
    ratingLayout->addWidget(m_fourOrMore);
    ratingLayout->addWidget(m_maxRating);

    QHBoxLayout* topLayout = new QHBoxLayout(this);
    topLayout->addLayout(typeLayout);
    topLayout->addLayout(timespanLayout);
    topLayout->addLayout(ratingLayout);
    topLayout->addStretch();

    m_anyType->setChecked(true);
    m_anytime->setChecked(true);
    m_anyRating->setChecked(true);
}

DolphinFacetsWidget::~DolphinFacetsWidget()
{
}

#ifdef HAVE_BALOO
Baloo::Term DolphinFacetsWidget::ratingTerm() const
{
    Baloo::Term ratingTerm;
    Baloo::Term modifiedTerm;

    if (!m_anyRating->isChecked()) {
        int stars = 1; // represents m_oneOrMore
        if (m_twoOrMore->isChecked()) {
            stars = 2;
        } else if (m_threeOrMore->isChecked()) {
            stars = 3;
        } else if (m_fourOrMore->isChecked()) {
            stars = 4;
        } else if (m_maxRating->isChecked()) {
            stars = 5;
        }

        const int rating = stars * 2;
        ratingTerm = Baloo::Term("rating", rating, Baloo::Term::GreaterEqual);
    }

    if (!m_anytime->isChecked()) {
        QDate date = QDate::currentDate(); // represents m_today
        if (m_yesterday->isChecked()) {
            date = date.addDays(-1);
        } else if (m_thisWeek->isChecked()) {
            date = date.addDays(1 - date.dayOfWeek());
        } else if (m_thisMonth->isChecked()) {
            date = date.addDays(1 - date.day());
        } else if (m_thisYear->isChecked()) {
            date = date.addDays(1 - date.dayOfYear());
        }

        modifiedTerm = Baloo::Term("modified", date, Baloo::Term::GreaterEqual);
    }

    if (ratingTerm.isValid() && modifiedTerm.isValid()) {
        Baloo::Term term(Baloo::Term::And);
        term.addSubTerm(ratingTerm);
        term.addSubTerm(modifiedTerm);

        return term;
    } else if (modifiedTerm.isValid()) {
        return modifiedTerm;
    } else if (ratingTerm.isValid()) {
        return ratingTerm;
    }

    return Baloo::Term();
}

QString DolphinFacetsWidget::facetType() const
{
    if (m_documents->isChecked()) {
        return QLatin1String("Document");
    } else if (m_images->isChecked()) {
        return QLatin1String("Image");
    } else if (m_audio->isChecked()) {
        return QLatin1String("Audio");
    } else if (m_videos->isChecked()) {
        return QLatin1String("Video");
    }

    return QString();
}

bool DolphinFacetsWidget::isRatingTerm(const Baloo::Term& term) const
{
    const QList<Baloo::Term> subTerms = term.subTerms();
    if (subTerms.isEmpty()) {
        // If term has no sub terms, then the term itself is either a "rating" term
        // or a "modified" term.
        return term.property() == QLatin1String("modified") ||
               term.property() == QLatin1String("rating");

    } else if (subTerms.size() == 2) {
        // If term has sub terms, then the sub terms are always "rating" and "modified" terms.

        QStringList properties;
        foreach (const Baloo::Term& subTerm, subTerms) {
            properties << subTerm.property();
        }

        return properties.contains(QLatin1String("modified")) &&
               properties.contains(QLatin1String("rating"));
    }

    return false;
}

void DolphinFacetsWidget::setRatingTerm(const Baloo::Term& term)
{
    // If term has sub terms, then the sub terms are always "rating" and "modified" terms.
    // If term has no sub terms, then the term itself is either a "rating" term or a "modified"
    // term. To avoid code duplication we add term to subTerms list, if the list is empty.
    QList<Baloo::Term> subTerms = term.subTerms();
    if (subTerms.isEmpty()) {
        subTerms << term;
    }

    foreach (const Baloo::Term& subTerm, subTerms) {
        const QString property = subTerm.property();

        if (property == QLatin1String("modified")) {
            const QDate date = subTerm.value().toDate();
            setTimespan(date);
        } else if (property == QLatin1String("rating")) {
            const int stars = subTerm.value().toInt() / 2;
            setRating(stars);
        }
    }
}

#endif

void DolphinFacetsWidget::setFacetType(const QString& type)
{
    if (type == QLatin1String("Document")) {
        m_documents->setChecked(true);
    } else if (type == QLatin1String("Image")) {
        m_images->setChecked(true);
    } else if (type == QLatin1String("Audio")) {
        m_audio->setChecked(true);
    } else if (type == QLatin1String("Video")) {
        m_videos->setChecked(true);
    } else {
        m_anyType->setChecked(true);
    }
}

void DolphinFacetsWidget::setRating(const int stars)
{
    switch (stars) {
    case 5:
        m_maxRating->setChecked(true);
        break;

    case 4:
        m_fourOrMore->setChecked(true);
        break;

    case 3:
        m_threeOrMore->setChecked(true);
        break;

    case 2:
        m_twoOrMore->setChecked(true);
        break;

    case 1:
        m_oneOrMore->setChecked(true);
        break;

    default:
        m_anyRating->setChecked(true);
    }
}

void DolphinFacetsWidget::setTimespan(const QDate& date)
{
    const QDate currentDate = QDate::currentDate();
    const int days = date.daysTo(currentDate);

    if (days <= 0) {
        m_today->setChecked(true);
    } else if (days <= 1) {
        m_yesterday->setChecked(true);
    } else if (days <= currentDate.dayOfWeek()) {
        m_thisWeek->setChecked(true);
    } else if (days <= currentDate.day()) {
        m_thisMonth->setChecked(true);
    } else if (days <= currentDate.dayOfYear()) {
        m_thisYear->setChecked(true);
    } else {
        m_anytime->setChecked(true);
    }
}

QRadioButton* DolphinFacetsWidget::createRadioButton(const QString& text,
                                                     QButtonGroup* group)
{
    QRadioButton* button = new QRadioButton(text);
    connect(button, SIGNAL(clicked()), this, SIGNAL(facetChanged()));
    group->addButton(button);
    return button;
}

#include "dolphinfacetswidget.moc"
