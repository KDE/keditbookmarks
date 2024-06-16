/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef kbookmarkmodel__commands_h
#define kbookmarkmodel__commands_h

#include "kbookmarkmodel_export.h"
#include <KBookmark>
#include <QUndoCommand>
#include <QUrl>
class KBookmarkModel;

// Interface adds the affectedBookmarks method
// Any command class should on call add those bookmarks which are
// affected by executing or unexecuting the command
// Or a common parent of the affected bookmarks
// see KBookmarkManager::notifyChange(KBookmarkGroup)
class KBOOKMARKMODEL_EXPORT IKEBCommand
{
public:
    IKEBCommand()
    {
    }
    virtual ~IKEBCommand()
    {
    }
    virtual QString affectedBookmarks() const = 0;
    // If only QUndoCommand had setData(QVariant), we would save a lot of casting...
};

class KBOOKMARKMODEL_EXPORT KEBMacroCommand : public QUndoCommand, public IKEBCommand
{
public:
    explicit KEBMacroCommand(const QString &name, QUndoCommand *parent = nullptr)
        : QUndoCommand(name, parent)
    {
    }
    ~KEBMacroCommand() override
    {
    }
    QString affectedBookmarks() const override;
};

class KBOOKMARKMODEL_EXPORT DeleteManyCommand : public KEBMacroCommand
{
public:
    DeleteManyCommand(KBookmarkModel *model, const QString &name, const QList<KBookmark> &bookmarks);
    ~DeleteManyCommand() override
    {
    }
};

class KBOOKMARKMODEL_EXPORT CreateCommand : public QUndoCommand, public IKEBCommand
{
public:
    // separator
    CreateCommand(KBookmarkModel *model, const QString &address, QUndoCommand *parent = nullptr);

    // bookmark
    CreateCommand(KBookmarkModel *model, const QString &address, const QString &text, const QString &iconPath, const QUrl &url, QUndoCommand *parent = nullptr);

    // folder
    CreateCommand(KBookmarkModel *model, const QString &address, const QString &text, const QString &iconPath, bool open, QUndoCommand *parent = nullptr);

    // clone existing bookmark
    CreateCommand(KBookmarkModel *model, const QString &address, const KBookmark &original, const QString &name = QString(), QUndoCommand *parent = nullptr);

    QString finalAddress() const;

    ~CreateCommand() override
    {
    }
    void redo() override;
    void undo() override;
    QString affectedBookmarks() const override;

private: // TODO move it all to a d pointer
    KBookmarkModel *m_model;
    QString m_to;
    QString m_text;
    QString m_iconPath;
    QUrl m_url;
    bool m_group : 1;
    bool m_separator : 1;
    bool m_open : 1;
    KBookmark m_originalBookmark;
    QDomDocument m_originalBookmarkDocRef; // so that it lives at least as long as m_originalBookmark
};

class KBOOKMARKMODEL_EXPORT EditCommand : public QUndoCommand, public IKEBCommand
{
public:
    EditCommand(KBookmarkModel *model, const QString &address, int col, const QString &newValue, QUndoCommand *parent = nullptr);
    ~EditCommand() override
    {
    }
    void redo() override;
    void undo() override;
    QString affectedBookmarks() const override
    {
        return KBookmark::parentAddress(mAddress);
    }
    void modify(const QString &newValue);

private:
    KBookmarkModel *m_model;
    QString mAddress;
    int mCol;
    QString mNewValue;
    QString mOldValue;
};

class KBOOKMARKMODEL_EXPORT DeleteCommand : public QUndoCommand, public IKEBCommand
{
public:
    explicit DeleteCommand(KBookmarkModel *model, const QString &from, bool contentOnly = false, QUndoCommand *parent = nullptr);
    ~DeleteCommand() override
    {
        delete m_cmd;
        delete m_subCmd;
    }
    void redo() override;
    void undo() override;
    QString affectedBookmarks() const override;
    static KEBMacroCommand *deleteAll(KBookmarkModel *model, const KBookmarkGroup &parentGroup);

private: // TODO d pointer
    KBookmarkModel *m_model;
    QString m_from;
    QUndoCommand *m_cmd;
    KEBMacroCommand *m_subCmd;
    bool m_contentOnly;
};

class SortItem;

class KBOOKMARKMODEL_EXPORT SortCommand : public KEBMacroCommand
{
public:
    SortCommand(KBookmarkModel *model, const QString &name, const QString &groupAddress, QUndoCommand *parent = nullptr);
    ~SortCommand() override
    {
    }
    void redo() override;
    void undo() override;
    QString affectedBookmarks() const override;
    // internal
    void moveAfter(const SortItem &moveMe, const SortItem &afterMe);

private:
    KBookmarkModel *m_model;
    QString m_groupAddress;
};

// TODO RENAME -- or maybe move these to KBookmarkModel?
class KBOOKMARKMODEL_EXPORT CmdGen
{
public:
    static KEBMacroCommand *setAsToolbar(KBookmarkModel *model, const KBookmark &bk);
    static KEBMacroCommand *insertMimeSource(KBookmarkModel *model, const QString &cmdName, const QMimeData *data, const QString &addr);
    // TODO remove "bool copy"
    static KEBMacroCommand *itemsMoved(KBookmarkModel *model, const QList<KBookmark> &items, const QString &newAddress, bool copy);

private:
    CmdGen()
    {
    }
};

#endif
