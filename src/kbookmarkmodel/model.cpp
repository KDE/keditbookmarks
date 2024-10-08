/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Daniel Teske <teske@squorn.de>
   SPDX-FileCopyrightText: 2010 David Faure <faure@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#include "model.h"
#include "commandhistory.h"
#include "commands.h"
#include "treeitem_p.h"

#include <KBookmarkManager>
#include <KLocalizedString>

#include "keditbookmarks_debug.h"
#include <QIcon>
#include <QMimeData>
#include <QStringList>

class KBookmarkModel::Private
{
public:
    Private(KBookmarkModel *qq, const KBookmark &root, CommandHistory *commandHistory)
        : q(qq)
        , mRoot(root)
        , mCommandHistory(commandHistory)
        , mInsertionData(nullptr)
        , mIgnoreNext(0)
    {
        mRootItem = new TreeItem(root, nullptr);
    }
    ~Private()
    {
        delete mRootItem;
        mRootItem = nullptr;
    }

    void _kd_slotBookmarksChanged(const QString &groupAddress);

    KBookmarkModel *q;
    TreeItem *mRootItem;
    KBookmark mRoot;
    CommandHistory *mCommandHistory;

    class InsertionData
    {
    public:
        InsertionData(const QModelIndex &parent, int first, int last)
            : mFirst(first)
            , mLast(last)
        {
            mParentItem = static_cast<TreeItem *>(parent.internalPointer());
        }
        void insertChildren()
        {
            mParentItem->insertChildren(mFirst, mLast);
        }
        TreeItem *mParentItem;
        int mFirst;
        int mLast;
    };
    InsertionData *mInsertionData;
    int mIgnoreNext;
};

KBookmarkModel::KBookmarkModel(const KBookmark &root, CommandHistory *commandHistory, QObject *parent)
    : QAbstractItemModel(parent)
    , d(new Private(this, root, commandHistory))
{
    connect(commandHistory, &CommandHistory::notifyCommandExecuted, this, &KBookmarkModel::notifyManagers);
    Q_ASSERT(bookmarkManager());
    // update when the model updates after a D-Bus signal, coming from this
    // process or from another one
    connect(bookmarkManager(), &KBookmarkManager::changed, this, [this](const QString &groupAddress) {
        d->_kd_slotBookmarksChanged(groupAddress);
    });
}

void KBookmarkModel::setRoot(const KBookmark &root)
{
    d->mRoot = root;
    resetModel();
}

KBookmarkModel::~KBookmarkModel()
{
    delete d;
}

void KBookmarkModel::resetModel()
{
    beginResetModel();
    delete d->mRootItem;
    d->mRootItem = new TreeItem(d->mRoot, nullptr);
    endResetModel();
}

QVariant KBookmarkModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // Text
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const KBookmark bk = bookmarkForIndex(index);
        if (bk.address().isEmpty()) {
            if (index.column() == NameColumnId)
                return QVariant(i18nc("name of the container of all browser bookmarks", "Bookmarks"));
            else
                return QVariant();
        }

        switch (index.column()) {
        case NameColumnId:
            return bk.fullText();
        case UrlColumnId:
            return bk.url().url(QUrl::PreferLocalFile);
        case CommentColumnId:
            return bk.description();
        case StatusColumnId: {
            QString text1 = bk.metaDataItem(QStringLiteral("favstate")); // favicon state
            QString text2 = bk.metaDataItem(QStringLiteral("linkstate"));
            if (text1.isEmpty() || text2.isEmpty())
                return QVariant(text1 + text2);
            else
                return QVariant(text1 + QLatin1String("  --  ") + text2);
        }
        default:
            return QVariant(); // can't happen
        }
    }

    // Icon
    if (role == Qt::DecorationRole && index.column() == NameColumnId) {
        KBookmark bk = bookmarkForIndex(index);
        if (bk.address().isEmpty())
            return QIcon::fromTheme(QStringLiteral("bookmarks"));
        return QIcon::fromTheme(bk.icon());
    }

    // Special roles
    if (role == KBookmarkRole) {
        KBookmark bk = bookmarkForIndex(index);
        return QVariant::fromValue(bk);
    }
    return QVariant();
}

