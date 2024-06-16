/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000, 2009 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
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

#include "moc_commandhistory.cpp"
