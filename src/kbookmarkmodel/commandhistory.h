/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000, 2009 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef COMMANDHISTORY_H
#define COMMANDHISTORY_H

#include "kbookmarkmodel_export.h"
#include <QObject>
class KBookmarkGroup;
class KBookmarkManager;
class QUndoCommand;
class KActionCollection;

// TODO namespacing
class KBOOKMARKMODEL_EXPORT CommandHistory : public QObject
{
    Q_OBJECT
public:
    explicit CommandHistory(QObject *parent = nullptr);
    ~CommandHistory() override;

    // Call this before putting any commands into the history!
    void setBookmarkManager(KBookmarkManager *manager);
    KBookmarkManager *bookmarkManager();

    void createActions(KActionCollection *collection);

    void notifyDocSaved();

    void clearHistory();
    void addCommand(QUndoCommand *);

Q_SIGNALS:
    void notifyCommandExecuted(const KBookmarkGroup &);

public Q_SLOTS:
    void undo();
    void redo();

private:
    void commandExecuted(const QUndoCommand *k);

private:
    class Private;
    Private *const d;
};

#endif /* COMMANDHISTORY_H */