// FIXME QModelIndex KBookmarkModel::buddy(const QModelIndex & index) //return parent for empty folder padders
// no empty folder padders atm

Qt::ItemFlags KBookmarkModel::flags(const QModelIndex &index) const
{
    const Qt::ItemFlags baseFlags = QAbstractItemModel::flags(index);

    if (!index.isValid())
        return (Qt::ItemIsDropEnabled | baseFlags);

    static const Qt::ItemFlags groupFlags = Qt::ItemIsDropEnabled;
    static const Qt::ItemFlags groupDragEditFlags = groupFlags | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
    static const Qt::ItemFlags groupEditFlags = groupFlags | Qt::ItemIsEditable;
    static const Qt::ItemFlags rootFlags = groupFlags;
    static const Qt::ItemFlags bookmarkFlags = Qt::NoItemFlags;
    static const Qt::ItemFlags bookmarkDragEditFlags = bookmarkFlags | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
    static const Qt::ItemFlags bookmarkEditFlags = bookmarkFlags | Qt::ItemIsEditable;

    Qt::ItemFlags result = baseFlags;

    const int column = index.column();
    const KBookmark bookmark = bookmarkForIndex(index);
    if (bookmark.isGroup()) {
        const bool isRoot = bookmark.address().isEmpty();
        result |= (isRoot)                ? rootFlags
            : (column == NameColumnId)    ? groupDragEditFlags
            : (column == CommentColumnId) ? groupEditFlags
                                          :
                                          /*else*/ groupFlags;
    } else {
        result |= (column == NameColumnId) ? bookmarkDragEditFlags
            : (column != StatusColumnId)   ? bookmarkEditFlags
                                         /* else */
                                         : bookmarkFlags;
    }

    return result;
}

bool KBookmarkModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {
        qCDebug(KEDITBOOKMARKS_LOG) << value.toString();
        d->mCommandHistory->addCommand(new EditCommand(this, bookmarkForIndex(index).address(), index.column(), value.toString()));
        return true;
    }
    return false;
}

QVariant KBookmarkModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        QString result;
        switch (section) {
        case NameColumnId:
            result = i18nc("@title:column name of a bookmark", "Name");
            break;
        case UrlColumnId:
            result = i18nc("@title:column name of a bookmark", "Location");
            break;
        case CommentColumnId:
            result = i18nc("@title:column comment for a bookmark", "Comment");
            break;
        case StatusColumnId:
            result = i18nc("@title:column status of a bookmark", "Status");
            break;
        }
        return result;
    } else
        return QVariant();
}

QModelIndex KBookmarkModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, d->mRootItem);

    TreeItem *item = static_cast<TreeItem *>(parent.internalPointer());
    if (row == item->childCount()) { // Received drop below last row, simulate drop on last row
        return createIndex(row - 1, column, item->child(row - 1));
    }

    return createIndex(row, column, item->child(row));
}

QModelIndex KBookmarkModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        // qt asks for the parent of an invalid parent
        // either we are in a inconsistent case or more likely
        // we are dragging and dropping and qt didn't check
        // what itemAt() returned
        return index;
    }
    KBookmark bk = bookmarkForIndex(index);
    ;
    const QString rootAddress = d->mRoot.address();

    if (bk.address() == rootAddress)
        return QModelIndex();

    KBookmarkGroup parent = bk.parentGroup();
    TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
    if (parent.address() != rootAddress)
        return createIndex(parent.positionInParent(), 0, item->parent());
    else // parent is root
        return createIndex(0, 0, item->parent());
}

int KBookmarkModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<TreeItem *>(parent.internalPointer())->childCount();
    else // root case
        return 1; // Only one child: "Bookmarks"
}

