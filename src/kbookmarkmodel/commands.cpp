/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#include "commands.h"
#include "commands_p.h"
#include "kinsertionsort_p.h"
#include "model.h"

#include "keditbookmarks_debug.h"
#include <KBookmarkManager>
#include <KLocalizedString>

QString KEBMacroCommand::affectedBookmarks() const
{
    const int commandCount = childCount();
    if (commandCount == 0) {
        return QString();
    }
    // Need to use dynamic_cast here due to going cross-hierarchy, but it should never return 0.
    int i = 0;
    QString affectBook = dynamic_cast<const IKEBCommand *>(child(i))->affectedBookmarks();
    ++i;
    for (; i < commandCount; ++i) {
        affectBook = KBookmark::commonParent(affectBook, dynamic_cast<const IKEBCommand *>(child(i))->affectedBookmarks());
    }
    return affectBook;
}

DeleteManyCommand::DeleteManyCommand(KBookmarkModel *model, const QString &name, const QList<KBookmark> &bookmarks)
    : KEBMacroCommand(name)
{
    QList<KBookmark>::const_iterator it, begin;
    begin = bookmarks.constBegin();
    it = bookmarks.constEnd();
    while (begin != it) {
        --it;
        new DeleteCommand(model, (*it).address(), false, this);
    }
}

////

