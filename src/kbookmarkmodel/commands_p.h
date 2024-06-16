/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000, 2010 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef KBOOKMARKMODEL_COMMANDS_P_H
#define KBOOKMARKMODEL_COMMANDS_P_H

// Used internally by the "sort" command
class MoveCommand : public QUndoCommand, public IKEBCommand
{
public:
    MoveCommand(KBookmarkModel *model, const QString &from, const QString &to, const QString &name = QString(), QUndoCommand *parent = nullptr);
    QString finalAddress() const;
    ~MoveCommand() override
    {
    }
    void redo() override;
    void undo() override;
    QString affectedBookmarks() const override;

private:
    KBookmarkModel *m_model;
    QString m_from;
    QString m_to;
    CreateCommand *m_cc;
    DeleteCommand *m_dc;
};

#endif /* KBOOKMARKMODEL_COMMANDS_P_H */