int KBookmarkModel::columnCount(const QModelIndex &) const
{
    return NoOfColumnIds;
}

QModelIndex KBookmarkModel::indexForBookmark(const KBookmark &bk) const
{
    TreeItem *item = d->mRootItem->treeItemForBookmark(bk);
    if (!item) {
        qCWarning(KEDITBOOKMARKS_LOG) << "Bookmark not found" << bk.address();
        Q_ASSERT(item);
    }
    return createIndex(KBookmark::positionInParent(bk.address()), 0, item);
}

void KBookmarkModel::emitDataChanged(const KBookmark &bk)
{
    QModelIndex idx = indexForBookmark(bk);
    qCDebug(KEDITBOOKMARKS_LOG) << idx;
    Q_EMIT dataChanged(idx, idx.sibling(idx.row(), columnCount() - 1));
}

static const char *s_mime_bookmark_addresses = "application/x-kde-bookmarkaddresses";

QMimeData *KBookmarkModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData;
    KBookmark::List bookmarks;
    QByteArray addresses;

    for (const auto &it : indexes) {
        if (it.column() == NameColumnId) {
            bookmarks.push_back(bookmarkForIndex(it));
            if (!addresses.isEmpty()) {
                addresses.append(';');
            }
            addresses.append(bookmarkForIndex(it).address().toLatin1());
            qCDebug(KEDITBOOKMARKS_LOG) << "appended" << bookmarkForIndex(it).address();
        }
    }

    bookmarks.populateMimeData(mimeData);
    mimeData->setData(QLatin1String(s_mime_bookmark_addresses), addresses);
    return mimeData;
}

Qt::DropActions KBookmarkModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList KBookmarkModel::mimeTypes() const
{
    return KBookmark::List::mimeDataTypes();
}

bool KBookmarkModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    QModelIndex dropDestIndex;
    bool isInsertBetweenOp = false;
    if (row == -1) {
        dropDestIndex = parent;
    } else {
        isInsertBetweenOp = true;
        dropDestIndex = index(row, column, parent);
    }

    KBookmark dropDestBookmark = bookmarkForIndex(dropDestIndex);
    if (dropDestBookmark.isNull()) {
        // Presumably an invalid index: assume we want to place this in the root bookmark
        // folder.
        dropDestBookmark = d->mRoot;
    }

    QString addr = dropDestBookmark.address();
    if (dropDestBookmark.isGroup() && !isInsertBetweenOp) {
        addr += QLatin1String("/0");
    }
    // bookmarkForIndex(...) does not distinguish between the last item in the folder
    // and the point *after* the last item in the folder (and its hard to see how to
    // modify it so it does), so do the check here.
    if (isInsertBetweenOp && row == dropDestBookmark.positionInParent() + 1) {
        // Attempting to insert underneath the last item in a folder; adjust the address.
        addr = KBookmark::nextAddress(addr);
    }

    if (action == Qt::CopyAction) {
        KEBMacroCommand *cmd = CmdGen::insertMimeSource(this, QStringLiteral("Copy"), data, addr);
        d->mCommandHistory->addCommand(cmd);
    } else if (action == Qt::MoveAction) {
        if (data->hasFormat(QLatin1String(s_mime_bookmark_addresses))) {
            KBookmark::List bookmarks;
            QList<QByteArray> addresses = data->data(QLatin1String(s_mime_bookmark_addresses)).split(';');
            std::sort(addresses.begin(), addresses.end());
            for (const auto &address : std::as_const(addresses)) {
                KBookmark bk = bookmarkManager()->findByAddress(QString::fromLatin1(address));
                qCDebug(KEDITBOOKMARKS_LOG) << "Extracted bookmark:" << bk.address();
                bookmarks.prepend(bk); // reverse order, so that we don't invalidate addresses (#287038)
            }

            KEBMacroCommand *cmd = CmdGen::itemsMoved(this, bookmarks, addr, false);
            d->mCommandHistory->addCommand(cmd);
        } else {
            qCDebug(KEDITBOOKMARKS_LOG) << "NO FORMAT";
            KEBMacroCommand *cmd = CmdGen::insertMimeSource(this, QStringLiteral("Copy"), data, addr);
            d->mCommandHistory->addCommand(cmd);
        }
    }

    return true;
}