CreateCommand::CreateCommand(KBookmarkModel *model, const QString &address, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_to(address)
    , m_group(false)
    , m_separator(true)
    , m_originalBookmark(QDomElement())
{
    setText(i18nc("(qtundo-format)", "Insert Separator"));
}

CreateCommand::CreateCommand(KBookmarkModel *model, const QString &address, const QString &text, const QString &iconPath, const QUrl &url, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_to(address)
    , m_text(text)
    , m_iconPath(iconPath)
    , m_url(url)
    , m_group(false)
    , m_separator(false)
    , m_originalBookmark(QDomElement())
{
    setText(i18nc("(qtundo-format)", "Create Bookmark"));
}

CreateCommand::CreateCommand(KBookmarkModel *model, const QString &address, const QString &text, const QString &iconPath, bool open, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_to(address)
    , m_text(text)
    , m_iconPath(iconPath)
    , m_group(true)
    , m_separator(false)
    , m_open(open)
    , m_originalBookmark(QDomElement())
{
    setText(i18nc("(qtundo-format)", "Create Folder"));
}

CreateCommand::CreateCommand(KBookmarkModel *model, const QString &address, const KBookmark &original, const QString &name, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_to(address)
    , m_group(false)
    , m_separator(false)
    , m_open(false)
    , m_originalBookmark(original)
    , m_originalBookmarkDocRef(m_originalBookmark.internalElement().ownerDocument())
{
    setText(i18nc("(qtundo-format)", "Copy %1", name));
}

void CreateCommand::redo()
{
    QString parentAddress = KBookmark::parentAddress(m_to);
    KBookmarkGroup parentGroup = m_model->bookmarkManager()->findByAddress(parentAddress).toGroup();

    QString previousSibling = KBookmark::previousAddress(m_to);

    // qCDebug(KEDITBOOKMARKS_LOG) << "previousSibling=" << previousSibling;
    KBookmark prev = (previousSibling.isEmpty()) ? KBookmark(QDomElement()) : m_model->bookmarkManager()->findByAddress(previousSibling);

    KBookmark bk = KBookmark(QDomElement());
    const int pos = KBookmark::positionInParent(m_to);
    m_model->beginInsert(parentGroup, pos, pos);

    if (m_separator) {
        bk = parentGroup.createNewSeparator();

    } else if (m_group) {
        Q_ASSERT(!m_text.isEmpty());
        bk = parentGroup.createNewFolder(m_text);
        bk.internalElement().setAttribute(QStringLiteral("folded"), (m_open ? QLatin1String("no") : QLatin1String("yes")));
        if (!m_iconPath.isEmpty()) {
            bk.setIcon(m_iconPath);
        }
    } else if (!m_originalBookmark.isNull()) {
        QDomElement element = m_originalBookmark.internalElement().cloneNode().toElement();
        bk = KBookmark(element);
        parentGroup.addBookmark(bk);
    } else {
        bk = parentGroup.addBookmark(m_text, m_url, m_iconPath);
    }

    // move to right position
    bool ok = parentGroup.moveBookmark(bk, prev);
    Q_UNUSED(ok);
    // TODO (requires KBookmarks >= 5.25)
    // Q_ASSERT(ok);
    if (!(text().isEmpty()) && !parentAddress.isEmpty()) {
        // open the parent (useful if it was empty) - only for manual commands
        Q_ASSERT(parentGroup.internalElement().tagName() != QLatin1String("xbel"));
        parentGroup.internalElement().setAttribute(QStringLiteral("folded"), QStringLiteral("no"));
    }

    Q_ASSERT(bk.address() == m_to);
    m_model->endInsert();
}

QString CreateCommand::finalAddress() const
{
    Q_ASSERT(!m_to.isEmpty());
    return m_to;
}

void CreateCommand::undo()
{
    KBookmark bk = m_model->bookmarkManager()->findByAddress(m_to);
    Q_ASSERT(!bk.isNull() && !bk.parentGroup().isNull());

    m_model->removeBookmark(bk);
}

QString CreateCommand::affectedBookmarks() const
{
    return KBookmark::parentAddress(m_to);
}

/* -------------------------------------- */

EditCommand::EditCommand(KBookmarkModel *model, const QString &address, int col, const QString &newValue, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , mAddress(address)
    , mCol(col)
{
    qCDebug(KEDITBOOKMARKS_LOG) << address << col << newValue;
    if (mCol == 1) {
        const QUrl u(newValue);
        if (!(u.isEmpty() && !newValue.isEmpty())) // prevent emptied line if the currently entered url is invalid
            mNewValue = u.toString();
        else
            mNewValue = newValue;
    } else
        mNewValue = newValue;

    // -2 is "toolbar" attribute change, but that's only used internally.
    if (mCol == -1)
        setText(i18nc("(qtundo-format)", "Icon Change"));
    else if (mCol == 0)
        setText(i18nc("(qtundo-format)", "Title Change"));
    else if (mCol == 1)
        setText(i18nc("(qtundo-format)", "URL Change"));
    else if (mCol == 2)
        setText(i18nc("(qtundo-format)", "Comment Change"));
}

void EditCommand::redo()
{
    KBookmark bk = m_model->bookmarkManager()->findByAddress(mAddress);
    if (mCol == -2) {
        if (mOldValue.isEmpty())
            mOldValue = bk.internalElement().attribute(QStringLiteral("toolbar"));
        bk.internalElement().setAttribute(QStringLiteral("toolbar"), mNewValue);
    } else if (mCol == -1) {
        if (mOldValue.isEmpty())
            mOldValue = bk.icon();
        bk.setIcon(mNewValue);
    } else if (mCol == 0) {
        if (mOldValue.isEmpty()) // only the first time, not when compressing changes in modify()
            mOldValue = bk.fullText();
        qCDebug(KEDITBOOKMARKS_LOG) << "mOldValue=" << mOldValue;
        bk.setFullText(mNewValue);
    } else if (mCol == 1) {
        if (mOldValue.isEmpty())
            mOldValue = bk.url().toDisplayString();
        const QUrl newUrl(mNewValue);
        if (!(newUrl.isEmpty() && !mNewValue.isEmpty())) // prevent emptied line if the currently entered url is invalid
            bk.setUrl(newUrl);
    } else if (mCol == 2) {
        if (mOldValue.isEmpty())
            mOldValue = bk.description();
        bk.setDescription(mNewValue);
    }
    m_model->emitDataChanged(bk);
}

void EditCommand::undo()
{
    qCDebug(KEDITBOOKMARKS_LOG) << "Setting old value" << mOldValue << "in bk" << mAddress << "col" << mCol;
    KBookmark bk = m_model->bookmarkManager()->findByAddress(mAddress);
    if (mCol == -2) {
        bk.internalElement().setAttribute(QStringLiteral("toolbar"), mOldValue);
    } else if (mCol == -1) {
        bk.setIcon(mOldValue);
    } else if (mCol == 0) {
        bk.setFullText(mOldValue);
    } else if (mCol == 1) {
        bk.setUrl(QUrl(mOldValue));
    } else if (mCol == 2) {
        bk.setDescription(mOldValue);
    }
    m_model->emitDataChanged(bk);
}

void EditCommand::modify(const QString &newValue)
{
    if (mCol == 1) {
        const QUrl u(newValue);
        if (!(u.isEmpty() && !newValue.isEmpty())) // prevent emptied line if the currently entered url is invalid
            mNewValue = u.toString();
        else
            mNewValue = newValue;
    } else
        mNewValue = newValue;
}

/* -------------------------------------- */

DeleteCommand::DeleteCommand(KBookmarkModel *model, const QString &from, bool contentOnly, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_from(from)
    , m_cmd(nullptr)
    , m_subCmd(nullptr)
    , m_contentOnly(contentOnly)
{
    // NOTE - DeleteCommand needs no text, it is always embedded in a macrocommand
}

void DeleteCommand::redo()
{
    KBookmark bk = m_model->bookmarkManager()->findByAddress(m_from);
    Q_ASSERT(!bk.isNull());

    if (m_contentOnly) {
        QDomElement groupRoot = bk.internalElement();

        QDomNode n = groupRoot.firstChild();
        while (!n.isNull()) {
            QDomElement e = n.toElement();
            if (!e.isNull()) {
                // qCDebug(KEDITBOOKMARKS_LOG) << e.tagName();
            }
            QDomNode next = n.nextSibling();
            groupRoot.removeChild(n);
            n = next;
        }
        return;
    }

    // TODO - bug - unparsed xml is lost after undo,
    //              we must store it all therefore

    // FIXME this removes the comments, that's bad!
    if (!m_cmd) {
        if (bk.isGroup()) {
            m_cmd =
                new CreateCommand(m_model, m_from, bk.fullText(), bk.icon(), bk.internalElement().attribute(QStringLiteral("folded")) == QLatin1String("no"));
            m_subCmd = deleteAll(m_model, bk.toGroup());
            m_subCmd->redo();

        } else {
            m_cmd = (bk.isSeparator()) ? new CreateCommand(m_model, m_from) : new CreateCommand(m_model, m_from, bk.fullText(), bk.icon(), bk.url());
        }
    }
    m_cmd->undo();
}

void DeleteCommand::undo()
{
    // qCDebug(KEDITBOOKMARKS_LOG) << "DeleteCommand::undo " << m_from;

    if (m_contentOnly) {
        // TODO - recover saved metadata
        return;
    }

    m_cmd->redo();

    if (m_subCmd) {
        m_subCmd->undo();
    }
}

QString DeleteCommand::affectedBookmarks() const
{
    return KBookmark::parentAddress(m_from);
}

KEBMacroCommand *DeleteCommand::deleteAll(KBookmarkModel *model, const KBookmarkGroup &parentGroup)
{
    KEBMacroCommand *cmd = new KEBMacroCommand(QString());
    QStringList lstToDelete;
    // we need to delete from the end, to avoid index shifting
    for (KBookmark bk = parentGroup.first(); !bk.isNull(); bk = parentGroup.next(bk))
        lstToDelete.prepend(bk.address());
    for (QStringList::const_iterator it = lstToDelete.constBegin(); it != lstToDelete.constEnd(); ++it) {
        new DeleteCommand(model, (*it), false, cmd);
    }
    return cmd;
}

/* -------------------------------------- */

MoveCommand::MoveCommand(KBookmarkModel *model, const QString &from, const QString &to, const QString &name, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_from(from)
    , m_to(to)
    , m_cc(nullptr)
    , m_dc(nullptr)
{
    setText(i18nc("(qtundo-format)", "Move %1", name));
}

void MoveCommand::redo()
{
    // qCDebug(KEDITBOOKMARKS_LOG) << "Moving from=" << m_from << "to=" << m_to;

    KBookmark fromBk = m_model->bookmarkManager()->findByAddress(m_from);
    Q_ASSERT(fromBk.address() == m_from);

    // qCDebug(KEDITBOOKMARKS_LOG) << "  1) creating" << m_to;
    m_cc = new CreateCommand(m_model, m_to, fromBk, QString());
    m_cc->redo();

    // qCDebug(KEDITBOOKMARKS_LOG) << "  2) deleting" << fromBk.address();
    m_dc = new DeleteCommand(m_model, fromBk.address());
    m_dc->redo();
}

QString MoveCommand::finalAddress() const
{
    Q_ASSERT(!m_to.isEmpty());
    return m_to;
}

void MoveCommand::undo()
{
    m_dc->undo();
    m_cc->undo();
}

QString MoveCommand::affectedBookmarks() const
{
    return KBookmark::commonParent(KBookmark::parentAddress(m_from), KBookmark::parentAddress(m_to));
}

/* -------------------------------------- */

class SortItem
{
public:
    SortItem(const KBookmark &bk)
        : m_bk(bk)
    {
    }

    bool operator==(const SortItem &s)
    {
        return (m_bk.internalElement() == s.m_bk.internalElement());
    }

    bool isNull() const
    {
        return m_bk.isNull();
    }

    SortItem previousSibling() const
    {
        return m_bk.parentGroup().previous(m_bk);
    }

    SortItem nextSibling() const
    {
        return m_bk.parentGroup().next(m_bk);
    }

    const KBookmark &bookmark() const
    {
        return m_bk;
    }

private:
    KBookmark m_bk;
};

class SortByName
{
public:
    static QString key(const SortItem &item)
    {
        return (item.bookmark().isGroup() ? QStringLiteral("a") : QStringLiteral("b")) + (item.bookmark().fullText().toLower());
    }
};

/* -------------------------------------- */

SortCommand::SortCommand(KBookmarkModel *model, const QString &name, const QString &groupAddress, QUndoCommand *parent)
    : KEBMacroCommand(name, parent)
    , m_model(model)
    , m_groupAddress(groupAddress)
{
}

void SortCommand::redo()
{
    if (childCount() == 0) {
        KBookmarkGroup grp = m_model->bookmarkManager()->findByAddress(m_groupAddress).toGroup();
        Q_ASSERT(!grp.isNull());
        SortItem firstChild(grp.first());
        // this will call moveAfter, which will add
        // the subcommands for moving the items
        kInsertionSort<SortItem, SortByName, QString, SortCommand>(firstChild, (*this));

    } else {
        // don't redo for second time on addCommand(cmd)
        KEBMacroCommand::redo();
    }
}

void SortCommand::moveAfter(const SortItem &moveMe, const SortItem &afterMe)
{
    const QString destAddress = afterMe.isNull()
        // move as first child
        ? KBookmark::parentAddress(moveMe.bookmark().address()) + QStringLiteral("/0")
        // move after "afterMe"
        : KBookmark::nextAddress(afterMe.bookmark().address());

    MoveCommand *cmd = new MoveCommand(m_model, moveMe.bookmark().address(), destAddress, QString(), this);
    cmd->redo();
}

void SortCommand::undo()
{
    KEBMacroCommand::undo();
}

QString SortCommand::affectedBookmarks() const
{
    return m_groupAddress;
}

/* -------------------------------------- */

KEBMacroCommand *CmdGen::setAsToolbar(KBookmarkModel *model, const KBookmark &bk)
{
    KEBMacroCommand *mcmd = new KEBMacroCommand(i18nc("(qtundo-format)", "Set as Bookmark Toolbar"));

    KBookmarkGroup oldToolbar = model->bookmarkManager()->toolbar();
    if (!oldToolbar.isNull()) {
        new EditCommand(model, oldToolbar.address(), -2, QStringLiteral("no"), mcmd); // toolbar
        new EditCommand(model, oldToolbar.address(), -1, QLatin1String(""), mcmd); // icon
    }

    new EditCommand(model, bk.address(), -2, QStringLiteral("yes"), mcmd);
    new EditCommand(model, bk.address(), -1, QStringLiteral("bookmark-toolbar"), mcmd);

    return mcmd;
}

KEBMacroCommand *CmdGen::insertMimeSource(KBookmarkModel *model, const QString &cmdName, const QMimeData *data, const QString &addr)
{
    KEBMacroCommand *mcmd = new KEBMacroCommand(cmdName);
    QString currentAddress = addr;
    QDomDocument doc;
    const auto bookmarks = KBookmark::List::fromMimeData(data, doc);
    for (const KBookmark &bk : bookmarks) {
        new CreateCommand(model, currentAddress, bk, QString(), mcmd);
        currentAddress = KBookmark::nextAddress(currentAddress);
    }
    return mcmd;
}

KEBMacroCommand *CmdGen::itemsMoved(KBookmarkModel *model, const QList<KBookmark> &items, const QString &newAddress, bool copy)
{
    Q_ASSERT(!copy); // always called for a move, never for a copy (that's what insertMimeSource is about)
    Q_UNUSED(copy); // TODO: remove

    KEBMacroCommand *mcmd = new KEBMacroCommand(copy ? i18nc("(qtundo-format)", "Copy Items") : i18nc("(qtundo-format)", "Move Items"));
    QString bkInsertAddr = newAddress;
    for (const KBookmark &bk : items) {
        new CreateCommand(model, bkInsertAddr, KBookmark(bk.internalElement().cloneNode(true).toElement()), bk.text(), mcmd);
        bkInsertAddr = KBookmark::nextAddress(bkInsertAddr);
    }

    // Do the copying, and get the updated addresses of the bookmarks to remove.
    mcmd->redo();
    QStringList addresses;
    for (const KBookmark &bk : items) {
        addresses.append(bk.address());
    }
    mcmd->undo();

    for (const auto &address : std::as_const(addresses)) {
        new DeleteCommand(model, address, false, mcmd);
    }

    return mcmd;
}
