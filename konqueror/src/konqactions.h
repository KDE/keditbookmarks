/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_actions_h__
#define __konq_actions_h__

#include <ktoolbarpopupaction.h>
#include <konqhistorymanager.h>
#include <kactionmenu.h>
#include <QtGui/QWidget>
#include <QtCore/QList>

struct HistoryEntry;
class QMenu;

namespace KonqActions {
    void fillHistoryPopup(const QList<HistoryEntry*> &history, int historyIndex,
                          QMenu * popup,
                          bool onlyBack,
                          bool onlyForward);
}

#if 0
/**
 * Plug this action into a menu to get a bidirectional history
 * (both back and forward, including current location)
 */
class KonqBidiHistoryAction : public KToolBarPopupAction
{
  Q_OBJECT
public:
    KonqBidiHistoryAction( const QString & text, QObject * parent );
    virtual ~KonqBidiHistoryAction();

    void fillGoMenu( const QList<HistoryEntry*> &history, int historyIndex );

protected Q_SLOTS:
    void slotTriggered( QAction* action );

Q_SIGNALS:
    void menuAboutToShow();
    // -1 for one step back, 0 for don't move, +1 for one step forward, etc.
    void step( int );
private:
    int m_startPos;
    int m_currentPos; // == history.at()
};

#endif

/////

class KonqMostOftenURLSAction : public KActionMenu
{
    Q_OBJECT

public:
    KonqMostOftenURLSAction( const QString& text, QObject* parent );
    virtual ~KonqMostOftenURLSAction();

    static bool numberOfVisitOrder( const KonqHistoryEntry& lhs, const KonqHistoryEntry& rhs ) {
        return lhs.numberOfTimesVisited < rhs.numberOfTimesVisited;
    }

Q_SIGNALS:
    void activated(const KUrl&);

private Q_SLOTS:
    void slotHistoryCleared();
    void slotEntryAdded( const KonqHistoryEntry& entry );
    void slotEntryRemoved( const KonqHistoryEntry& entry );

    void slotFillMenu();
    void slotActivated(QAction* action);

private:
    void init();
    void parseHistory();
    static void inSort( const KonqHistoryEntry& entry );
    bool m_parsingDone;
};

/////

class KonqHistoryAction : public KActionMenu
{
    Q_OBJECT

public:
    KonqHistoryAction(const QString& text, QObject* parent);
    virtual ~KonqHistoryAction();

Q_SIGNALS:
    void activated(const KUrl&);

private Q_SLOTS:
    void slotFillMenu();
    void slotActivated(QAction* action);
};

#endif
