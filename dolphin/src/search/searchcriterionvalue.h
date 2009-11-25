/***************************************************************************
 *   Copyright (C) 2009 by Adam Kidder <thekidder@gmail.com>               *
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

#ifndef SEARCHCRITERIONVALUE_H
#define SEARCHCRITERIONVALUE_H

#define DISABLE_NEPOMUK_LEGACY
#include <nepomuk/literalterm.h>

#include <QWidget>

class QComboBox;
class KDateWidget;
class KRatingWidget;
class KLineEdit;

/**
 * @brief Helper class for SearchCriterionSelector.
 * Represents an input widget for the value of a search criterion.
 */
class SearchCriterionValue : public QWidget
{
    Q_OBJECT

public:
    SearchCriterionValue(QWidget* parent = 0);
    virtual ~SearchCriterionValue();

    /**
     * Must be overwritten by derived classes and returns
     * the literal term of the search criterion value.
     */
    virtual Nepomuk::Query::LiteralTerm value() const = 0;

    /**
     * Initializes the widget on the base of the given value-type.
     * It is in the hand of the derived classes to interprete
     * the value-type string and create a corresponding value for
     * the widget (@see SearchCriterionSelector::Comparator).
     * The default implementation is empty.
     */
    virtual void initializeValue(const QString& valueType);

signals:
    void valueChanged(const Nepomuk::Query::LiteralTerm& value);
};



/** @brief Allows to input a date value as search criterion. */
class DateValue : public SearchCriterionValue
{
    Q_OBJECT

public:
    DateValue(QWidget* parent = 0);
    virtual ~DateValue();
    virtual Nepomuk::Query::LiteralTerm value() const;
    virtual void initializeValue(const QString& valueType);

private:
    KDateWidget* m_dateWidget;
};



/** @brief Allows to input a tag  as search criterion. */
class TagValue : public SearchCriterionValue
{
    Q_OBJECT

public:
    TagValue(QWidget* parent = 0);
    virtual ~TagValue();
    virtual Nepomuk::Query::LiteralTerm value() const;

protected:
    virtual void showEvent(QShowEvent* event);

private:
    QComboBox* m_tags;
};



/** @brief Allows to input a file size value as search criterion. */
class SizeValue : public SearchCriterionValue
{
    Q_OBJECT

public:
    SizeValue(QWidget* parent = 0);
    virtual ~SizeValue();
    virtual Nepomuk::Query::LiteralTerm value() const;

 private:
    KLineEdit* m_lineEdit;
    QComboBox* m_units;
};

/** @brief Allows to input a rating value as search criterion. */
class RatingValue : public SearchCriterionValue
{
    Q_OBJECT

public:
    RatingValue(QWidget* parent = 0);
    virtual ~RatingValue();
    virtual Nepomuk::Query::LiteralTerm value() const;

 private:
    KRatingWidget* m_ratingWidget;
};

#endif // SEARCHCRITERIONVALUE_H
