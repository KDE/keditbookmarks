/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KINSERTIONSORT_P_H
#define KINSERTIONSORT_P_H

/**
 * A template-based insertion sort algorithm, but not really 100%
 * generic. It is mostly written for lists, not for arrays.
 *
 * A good reason to use insertion sort over faster algorithms like
 * heap sort or quick sort, is that it minimizes the number of
 * movements of the items. This is important in applications which support
 * undo, because the number of commands is kept to a minimum.
 */

// Item must define isNull(), previousSibling(), nextSibling()
// SortHelper must define  moveAfter( const Item &, const Item & )
// Criteria must define  static Key key(const Item &)
template<class Item, class Criteria, class Key, class SortHelper>
inline void kInsertionSort(Item &firstChild, SortHelper &sortHelper)
{
    if (firstChild.isNull())
        return;
    Item j = firstChild.nextSibling();
    while (!j.isNull()) {
        Key key = Criteria::key(j);
        // qCDebug(KEDITBOOKMARKS_LOG) << "Looking at j=" << key;
        // Insert A[j] into the sorted sequence A[1..j-1]
        Item i = j.previousSibling();
        Item next = j.nextSibling();
        bool moved = false;
        while (!i.isNull() && Criteria::key(i) > key) {
            i = i.previousSibling();
            moved = true;
        }
        if (moved) {
            // qCDebug(KEDITBOOKMARKS_LOG) << "moveAfter(" << Criteria::key(j) << "," << (i.isNull() ? "null" : Criteria::key(i)) << ")";
            sortHelper.moveAfter(j, i); // move j right after i. If i is null, move to first position.
        }
        j = next;
        // qCDebug(KEDITBOOKMARKS_LOG) << "Now j is" << Criteria::key(next);
    }
}

#endif
