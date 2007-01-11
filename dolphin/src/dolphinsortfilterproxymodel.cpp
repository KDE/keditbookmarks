/***************************************************************************
*   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
*   Copyright (C) 2006 by Dominic Battre <dominic battre de>              *
*   Copyright (C) 2006 by Martin Pool <mbp canonical com>                 *
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

#include "dolphinsortfilterproxymodel.h"

#include <kdirmodel.h>
#include <kfileitem.h>

static const int dolphinMapSize = 3;
static int dolphinViewToDirModelColumn[] = {
    KDirModel::Name,        // DolphinView::SortByName
    KDirModel::Size,        // DolphinView::SortBySize
    KDirModel::ModifiedTime // DolphinView::SortByDate
};

static DolphinView::Sorting dirModelColumnToDolphinView[] = {
    DolphinView::SortByName, // KDirModel::Name
    DolphinView::SortBySize, // KDirModel::Size
    DolphinView::SortByDate  // KDirModel::ModifiedTime
};


DolphinSortFilterProxyModel::DolphinSortFilterProxyModel(QObject* parent) :
    QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);

    // sort by the user visible string for now
    setSortRole(Qt::DisplayRole);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    sort(KDirModel::Name, Qt::Ascending);
}

DolphinSortFilterProxyModel::~DolphinSortFilterProxyModel()
{
}

void DolphinSortFilterProxyModel::setSorting(DolphinView::Sorting sorting)
{
    // Update the sort column by mapping DolpginView::Sorting to
    // KDirModel::ModelColumns. We will keep the sortOrder.
    Q_ASSERT(static_cast<int>(sorting) >= 0 && static_cast<int>(sorting) <= dolphinMapSize);
    sort(dolphinViewToDirModelColumn[static_cast<int>(sorting)],
         m_sortOrder );
}

void DolphinSortFilterProxyModel::sort(int column, Qt::SortOrder sortOrder)
{
    m_sortOrder = sortOrder;
    m_sorting = (column >= 0) && (column <= dolphinMapSize) ?
                dirModelColumnToDolphinView[column]  :
                DolphinView::SortByName;
    QSortFilterProxyModel::sort(column,sortOrder);
}

void DolphinSortFilterProxyModel::setSortOrder(Qt::SortOrder sortOrder)
{
    // change the sort order by keeping the current column
    sort(dolphinViewToDirModelColumn[m_sorting], sortOrder);
}

static int naturalCompare(const QString& a, const QString& b)
{
    // This method chops the input a and b into pieces of
    // digits and non-digits (a1.05 becomes a | 1 | . | 05)
    // and compares these pieces of a and b to each other
    // (first with first, second with second, ...).
    //
    // This is based on the natural sort order code code by Martin Pool
    // http://sourcefrog.net/projects/natsort/
    // Martin Pool agreed to license this under LGPL or GPL.

    const QChar* currA = a.unicode(); // iterator over a
    const QChar* currB = b.unicode(); // iterator over b

    if (currA == currB) {
        return 0;
    }

    const QChar* begSeqA = currA; // beginning of a new character sequence of a
    const QChar* begSeqB = currB;

    while (!currA->isNull() && !currB->isNull()) {
        // find sequence of characters ending at the first non-character
        while (!currA->isNull() && !currA->isDigit()) {
            ++currA;
        }

        while (!currB->isNull() && !currB->isDigit()) {
            ++currB;
        }

        // compare these sequences
        const QString sub_a(begSeqA, currA - begSeqA);
        const QString sub_b(begSeqB, currB - begSeqB);
        int cmp = QString::localeAwareCompare(sub_a, sub_b);
        if (cmp != 0) {
            return cmp;
        }

        if (currA->isNull() || currB->isNull()) {
            break;
        }

        // now some digits follow...
        if ((*currA == '0') || (*currB == '0')) {
            // one digit-sequence starts with 0 -> assume we are in a fraction part
            // do left aligned comparison (numbers are considered left aligend)
            while ( 1 ) {
                if (!currA->isDigit() && !currB->isDigit()) {
                    break;
                }
                else if (!currA->isDigit()) {
                    return -1;
                }
                else if (!currB->isDigit()) {
                    return +1;
                }
                else if (*currA < *currB ) {
                    return -1;
                }
                else if (*currA > *currB) {
                    return +1;
                }
                ++currA;
                ++currB;
            }
        }
        else {
            // no digit-sequence starts with 0 -> assume we are looking at some integer
            // do right aligned comparison

            // The longest run of digits wins.  That aside, the greatest
            // value wins, but we can't know that it will until we've scanned
            // both numbers to know that they have the same magnitude, so we
            // remember it in 'weight'.

            int weight = 0;
            while (1) {
                if (!currA->isDigit() && !currB->isDigit()) {
                    if (weight != 0) {
                        return weight;
                    }
                    break;
                }
                else if (!currA->isDigit()) {
                    return -1;
                }
                else if (!currB->isDigit()) {
                    return +1;
                }
                else if ((*currA < *currB) && (weight != 0)) {
                    weight = -1;
                }
                else if ((*currA > *currB) && (weight != 0)) {
                    weight = +1;
                }
                ++currA;
                ++currB;
            }

        }

        begSeqA = currA;
        begSeqB = currB;
    }

    if (currA->isNull() && currB->isNull()) {
        return 0;
    }

    return currA->isNull() ? -1 : +1;
}


bool DolphinSortFilterProxyModel::lessThan(const QModelIndex& left,
                                           const QModelIndex& right) const
{

    QVariant leftData = sourceModel()->data(left, sortRole());
    QVariant rightData = sourceModel()->data(right, sortRole());

    if (leftData.type() == QVariant::String && rightData.type() == QVariant::String) {
        const QString left = leftData.toString();
        const QString right = rightData.toString();

        return sortCaseSensitivity() ? (naturalCompare(left, right) < 0) :
                                       (naturalCompare(left.toLower(), right.toLower()) < 0);
    }

    // We have set a SortRole and trust the ProxyModel to do
    // the right thing for now.
    return QSortFilterProxyModel::lessThan(left,right);
}

#include "dolphinsortfilterproxymodel.moc"
