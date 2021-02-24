/* This file is part of the KDE project
   Copyright (C) 2000, 2009 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "commandhistory.h"
#include "commands.h"

#include <KActionCollection>
#include <KBookmarkManager>

#include <QAction>
#include <QUndoCommand>
#include <QUndoStack>

class CommandHistory::Private
{
public:
    Private()
        : m_manager(nullptr)
        , m_undoStack()
    {
    }
    KBookmarkManager *m_manager;
    QUndoStack m_undoStack;
};

CommandHistory::CommandHistory(QObject *parent)
    : QObject(parent)
    , d(new CommandHistory::Private)
{
}

CommandHistory::~CommandHistory()
{
    delete d;
}

void CommandHistory::setBookmarkManager(KBookmarkManager *manager)
{
    clearHistory(); // we can't keep old commands pointing to the wrong model/manager...
    d->m_manager = manager;
}

void CommandHistory::createActions(KActionCollection *actionCollection)
{
    // TODO use QUndoView?

    QAction *standardAction = KStandardAction::create(KStandardAction::Undo, nullptr, nullptr, nullptr);
    QAction *undoAction = d->m_undoStack.createUndoAction(actionCollection);
    undoAction->setIcon(standardAction->icon());
    actionCollection->addAction(KStandardAction::name(KStandardAction::Undo), undoAction);
    actionCollection->setDefaultShortcuts(undoAction, standardAction->shortcuts());
    disconnect(undoAction, &QAction::triggered, &d->m_undoStack, nullptr);
    connect(undoAction, &QAction::triggered, this, &CommandHistory::undo);
    delete standardAction;

    standardAction = KStandardAction::create(KStandardAction::Redo, nullptr, nullptr, nullptr);
    QAction *redoAction = d->m_undoStack.createRedoAction(actionCollection);
    redoAction->setIcon(standardAction->icon());
    actionCollection->addAction(KStandardAction::name(KStandardAction::Redo), redoAction);
    actionCollection->setDefaultShortcuts(redoAction, standardAction->shortcuts());
    disconnect(redoAction, &QAction::triggered, &d->m_undoStack, nullptr);
    connect(redoAction, &QAction::triggered, this, &CommandHistory::redo);
    delete standardAction;
}

void CommandHistory::undo()
{
    const int idx = d->m_undoStack.index();
    const QUndoCommand *cmd = d->m_undoStack.command(idx - 1);
    if (cmd) {
        d->m_undoStack.undo();
        commandExecuted(cmd);
    }
}

void CommandHistory::redo()
{
    const int idx = d->m_undoStack.index();
    const QUndoCommand *cmd = d->m_undoStack.command(idx);
    if (cmd) {
        d->m_undoStack.redo();
        commandExecuted(cmd);
    }
}

void CommandHistory::commandExecuted(const QUndoCommand *k)
{
    const IKEBCommand *cmd = dynamic_cast<const IKEBCommand *>(k);
    Q_ASSERT(cmd);

    KBookmark bk = d->m_manager->findByAddress(cmd->affectedBookmarks());
    Q_ASSERT(bk.isGroup());

    Q_EMIT notifyCommandExecuted(bk.toGroup());
}

void CommandHistory::notifyDocSaved()
{
    d->m_undoStack.setClean();
}

void CommandHistory::addCommand(QUndoCommand *cmd)
{
    if (!cmd)
        return;
    d->m_undoStack.push(cmd); // calls cmd->redo()
    CommandHistory::commandExecuted(cmd);
}

void CommandHistory::clearHistory()
{
    if (d->m_undoStack.count() > 0) {
        d->m_undoStack.clear();
        Q_EMIT notifyCommandExecuted(d->m_manager->root()); // not really, but we still want to update the GUI
    }
}

KBookmarkManager *CommandHistory::bookmarkManager()
{
    return d->m_manager;
}
