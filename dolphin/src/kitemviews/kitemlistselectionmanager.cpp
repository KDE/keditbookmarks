/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2011 by Frank Reininghaus <frank78ac@googlemail.com>    *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs:                *
 *   http://qt.gitorious.org/qt-labs/itemviews-ng                          *
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

#include "kitemlistselectionmanager.h"

#include "kitemmodelbase.h"
#include <KDebug>

KItemListSelectionManager::KItemListSelectionManager(QObject* parent) :
    QObject(parent),
    m_currentItem(-1),
    m_anchorItem(-1),
    m_selectedItems(),
    m_isAnchoredSelectionActive(false),
    m_model(0)
{
}

KItemListSelectionManager::~KItemListSelectionManager()
{
}

void KItemListSelectionManager::setCurrentItem(int current)
{
    const int previous = m_currentItem;
    const QSet<int> previousSelection = selectedItems();

    if (m_model && current >= 0 && current < m_model->count()) {
        m_currentItem = current;
    } else {
        m_currentItem = -1;
    }

    if (m_currentItem != previous) {
        emit currentChanged(m_currentItem, previous);

        if (m_isAnchoredSelectionActive) {
            const QSet<int> selection = selectedItems();
            if (selection != previousSelection) {
                emit selectionChanged(selection, previousSelection);
            }
        }
    }
}

int KItemListSelectionManager::currentItem() const
{
    return m_currentItem;
}

void KItemListSelectionManager::setSelectedItems(const QSet<int>& items)
{
    if (m_selectedItems != items) {
        const QSet<int> previous = m_selectedItems;
        m_selectedItems = items;
        emit selectionChanged(m_selectedItems, previous);
    }
}

QSet<int> KItemListSelectionManager::selectedItems() const
{
    QSet<int> selectedItems = m_selectedItems;

    if (m_isAnchoredSelectionActive && m_anchorItem != m_currentItem) {
        Q_ASSERT(m_anchorItem >= 0);
        Q_ASSERT(m_currentItem >= 0);
        const int from = qMin(m_anchorItem, m_currentItem);
        const int to = qMax(m_anchorItem, m_currentItem);

        for (int index = from; index <= to; ++index) {
            selectedItems.insert(index);
        }
    }

    return selectedItems;
}

bool KItemListSelectionManager::hasSelection() const
{
    return !m_selectedItems.isEmpty() || (m_isAnchoredSelectionActive && m_anchorItem != m_currentItem);
}

void KItemListSelectionManager::setSelected(int index, int count, SelectionMode mode)
{
    if (index < 0 || count < 1 || !m_model || index >= m_model->count()) {
        return;
    }

    endAnchoredSelection();
    const QSet<int> previous = selectedItems();

    count = qMin(count, m_model->count() - index);

    const int endIndex = index + count -1;
    switch (mode) {
    case Select:
        for (int i = index; i <= endIndex; ++i) {
            m_selectedItems.insert(i);
        }
        break;

    case Deselect:
        for (int i = index; i <= endIndex; ++i) {
            m_selectedItems.remove(i);
        }
        break;

    case Toggle:
        for (int i = index; i <= endIndex; ++i) {
            if (m_selectedItems.contains(i)) {
                m_selectedItems.remove(i);
            } else {
                m_selectedItems.insert(i);
            }
        }
        break;

    default:
        Q_ASSERT(false);
        break;
    }

    const QSet<int> selection = selectedItems();
    if (selection != previous) {
        emit selectionChanged(selection, previous);
    }
}

void KItemListSelectionManager::clearSelection()
{
    const QSet<int> previous = selectedItems();
    if (!previous.isEmpty()) {
        m_selectedItems.clear();
        m_isAnchoredSelectionActive = false;
        emit selectionChanged(QSet<int>(), previous);
    }
}

void KItemListSelectionManager::beginAnchoredSelection(int anchor)
{
    if (anchor >= 0 && m_model && anchor < m_model->count()) {
        m_isAnchoredSelectionActive = true;
        m_anchorItem = anchor;
    }
}

void KItemListSelectionManager::endAnchoredSelection()
{
    if (m_isAnchoredSelectionActive && (m_anchorItem != m_currentItem)) {
        Q_ASSERT(m_anchorItem >= 0);
        Q_ASSERT(m_currentItem >= 0);
        const int from = qMin(m_anchorItem, m_currentItem);
        const int to = qMax(m_anchorItem, m_currentItem);

        for (int index = from; index <= to; ++index) {
            m_selectedItems.insert(index);
        }
    }

    m_isAnchoredSelectionActive = false;
}