KBookmark KBookmarkModel::bookmarkForIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return KBookmark();
    }
    return static_cast<TreeItem *>(index.internalPointer())->bookmark();
}

void KBookmarkModel::beginInsert(const KBookmarkGroup &group, int first, int last)
{
    Q_ASSERT(!d->mInsertionData);
    const QModelIndex parent = indexForBookmark(group);
    d->mInsertionData = new Private::InsertionData(parent, first, last);
    beginInsertRows(parent, first, last);
}

void KBookmarkModel::endInsert()
{
    Q_ASSERT(d->mInsertionData);
    d->mInsertionData->insertChildren();
    delete d->mInsertionData;
    d->mInsertionData = nullptr;
    endInsertRows();
}

#if 0 // Probably correct, but not needed at the moment
void KBookmarkModel::removeBookmarks(KBookmarkGroup parent, int first, int last)
{
    const QModelIndex parentIndex = indexForBookmark(parent);
    beginRemoveRows(parentIndex, first, last);
    TreeItem* parentItem = static_cast<TreeItem *>(parentIndex.internalPointer());

    // Go to the last bookmark to remove
    KBookmark bk = parent.first();
    for (int i = 1; i < last; ++i)
        bk = parent.next(bk);
    // Then remove bookmarks, iterating backwards until 'first'
    // (so that numbering still works)
    for (int i = last; i >= first; --i) {
        KBookmark prev = parent.previous(bk);
        parent.deleteBookmark(bk);
        bk = prev;
    }

    parentItem->deleteChildren(first, last);
    endRemoveRows();
}
#endif

void KBookmarkModel::removeBookmark(const KBookmark &bookmark)
{
    KBookmarkGroup parentGroup = bookmark.parentGroup();
    const QModelIndex parentIndex = indexForBookmark(parentGroup);
    Q_ASSERT(parentIndex.isValid());
    const int pos = bookmark.positionInParent();
    beginRemoveRows(parentIndex, pos, pos);
    TreeItem *parentItem = static_cast<TreeItem *>(parentIndex.internalPointer());
    Q_ASSERT(parentItem);

    parentGroup.deleteBookmark(bookmark);

    parentItem->deleteChildren(pos, pos);
    endRemoveRows();
}

CommandHistory *KBookmarkModel::commandHistory()
{
    return d->mCommandHistory;
}

KBookmarkManager *KBookmarkModel::bookmarkManager()
{
    return d->mCommandHistory->bookmarkManager();
}

void KBookmarkModel::Private::_kd_slotBookmarksChanged(const QString &groupAddress)
{
    Q_UNUSED(groupAddress);
    // qCDebug(KEDITBOOKMARKS_LOG) << "_kd_slotBookmarksChanged" << groupAddress << "mIgnoreNext=" << mIgnoreNext;
    if (mIgnoreNext > 0) { // We ignore the first changed signal after every change we did
        --mIgnoreNext;
        return;
    }

    // qCDebug(KEDITBOOKMARKS_LOG) << " setRoot!";
    q->setRoot(q->bookmarkManager()->root());

    mCommandHistory->clearHistory();
}

void KBookmarkModel::notifyManagers(const KBookmarkGroup &grp)
{
    ++d->mIgnoreNext;
    // qCDebug(KEDITBOOKMARKS_LOG) << "notifyManagers -> mIgnoreNext=" << d->mIgnoreNext;
    bookmarkManager()->emitChanged(grp);
}

#include "moc_model.cpp"
