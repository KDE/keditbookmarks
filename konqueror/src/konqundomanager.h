/* This file is part of the KDE project
   Copyright 2007 David Faure <faure@kde.org>
   Copyright 2007 Eduardo Robles Elvira <edulix@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQUNDOMANAGER_H
#define KONQUNDOMANAGER_H

#include "konqprivate_export.h"
#include <QObject>
#include <QString>
#include <QList>
class KonqClosedWindowItem;
class KonqClosedTabItem;
class KonqClosedItem;
class QAction;

/**
 * Note that there is one KonqUndoManager per mainwindow.
 * It integrates KonqFileUndoManager (undoing file operations)
 * and undoing the closing of tabs.
 */
class KONQ_TESTS_EXPORT KonqUndoManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor
     * @param parent the parent QObject, also used as the parent widget for KonqFileUndoManager::UiInterface.
     */
    explicit KonqUndoManager(QWidget* parent);
    ~KonqUndoManager();

    bool undoAvailable() const;
    QString undoText() const;
    quint64 newCommandSerialNumber();

    /**
     * This method is not constant because when calling it the m_closedItemsList
     * might get filled because of delayed initialization.
     */
    const QList<KonqClosedItem* >& closedItemsList();
    void undoClosedItem(int index);
    void addClosedTabItem(KonqClosedTabItem* closedTabItem);
    /**
     * Add current window as a closed window item to other windows
     */
    void addClosedWindowItem(KonqClosedWindowItem *closedWindowItem);
    void updateSupportsFileUndo(bool enable);

public Q_SLOTS:
    void undo();
    void clearClosedItemsList(bool onlyInthisWindow = false);
    void undoLastClosedItem();
    /**
     * Opens in a new tab/window the item the user selected from the closed tabs
     * menu (by emitting openClosedTab/Window), and takes it from the list.
     */
    void slotClosedItemsActivated(QAction* action);
    void slotAddClosedWindowItem(KonqUndoManager *real_sender,
        KonqClosedWindowItem *closedWindowItem);

Q_SIGNALS:
    void undoAvailable(bool canUndo);
    void undoTextChanged(const QString& text);

    /// Emitted when a closed tab should be reopened
    void openClosedTab(const KonqClosedTabItem&);
    /// Emitted when a closed window should be reopened
    void openClosedWindow(const KonqClosedWindowItem&);
    /// Emitted when closedItemsList() has changed.
    void closedItemsListChanged();

    /// Emitted to be received in other window instances, uing the singleton
    /// communicator
    void removeWindowInOtherInstances(KonqUndoManager *real_sender, const
        KonqClosedWindowItem *closedWindowItem);
    void addWindowInOtherInstances(KonqUndoManager *real_sender,
        KonqClosedWindowItem *closedWindowItem);
private Q_SLOTS:
    void slotFileUndoAvailable(bool);
    void slotFileUndoTextChanged(const QString& text);

    /**
     * Received from other window instances, removes/adds a reference of a
     * window from m_closedItemList.
     */
    void slotRemoveClosedWindowItem(KonqUndoManager *real_sender, const
        KonqClosedWindowItem *closedWindowItem);

private:
    /// Fill the m_closedItemList with closed windows
    void populate();

    QList<KonqClosedItem *> m_closedItemList;
    bool m_supportsFileUndo;
    bool m_populated;
};

#endif /* KONQUNDOMANAGER_H */