int KItemListSelectionManager::anchorItem() const
{
    return m_anchorItem;
}

bool KItemListSelectionManager::isAnchoredSelectionActive() const
{
    return m_isAnchoredSelectionActive;
}

KItemModelBase* KItemListSelectionManager::model() const
{
    return m_model;
}

void KItemListSelectionManager::setModel(KItemModelBase* model)
{
    m_model = model;
    if (model && model->count() > 0) {
        m_currentItem = 0;
    }
}

void KItemListSelectionManager::itemsInserted(const KItemRangeList& itemRanges)
{
    // Store the current selection (needed in the selectionChanged() signal)
    const QSet<int> previousSelection = selectedItems();

    // Update the current item
    if (m_currentItem < 0) {
        setCurrentItem(0);
    } else {
        const int previousCurrent = m_currentItem;
        int inc = 0;
        foreach (const KItemRange& itemRange, itemRanges) {
            if (m_currentItem < itemRange.index) {
                break;
            }
            inc += itemRange.count;
        }
        // Calling setCurrentItem would trigger the selectionChanged signal, but we want to
        // emit it only once in this function -> change the current item manually and emit currentChanged
        m_currentItem += inc;
        emit currentChanged(m_currentItem, previousCurrent);
    }

    // Update the anchor item
    if (m_anchorItem < 0) {
        m_anchorItem = 0;
    } else {
        int inc = 0;
        foreach (const KItemRange& itemRange, itemRanges) {
            if (m_anchorItem < itemRange.index) {
                break;
            }
            inc += itemRange.count;
        }
        m_anchorItem += inc;
    }

    // Update the selections
    if (!m_selectedItems.isEmpty()) {
        const QSet<int> previous = m_selectedItems;
        m_selectedItems.clear();
        QSetIterator<int> it(previous);
        while (it.hasNext()) {
            const int index = it.next();
            int inc = 0;
            foreach (const KItemRange& itemRange, itemRanges) {
                if (index < itemRange.index) {
                    break;
                }
                inc += itemRange.count;
            }
            m_selectedItems.insert(index + inc);
        }
    }

    const QSet<int> selection = selectedItems();
    if (selection != previousSelection) {
        emit selectionChanged(selection, previousSelection);
    }
}

void KItemListSelectionManager::itemsRemoved(const KItemRangeList& itemRanges)
{
    // Store the current selection (needed in the selectionChanged() signal)
    const QSet<int> previousSelection = selectedItems();

    // Update the current item
    if (m_currentItem >= 0) {
        const int previousCurrent = m_currentItem;
        int currentItem = m_currentItem;
        foreach (const KItemRange& itemRange, itemRanges) {
            if (currentItem < itemRange.index) {
                break;
            }
            if (currentItem >= itemRange.index + itemRange.count) {
                currentItem -= itemRange.count;
            } else if (currentItem >= m_model->count()) {
                currentItem = m_model->count() - 1;
            }
        }
        // Calling setCurrentItem would trigger the selectionChanged signal, but we want to
        // emit it only once in this function -> change the current item manually and emit currentChanged
        m_currentItem = currentItem;
        emit currentChanged(m_currentItem, previousCurrent);
    }

    // Update the anchor item
    if (m_anchorItem >= 0) {
        int anchorItem = m_anchorItem;
        foreach (const KItemRange& itemRange, itemRanges) {
            if (anchorItem < itemRange.index) {
                break;
            }
            if (anchorItem >= itemRange.index + itemRange.count) {
                anchorItem -= itemRange.count;
            } else if (anchorItem >= m_model->count()) {
                anchorItem = m_model->count() - 1;
            }
        }
        m_anchorItem = anchorItem;
        if (m_anchorItem < 0) {
            m_isAnchoredSelectionActive = false;
        }
    }

    // Update the selections
    if (!m_selectedItems.isEmpty()) {
        const QSet<int> previous = m_selectedItems;
        m_selectedItems.clear();
        QSetIterator<int> it(previous);
        while (it.hasNext()) {
            int index = it.next();
            int dec = 0;
            foreach (const KItemRange& itemRange, itemRanges) {
                if (index < itemRange.index) {
                    break;
                }

                if (index < itemRange.index + itemRange.count) {
                    // The selection is part of the removed range
                    // and will get deleted
                    index = -1;
                    break;
                }

                dec += itemRange.count;
            }
            index -= dec;
            if (index >= 0)  {
                m_selectedItems.insert(index);
            }
        }
    }

    const QSet<int> selection = selectedItems();
    if (selection != previousSelection) {
        emit selectionChanged(selection, previousSelection);
    }
}

#include "kitemlistselectionmanager.moc"
